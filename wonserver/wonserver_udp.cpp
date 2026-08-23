// wonserver_udp.cpp -- emulated per-room status servers (player-count
// responder).

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "wonserver_udp.h"
#include "wonserver_room.h"
#include "won_dir.h"

#pragma comment( lib, "ws2_32.lib" )

#define WONUDP_MAX_PORTS	64

// The published room table (port -> player count), guarded by g_cs.
static unsigned short	g_ports[WONUDP_MAX_PORTS];
static int				g_players[WONUDP_MAX_PORTS];
static int				g_nPorts = 0;
static int				g_dirty  = 0;

static CRITICAL_SECTION	g_cs;
static LONG				g_csInit  = 0;
static LONG				g_started = 0;
static HANDLE			g_thread  = NULL;

static void WonUdp_Lock( void )
{
	// One-time critical-section init (the registry calls in on the UI thread before
	// the responder thread exists, so a simple interlocked guard is enough).
	if ( InterlockedCompareExchange( &g_csInit, 1, 0 ) == 0 )
		InitializeCriticalSection( &g_cs );
	EnterCriticalSection( &g_cs );
}

static void WonUdp_Unlock( void )
{
	LeaveCriticalSection( &g_cs );
}

void WonUdp_Clear( void )
{
	WonUdp_Lock();
	g_nPorts = 0;
	g_dirty  = 1;
	WonUdp_Unlock();
}

void WonUdp_SetRoom( unsigned short nPort, int nPlayers )
{
	WonUdp_Lock();
	int	idx = -1;
	for ( int i = 0; i < g_nPorts; i++ )
		if ( g_ports[i] == nPort )
		{
			idx = i;
			break;
		}
	if ( idx < 0 && g_nPorts < WONUDP_MAX_PORTS )
		idx = g_nPorts++;
	if ( idx >= 0 )
	{
		g_ports[idx]   = nPort;
		g_players[idx] = nPlayers;
		g_dirty        = 1;
	}
	WonUdp_Unlock();
}

// Bind one loopback UDP socket for nPort; INVALID_SOCKET on failure.
static SOCKET WonUdp_Bind( unsigned short nPort )
{
	SOCKET	s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( s == INVALID_SOCKET )
		return INVALID_SOCKET;

	struct sockaddr_in	addr;
	memset( &addr, 0, sizeof( addr ) );
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
	addr.sin_port        = htons( nPort );

	if ( bind( s, (struct sockaddr*)&addr, sizeof( addr ) ) != 0 )
	{
		closesocket( s );
		return INVALID_SOCKET;
	}
	return s;
}

static DWORD WINAPI WonUdp_ThreadProc( LPVOID )
{
	WSADATA	wsa;
	WSAStartup( MAKEWORD( 2, 2 ), &wsa );	// ref-counted; the launcher also starts WinSock

	// The thread's own bound-socket view (rebuilt from the table whenever it changes).
	SOCKET			socks[WONUDP_MAX_PORTS];
	int				sockPlayers[WONUDP_MAX_PORTS];
	unsigned short	sockPorts[WONUDP_MAX_PORTS];
	int				nSocks = 0;

	for ( ;; )
	{
		// Republish the table -> bound sockets on a change.
		WonUdp_Lock();
		int	dirty = g_dirty;
		g_dirty = 0;
		unsigned short	wantPorts[WONUDP_MAX_PORTS];
		int				wantPlayers[WONUDP_MAX_PORTS];
		int				nWant = g_nPorts;
		for ( int i = 0; i < nWant; i++ )
		{
			wantPorts[i]   = g_ports[i];
			wantPlayers[i] = g_players[i];
		}
		WonUdp_Unlock();

		if ( dirty )
		{
			for ( int i = 0; i < nSocks; i++ )
				closesocket( socks[i] );
			nSocks = 0;
			for ( int i = 0; i < nWant; i++ )
			{
				SOCKET	s = WonUdp_Bind( wantPorts[i] );
				if ( s != INVALID_SOCKET )
				{
					socks[nSocks]       = s;
					sockPlayers[nSocks] = wantPlayers[i];
					sockPorts[nSocks]   = wantPorts[i];
					nSocks++;
				}
			}
		}

		if ( nSocks == 0 )
		{
			Sleep( 100 );	// nothing bound yet -- idle until rooms are published
			continue;
		}

		fd_set	rfds;
		FD_ZERO( &rfds );
		for ( int i = 0; i < nSocks; i++ )
			FD_SET( socks[i], &rfds );

		struct timeval	tv;
		tv.tv_sec  = 0;
		tv.tv_usec = 200000;	// 200 ms -- responsive to table changes

		if ( select( 0, &rfds, NULL, NULL, &tv ) <= 0 )
			continue;

		for ( int i = 0; i < nSocks; i++ )
		{
			if ( !FD_ISSET( socks[i], &rfds ) )
				continue;

			unsigned char		query[64];
			struct sockaddr_in	from;
			int					fromlen = sizeof( from );
			int					n = recvfrom( socks[i], (char*)query, sizeof( query ), 0,
											  (struct sockaddr*)&from, &fromlen );

			// Info query: [03 01 01 <key-lo> <key-hi>].  Answer with the player count
			// as the third little-endian short, echoing back the key from the query.
			if ( n >= 5 && query[0] == ROOMQ_HEADER0 && query[1] == ROOMQ_HEADER1
			     && query[2] == ROOMQ_PLAYERCOUNT_REQUEST )
			{
				int				players = sockPlayers[i];
				unsigned char	reply[9];
				reply[0] = ROOMQ_HEADER0;
				reply[1] = ROOMQ_HEADER1;
				reply[2] = ROOMQ_PLAYERCOUNT_REPLY;
				reply[3] = query[3];							// echo key (room port lo)
				reply[4] = query[4];							// echo key (room port hi)
				reply[5] = (unsigned char)( players & 0xFF );
				reply[6] = (unsigned char)( ( players >> 8 ) & 0xFF );
				reply[7] = 0;
				reply[8] = 0;
				sendto( socks[i], (const char*)reply, (int)sizeof( reply ), 0,
						(struct sockaddr*)&from, fromlen );
				printf( "  room status probe on UDP %u -> %d players\n",
						sockPorts[i], players );
			}
			else if ( n >= 8 && query[0] == ROOMQ_HEADER0 && query[1] == ROOMQ_HEADER1
			          && query[2] == ROOMQ_FINDPLAYER_REQUEST )
			{
				int count = query[6] | ( query[7] << 8 );
				if ( count > 0 && 8 + count * 2 <= n )
				{
					char nick[256];
					int copy = count < (int)sizeof( nick ) - 1 ? count : sizeof( nick ) - 1;
					WideCharToMultiByte( CP_ACP, 0, (const wchar_t*)( query + 8 ), copy,
						nick, sizeof( nick ) - 1, NULL, NULL );
					nick[copy] = 0;
					if ( Room_HasMember( sockPorts[i], nick ) )
					{
						unsigned char reply[6];
						reply[0] = ROOMQ_HEADER0;
						reply[1] = ROOMQ_HEADER1;
						reply[2] = ROOMQ_FINDPLAYER_REPLY;
						reply[3] = 0;
						reply[4] = query[3];
						reply[5] = query[4];
						sendto( socks[i], (const char*)reply, sizeof( reply ), 0,
							(struct sockaddr*)&from, fromlen );
					}
				}
			}
		}
	}
	// not reached
}

void WonUdp_Sync( void )
{
	// Start the responder thread once; the table is already published via SetRoom.
	if ( InterlockedCompareExchange( &g_started, 1, 0 ) == 0 )
		g_thread = CreateThread( NULL, 0, WonUdp_ThreadProc, NULL, 0, NULL );
}
