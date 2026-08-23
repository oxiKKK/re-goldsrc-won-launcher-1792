// wonserver_gamesv.cpp -- mock GoldSrc game servers for the Internet games page.
//
// The browser gets a list of {ip,port} from the master (wonserver_master.cpp), then
// UDP-connects to each and sends [long -1]["infostring\n"].  CHLAsyncSocket::
// ParseInfoResponse (0x414B10) wants [long -1]["infostringresponse"][<infostring>],
// both strings NUL-terminated -- CMessageBuffer::ReadToken (0x429220) reads to the
// NUL, so values may contain spaces -- and pulls hostname/map/gamedir/description/
// players/max/protocol/type/os/password out of it with Info_ValueForKey.
//
// Nothing here is reverse-engineered: it is invented data so the page has rows.

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "wonserver_gamesv.h"

struct MockServer
{
	unsigned short	port;
	const char*		hostname;
	const char*		map;
	const char*		gamedir;
	const char*		description;	// the "Game Type" column
	int				players;
	int				maxplayers;
	char			type;			// 'd' dedicated, 'l' listen, 'p' proxy
	char			os;				// 'w' windows, 'l' linux
	int				password;
};

// Ports sit above the chat rooms (27015-27022) so the two responders never collide.
static const MockServer	g_servers[] =
{
	{ 27023, "Black Mesa Research Facility", "crossfire",   "valve",   "Half-Life",          12, 32, 'd', 'w', 0 },
	{ 27024, "[UK] Deathmatch Arena",        "boot_camp",   "valve",   "Half-Life",          24, 24, 'd', 'l', 0 },
	{ 27025, "de_dust 24/7 -- No Cheats",    "de_dust",     "cstrike", "Counter-Strike",     18, 20, 'd', 'l', 0 },
	{ 27026, "Clan Match (private)",         "de_aztec",    "cstrike", "Counter-Strike",      6, 10, 'd', 'w', 1 },
	{ 27027, "2Fort Forever",                "2fort",       "tfc",     "Team Fortress",      14, 32, 'd', 'l', 0 },
	{ 27028, "Omaha Beach",                  "dod_omaha",   "dod",     "Day of Defeat",      21, 32, 'd', 'w', 0 },
	{ 27029, "Rocket Arena DMC",             "dm4",         "dmc",     "Deathmatch Classic",  4, 16, 'l', 'w', 0 },
	{ 27030, "Newbie Practice -- all wel",   "stalkyard",   "valve",   "Half-Life",           2, 16, 'd', 'w', 0 },
};

static const int	g_nServers = (int)( sizeof( g_servers ) / sizeof( g_servers[0] ) );

static LONG		g_started = 0;
static HANDLE	g_thread  = NULL;

int WonGameSv_Count( void )
{
	return g_nServers;
}

unsigned short WonGameSv_Port( int i )
{
	return ( i >= 0 && i < g_nServers ) ? g_servers[i].port : 0;
}

static SOCKET BindUdp( unsigned short port )
{
	SOCKET	s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( s == INVALID_SOCKET )
		return INVALID_SOCKET;

	struct sockaddr_in	sa;
	memset( &sa, 0, sizeof( sa ) );
	sa.sin_family      = AF_INET;
	sa.sin_addr.s_addr = htonl( INADDR_ANY );
	sa.sin_port        = htons( port );
	if ( bind( s, (struct sockaddr*)&sa, sizeof( sa ) ) != 0 )
	{
		closesocket( s );
		return INVALID_SOCKET;
	}
	return s;
}

// [long -1]["infostringresponse"][<infostring>], both NUL-terminated.
static int BuildInfoReply( const MockServer* sv, char* out, int cbOut )
{
	int	o = 0;
	out[o++] = (char)0xFF; out[o++] = (char)0xFF;
	out[o++] = (char)0xFF; out[o++] = (char)0xFF;

	const char*	pszCmd = "infostringresponse";
	int			nCmd   = (int)strlen( pszCmd ) + 1;
	memcpy( out + o, pszCmd, nCmd );
	o += nCmd;

	char	info[1024];
	_snprintf( info, sizeof( info ) - 1,
		"\\hostname\\%s\\map\\%s\\gamedir\\%s\\description\\%s"
		"\\players\\%d\\max\\%d\\protocol\\46\\type\\%c\\os\\%c\\password\\%d"
		"\\proxytarget\\0\\proxyaddress\\\\modversion\\1",
		sv->hostname, sv->map, sv->gamedir, sv->description,
		sv->players, sv->maxplayers, sv->type, sv->os, sv->password );
	info[sizeof( info ) - 1] = 0;

	int	nInfo = (int)strlen( info ) + 1;
	if ( o + nInfo > cbOut )
		return 0;
	memcpy( out + o, info, nInfo );
	o += nInfo;
	return o;
}

static void PutShort( char* out, int* pOffset, unsigned short value )
{
	out[( *pOffset )++] = (char)( value & 0xFF );
	out[( *pOffset )++] = (char)( value >> 8 );
}

static void PutLong( char* out, int* pOffset, unsigned long value )
{
	out[( *pOffset )++] = (char)( value & 0xFF );
	out[( *pOffset )++] = (char)( value >> 8 );
	out[( *pOffset )++] = (char)( value >> 16 );
	out[( *pOffset )++] = (char)( value >> 24 );
}

static void PutString( char* out, int* pOffset, const char* value )
{
	int len = (int)strlen( value ) + 1;
	memcpy( out + *pOffset, value, len );
	*pOffset += len;
}

static int BuildPlayersReply( const MockServer* sv, char* out, int cbOut )
{
	static const char* names[] =
	{
		"Gordon", "Barney", "Kleiner", "Eli", "Gina", "Colette", "Otis", "Walter"
	};
	int count = sv->players < 8 ? sv->players : 8;
	int o = 0;

	if ( cbOut < 64 )
		return 0;
	PutLong( out, &o, 0xFFFFFFFF );
	out[o++] = 'D';
	out[o++] = (char)count;
	for ( int i = 0; i < count; i++ )
	{
		float seconds = (float)( 90 + i * 37 );
		unsigned long raw;
		memcpy( &raw, &seconds, sizeof( raw ) );
		out[o++] = (char)i;
		PutString( out, &o, names[i] );
		PutLong( out, &o, (unsigned long)( 20 - i * 3 ) );
		PutLong( out, &o, raw );
	}
	return o <= cbOut ? o : 0;
}

static int BuildRulesReply( const MockServer* sv, char* out, int cbOut )
{
	static const char* keys[] =
	{
		"mp_timelimit", "mp_fraglimit", "friendlyfire", "hostname", "map"
	};
	const char* values[5];
	int o = 0;

	values[0] = "30";
	values[1] = "0";
	values[2] = "0";
	values[3] = sv->hostname;
	values[4] = sv->map;
	PutLong( out, &o, 0xFFFFFFFF );
	out[o++] = 'E';
	PutShort( out, &o, 5 );
	for ( int i = 0; i < 5; i++ )
	{
		PutString( out, &o, keys[i] );
		PutString( out, &o, values[i] );
	}
	return o <= cbOut ? o : 0;
}

static DWORD WINAPI WonGameSv_ThreadProc( LPVOID )
{
	SOCKET	socks[16];
	int		sockSv[16];		// which g_servers[] entry each socket serves
	int		nSocks = 0;

	for ( int i = 0; i < g_nServers && nSocks < 16; i++ )
	{
		SOCKET	s = BindUdp( g_servers[i].port );
		if ( s != INVALID_SOCKET )
		{
			socks[nSocks]  = s;
			sockSv[nSocks] = i;		// a failed bind must not shift the mapping
			nSocks++;
		}
		else
		{
			printf( "wonserverd[gamesv]: bind UDP %u failed (%d)\n",
					g_servers[i].port, WSAGetLastError() );
		}
	}
	printf( "wonserverd[gamesv]: %d mock game servers on UDP %u-%u\n",
			nSocks, g_servers[0].port, g_servers[g_nServers - 1].port );
	fflush( stdout );

	for ( ;; )
	{
		fd_set	rfds;
		FD_ZERO( &rfds );
		for ( int i = 0; i < nSocks; i++ )
			FD_SET( socks[i], &rfds );

		struct timeval	tv;
		tv.tv_sec  = 0;
		tv.tv_usec = 200000;
		if ( select( 0, &rfds, NULL, NULL, &tv ) <= 0 )
			continue;

		for ( int i = 0; i < nSocks; i++ )
		{
			if ( !FD_ISSET( socks[i], &rfds ) )
				continue;

			char				query[512];
			struct sockaddr_in	from;
			int					fromlen = sizeof( from );
			int					n = recvfrom( socks[i], query, sizeof( query ) - 1, 0,
											  (struct sockaddr*)&from, &fromlen );
			if ( n < 5 )
				continue;
			query[n] = 0;

			// Connectionless header, then the command string.
			if ( (unsigned char)query[0] != 0xFF || (unsigned char)query[1] != 0xFF
			  || (unsigned char)query[2] != 0xFF || (unsigned char)query[3] != 0xFF )
				continue;

			const char*			pszCmd = query + 4;
			const MockServer*	sv     = &g_servers[sockSv[i]];

			// Latency probe: [long -1]["ping"] -> [long -1]['j'] (0x4150A0).  Without
			// it the browser only leaves its ping phase on the 5s timeout, once per
			// sample, and takes half a minute to reach the infostring request.
			if ( _strnicmp( pszCmd, "ping", 4 ) == 0 )
			{
				char	pong[5];
				pong[0] = (char)0xFF; pong[1] = (char)0xFF;
				pong[2] = (char)0xFF; pong[3] = (char)0xFF;
				pong[4] = 'j';
				sendto( socks[i], pong, 5, 0, (struct sockaddr*)&from, fromlen );
				continue;
			}

			char	reply[1400];
			int		cb = 0;
			if ( _strnicmp( pszCmd, "infostring", 10 ) == 0 )
				cb = BuildInfoReply( sv, reply, (int)sizeof( reply ) );
			else if ( _strnicmp( pszCmd, "players", 7 ) == 0 )
				cb = BuildPlayersReply( sv, reply, (int)sizeof( reply ) );
			else if ( _strnicmp( pszCmd, "rules", 5 ) == 0 )
				cb = BuildRulesReply( sv, reply, (int)sizeof( reply ) );
			if ( cb > 0 )
			{
				sendto( socks[i], reply, cb, 0, (struct sockaddr*)&from, fromlen );
				printf( "  game server UDP %u answered %.10s\n", sv->port, pszCmd );
				fflush( stdout );
			}
		}
	}
	// not reached
}

void WonGameSv_Start( void )
{
	if ( InterlockedCompareExchange( &g_started, 1, 0 ) == 0 )
		g_thread = CreateThread( NULL, 0, WonGameSv_ThreadProc, NULL, 0, NULL );
}
