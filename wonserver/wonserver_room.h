// wonserver_room.h -- live per-room membership, so several launchers can share a
// room and see each other's joins, chat and departures.
//
// The client already implements every half of this (ServiceChat dispatches msg 2 =
// incremental add, 18 = full roster, 5 = users left, 7 = chat text), so the room
// state is purely a server concern.

#ifndef WONSERVER_ROOM_H
#define WONSERVER_ROOM_H

#include <winsock2.h>

void	Room_Init();
void	Room_Shutdown();

// Register a joiner.  pszNick comes off the join request (svc 50 msg 0); the room is
// identified by the port its listener is on.  Returns the member id handed back in
// the join reply, or 0 if the room is full.  On success the caller must push the
// roster with Room_PushRoster.
long	Room_Join( unsigned short nRoomPort, SOCKET s, const char* pszNick );

// Send the joiner the roster of everyone *else* (msg 18 -- the client adds itself
// from its own identity), then tell everyone else about the joiner (msg 2).
void	Room_PushRoster( unsigned short nRoomPort, SOCKET s );

// Relay one chat line (payload of svc 50 msg 7) to every member of the room,
// including the sender -- the launcher has no local echo, so the broadcast is what
// puts the line on screen.
void	Room_BroadcastChat( unsigned short nRoomPort, const unsigned char* pPayload, int cbPayload );

// Drop a member and tell the room (msg 5).  Safe to call for a socket that never
// joined.
void	Room_Leave( unsigned short nRoomPort, SOCKET s );

int		Room_HasMember( unsigned short nRoomPort, const char* pszNick );

#endif // WONSERVER_ROOM_H
