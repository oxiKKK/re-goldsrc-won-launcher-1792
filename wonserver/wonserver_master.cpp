// wonserver_master.cpp -- emulated WON master server.

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "wonserver_master.h"
#include "wonserver_gamesv.h"

#pragma comment( lib, "ws2_32.lib" )

static LONG				g_started = 0;
static unsigned short	g_port    = 27010;
static unsigned short	g_servicePort = 6002;

// Dump a packet as `hex  | ascii` rows of 16 bytes (so the real query is legible).
static void HexDump( const char* tag, const unsigned char* p, int n )
{
	printf( "wonserverd[master]: %s %d bytes\n", tag, n );
	for ( int row = 0; row < n; row += 16 )
	{
		printf( "  %04x  ", row );
		for ( int i = 0; i < 16; i++ )
			if ( row + i < n ) printf( "%02x ", p[row + i] );
			else               printf( "   " );
		printf( " |" );
		for ( int i = 0; i < 16 && row + i < n; i++ )
		{
			unsigned char c = p[row + i];
			putchar( ( c >= 32 && c < 127 ) ? c : '.' );
		}
		printf( "|\n" );
	}
	fflush( stdout );
}

// Append a {4 IP octets, 2-byte big-endian port} server record (the master wire form;
// ParseServerList/ParseVersionReply read the port with an ntohs byte-swap).
static int PutServerRecord( unsigned char* out, int o, const char* ipDotted, unsigned short port )
{
	unsigned long ip = inet_addr( ipDotted );		// network order (big-endian octets)
	const unsigned char* b = (const unsigned char*)&ip;
	out[o++] = b[0]; out[o++] = b[1]; out[o++] = b[2]; out[o++] = b[3];
	out[o++] = (unsigned char)( ( port >> 8 ) & 0xFF );	// big-endian port
	out[o++] = (unsigned char)( port & 0xFF );
	return o;
}

// Build a reply for a master query.
static int WonMaster_BuildReply( const unsigned char* query, int qlen, unsigned char* out )
{
	int	o = 0;
	out[o++] = 0xFF; out[o++] = 0xFF; out[o++] = 0xFF; out[o++] = 0xFF;	// connectionless

	unsigned char op = qlen > 0 ? query[0] : 0;

	if ( op == 'e' || op == '1' )		// server-list batch request
	{
		out[o++] = 'f';					// command
		out[o++] = 0x0a;				// header byte (canonical GoldSrc separator)
		out[o++] = 0; out[o++] = 0; out[o++] = 0; out[o++] = 0;	// resume token = 0 (end)
		// The mock game servers -- they answer the browser's infostring query.
		for ( int i = 0; i < WonGameSv_Count(); i++ )
			o = PutServerRecord( out, o, "127.0.0.1", WonGameSv_Port( i ) );
		// No 0.0.0.0 terminator: the resume token above already signals the end, and
		// ParseServerList adds every record it reads -- the terminator showed up as a
		// bogus "0.0.0.0  0/0" row in the browser.
	}
	else								// version / special-server bootstrap
	{
		out[o++] = 'w';					// command
		out[o++] = 0;					// body byte (NOT "outofdate" -> version OK)
		// Group 0: auth servers.
		o = PutServerRecord( out, o, "127.0.0.1", g_servicePort );
		out[o++]=0xFF; out[o++]=0xFF; out[o++]=0xFF; out[o++]=0xFF;	// group separator
		// Group 1: Titan/directory servers.
		o = PutServerRecord( out, o, "127.0.0.1", g_servicePort );
		out[o++]=0xFF; out[o++]=0xFF; out[o++]=0xFF; out[o++]=0xFF;
		// Group 2: master servers.
		o = PutServerRecord( out, o, "127.0.0.1", g_port );
		// ParseVersionReply's size arithmetic leaves a four-byte trailer after
		// three groups and two separators.
		out[o++] = 0; out[o++] = 0; out[o++] = 0; out[o++] = 0;
	}
	return o;
}

static bool SendAll( SOCKET s, const char* p, int n )
{
	int sent = 0;
	while ( sent < n )
	{
		int wrote = send( s, p + sent, n - sent, 0 );
		if ( wrote <= 0 )
			return false;
		sent += wrote;
	}
	return true;
}

static int PutCString( unsigned char* out, int offset, const char* value )
{
	int len = (int)strlen( value ) + 1;
	memcpy( out + offset, value, len );
	return offset + len;
}

static int WonMaster_BuildModStatsReply( unsigned char* out )
{
	int o = 0;
	out[o++] = 0xFF; out[o++] = 0xFF; out[o++] = 0xFF; out[o++] = 0xFF;
	out[o++] = 'y';
	out[o++] = 0;
	o = PutCString( out, o, "valve" );
	o = PutCString( out, o, "2" );
	o = PutCString( out, o, "36" );
	o = PutCString( out, o, "cstrike" );
	o = PutCString( out, o, "2" );
	o = PutCString( out, o, "24" );
	o = PutCString( out, o, "tfc" );
	o = PutCString( out, o, "1" );
	o = PutCString( out, o, "14" );
	o = PutCString( out, o, "dod" );
	o = PutCString( out, o, "1" );
	o = PutCString( out, o, "21" );
	o = PutCString( out, o, "end-of-list" );
	return o;
}

static DWORD WINAPI WonMaster_TcpThreadProc( LPVOID )
{
	SOCKET listener = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( listener == INVALID_SOCKET )
		return 1;

	BOOL exclusive = TRUE;
	setsockopt( listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
		(const char*)&exclusive, sizeof( exclusive ) );
	struct sockaddr_in addr;
	memset( &addr, 0, sizeof( addr ) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl( INADDR_ANY );
	addr.sin_port = htons( g_port );
	if ( bind( listener, (struct sockaddr*)&addr, sizeof( addr ) ) != 0
	  || listen( listener, SOMAXCONN ) != 0 )
	{
		printf( "wonserverd[master]: bind TCP %u failed (%d)\n", g_port,
			WSAGetLastError() );
		closesocket( listener );
		return 1;
	}

	printf( "wonserverd[master]: mod statistics on TCP %u\n", g_port );
	for ( ;; )
	{
		SOCKET client = accept( listener, NULL, NULL );
		if ( client == INVALID_SOCKET )
			continue;
		char request[512];
		int got = recv( client, request, sizeof( request ), 0 );
		if ( got > 0 && ( request[0] == 'x' || request[0] == 0 ) )
		{
			unsigned char reply[1024];
			int len = WonMaster_BuildModStatsReply( reply );
			SendAll( client, (const char*)reply, len );
		}
		closesocket( client );
	}
}

static DWORD WINAPI WonMaster_ThreadProc( LPVOID )
{
	WSADATA	wsa; WSAStartup( MAKEWORD( 2, 2 ), &wsa );

	SOCKET	s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( s == INVALID_SOCKET )
	{
		printf( "wonserverd[master]: socket() failed\n" );
		return 1;
	}

	BOOL reuse = TRUE;
	setsockopt( s, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof( reuse ) );

	struct sockaddr_in addr;
	memset( &addr, 0, sizeof( addr ) );
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl( INADDR_ANY );
	addr.sin_port        = htons( g_port );
	if ( bind( s, (struct sockaddr*)&addr, sizeof( addr ) ) != 0 )
	{
		printf( "wonserverd[master]: bind UDP %u failed (%d)\n", g_port, WSAGetLastError() );
		closesocket( s );
		return 1;
	}

	printf( "wonserverd[master]: listening on UDP %u\n", g_port );

	for ( ;; )
	{
		unsigned char		q[2048];
		struct sockaddr_in	from;
		int					fl = sizeof( from );
		int					n = recvfrom( s, (char*)q, sizeof( q ), 0, (struct sockaddr*)&from, &fl );
		if ( n <= 0 )
			continue;

		printf( "wonserverd[master]: query from %s:%u\n",
				inet_ntoa( from.sin_addr ), ntohs( from.sin_port ) );
		HexDump( "<<", q, n );

		unsigned char	reply[1024];
		int				rn = WonMaster_BuildReply( q, n, reply );
		if ( rn > 0 )
		{
			sendto( s, (const char*)reply, rn, 0, (struct sockaddr*)&from, fl );
			HexDump( ">>", reply, rn );
		}
	}
	// not reached
}

void WonMaster_Start( unsigned short nPort, unsigned short nServicePort )
{
	g_port = nPort ? nPort : 27010;
	g_servicePort = nServicePort ? nServicePort : 6002;
	if ( InterlockedCompareExchange( &g_started, 1, 0 ) == 0 )
	{
		CreateThread( NULL, 0, WonMaster_ThreadProc, NULL, 0, NULL );
		CreateThread( NULL, 0, WonMaster_TcpThreadProc, NULL, 0, NULL );
	}
}
