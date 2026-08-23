// dirtest.cpp -- fetch the room directory from wonserverd through the launcher's own
// WON code path, and print what it parsed.
//
//   wondirtest [host] [port]      (default localhost 6002)

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "TitanRequest.h"
#include "won_dir.h"
#include "won_msg.h"

#pragma comment( lib, "ws2_32.lib" )

// The logger the WON glue calls.
void Console_Printf( char* fmt, ... )
{
	va_list	ap;
	va_start( ap, fmt );
	vprintf( fmt, ap );
	va_end( ap );
}

// Two globals cryptapi.cpp normally owns; this harness does not run CryptApi_Init.
int		g_authIsServer = 0;
char*	( *Callback_GetLocalizedString )( unsigned int ) = NULL;

int main( int argc, char** argv )
{
	const char*	host = ( argc > 1 ) ? argv[1] : "localhost";
	int			port = ( argc > 2 ) ? atoi( argv[2] ) : 6002;

	WSADATA	wsa;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsa ) != 0 )
	{
		printf( "WSAStartup failed\n" );
		return 1;
	}

	printf( "wondirtest: GetDirectory \"%ls\" from %s:%d\n\n", WON_DIR_PUBLIC, host, port );

	TitanRequest	request( host, port );
	CWONMsg			reply;

	int	nCount = WONComm_GetDirectory( &request, WON_DIR_PUBLIC, &reply );
	if ( nCount <= 0 )
	{
		printf( "\nFAILED: no directory entries (is wonserverd running?)\n" );
		return 1;
	}

	printf( "directory reported %d entries\n\n", nCount );

	int	nParsed = 0;
	for ( int i = 0; i < nCount; i++ )
	{
		direntry_t	entry;
		memset( &entry, 0, sizeof( entry ) );

		if ( !WON_ParseDirReply( &reply, &entry ) )
		{
			printf( "  entry %d: PARSE FAILED\n", i );
			return 1;
		}
		++nParsed;

		if ( entry.m_type != ET_SERVICE )
			continue;

		struct in_addr	addr;
		addr.S_un.S_addr = entry.m_addr;
		printf( "  '%c' %-22ls %s:%d\n", entry.m_type, entry.m_wsName,
				inet_ntoa( addr ), (int)ntohs( entry.m_port ) );
	}

	printf( "\nparsed %d/%d entries OK\n", nParsed, nCount );
	return ( nParsed == nCount ) ? 0 : 1;
}
