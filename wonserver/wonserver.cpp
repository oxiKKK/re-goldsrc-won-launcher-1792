// wonserver.cpp -- the emulated WON server core.

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WriteBuffer.h"
#include "wonwire.h"
#include "wonserver.h"
#include "wonserver_udp.h"

#define WONSERVER_MAX_ROOMS		64

struct MockRoom
{
	char			szName[64];
	char			szIp[32];
	char			szPassword[64];
	unsigned short	nPort;
	int				nPlayers;
};

static MockRoom	g_rooms[WONSERVER_MAX_ROOMS];
static int		g_nRooms        = 0;
static int		g_bDefaultsDone = 0;
static unsigned short g_nServicePort = 6002;
static int		g_nNextFactoryPort = 0;

#define WONSERVER_FACTORY_FIRST_PORT	27100
#define WONSERVER_FACTORY_PORT_COUNT	16

void WonServer_SetServicePort( unsigned short nPort )
{
	g_nServicePort = nPort;
}

unsigned short WonServer_GetFactoryPort( int i )
{
	if ( i < 0 || i >= WONSERVER_FACTORY_PORT_COUNT )
		return 0;
	return (unsigned short)( WONSERVER_FACTORY_FIRST_PORT + i );
}

int WonServer_GetFactoryPortCount( void )
{
	return WONSERVER_FACTORY_PORT_COUNT;
}

void WonServer_ClearRooms( void )
{
	g_nRooms = 0;
	WonUdp_Clear();
}

int WonServer_GetRoomCount( void )
{
	return g_nRooms;
}

unsigned short WonServer_GetRoomPort( int i )
{
	if ( i < 0 || i >= g_nRooms )
		return 0;
	return g_rooms[i].nPort;
}

void WonServer_AddRoom( const char* pszName, const char* pszIp, unsigned short nPort, int nPlayers )
{
	if ( !pszName || !*pszName )
		return;

	// Match by name (update in place) -- mirrors CRoomList_AddRoom's find-or-create.
	int	idx = -1;
	for ( int i = 0; i < g_nRooms; i++ )
	{
		if ( _stricmp( g_rooms[i].szName, pszName ) == 0 )
		{
			idx = i;
			break;
		}
	}
	if ( idx < 0 )
	{
		if ( g_nRooms >= WONSERVER_MAX_ROOMS )
			return;
		idx = g_nRooms++;
	}

	MockRoom*	r = &g_rooms[idx];
	lstrcpynA( r->szName, pszName, sizeof( r->szName ) );
	lstrcpynA( r->szIp, ( pszIp && *pszIp ) ? pszIp : "127.0.0.1", sizeof( r->szIp ) );
	r->nPort    = nPort;
	r->nPlayers = nPlayers;
	r->szPassword[0] = 0;

	WonUdp_SetRoom( nPort, nPlayers );
	WonUdp_Sync();
}

static void WonServer_AddPrivateRoom( const char* pszName, unsigned short nPort,
	const char* pszPassword )
{
	WonServer_AddRoom( pszName, "127.0.0.1", nPort, 0 );
	for ( int i = 0; i < g_nRooms; i++ )
	{
		if ( g_rooms[i].nPort == nPort )
		{
			lstrcpynA( g_rooms[i].szPassword, pszPassword ? pszPassword : "",
				sizeof( g_rooms[i].szPassword ) );
			break;
		}
	}
}

int WonServer_CheckRoomPassword( unsigned short nPort, const char* pszPassword )
{
	for ( int i = 0; i < g_nRooms; i++ )
	{
		if ( g_rooms[i].nPort != nPort )
			continue;
		if ( !g_rooms[i].szPassword[0] )
			return 1;
		return pszPassword && !strcmp( g_rooms[i].szPassword, pszPassword );
	}
	return 1;
}

void WonServer_AddDefaultRooms( void )
{
	if ( g_bDefaultsDone )
		return;
	g_bDefaultsDone = 1;

	// A believable WON-era room set, all on loopback so the UDP status responder can
	// answer the player-count probe.  Ports are arbitrary loopback ports.
	WonServer_AddRoom( "Lobby",              "127.0.0.1", 27015, 24 );
	WonServer_AddRoom( "Counter-Strike",     "127.0.0.1", 27016, 47 );
	WonServer_AddRoom( "Team Fortress",      "127.0.0.1", 27017, 12 );
	WonServer_AddRoom( "Day of Defeat",      "127.0.0.1", 27018, 8 );
	WonServer_AddRoom( "Deathmatch Classic", "127.0.0.1", 27019, 5 );
	WonServer_AddRoom( "Newbie Help",        "127.0.0.1", 27020, 3 );
	WonServer_AddRoom( "Clan Wars",          "127.0.0.1", 27021, 31 );
	WonServer_AddRoom( "Europe",             "127.0.0.1", 27022, 18 );
}

static WriteBuffer	g_reply( 0x1000 );		// transaction replies (WonServer_Handle)
static WriteBuffer	g_chatMsg( 0x400 );		// chat-stream builders

static void WonServer_AppendRoomRecord( WriteBuffer& b, const MockRoom* r )
{
	b.appendByte( (BYTE)'S' );			// record type: service/room leaf
	b.appendWString( WonWide( r->szName ) );		// +04 first wstring
	b.appendWString( WonWide( "" ) );				// +14 second wstring
	b.appendWString( WonWide( r->szName ) );		// +24 THE DISPLAYED NAME
	b.appendLong( 0 );					// field6C
	b.appendLong( 0 );					// field70

	unsigned long	ipNet   = inet_addr( r->szIp );		// network order
	unsigned short	portNet = htons( r->nPort );		// network order

	// non-'D' tail (field order per WON_ParseDirReply 0x4095E0)
	b.appendWString( WonWide( "" ) );				// field34
	b.appendWString( WonWide( "" ) );				// field44
	b.appendWString( WonWide( "" ) );				// field54

	// 0x409684 reads the first short into the reused parameter slot and throws it
	// away; FetchRoomList's port comes from the SECOND short (+0x68, its `netshort`
	// local at esp+0xC0) and the address from the long that follows (+0x64).
	b.appendShort( 0 );				// discarded by the reader
	b.appendShort( portNet );			// the port the launcher uses
	b.appendLong( ipNet );				// addr

	// Trailing blob: the launcher only skips it, but a real reply carried the
	// address here too, and dirtest still reads it.
	b.appendShort( 6 );				// cbData
	b.append( &ipNet, 4 );
	b.append( &portNet, 2 );
}

static void WonServer_AppendFactoryRecord( WriteBuffer& b )
{
	MockRoom factory;

	memset( &factory, 0, sizeof( factory ) );
	strcpy( factory.szName, "TitanFactoryServer" );
	strcpy( factory.szIp, "127.0.0.1" );
	factory.nPort = g_nServicePort;

	b.appendByte( (BYTE)'S' );
	b.appendWString( WonWide( "" ) );
	b.appendWString( WonWide( "TitanFactoryServer" ) );
	b.appendWString( WonWide( "TitanFactoryServer" ) );
	b.appendLong( 0 );
	b.appendLong( 0 );
	b.appendWString( WonWide( "" ) );
	b.appendWString( WonWide( "" ) );
	b.appendWString( WonWide( "" ) );
	b.appendShort( 0 );
	b.appendShort( htons( factory.nPort ) );
	b.appendLong( inet_addr( factory.szIp ) );
	b.appendShort( 0 );
}

static int WonServer_BuildDirectoryReply( const BYTE** ppReply, int* pReplyLen )
{
	g_reply.rewind();
	g_reply.appendShort( 0 );						// status = success
	g_reply.appendShort( (unsigned short)g_nRooms );	// entry count
	for ( int i = 0; i < g_nRooms; i++ )
		WonServer_AppendRoomRecord( g_reply, &g_rooms[i] );

	// Rooms exist -> make sure the UDP status responder is live so the per-room
	// player-count probe in FetchRoomList's tail gets answered.
	WonUdp_Sync();

	*ppReply   = g_reply.getBuffer();
	*pReplyLen = g_reply.getSize();
	return 1;
}

static int WonServer_BuildFactoryDirectoryReply( const BYTE** ppReply, int* pReplyLen )
{
	g_reply.rewind();
	g_reply.appendShort( 0 );
	g_reply.appendShort( 1 );
	WonServer_AppendFactoryRecord( g_reply );
	*ppReply   = g_reply.getBuffer();
	*pReplyLen = g_reply.getSize();
	return 1;
}

static int WonServer_ReadWString( const BYTE* p, int len, int* pOffset,
	char* pszOut, int cbOut )
{
	int offset = *pOffset;
	int count;

	if ( offset + 2 > len )
		return 0;
	count = p[offset] | ( p[offset + 1] << 8 );
	offset += 2;
	if ( count < 0 || offset + count * 2 > len )
		return 0;
	if ( pszOut && cbOut > 0 )
	{
		int copy = count < cbOut - 1 ? count : cbOut - 1;
		WideCharToMultiByte( CP_ACP, 0, (const wchar_t*)( p + offset ), copy,
			pszOut, cbOut - 1, NULL, NULL );
		pszOut[copy] = 0;
	}
	*pOffset = offset + count * 2;
	return 1;
}

static int WonServer_ReadString( const BYTE* p, int len, int* pOffset,
	char* pszOut, int cbOut )
{
	int offset = *pOffset;
	int count;

	if ( offset + 2 > len )
		return 0;
	count = p[offset] | ( p[offset + 1] << 8 );
	offset += 2;
	if ( count < 0 || offset + count > len )
		return 0;
	if ( pszOut && cbOut > 0 )
	{
		int copy = count < cbOut - 1 ? count : cbOut - 1;
		memcpy( pszOut, p + offset, copy );
		pszOut[copy] = 0;
	}
	*pOffset = offset + count;
	return 1;
}

static int WonServer_BuildFactoryReply( const BYTE* pPayload, int nPayload,
	const BYTE** ppReply, int* pReplyLen )
{
	char serverName[64];
	char passwordArg[128];
	char roomName[64];
	int offset = 0;

	serverName[0] = 0;
	passwordArg[0] = 0;
	roomName[0] = 0;
	if ( !WonServer_ReadString( pPayload, nPayload, &offset,
		serverName, sizeof( serverName ) ) || offset + 1 > nPayload )
		return 0;
	++offset;
	if ( !WonServer_ReadString( pPayload, nPayload, &offset, passwordArg,
		sizeof( passwordArg ) ) || offset + 4 > nPayload )
		return 0;
	offset += 4;
	if ( !WonServer_ReadString( pPayload, nPayload, &offset, NULL, 0 )
	  || !WonServer_ReadWString( pPayload, nPayload, &offset, roomName,
			sizeof( roomName ) ) )
		return 0;

	unsigned short port = WonServer_GetFactoryPort(
		g_nNextFactoryPort++ % WonServer_GetFactoryPortCount() );
	const char* password = passwordArg;
	if ( !_strnicmp( password, "-password ", 10 ) )
		password += 10;
	WonServer_AddPrivateRoom( roomName[0] ? roomName : "New Room", port, password );

	g_reply.rewind();
	g_reply.appendShort( 2 );
	g_reply.appendByte( 1 );
	g_reply.appendShort( port );
	*ppReply   = g_reply.getBuffer();
	*pReplyLen = g_reply.getSize();
	return 1;
}

// The chat session id the join hands back; the client keeps it as its own member
// id (m_nChatSessionId) and stamps it into the local user row.
#define WONSERVER_CHAT_SESSION_ID	0x1001

int WonServer_BuildChatJoinReply( short nResult, long lMemberId,
	const BYTE** ppReply, int* pReplyLen )
{
	// JoinRoom reads [u16 result][u32 status]: a short alone leaves its ReadLong
	// failing, which it treats as a bad reply and retries.
	g_reply.rewind();
	g_reply.appendShort( (unsigned short)nResult );
	g_reply.appendLong( (unsigned long)lMemberId );
	*ppReply   = g_reply.getBuffer();
	*pReplyLen = g_reply.getSize();
	return 1;
}

int WonServer_Handle( const BYTE* pReq, int nReqLen,
					  const BYTE** ppReply, int* pReplyLen,
					  unsigned long* pReplySvc, unsigned long* pReplyMsg )
{
	const BYTE*		reply = NULL;
	int				replyLen = 0;
	unsigned long	rsvc = 0, rmsg = 0;
	if ( !ppReply )		ppReply = &reply;
	if ( !pReplyLen )	pReplyLen = &replyLen;
	if ( !pReplySvc )	pReplySvc = &rsvc;
	if ( !pReplyMsg )	pReplyMsg = &rmsg;

	*ppReply   = NULL;
	*pReplyLen = 0;

	// Request header: [u32 length][u32 service][u32 message][payload].
	if ( !pReq || nReqLen < 12 )
		return 0;

	unsigned long	svc = (unsigned long)pReq[4]
						| ( (unsigned long)pReq[5] << 8 )
						| ( (unsigned long)pReq[6] << 16 )
						| ( (unsigned long)pReq[7] << 24 );

	*pReplySvc = svc;
	unsigned long msg = (unsigned long)pReq[8]
		| ( (unsigned long)pReq[9] << 8 )
		| ( (unsigned long)pReq[10] << 16 )
		| ( (unsigned long)pReq[11] << 24 );

	switch ( svc )
	{
	case 30:									// directory
		*pReplyMsg = 3;
		if ( msg != 2 )
			return 0;
		{
			char path[128];
			int offset = 12;
			if ( WonServer_ReadWString( pReq, nReqLen, &offset, path, sizeof( path ) )
			  && !_stricmp( path, "/Half-Life" ) )
				return WonServer_BuildFactoryDirectoryReply( ppReply, pReplyLen );
		}
		return WonServer_BuildDirectoryReply( ppReply, pReplyLen );
	case 10:									// factory
		*pReplyMsg = 1;
		if ( msg != 2 )
			return 0;
		return WonServer_BuildFactoryReply( pReq + 12, nReqLen - 12,
			ppReply, pReplyLen );
	case 50:									// chat register/join
		*pReplyMsg = 1;
		if ( msg != 0 )
			return 0;
		return WonServer_BuildChatJoinReply( 0, WONSERVER_CHAT_SESSION_ID,
			ppReply, pReplyLen );
	default:	return 0;	// unhandled service -> caller's no-connection path
	}
}

const BYTE* WonServer_BuildMemberList( const char* const* ppszNicks, const long* pIds,
									   int nCount, int* pLen )
{
	g_chatMsg.rewind();
	g_chatMsg.appendShort( (unsigned short)nCount );
	for ( int i = 0; i < nCount; i++ )
	{
		g_chatMsg.appendLong( (unsigned long)( pIds ? pIds[i] : i + 1 ) );	// id (== status field)
		g_chatMsg.appendWString( WonWide( ppszNicks ? ppszNicks[i] : "" ) );
	}
	if ( pLen )	*pLen = g_chatMsg.getSize();
	return g_chatMsg.getBuffer();
}

const BYTE* WonServer_BuildChatText( long lFromId, int nFlag, const char* pszText, int* pLen )
{
	g_chatMsg.rewind();
	g_chatMsg.appendLong( (unsigned long)lFromId );
	g_chatMsg.appendShort( (unsigned short)nFlag );
	g_chatMsg.append( pszText, pszText ? (int)strlen( pszText ) : 0 );
	if ( pLen )	*pLen = g_chatMsg.getSize();
	return g_chatMsg.getBuffer();
}

const BYTE* WonServer_BuildUsersLeft( const long* pIds, int nCount, int* pLen )
{
	g_chatMsg.rewind();
	g_chatMsg.appendShort( (unsigned short)nCount );
	for ( int i = 0; i < nCount; i++ )
		g_chatMsg.appendLong( (unsigned long)( pIds ? pIds[i] : 0 ) );
	if ( pLen )	*pLen = g_chatMsg.getSize();
	return g_chatMsg.getBuffer();
}
