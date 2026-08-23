// wonserverd_main.cpp -- the standalone WON server emulator executable.

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WriteBuffer.h"
#include "wonwire.h"
#include "wonserver.h"
#include "wonserver_udp.h"
#include "wonserver_master.h"
#include "wonserver_gamesv.h"
#include "wonserver_mod.h"
#include "wonserver_auth.h"
#include "wonserver_room.h"

#pragma comment( lib, "ws2_32.lib" )

// The launcher logger symbol the hlwon/won glue references; standalone we just print.
extern "C" void Console_Printf( char* fmt, ... )
{
	va_list ap; va_start( ap, fmt );
	vprintf( fmt, ap );
	va_end( ap );
}

#define WONSERVERD_DEFAULT_PORT		6002
#define WONSERVERD_MAX_MSG			0x100000

// On by default now that svc 203 + the encrypted envelope are implemented: the
// stock launcher needs a certificate before it will talk to us at all.  "-noauth"
// refuses service 202 instead, which makes the launcher fall back to plaintext.
static bool	g_bAuthEnabled = true;
static CRITICAL_SECTION g_coreReplyLock;

static bool RecvAll( SOCKET s, char* buf, int n )
{
	int got = 0;
	while ( got < n )
	{
		int r = recv( s, buf + got, n - got, 0 );
		if ( r <= 0 ) return false;
		got += r;
	}
	return true;
}

static bool SendAll( SOCKET s, const char* buf, int n )
{
	int sent = 0;
	while ( sent < n )
	{
		int w = send( s, buf + sent, n - sent, 0 );
		if ( w <= 0 ) return false;
		sent += w;
	}
	return true;
}

static bool SendFramed( SOCKET s, unsigned long svc, unsigned long msg,
						const BYTE* payload, int payloadLen );

// Reply on an established session channel, mirroring WON_BuildRequest (0x465a30):
//   [u32 len][u8 flags=2][u16 sessionId][BF( [u16 seq][u32 svc][u32 msg][payload] )]
// The sequence number lives inside the ciphertext and counts up per message.
static bool SendEncrypted( SOCKET s, void* authSess, unsigned long svc, unsigned long msg,
						   const BYTE* payload, int payloadLen, unsigned short* pSendSeq )
{
	unsigned short	sessionId = 0;
	if ( !WonAuth_SessionChannelKey( authSess, NULL, &sessionId ) )
		return false;

	WriteBuffer	inner( 16 + payloadLen );
	inner.appendShort( (*pSendSeq)++ );
	inner.appendLong( svc );
	inner.appendLong( msg );
	inner.append( payload, payloadLen );

	BYTE*	cipher = NULL;
	int		cbCipher = 0;
	if ( !WonAuth_SessionEncrypt( authSess, inner.getBuffer(), inner.getSize(),
								  &cipher, &cbCipher ) )
	{
		printf( "  [chan] encrypt FAILED (inner %d bytes)\n", inner.getSize() );
		return false;
	}
	printf( "  [chan] reply svc=%lu msg=%lu seq=%u sid=%u inner=%d cipher=%d\n",
			svc, msg, (unsigned)( *pSendSeq - 1 ), (unsigned)sessionId,
			inner.getSize(), cbCipher );
	{
		// inner = [u16 seq][u32 svc][u32 msg][u16 status][u16 count][records...]
		// so the first record's type byte lands at offset 14.
		const unsigned char* ib = (const unsigned char*)inner.getBuffer();
		printf( "  [chan] inner[10..29]:" );
		for ( int k = 10; k < 30 && k < inner.getSize(); k++ )
			printf( " %02X", ib[k] );
		printf( "\n" );
	}

	// Mirror the client's own request layout, which decodes cleanly at flags[4] /
	// ciphertext[7]: [u32 len][u8 flags][u16 sessionId][ciphertext].
	WriteBuffer	out( 8 + cbCipher );
	out.appendLong( (unsigned long)( 7 + cbCipher ) );
	out.appendByte( 2 );								// flags: encrypted
	out.appendShort( sessionId );
	out.append( cipher, cbCipher );
	delete[] cipher;

	return SendAll( s, (const char*)out.getBuffer(), out.getSize() );
}

// Serve one accepted connection: read framed requests, dispatch, write framed
// replies, until the peer disconnects.  The listener port doubles as the room id.
struct ConnCtx
{
	SOCKET			sock;
	unsigned short	port;
};

static DWORD WINAPI ServeConn( LPVOID param )
{
	ConnCtx*		ctx = (ConnCtx*)param;
	SOCKET			s = ctx->sock;
	unsigned short	roomPort = ctx->port;
	free( ctx );

	void*	authSess = WonAuth_SessionCreate();		// per-connection auth state (svc 202/203)
	unsigned short	recvSeq = 0;					// sequence carried inside the ciphertext
	unsigned short	sendSeq = 1;					// WON_Authenticate starts both at 1
	bool			bEncrypted = false;				// set per request, below

	for ( ;; )
	{
		bEncrypted = false;

		// [u32 totalLen]
		unsigned char	lenBuf[4];
		if ( !RecvAll( s, (char*)lenBuf, 4 ) )
			break;
		unsigned int total = lenBuf[0] | (lenBuf[1]<<8) | (lenBuf[2]<<16) | ((unsigned)lenBuf[3]<<24);
		if ( total < 12 || total > WONSERVERD_MAX_MSG )
			break;

		// Reassemble the full request ([len][svc][msg][payload]) for WonServer_Handle.
		char*	req = (char*)malloc( total );
		if ( !req ) break;
		memcpy( req, lenBuf, 4 );
		if ( !RecvAll( s, req + 4, (int)total - 4 ) ) { free( req ); break; }

		// Once the svc 203 handshake has run, the client wraps every message:
		//   [u32 len][u8 flags=2][u16 sessionId][BF( [u16 seq][u32 svc][u32 msg][payload] )]
		// Unwrap in place so the dispatch below stays plaintext.
		BYTE*	plain = NULL;

		// A flagged request on a connection that never handshook is a resumed
		// session: adopt its key by the id in the clear header.  "List rooms" opens
		// a fresh socket and quotes the id instead of repeating svc 203; without
		// this the ciphertext gets parsed as a plaintext svc/msg pair and the
		// launcher reports "Could not obtain room list".
		if ( (unsigned char)req[4] == 2 && !WonAuth_SessionChannelKey( authSess, NULL, NULL ) )
		{
			unsigned short	sid = (unsigned short)( (unsigned char)req[5]
								| ( (unsigned char)req[6] << 8 ) );
			if ( WonAuth_SessionAdopt( authSess, sid ) )
				printf( "  [chan] resumed session sid=%u on a new connection\n", (unsigned)sid );
			else
				printf( "  [chan] unknown session sid=%u\n", (unsigned)sid );
		}

		if ( WonAuth_SessionChannelKey( authSess, NULL, NULL ) && (unsigned char)req[4] == 2 )
		{
			int		cbPlain = 0;
			if ( !WonAuth_SessionDecrypt( authSess, (const BYTE*)req + 7, (int)total - 7,
										  &plain, &cbPlain ) || cbPlain < 10 )
			{
				printf( "  decrypt failed (%d bytes)\n", (int)total - 7 );
				free( req ); if ( plain ) delete[] plain; break;
			}
			recvSeq = (unsigned short)( plain[0] | ( plain[1] << 8 ) );

			// The client's counter keeps running across connections (a resumed
			// session's first request arrived as seq 7, not 1), and it drops any
			// reply whose sequence does not match the request it answers.  Echo it
			// rather than running a counter of our own, which resets per socket.
			sendSeq = recvSeq;
			printf( "  [chan] req sid=%u cbCipher=%d plain=%d first12:",
					(unsigned)( (unsigned char)req[5] | ( (unsigned char)req[6] << 8 ) ),
					(int)total - 7, cbPlain );
			for ( int k = 0; k < 12 && k < cbPlain; k++ )
				printf( " %02X", plain[k] );
			printf( "\n" );

			// Re-shape as [len][svc][msg][payload] so nothing downstream changes.
			int	newTotal = 12 + ( cbPlain - 10 );
			char*	rebuilt = (char*)malloc( newTotal );
			if ( !rebuilt ) { free( req ); delete[] plain; break; }
			rebuilt[0] = (char)( newTotal & 0xFF );
			rebuilt[1] = (char)( ( newTotal >> 8 ) & 0xFF );
			rebuilt[2] = (char)( ( newTotal >> 16 ) & 0xFF );
			rebuilt[3] = (char)( ( newTotal >> 24 ) & 0xFF );
			memcpy( rebuilt + 4, plain + 2, cbPlain - 2 );
			delete[] plain; plain = NULL;
			free( req );
			req   = rebuilt;
			total = (unsigned int)newTotal;
			bEncrypted = true;
		}

		unsigned int svc = (unsigned char)req[4] | ((unsigned char)req[5]<<8)
						  | ((unsigned char)req[6]<<16) | ((unsigned)(unsigned char)req[7]<<24);
		unsigned int msg = (unsigned char)req[8] | ((unsigned char)req[9]<<8)
						  | ((unsigned char)req[10]<<16) | ((unsigned)(unsigned char)req[11]<<24);

		const BYTE*		payload = NULL;
		int				payloadLen = 0;
		unsigned long	rsvc = svc, rmsg = msg;		// reply header (auth may override)
		int				ok;
		long			joinedId = 0;
		bool			bOneWay = false;
		bool			bCoreReply = false;
		BYTE*			ownedReply = NULL;

		if ( svc == 202 && !g_bAuthEnabled )
		{
			static const BYTE	rgDecline[4] = { 1, 0, 0, 0 };	// [u16 status=1][u16 len=0]
			payload    = rgDecline;
			payloadLen = sizeof( rgDecline );
			rsvc       = 202;
			rmsg       = 2;
			ok         = 1;
		}
		else if ( svc == 202 )	// the WONCrypt login handshake
			ok = WonAuth_SessionHandle( authSess, msg, (const BYTE*)req + 12, (int)total - 12,
										&payload, &payloadLen, &rsvc, &rmsg );
		else if ( svc == 203 )	// the peer handshake that establishes the channel key
			ok = WonAuth_PeerHandle( authSess, msg, (const BYTE*)req + 12, (int)total - 12,
									 &payload, &payloadLen, &rsvc, &rmsg );
		else if ( svc == 50 && msg == 0 )
		{
			char szNick[256];
			char szPassword[256];
			WonWire_ReadJoin( (const BYTE*)req + 12, (int)total - 12,
				szNick, sizeof( szNick ), szPassword, sizeof( szPassword ) );
			rsvc = 50;
			rmsg = 1;
			EnterCriticalSection( &g_coreReplyLock );
			bCoreReply = true;
			if ( !WonServer_CheckRoomPassword( roomPort, szPassword ) )
				ok = WonServer_BuildChatJoinReply( -1103, 0, &payload, &payloadLen );
			else
			{
				joinedId = Room_Join( roomPort, s, szNick );
				ok = WonServer_BuildChatJoinReply( joinedId ? 0 : -1, joinedId,
					&payload, &payloadLen );
			}
		}
		else if ( svc == 50 && msg == 7 )
		{
			ok = 1;
			bOneWay = true;
		}
		else
		{
			EnterCriticalSection( &g_coreReplyLock );
			bCoreReply = true;
			ok = WonServer_Handle( (const BYTE*)req, (int)total, &payload, &payloadLen,
								   &rsvc, &rmsg );
		}

		if ( bCoreReply )
		{
			if ( ok && payloadLen > 0 )
			{
				ownedReply = (BYTE*)malloc( payloadLen );
				if ( ownedReply )
				{
					memcpy( ownedReply, payload, payloadLen );
					payload = ownedReply;
				}
				else
				{
					ok = 0;
				}
			}
			LeaveCriticalSection( &g_coreReplyLock );
		}

		printf( "  request svc=%u msg=%u -> %s (%d byte payload) reply svc=%lu msg=%lu enc=%d\n",
				svc, msg, ok ? "ok" : "unhandled", ok ? payloadLen : 0,
				rsvc, rmsg, (int)bEncrypted );
		fflush( stdout );

		if ( ok )
		{
			if ( !bOneWay )
			{
				if ( bEncrypted )
					SendEncrypted( s, authSess, rsvc, rmsg, payload, payloadLen, &sendSeq );
				else
				{
					// Frame the reply: [u32 12+payloadLen][u32 svc][u32 msg][payload].
					WriteBuffer	out( 12 + payloadLen );
					out.appendLong( (unsigned long)( 12 + payloadLen ) );
					out.appendLong( rsvc );
					out.appendLong( rmsg );
					out.append( payload, payloadLen );
					SendAll( s, (const char*)out.getBuffer(), out.getSize() );
				}
			}

			// A successful room join (svc 50 / msg 0) registers the client in the
			// room, then pushes the live roster: msg 18 to the joiner and msg 2 to
			// everyone already there.
			if ( joinedId )
				Room_PushRoster( roomPort, s );

			// Chat text from the client (svc 50 / msg 7) carries
			// [u32 senderId][u16 -2][text] -- exactly the shape OnChatText reads.
			// Relay it to the whole room, sender included: the launcher has no
			// local echo, so the broadcast is what puts the line on screen.
			if ( svc == 50 && msg == 7 && (int)total > 12 )
				Room_BroadcastChat( roomPort, (const BYTE*)req + 12, (int)total - 12 );
		}
		if ( ownedReply )
			free( ownedReply );
		free( req );

		if ( !ok )
			break;	// unhandled service: drop the connection
	}

	Room_Leave( roomPort, s );			// tells the room (msg 5)
	WonAuth_SessionDestroy( authSess );
	closesocket( s );
	return 0;
}

// Write one framed Titan message: [u32 12+len][u32 svc][u32 msg][payload].
static bool SendFramed( SOCKET s, unsigned long svc, unsigned long msg,
						const BYTE* payload, int payloadLen )
{
	WriteBuffer	out( 12 + payloadLen );

	out.appendLong( (unsigned long)( 12 + payloadLen ) );
	out.appendLong( svc );
	out.appendLong( msg );
	out.append( payload, payloadLen );

	return SendAll( s, (const char*)out.getBuffer(), out.getSize() );
}

// Accept loop for one listening socket; one thread per listener.
static DWORD WINAPI AcceptLoop( LPVOID param )
{
	ConnCtx*		lctx = (ConnCtx*)param;
	SOCKET			listenSock = lctx->sock;
	unsigned short	listenPort = lctx->port;
	free( lctx );

	for ( ;; )
	{
		struct sockaddr_in	peer;
		int					plen = sizeof( peer );
		SOCKET				c = accept( listenSock, (struct sockaddr*)&peer, &plen );
		if ( c == INVALID_SOCKET )
			continue;

		printf( "wonserverd: connection from %s:%u\n",
				inet_ntoa( peer.sin_addr ), ntohs( peer.sin_port ) );

		ConnCtx*	ctx = (ConnCtx*)malloc( sizeof( ConnCtx ) );
		if ( !ctx ) { closesocket( c ); continue; }
		ctx->sock = c;
		ctx->port = listenPort;

		HANDLE	h = CreateThread( NULL, 0, ServeConn, (LPVOID)ctx, 0, NULL );
		if ( h )	CloseHandle( h );
		else		{ free( ctx ); closesocket( c ); }
	}
	return 0;
}

// Bind + listen on one TCP port and start its accept thread.  Returns false (and
// explains) when the port is taken, so a second instance cannot silently shadow us.
static bool StartListener( unsigned short port, bool bRequired )
{
	SOCKET	s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( s == INVALID_SOCKET )
		return false;

	BOOL	exclusive = TRUE;
	setsockopt( s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
				(const char*)&exclusive, sizeof( exclusive ) );

	struct sockaddr_in	addr;
	memset( &addr, 0, sizeof( addr ) );
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl( INADDR_ANY );
	addr.sin_port        = htons( port );

	if ( bind( s, (struct sockaddr*)&addr, sizeof( addr ) ) != 0
	  || listen( s, SOMAXCONN ) != 0 )
	{
		int	err = WSAGetLastError();
		if ( bRequired || err == WSAEADDRINUSE )
			fprintf( stderr, "wonserverd: TCP %u unavailable (%d)%s\n", port, err,
					 ( err == WSAEADDRINUSE )
						? " -- another wonserverd is already on this port" : "" );
		closesocket( s );
		return false;
	}

	ConnCtx*	lctx = (ConnCtx*)malloc( sizeof( ConnCtx ) );
	if ( !lctx ) { closesocket( s ); return false; }
	lctx->sock = s;
	lctx->port = port;

	HANDLE	h = CreateThread( NULL, 0, AcceptLoop, (LPVOID)lctx, 0, NULL );
	if ( !h )
	{
		free( lctx );
		closesocket( s );
		return false;
	}
	CloseHandle( h );
	return true;
}

int main( int argc, char** argv )
{
	Room_Init();
	InitializeCriticalSection( &g_coreReplyLock );

	unsigned short	port       = WONSERVERD_DEFAULT_PORT;
	unsigned short	masterPort = 27010;
	unsigned short	modPort    = 27011;
	const char*		keyDir     = ".";
	for ( int i = 1; i < argc; i++ )
	{
		if ( !strcmp( argv[i], "-port" ) && i + 1 < argc )
			port = (unsigned short)atoi( argv[++i] );
		else if ( !strcmp( argv[i], "-masterport" ) && i + 1 < argc )
			masterPort = (unsigned short)atoi( argv[++i] );
		else if ( !strcmp( argv[i], "-modport" ) && i + 1 < argc )
			modPort = (unsigned short)atoi( argv[++i] );
		else if ( !strcmp( argv[i], "-keydir" ) && i + 1 < argc )
			keyDir = argv[++i];
		else if ( !strcmp( argv[i], "-auth" ) )
			g_bAuthEnabled = true;
		else if ( !strcmp( argv[i], "-noauth" ) )
			g_bAuthEnabled = false;
	}

	setvbuf( stdout, NULL, _IONBF, 0 );		// unbuffered: logs survive even if killed

	WSADATA	wsa;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsa ) != 0 )
	{
		fprintf( stderr, "WSAStartup failed\n" );
		return 1;
	}

	// Seed the emulated world (rooms + their UDP status responders).
	WonServer_SetServicePort( port );
	WonServer_AddDefaultRooms();

	// Start the master responder (the chat bootstrap the launcher hits first).
	WonMaster_Start( masterPort, port );
	WonMod_Start( modPort );
	WonGameSv_Start();		// mock game servers for the Internet games page

	// Initialise the Auth trust root (generates keys + kver.kp on first run).
	if ( !WonAuth_Init( keyDir ) )
		printf( "wonserverd: auth init failed -- service 202 will not be available\n" );

	if ( !StartListener( port, true ) )
		return 1;

	printf( "wonserverd: WON server emulator listening on TCP %u\n", port );
	printf( "wonserverd: %d rooms advertised (UDP status responder active)\n",
			WonServer_GetRoomCount() );

	// Each advertised room is its own chat server, so accept on its port too --
	// otherwise the launcher's auto-join sits in connect retries for ~10 seconds.
	int	nRoomPorts = 0;
	for ( int i = 0; i < WonServer_GetRoomCount(); i++ )
	{
		unsigned short	roomPort = WonServer_GetRoomPort( i );
		if ( roomPort && roomPort != port && StartListener( roomPort, false ) )
			++nRoomPorts;
	}
	for ( int i = 0; i < WonServer_GetFactoryPortCount(); i++ )
	{
		unsigned short factoryPort = WonServer_GetFactoryPort( i );
		if ( factoryPort && StartListener( factoryPort, false ) )
			++nRoomPorts;
	}
	printf( "wonserverd: chat listeners on %d room ports\n", nRoomPorts );

	printf( "wonserverd: point the woncomm.lst Titan block at this host:%u\n", port );
	printf( "wonserverd: master responder on UDP %u (woncomm.lst Master block)\n", masterPort );
	printf( "wonserverd: mod-list responder on UDP %u (woncomm.lst ModServer block)\n", modPort );

	for ( ;; )
		Sleep( 1000 );

	// not reached
}
