#ifndef WONSERVER_H
#define WONSERVER_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>

class CWONMsg;

#ifdef __cplusplus
extern "C" {
#endif

// Configure the emulated world.
void	WonServer_AddRoom( const char* pszName, const char* pszIp, unsigned short nPort, int nPlayers );
void	WonServer_ClearRooms( void );
int		WonServer_GetRoomCount( void );
// The TCP port room `i` is advertised on -- each room is its own chat server, so
// wonserverd must accept there too or JoinRoom stalls on connect.
unsigned short	WonServer_GetRoomPort( int i );
void	WonServer_SetServicePort( unsigned short nPort );
unsigned short	WonServer_GetFactoryPort( int i );
int		WonServer_GetFactoryPortCount( void );
int		WonServer_CheckRoomPassword( unsigned short nPort, const char* pszPassword );

// Seed a handful of believable default rooms (once).  Safe to call repeatedly.
void	WonServer_AddDefaultRooms( void );

// Transport dispatch.  Also reports the service/message type the reply must be
// framed with: TitanRequest::request rejects a reply whose header does not match
// what the caller asked for (svc 30 -> msg 3, svc 10 -> msg 1, svc 50 -> msg 1).
int		WonServer_Handle( const BYTE* pReq, int nReqLen,
						  const BYTE** ppReply, int* pReplyLen,
						  unsigned long* pReplySvc, unsigned long* pReplyMsg );

int		WonServer_BuildChatJoinReply( short nResult, long lMemberId,
								  const BYTE** ppReply, int* pReplyLen );

// Chat-stream builders -- the per-opcode payloads the chat dispatch handlers
// read (the outer dispatcher strips the opcode byte before calling them, so
// these start at the handler's first field).
const BYTE*	WonServer_BuildMemberList( const char* const* ppszNicks, const long* pIds,
									   int nCount, int* pLen );
// OnChatText (opcode 7): [u32 fromId][u16 flag][raw text bytes].  flag 0 = normal,
// -1 (0xFFFF) = server-error line, -2 (0xFFFE) = silently dropped.
const BYTE*	WonServer_BuildChatText( long lFromId, int nFlag, const char* pszText, int* pLen );
// OnUsersLeft (opcode 5): [u16 count]{ [u32 id] }.
const BYTE*	WonServer_BuildUsersLeft( const long* pIds, int nCount, int* pLen );


#ifdef __cplusplus
}
#endif

#endif // WONSERVER_H
