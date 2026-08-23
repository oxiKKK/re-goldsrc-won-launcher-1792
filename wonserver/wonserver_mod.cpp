// wonserver_mod.cpp -- emulated WON mod-list and install-notify server.

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "wonserver_mod.h"

struct MockMod
{
	const char* gamedir;
	const char* game;
	const char* version;
	const char* size;
	const char* rating;
};

static const MockMod g_mods[] =
{
	{ "valve",   "Half-Life",          "1.1.0.8", "0", "5" },
	{ "cstrike", "Counter-Strike",      "1.3",     "0", "5" },
	{ "tfc",     "Team Fortress Classic", "1.5",   "0", "4" },
	{ "dod",     "Day of Defeat",       "1.0",     "0", "4" },
	{ "dmc",     "Deathmatch Classic",  "1.0",     "0", "4" }
};

static LONG g_started = 0;
static unsigned short g_port = 27011;

static int PutLong( unsigned char* out, int o, unsigned long value )
{
	out[o++] = (unsigned char)value;
	out[o++] = (unsigned char)( value >> 8 );
	out[o++] = (unsigned char)( value >> 16 );
	out[o++] = (unsigned char)( value >> 24 );
	return o;
}

static int PutShort( unsigned char* out, int o, unsigned short value )
{
	out[o++] = (unsigned char)value;
	out[o++] = (unsigned char)( value >> 8 );
	return o;
}

static int PutString( unsigned char* out, int o, const char* value )
{
	int len = (int)strlen( value ) + 1;
	memcpy( out + o, value, len );
	return o + len;
}

static int PutPair( unsigned char* out, int o, const char* key, const char* value )
{
	o = PutString( out, o, key );
	o = PutString( out, o, value );
	return o;
}

static int BuildModList( unsigned char* out )
{
	int o = 0;
	o = PutLong( out, o, 0xFFFFFFFF );
	out[o++] = 'o';
	out[o++] = 0;
	o = PutLong( out, o, 0 );
	o = PutShort( out, o, (unsigned short)( sizeof( g_mods ) / sizeof( g_mods[0] ) ) );
	for ( int i = 0; i < (int)( sizeof( g_mods ) / sizeof( g_mods[0] ) ); i++ )
	{
		o = PutPair( out, o, "gamedir", g_mods[i].gamedir );
		o = PutPair( out, o, "game", g_mods[i].game );
		o = PutPair( out, o, "version", g_mods[i].version );
		o = PutPair( out, o, "size", g_mods[i].size );
		o = PutPair( out, o, "rating", g_mods[i].rating );
		o = PutPair( out, o, "type", "multiplayer_only" );
		o = PutString( out, o, "" );
	}
	return o;
}

static DWORD WINAPI WonMod_ThreadProc( LPVOID )
{
	SOCKET sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if ( sock == INVALID_SOCKET )
		return 1;

	struct sockaddr_in addr;
	memset( &addr, 0, sizeof( addr ) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl( INADDR_ANY );
	addr.sin_port = htons( g_port );
	if ( bind( sock, (struct sockaddr*)&addr, sizeof( addr ) ) != 0 )
	{
		printf( "wonserverd[mod]: bind UDP %u failed (%d)\n", g_port,
			WSAGetLastError() );
		closesocket( sock );
		return 1;
	}

	printf( "wonserverd[mod]: listening on UDP %u\n", g_port );
	for ( ;; )
	{
		unsigned char query[512];
		struct sockaddr_in from;
		int fromLen = sizeof( from );
		int len = recvfrom( sock, (char*)query, sizeof( query ), 0,
			(struct sockaddr*)&from, &fromLen );
		if ( len < 1 )
			continue;

		if ( query[0] == 'n' )
		{
			unsigned char reply[4096];
			int replyLen = BuildModList( reply );
			sendto( sock, (const char*)reply, replyLen, 0,
				(struct sockaddr*)&from, fromLen );
		}
		else if ( query[0] == 'p' )
		{
			unsigned char reply[5] = { 0xFF, 0xFF, 0xFF, 0xFF, 'j' };
			sendto( sock, (const char*)reply, sizeof( reply ), 0,
				(struct sockaddr*)&from, fromLen );
		}
	}
}

void WonMod_Start( unsigned short nPort )
{
	g_port = nPort ? nPort : 27011;
	if ( InterlockedCompareExchange( &g_started, 1, 0 ) == 0 )
		CreateThread( NULL, 0, WonMod_ThreadProc, NULL, 0, NULL );
}
