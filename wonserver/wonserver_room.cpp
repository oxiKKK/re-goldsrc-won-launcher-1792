// wonserver_room.cpp -- live per-room membership (see wonserver_room.h).

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "wonserver_room.h"
#include "wonserver.h"

#define ROOM_MAX_MEMBERS	32

struct RoomMember
{
	SOCKET		sock;
	long		id;
	std::string	nick;
};

struct Room
{
	unsigned short			port;
	std::vector<RoomMember>	members;
};

static std::vector<Room>	g_rooms;
static CRITICAL_SECTION		g_lock;
static bool					g_ready   = false;
static long					g_nextId  = 0x1001;		// member ids the client keys chat off

void Room_Init()
{
	if ( g_ready )
		return;
	InitializeCriticalSection( &g_lock );
	g_ready = true;
}

void Room_Shutdown()
{
	if ( !g_ready )
		return;
	g_ready = false;
	DeleteCriticalSection( &g_lock );
}

static Room* FindRoom( unsigned short port )
{
	for ( size_t i = 0; i < g_rooms.size(); i++ )
		if ( g_rooms[i].port == port )
			return &g_rooms[i];

	Room r;
	r.port = port;
	g_rooms.push_back( r );
	return &g_rooms.back();
}

// One framed push: [u32 12+len][u32 svc][u32 msg][payload].  Sends are serialised by
// the room lock, so two pushes never interleave on one socket.
static void SendTo( SOCKET s, unsigned long svc, unsigned long msg,
					const unsigned char* p, int n )
{
	int				total = 12 + n;
	unsigned char	hdr[12];
	hdr[0] = (unsigned char)( total       & 0xFF );
	hdr[1] = (unsigned char)( total >> 8  & 0xFF );
	hdr[2] = (unsigned char)( total >> 16 & 0xFF );
	hdr[3] = (unsigned char)( total >> 24 & 0xFF );
	hdr[4] = (unsigned char)( svc         & 0xFF );
	hdr[5] = (unsigned char)( svc  >> 8   & 0xFF );
	hdr[6] = (unsigned char)( svc  >> 16  & 0xFF );
	hdr[7] = (unsigned char)( svc  >> 24  & 0xFF );
	hdr[8]  = (unsigned char)( msg        & 0xFF );
	hdr[9]  = (unsigned char)( msg >> 8   & 0xFF );
	hdr[10] = (unsigned char)( msg >> 16  & 0xFF );
	hdr[11] = (unsigned char)( msg >> 24  & 0xFF );

	if ( send( s, (const char*)hdr, 12, 0 ) != 12 )
		return;
	int sent = 0;
	while ( sent < n )
	{
		int w = send( s, (const char*)p + sent, n - sent, 0 );
		if ( w <= 0 )
			return;
		sent += w;
	}
}

// Two launchers on one machine share the same profile, so both arrive as "Player".
// Disambiguate here rather than diverging the client.
static std::string UniqueNick( const Room* pRoom, const char* pszNick )
{
	std::string base = ( pszNick && *pszNick ) ? pszNick : "Player";
	std::string want = base;

	for ( int suffix = 2; suffix < 100; suffix++ )
	{
		bool taken = false;
		for ( size_t i = 0; i < pRoom->members.size(); i++ )
			if ( pRoom->members[i].nick == want ) { taken = true; break; }
		if ( !taken )
			return want;

		char buf[64];
		_snprintf( buf, sizeof( buf ) - 1, " (%d)", suffix );
		buf[sizeof( buf ) - 1] = 0;
		want = base + buf;
	}
	return want;
}

long Room_Join( unsigned short nRoomPort, SOCKET s, const char* pszNick )
{
	if ( !g_ready )
		return 0;

	long id = 0;
	EnterCriticalSection( &g_lock );
	{
		Room* pRoom = FindRoom( nRoomPort );
		if ( (int)pRoom->members.size() < ROOM_MAX_MEMBERS )
		{
			RoomMember m;
			m.sock = s;
			m.id   = g_nextId++;
			m.nick = UniqueNick( pRoom, pszNick );
			pRoom->members.push_back( m );
			id = m.id;
			printf( "  room %u: \"%s\" joined as id 0x%lX (%d in room)\n",
					nRoomPort, m.nick.c_str(), (unsigned long)m.id,
					(int)pRoom->members.size() );
		}
	}
	LeaveCriticalSection( &g_lock );
	return id;
}

void Room_PushRoster( unsigned short nRoomPort, SOCKET s )
{
	if ( !g_ready )
		return;

	EnterCriticalSection( &g_lock );
	{
		Room* pRoom = FindRoom( nRoomPort );

		// Everyone except the joiner -- OnMemberList(reset) adds "self" from the
		// client's own identity record, so including them would duplicate the row.
		std::vector<const char*>	nicks;
		std::vector<long>			ids;
		const RoomMember*			pJoiner = NULL;

		for ( size_t i = 0; i < pRoom->members.size(); i++ )
		{
			if ( pRoom->members[i].sock == s ) { pJoiner = &pRoom->members[i]; continue; }
			nicks.push_back( pRoom->members[i].nick.c_str() );
			ids.push_back( pRoom->members[i].id );
		}

		int			len = 0;
		const BYTE*	p = WonServer_BuildMemberList( nicks.empty() ? NULL : &nicks[0],
												   ids.empty()   ? NULL : &ids[0],
												   (int)nicks.size(), &len );
		if ( p )
			SendTo( s, 50, 18, p, len );

		// Tell the incumbents about the newcomer (msg 2 = incremental add).
		if ( pJoiner )
		{
			const char*	one   = pJoiner->nick.c_str();
			long		oneId = pJoiner->id;
			p = WonServer_BuildMemberList( &one, &oneId, 1, &len );
			if ( p )
			{
				for ( size_t i = 0; i < pRoom->members.size(); i++ )
					if ( pRoom->members[i].sock != s )
						SendTo( pRoom->members[i].sock, 50, 2, p, len );
			}
		}
	}
	LeaveCriticalSection( &g_lock );
}

void Room_BroadcastChat( unsigned short nRoomPort, const unsigned char* pPayload, int cbPayload )
{
	if ( !g_ready || !pPayload || cbPayload <= 0 )
		return;

	EnterCriticalSection( &g_lock );
	{
		Room* pRoom = FindRoom( nRoomPort );
		for ( size_t i = 0; i < pRoom->members.size(); i++ )
			SendTo( pRoom->members[i].sock, 50, 7, pPayload, cbPayload );
		printf( "  room %u: relayed chat to %d member(s)\n",
				nRoomPort, (int)pRoom->members.size() );
	}
	LeaveCriticalSection( &g_lock );
}

void Room_Leave( unsigned short nRoomPort, SOCKET s )
{
	if ( !g_ready )
		return;

	EnterCriticalSection( &g_lock );
	{
		Room*	pRoom = FindRoom( nRoomPort );
		long	goneId = 0;

		for ( size_t i = 0; i < pRoom->members.size(); i++ )
		{
			if ( pRoom->members[i].sock != s )
				continue;
			goneId = pRoom->members[i].id;
			printf( "  room %u: \"%s\" left\n", nRoomPort, pRoom->members[i].nick.c_str() );
			pRoom->members.erase( pRoom->members.begin() + i );
			break;
		}

		if ( goneId )
		{
			int			len = 0;
			const BYTE*	p = WonServer_BuildUsersLeft( &goneId, 1, &len );
			if ( p )
			{
				for ( size_t i = 0; i < pRoom->members.size(); i++ )
					SendTo( pRoom->members[i].sock, 50, 5, p, len );
			}
		}
	}
	LeaveCriticalSection( &g_lock );
}

int Room_HasMember( unsigned short nRoomPort, const char* pszNick )
{
	int found = 0;

	if ( !g_ready || !pszNick || !*pszNick )
		return 0;

	EnterCriticalSection( &g_lock );
	Room* pRoom = FindRoom( nRoomPort );
	for ( size_t i = 0; i < pRoom->members.size(); i++ )
	{
		if ( !_stricmp( pRoom->members[i].nick.c_str(), pszNick ) )
		{
			found = 1;
			break;
		}
	}
	LeaveCriticalSection( &g_lock );
	return found;
}
