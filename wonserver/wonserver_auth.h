#ifndef WONSERVER_AUTH_H
#define WONSERVER_AUTH_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Load (or, first run, generate) the verifier + auth keypairs from keyDir, and
// write keyDir\kver.kp (the verifier public key the client must trust -- copy it
// into the HL folder).
int		WonAuth_Init( const char* keyDir );

// Per-connection auth session.
void*	WonAuth_SessionCreate( void );
void	WonAuth_SessionDestroy( void* sess );

// Handle one service-202 message (msgType = Auth1GetPubKeys /
// Auth1LoginRequestHL / Auth1LoginConfirmHL).
int		WonAuth_SessionHandle( void* sess, unsigned long msgType,
							   const BYTE* payload, int len,
							   const BYTE** ppReply, int* pReplyLen,
							   unsigned long* pReplySvc, unsigned long* pReplyMsg );

// Handle one service-203 peer-handshake message (50 = request, 52 = confirm).
// Succeeds only after the client has taken a public-key block over svc 202.
int		WonAuth_PeerHandle( void* sess, unsigned long msgType,
							const BYTE* payload, int len,
							const BYTE** ppReply, int* pReplyLen,
							unsigned long* pReplySvc, unsigned long* pReplyMsg );

// Non-NULL once the peer handshake completed: from then on every message on the
// connection is Blowfish-encrypted under this key and carries pSessionId.
const void*	WonAuth_SessionChannelKey( void* sess, int* pKeyLen, unsigned short* pSessionId );

// Resume a session negotiated on an earlier connection: installs its channel key
// into this one.  The launcher opens a fresh socket per transaction and quotes the
// session id rather than repeating the svc 203 handshake.
int		WonAuth_SessionAdopt( void* sess, unsigned short sessionId );

// Blowfish helpers over the channel key.  *ppOut is new[]-allocated on success.
int		WonAuth_SessionDecrypt( void* sess, const BYTE* pIn, int cbIn, BYTE** ppOut, int* pcbOut );
int		WonAuth_SessionEncrypt( void* sess, const BYTE* pIn, int cbIn, BYTE** ppOut, int* pcbOut );

#ifdef __cplusplus
}
#endif

#endif // WONSERVER_AUTH_H
