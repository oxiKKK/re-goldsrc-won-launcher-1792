// wonserver_auth.cpp -- emulated WON Auth server.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypt/EGPrivateKey.h"
#include "crypt/EGPublicKey.h"
#include "crypt/BFSymmetricKey.h"
#include "crypt/CryptException.h"
#include "auth/Auth1PublicKeyBlock.h"
#include "auth/Auth1Certificate.h"

#include <time.h>
#include "wonwire.h"
#include "WriteBuffer.h"
#include "wonserver_auth.h"

// Service / message ids (HeaderTypes.h / TMsgTypesAuth.h).
#define AUTH_SVC				202		// Auth1LoginHL
#define AUTH_GETPUBKEYS			1		// Auth1GetPubKeys
#define AUTH_GETPUBKEYSREPLY	2		// Auth1GetPubKeysReply
#define AUTH_LOGINREQUEST		40		// Auth1LoginRequestHL
#define AUTH_LOGINCHALLENGE		41		// Auth1LoginChallengeHL
#define AUTH_LOGINCONFIRM		42		// Auth1LoginConfirmHL
#define AUTH_LOGINREPLY			44		// Auth1LoginReplyHL

#define PEER_SVC				203		// the peer (server-to-server / client-to-server) handshake
#define PEER_REQUEST			50		// client presents its certificate
#define PEER_REPLY				51		// we answer with the session key + our certificate
#define PEER_CONFIRM			52		// client proves it decrypted, contributes its own key
#define PEER_COMPLETE			53		// we echo its key back and hand out the session id

#define PEER_SESSION_KEYLEN		8		// Blowfish channel key we mint for the session

#define AUTH_BLOCK_ID			1
#define AUTH_CERT_LIFESPAN		(365*24*3600)

using namespace WONCrypt;
using namespace WONAuth;

// EG modulus size (bytes) for our generated keys.  512-bit: fast to generate, ample
// for signing an MD5 and EG-wrapping the small login blob.  Generated once, persisted.
#define WONAUTH_EG_KEYLEN	64

static EGPrivateKey*	g_verifierPriv = NULL;	// signs the pubkey block + certs' root of trust
static EGPrivateKey*	g_authPriv     = NULL;	// the served AuthServer key; signs certificates
static int				g_ready        = 0;

static EGPrivateKey* LoadPriv( const char* path )
{
	FILE* fp = fopen( path, "rb" );
	if ( !fp ) return NULL;
	fseek( fp, 0, SEEK_END ); long n = ftell( fp ); fseek( fp, 0, SEEK_SET );
	if ( n < 3 ) { fclose( fp ); return NULL; }
	unsigned char* buf = (unsigned char*)malloc( n );
	fread( buf, n, 1, fp ); fclose( fp );
	unsigned short len = (unsigned short)( buf[0] | ( buf[1] << 8 ) );
	EGPrivateKey* k = NULL;
	try { k = new EGPrivateKey( len, buf + 2 ); }
	catch ( ... ) { k = NULL; }
	free( buf );
	return k;
}

static int SavePriv( const char* path, const EGPrivateKey* k )
{
	FILE* fp = fopen( path, "wb" );
	if ( !fp ) return 0;
	unsigned short len = k->GetKeyLen();
	unsigned char hdr[2] = { (unsigned char)( len & 0xFF ), (unsigned char)( ( len >> 8 ) & 0xFF ) };
	fwrite( hdr, 2, 1, fp );
	fwrite( k->GetKey(), len, 1, fp );
	fclose( fp );
	return 1;
}

static int WriteVerifierPublic( const char* path, const EGPrivateKey* verifier )
{
	const EGPublicKey& pub = (const EGPublicKey&)verifier->GetPublicKey();
	FILE* fp = fopen( path, "wb" );
	if ( !fp ) return 0;
	fwrite( pub.GetKey(), pub.GetKeyLen(), 1, fp );
	fclose( fp );
	return 1;
}

static void JoinPath( char* out, int cap, const char* dir, const char* file )
{
	if ( dir && *dir ) _snprintf( out, cap, "%s\\%s", dir, file );
	else               _snprintf( out, cap, "%s", file );
	out[cap - 1] = 0;
}

static int VerifierRoundTrip( const char* kverPath )
{
	FILE* fp = fopen( kverPath, "rb" );
	if ( !fp ) return 0;
	fseek( fp, 0, SEEK_END ); long n = ftell( fp ); fseek( fp, 0, SEEK_SET );
	unsigned char* buf = (unsigned char*)malloc( n );
	fread( buf, n, 1, fp ); fclose( fp );

	int ok = 0;
	try
	{
		EGPublicKey pub( (unsigned short)n, buf );
		const char* msg = "wonserverd-trust-root-check";
		CryptKeyBase::CryptReturn sig = g_verifierPriv->Sign( msg, (unsigned long)strlen( msg ) );
		if ( sig.first )
		{
			ok = pub.Verify( sig.first, sig.second, msg, (unsigned long)strlen( msg ) ) ? 1 : 0;
			delete[] sig.first;
		}
	}
	catch ( ... ) { ok = 0; }
	free( buf );
	return ok;
}

int WonAuth_Init( const char* keyDir )
{
	char verifierPath[512], authPath[512], kverPath[512];
	JoinPath( verifierPath, sizeof( verifierPath ), keyDir, "verifier.key" );
	JoinPath( authPath,     sizeof( authPath ),     keyDir, "auth.key" );
	JoinPath( kverPath,     sizeof( kverPath ),     keyDir, "kver.kp" );

	g_verifierPriv = LoadPriv( verifierPath );
	g_authPriv     = LoadPriv( authPath );

	if ( !g_verifierPriv || !g_authPriv )
	{
		printf( "wonserverd[auth]: generating EG keypairs (%d-byte modulus, one-time)...\n",
				WONAUTH_EG_KEYLEN );
		try
		{
			if ( !g_verifierPriv ) { g_verifierPriv = new EGPrivateKey( WONAUTH_EG_KEYLEN ); SavePriv( verifierPath, g_verifierPriv ); }
			if ( !g_authPriv )     { g_authPriv     = new EGPrivateKey( WONAUTH_EG_KEYLEN ); SavePriv( authPath, g_authPriv ); }
		}
		catch ( CryptException& e )
		{
			printf( "wonserverd[auth]: key generation failed: %s\n", e.what() );
			return 0;
		}
		catch ( ... )
		{
			printf( "wonserverd[auth]: key generation failed (unknown)\n" );
			return 0;
		}
	}

	if ( !WriteVerifierPublic( kverPath, g_verifierPriv ) )
	{
		printf( "wonserverd[auth]: could not write %s\n", kverPath );
		return 0;
	}

	if ( !VerifierRoundTrip( kverPath ) )
	{
		printf( "wonserverd[auth]: WARNING -- kver.kp round-trip verify FAILED\n" );
		return 0;
	}

	printf( "wonserverd[auth]: trust root ready -- copy %s into your HL folder\n", kverPath );
	g_ready = 1;
	return 1;
}

struct WonAuthSession
{
	BFSymmetricKey*	loginKey;	// negotiated in the login request, used at confirm
	WriteBuffer		reply;		// owned reply payload (valid until next call)

	// -- svc 203 peer handshake (WONAuth_Handshake 0x47cbd0) ---------------------
	EGPublicKey*	peerPub;	// the client cert's public key, from msg 50
	BFSymmetricKey*	sessionKey;	// what we minted in msg 51; the channel key afterwards
	unsigned short	sessionId;	// handed back in msg 53, echoed in every later header

	WonAuthSession()
		: loginKey( NULL ), reply( 0x800 )
		, peerPub( NULL ), sessionKey( NULL ), sessionId( 0 ) { }
	~WonAuthSession() { delete loginKey; delete peerPub; delete sessionKey; }
};

static unsigned short	g_nextSessionId = 1;

// A WON session outlives the TCP connection that negotiated it: the launcher does
// the svc 203 handshake once, then opens a fresh connection per transaction (the
// room list is one) and just quotes the session id in the clear header.  Keep the
// channel keys process-wide so any connection can resume one.
struct SessionEntry
{
	unsigned short	id;
	int				keyLen;
	BYTE			key[64];
};

static SessionEntry		g_sessions[64];
static int				g_nSessions = 0;
static CRITICAL_SECTION	g_sessionLock;
static bool				g_sessionLockInit = false;

static void SessionLock()
{
	if ( !g_sessionLockInit )		// first use is before any worker thread exists
	{
		InitializeCriticalSection( &g_sessionLock );
		g_sessionLockInit = true;
	}
	EnterCriticalSection( &g_sessionLock );
}

static void SessionRegister( unsigned short id, const BYTE* pKey, int cbKey )
{
	if ( cbKey <= 0 || cbKey > (int)sizeof( g_sessions[0].key ) )
		return;
	SessionLock();
	for ( int i = 0; i < g_nSessions; i++ )
	{
		if ( g_sessions[i].id == id )
		{
			g_sessions[i].keyLen = cbKey;
			memcpy( g_sessions[i].key, pKey, cbKey );
			LeaveCriticalSection( &g_sessionLock );
			return;
		}
	}
	if ( g_nSessions < (int)( sizeof( g_sessions ) / sizeof( g_sessions[0] ) ) )
	{
		g_sessions[g_nSessions].id     = id;
		g_sessions[g_nSessions].keyLen = cbKey;
		memcpy( g_sessions[g_nSessions].key, pKey, cbKey );
		g_nSessions++;
	}
	LeaveCriticalSection( &g_sessionLock );
}

// Install a previously negotiated session's channel key into this connection.
int WonAuth_SessionAdopt( void* sess, unsigned short sessionId )
{
	WonAuthSession* s = (WonAuthSession*)sess;
	if ( !s || !sessionId )
		return 0;

	BYTE	key[64];
	int		keyLen = 0;

	SessionLock();
	for ( int i = 0; i < g_nSessions; i++ )
	{
		if ( g_sessions[i].id == sessionId )
		{
			keyLen = g_sessions[i].keyLen;
			memcpy( key, g_sessions[i].key, keyLen );
			break;
		}
	}
	LeaveCriticalSection( &g_sessionLock );

	if ( !keyLen )
		return 0;

	try
	{
		delete s->sessionKey;
		s->sessionKey = new BFSymmetricKey();
		s->sessionKey->Create( (unsigned short)keyLen, key );
		s->sessionId  = sessionId;
	}
	catch ( ... ) { return 0; }
	return 1;
}

void* WonAuth_SessionCreate( void )
{
	return g_ready ? new WonAuthSession() : NULL;
}

void WonAuth_SessionDestroy( void* sess )
{
	delete (WonAuthSession*)sess;
}

// - the served AuthServer public-key block (auth key, signed by the verifier) ---
static int BuildPubKeyReply( WonAuthSession* s, const BYTE** ppReply, int* pReplyLen,
							 unsigned long* pReplySvc, unsigned long* pReplyMsg )
{
	try
	{
		Auth1PublicKeyBlock block( AUTH_BLOCK_ID );
		block.SetLifespan( time( NULL ), AUTH_CERT_LIFESPAN );
		block.KeyList().push_back( (const EGPublicKey&)g_authPriv->GetPublicKey() );
		if ( !block.Pack( *g_verifierPriv ) )
			return 0;

		printf( "  [auth] sizeof(time_t)=%d block=%u\n",
				(int)sizeof( time_t ), (unsigned)block.GetRawLen() );

		s->reply.rewind();
		s->reply.appendShort( 0 );						// status = success
		s->reply.appendShort( block.GetRawLen() );		// block length
		s->reply.append( block.GetRaw(), block.GetRawLen() );
	}
	catch ( ... ) { return 0; }

	*ppReply   = s->reply.getBuffer();
	*pReplyLen = s->reply.getSize();
	*pReplySvc = AUTH_SVC;
	*pReplyMsg = AUTH_GETPUBKEYSREPLY;
	return 1;
}

// - login request: recover the client's proposed session key, answer a challenge ---
static int BuildChallenge( WonAuthSession* s, const BYTE* payload, int len,
						   const BYTE** ppReply, int* pReplyLen,
						   unsigned long* pReplySvc, unsigned long* pReplyMsg )
{
	// Request body: [short blockId][byte noPrivKey][short encLen][EG-encrypted login blob].
	if ( len < 5 ) return 0;
	int encLen = (unsigned char)payload[3] | ( (unsigned char)payload[4] << 8 );
	if ( 5 + encLen > len ) return 0;
	const unsigned char* encData = (const unsigned char*)payload + 5;

	try
	{
		// Decrypt the login blob with the auth private key.
		CryptKeyBase::CryptReturn blob = g_authPriv->Decrypt( encData, (unsigned long)encLen );
		if ( !blob.first || blob.second < 4 ) { if ( blob.first ) delete[] blob.first; return 0; }

		// Login blob: [short blockId][short keyLen][session key bytes][string GUID...].
		const unsigned char* b = blob.first;
		int keyLen = b[2] | ( b[3] << 8 );
		if ( keyLen <= 0 || 4 + keyLen > (int)blob.second ) { delete[] blob.first; return 0; }

		delete s->loginKey;
		s->loginKey = new BFSymmetricKey();
		s->loginKey->Create( (unsigned short)keyLen, b + 4 );
		delete[] blob.first;

		// Challenge seed: 8 random bytes, encrypted under the session key.  The client
		// keeps it as the cached server key and salts its file-integrity response.
		BFSymmetricKey seedKey;
		seedKey.Create( 8 );
		CryptKeyBase::CryptReturn enc = s->loginKey->Encrypt( seedKey.GetKey(), seedKey.GetKeyLen() );
		if ( !enc.first ) return 0;

		// No message-type long here.  WON_Exchange (0x465c50) reads the service and
		// message-type longs off the message stream itself; the login call passes
		// expected-type 0, so it consumes only the service long and WONAuth_Login
		// reads the message type from the frame.  Repeating it in the payload makes
		// the client read 41 as the ciphertext length -> "Error in challenge message."
		s->reply.rewind();
		s->reply.appendShort( (unsigned short)enc.second );
		s->reply.append( enc.first, (int)enc.second );
		delete[] enc.first;
	}
	catch ( ... ) { return 0; }

	*ppReply   = s->reply.getBuffer();
	*pReplyLen = s->reply.getSize();
	*pReplySvc = AUTH_SVC;
	*pReplyMsg = AUTH_LOGINCHALLENGE;
	return 1;
}

// - login confirm: accept (we are the authority) and issue a signed certificate ---
static int BuildLoginReply( WonAuthSession* s, const BYTE* /*payload*/, int /*len*/,
							const BYTE** ppReply, int* pReplyLen,
							unsigned long* pReplySvc, unsigned long* pReplyMsg )
{
	if ( !s->loginKey ) return 0;	// confirm without a prior request

	try
	{
		// Mint a fresh client EG keypair: its public half goes in the cert, its private
		// half is returned (wrapped with the session key) so the client can do p2p auth.
		EGPrivateKey clientPriv( WONAUTH_EG_KEYLEN );

		Auth1Certificate cert( 0x10000001 /*userId*/, 1 /*communityId*/, 1 /*trustLevel*/ );
		cert.SetPublicKey( (const EGPublicKey&)clientPriv.GetPublicKey() );
		cert.SetLifespan( time( NULL ), AUTH_CERT_LIFESPAN );
		if ( !cert.Pack( *g_authPriv ) )		// signed by auth key -> verifies against the block
			return 0;

		CryptKeyBase::CryptReturn encPriv = s->loginKey->Encrypt( clientPriv.GetKey(), clientPriv.GetKeyLen() );
		if ( !encPriv.first ) return 0;

		// Login reply.
		s->reply.rewind();
		s->reply.appendShort( 0 );			// errorCode = success
		s->reply.appendShort( 2 );			// item count
		s->reply.appendByte( 1 );
		s->reply.appendShort( cert.GetRawLen() );
		s->reply.append( cert.GetRaw(), cert.GetRawLen() );
		s->reply.appendByte( 2 );
		s->reply.appendShort( (unsigned short)encPriv.second );
		s->reply.append( encPriv.first, (int)encPriv.second );
		delete[] encPriv.first;
	}
	catch ( ... ) { return 0; }

	*ppReply   = s->reply.getBuffer();
	*pReplyLen = s->reply.getSize();
	*pReplySvc = AUTH_SVC;
	*pReplyMsg = AUTH_LOGINREPLY;
	return 1;
}


// ---------------------------------------------------------------------------
// svc 203 -- the peer handshake WONAuth_Handshake (0x47cbd0) runs before any
// authenticated request.  Four messages; see wonserver/README.md for the layout.
// It only works once the client holds a public-key block from svc 202, because
// WONAuth_VerifyCert checks our certificate against it.
// ---------------------------------------------------------------------------

// msg 50: [u8 sessioned+1][u8 encrypt][u16 unsequenced][u16 certLen][client cert]
static int BuildPeerReply( WonAuthSession* s, const BYTE* payload, int len,
						   const BYTE** ppReply, int* pReplyLen,
						   unsigned long* pReplySvc, unsigned long* pReplyMsg )
{
	if ( len < 6 )
		return 0;
	int certLen = (unsigned char)payload[4] | ( (unsigned char)payload[5] << 8 );
	if ( certLen <= 0 || 6 + certLen > len )
		return 0;

	try
	{
		// The certificate is the one we minted in the login reply, so its public
		// half is what we encrypt the session key to.
		Auth1Certificate clientCert;
		if ( !clientCert.Unpack( payload + 6, (unsigned short)certLen ) )
			return 0;

		delete s->peerPub;
		s->peerPub = new EGPublicKey( clientCert.GetPubKey() );

		delete s->sessionKey;
		s->sessionKey = new BFSymmetricKey();
		s->sessionKey->Create( PEER_SESSION_KEYLEN );

		// secret = [u16 keyLen][key], EG-encrypted to the client's certificate key
		WriteBuffer secret( 0x80 );
		secret.appendShort( s->sessionKey->GetKeyLen() );
		secret.append( s->sessionKey->GetKey(), s->sessionKey->GetKeyLen() );

		CryptKeyBase::CryptReturn enc =
			s->peerPub->Encrypt( secret.getBuffer(), (unsigned long)secret.getSize() );
		if ( !enc.first )
			return 0;

		// Our own certificate, signed by the auth key the pubkey block advertises.
		Auth1Certificate ours( 0x20000001 /*userId*/, 1 /*communityId*/, 1 /*trustLevel*/ );
		ours.SetPublicKey( (const EGPublicKey&)g_authPriv->GetPublicKey() );
		ours.SetLifespan( time( NULL ), AUTH_CERT_LIFESPAN );
		if ( !ours.Pack( *g_authPriv ) )
		{
			delete[] enc.first;
			return 0;
		}

		// No message-type long in the payload.  WON_Exchange is called with
		// expected-type 0 for this leg, so it stops after the service long and the
		// caller's own ReadLong picks the type straight out of the frame header.
		s->reply.rewind();
		s->reply.appendShort( (unsigned short)enc.second );
		s->reply.append( enc.first, (int)enc.second );
		s->reply.appendShort( ours.GetRawLen() );
		s->reply.append( ours.GetRaw(), ours.GetRawLen() );
		delete[] enc.first;
	}
	catch ( ... ) { return 0; }

	*ppReply   = s->reply.getBuffer();
	*pReplyLen = s->reply.getSize();
	*pReplySvc = PEER_SVC;
	*pReplyMsg = PEER_REPLY;
	return 1;
}

// msg 52: [u16 cipherLen][EG(ourAuthKey, [u16 secretLen][our session key][client key])]
static int BuildPeerComplete( WonAuthSession* s, const BYTE* payload, int len,
							  const BYTE** ppReply, int* pReplyLen,
							  unsigned long* pReplySvc, unsigned long* pReplyMsg )
{
	if ( len < 2 || !s->sessionKey || !s->peerPub )
		return 0;
	int encLen = (unsigned char)payload[0] | ( (unsigned char)payload[1] << 8 );
	if ( encLen <= 0 || 2 + encLen > len )
		return 0;

	try
	{
		// Encrypted to the public key in the certificate we sent, i.e. the auth key.
		CryptKeyBase::CryptReturn blob = g_authPriv->Decrypt( payload + 2, (unsigned long)encLen );
		if ( !blob.first || blob.second < 2 ) { delete[] blob.first; return 0; }

		const unsigned char*	b = blob.first;
		int						secretLen = b[0] | ( b[1] << 8 );

		// It must echo the session key we sent; the tail is the client's own key.
		if ( secretLen != s->sessionKey->GetKeyLen()
		  || 2 + secretLen > (int)blob.second
		  || memcmp( b + 2, s->sessionKey->GetKey(), secretLen ) != 0 )
		{
			delete[] blob.first;
			return 0;
		}

		const unsigned char*	clientKey    = b + 2 + secretLen;
		int						clientKeyLen = (int)blob.second - 2 - secretLen;
		if ( clientKeyLen <= 0 ) { delete[] blob.first; return 0; }

		// Prove we decrypted: hand the client's own key back, wrapped to its cert.
		WriteBuffer echo( 0x80 );
		echo.appendShort( (unsigned short)clientKeyLen );
		echo.append( clientKey, clientKeyLen );
		delete[] blob.first;

		CryptKeyBase::CryptReturn enc =
			s->peerPub->Encrypt( echo.getBuffer(), (unsigned long)echo.getSize() );
		if ( !enc.first )
			return 0;

		if ( !s->sessionId )
			s->sessionId = g_nextSessionId++;

		// Publish it: later connections resume this session by id alone.
		SessionRegister( s->sessionId, (const BYTE*)s->sessionKey->GetKey(),
						 s->sessionKey->GetKeyLen() );

		s->reply.rewind();
		s->reply.appendShort( 0 );						// status >= 0 == success
		s->reply.appendShort( (unsigned short)enc.second );
		s->reply.append( enc.first, (int)enc.second );
		s->reply.appendShort( s->sessionId );
		delete[] enc.first;
	}
	catch ( ... ) { return 0; }

	*ppReply   = s->reply.getBuffer();
	*pReplyLen = s->reply.getSize();
	*pReplySvc = PEER_SVC;
	*pReplyMsg = PEER_COMPLETE;
	return 1;
}

int WonAuth_PeerHandle( void* sess, unsigned long msgType,
						const BYTE* payload, int len,
						const BYTE** ppReply, int* pReplyLen,
						unsigned long* pReplySvc, unsigned long* pReplyMsg )
{
	WonAuthSession* s = (WonAuthSession*)sess;
	if ( !s || !g_ready )
		return 0;

	switch ( msgType )
	{
	case PEER_REQUEST:	return BuildPeerReply( s, payload, len, ppReply, pReplyLen, pReplySvc, pReplyMsg );
	case PEER_CONFIRM:	return BuildPeerComplete( s, payload, len, ppReply, pReplyLen, pReplySvc, pReplyMsg );
	default:			return 0;
	}
}

// Once the handshake completes, every later message on the connection travels
// encrypted under this key with the session id in the clear header.
const void* WonAuth_SessionChannelKey( void* sess, int* pKeyLen, unsigned short* pSessionId )
{
	WonAuthSession* s = (WonAuthSession*)sess;
	if ( !s || !s->sessionKey || !s->sessionId )
		return NULL;
	if ( pKeyLen )		*pKeyLen = s->sessionKey->GetKeyLen();
	if ( pSessionId )	*pSessionId = s->sessionId;
	return s->sessionKey->GetKey();
}

int WonAuth_SessionDecrypt( void* sess, const BYTE* pIn, int cbIn, BYTE** ppOut, int* pcbOut )
{
	WonAuthSession* s = (WonAuthSession*)sess;
	if ( !s || !s->sessionKey ) return 0;
	try
	{
		CryptKeyBase::CryptReturn d = s->sessionKey->Decrypt( pIn, (unsigned long)cbIn );
		if ( !d.first ) return 0;
		*ppOut  = d.first;
		*pcbOut = (int)d.second;
		return 1;
	}
	catch ( ... ) { return 0; }
}

int WonAuth_SessionEncrypt( void* sess, const BYTE* pIn, int cbIn, BYTE** ppOut, int* pcbOut )
{
	WonAuthSession* s = (WonAuthSession*)sess;
	if ( !s || !s->sessionKey ) return 0;
	try
	{
		CryptKeyBase::CryptReturn e = s->sessionKey->Encrypt( pIn, (unsigned long)cbIn );
		if ( !e.first ) return 0;
		*ppOut  = e.first;
		*pcbOut = (int)e.second;
		return 1;
	}
	catch ( ... ) { return 0; }
}

int WonAuth_SessionHandle( void* sess, unsigned long msgType,
						   const BYTE* payload, int len,
						   const BYTE** ppReply, int* pReplyLen,
						   unsigned long* pReplySvc, unsigned long* pReplyMsg )
{
	WonAuthSession* s = (WonAuthSession*)sess;
	if ( !s || !g_ready )
		return 0;

	switch ( msgType )
	{
	case AUTH_GETPUBKEYS:		return BuildPubKeyReply( s, ppReply, pReplyLen, pReplySvc, pReplyMsg );
	case AUTH_LOGINREQUEST:		return BuildChallenge( s, payload, len, ppReply, pReplyLen, pReplySvc, pReplyMsg );
	case AUTH_LOGINCONFIRM:		return BuildLoginReply( s, payload, len, ppReply, pReplyLen, pReplySvc, pReplyMsg );
	default:					return 0;
	}
}
