// authtest.cpp -- exercise wonserverd's Auth service the way the launcher does.

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypt/EGPrivateKey.h"
#include "crypt/EGPublicKey.h"
#include "crypt/BFSymmetricKey.h"
#include "auth/Auth1PublicKeyBlock.h"
#include "auth/Auth1Certificate.h"

#pragma comment( lib, "ws2_32.lib" )
using namespace WONCrypt;
using namespace WONAuth;

#define SVC_AUTH	202

static SOCKET g_sock = INVALID_SOCKET;

static bool SendAll( const unsigned char* p, int n )
{
	int s = 0; while ( s < n ) { int w = send( g_sock, (const char*)p + s, n - s, 0 ); if ( w <= 0 ) return false; s += w; } return true;
}
static bool RecvAll( unsigned char* p, int n )
{
	int g = 0; while ( g < n ) { int r = recv( g_sock, (char*)p + g, n - g, 0 ); if ( r <= 0 ) return false; g += r; } return true;
}

// Send [len][svc][msg][body]; receive [len][svc][msg][payload]; return payload.
static unsigned char* Transact( unsigned long svc, unsigned long msg,
	const unsigned char* body, int bodyLen, int* outLen,
	unsigned long* outSvc = NULL, unsigned long* outMsg = NULL )
{
	int total = 12 + bodyLen;
	unsigned char* req = (unsigned char*)malloc( total );
	unsigned long* L = (unsigned long*)req;
	L[0] = total; L[1] = svc; L[2] = msg;
	if ( bodyLen ) memcpy( req + 12, body, bodyLen );
	bool ok = SendAll( req, total ); free( req );
	if ( !ok ) return NULL;

	unsigned char hdr[12];
	if ( !RecvAll( hdr, 4 ) ) return NULL;
	unsigned long rtotal = *(unsigned long*)hdr;
	if ( rtotal < 12 || rtotal > 0x100000 ) return NULL;
	if ( !RecvAll( hdr + 4, 8 ) ) return NULL;	// svc + msg
	if ( outSvc ) *outSvc = *(unsigned long*)( hdr + 4 );
	if ( outMsg ) *outMsg = *(unsigned long*)( hdr + 8 );
	int payLen = (int)rtotal - 12;
	unsigned char* pay = (unsigned char*)malloc( payLen > 0 ? payLen : 1 );
	if ( payLen && !RecvAll( pay, payLen ) ) { free( pay ); return NULL; }
	*outLen = payLen;
	return pay;
}

static unsigned short rd16( const unsigned char* p ) { return (unsigned short)( p[0] | ( p[1] << 8 ) ); }
static unsigned long  rd32( const unsigned char* p ) { return (unsigned long)( p[0] | ( p[1] << 8 ) | ( p[2] << 16 ) | ( (unsigned long)p[3] << 24 ) ); }
static void wr16( unsigned char* p, unsigned short v ) { p[0] = (unsigned char)v; p[1] = (unsigned char)( v >> 8 ); }
static void wr32( unsigned char* p, unsigned long v ) { p[0] = (unsigned char)v; p[1] = (unsigned char)( v >> 8 ); p[2] = (unsigned char)( v >> 16 ); p[3] = (unsigned char)( v >> 24 ); }

int main( int argc, char** argv )
{
	const char* kver = argc > 1 ? argv[1] : "kver.kp";
	const char* host = argc > 2 ? argv[2] : "127.0.0.1";
	int         port = argc > 3 ? atoi( argv[3] ) : 6002;

	WSADATA w; WSAStartup( MAKEWORD( 2, 2 ), &w );

	// Load the trust root (kver.kp) -> verifier public key.
	FILE* fp = fopen( kver, "rb" );
	if ( !fp ) { printf( "FAIL: open %s\n", kver ); return 1; }
	fseek( fp, 0, SEEK_END ); long kn = ftell( fp ); fseek( fp, 0, SEEK_SET );
	unsigned char* kbuf = (unsigned char*)malloc( kn ); fread( kbuf, kn, 1, fp ); fclose( fp );
	EGPublicKey verifier( (unsigned short)kn, kbuf );
	printf( "loaded kver.kp (%ld bytes)\n", kn );

	// Connect.
	g_sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	struct sockaddr_in a; memset( &a, 0, sizeof a ); a.sin_family = AF_INET; a.sin_addr.s_addr = inet_addr( host ); a.sin_port = htons( (unsigned short)port );
	if ( connect( g_sock, (sockaddr*)&a, sizeof a ) != 0 ) { printf( "FAIL: connect %s:%d\n", host, port ); return 1; }

	// - 1. GetPubKeys ---
	int n = 0;
	unsigned long replySvc = 0, replyMsg = 0;
	unsigned char* pay = Transact( SVC_AUTH, 1 /*GetPubKeys*/, NULL, 0, &n,
		&replySvc, &replyMsg );
	if ( !pay || n < 4 ) { printf( "FAIL: getpubkeys recv\n" ); return 1; }
	if ( replySvc != SVC_AUTH || replyMsg != 2 ) { printf( "FAIL: getpubkeys header\n" ); return 1; }
	unsigned short status = rd16( pay ), blen = rd16( pay + 2 );
	if ( status != 0 || blen != n - 4 ) { printf( "FAIL: getpubkeys status=%u blen=%u n=%d\n", status, blen, n ); return 1; }
	Auth1PublicKeyBlock block( pay + 4, blen );
	if ( !block.Verify( verifier ) ) { printf( "FAIL: block does NOT verify against kver.kp\n" ); return 1; }
	printf( "OK: pubkey block received + verified (blockId=%u, %u key(s))\n", block.GetBlockId(), (unsigned)block.KeyList().size() );
	free( pay );

	// - 2. LoginRequest (EG-wrap a session key under the block's first key) ---
	BFSymmetricKey loginKey; loginKey.Create( 8 );
	unsigned char blob[256]; int bl = 0;
	wr16( blob + bl, block.GetBlockId() ); bl += 2;
	wr16( blob + bl, loginKey.GetKeyLen() ); bl += 2;
	memcpy( blob + bl, loginKey.GetKey(), loginKey.GetKeyLen() ); bl += loginKey.GetKeyLen();
	wr16( blob + bl, 4 ); bl += 2; memcpy( blob + bl, "GUID", 4 ); bl += 4;	// dummy GUID

	const EGPublicKey& authPub = block.KeyList().front();
	EGPublicKey::CryptReturn enc = authPub.Encrypt( blob, bl );
	if ( !enc.first ) { printf( "FAIL: EG encrypt login blob\n" ); return 1; }

	unsigned char body[1024]; int b = 0;
	wr16( body + b, block.GetBlockId() ); b += 2;
	body[b++] = 1;								// noPrivKey
	wr16( body + b, (unsigned short)enc.second ); b += 2;
	memcpy( body + b, enc.first, enc.second ); b += (int)enc.second;
	delete[] enc.first;

	pay = Transact( SVC_AUTH, 40 /*LoginRequest*/, body, b, &n, &replySvc, &replyMsg );
	if ( !pay || n < 2 ) { printf( "FAIL: login recv\n" ); return 1; }
	if ( replySvc != SVC_AUTH || replyMsg != 41 )
		{ printf( "FAIL: expected challenge(41), got %lu\n", replyMsg ); return 1; }
	unsigned short encLen = rd16( pay );
	if ( 2 + encLen != n ) { printf( "FAIL: challenge len %u vs %d\n", encLen, n ); return 1; }
	CryptKeyBase::CryptReturn seed = loginKey.Decrypt( pay + 2, encLen );
	if ( !seed.first || seed.second < 8 ) { printf( "FAIL: decrypt challenge seed\n" ); return 1; }
	printf( "OK: challenge received + seed decrypted (%lu bytes)\n", seed.second );
	delete[] seed.first; free( pay );

	// - 3. LoginConfirm (dummy checksum; server is the authority) ---
	unsigned char chk[32]; memset( chk, 0xAB, sizeof chk );
	CryptKeyBase::CryptReturn encChk = loginKey.Encrypt( chk, sizeof chk );
	b = 0; wr16( body + b, (unsigned short)encChk.second ); b += 2;
	memcpy( body + b, encChk.first, encChk.second ); b += (int)encChk.second;
	delete[] encChk.first;

	pay = Transact( SVC_AUTH, 42 /*LoginConfirm*/, body, b, &n, &replySvc, &replyMsg );
	if ( !pay || n < 4 ) { printf( "FAIL: confirm recv\n" ); return 1; }
	if ( replySvc != SVC_AUTH || replyMsg != 44 ) { printf( "FAIL: confirm header\n" ); return 1; }
	unsigned short errorCode = rd16( pay ), itemCount = rd16( pay + 2 );
	if ( errorCode != 0 ) { printf( "FAIL: confirm errorCode=%u\n", errorCode ); return 1; }
	printf( "OK: login reply (errorCode=0, items=%u)\n", itemCount );

	// Parse items: cert (type 1) must verify; private key (type 2) must decrypt.
	Auth1Certificate* clientCert = NULL;
	EGPrivateKey* clientPriv = NULL;
	int off = 4; bool gotCert = false, gotKey = false;
	for ( int i = 0; i < itemCount && off + 3 <= n; i++ )
	{
		unsigned char type = pay[off++];
		unsigned short len = rd16( pay + off ); off += 2;
		if ( off + len > n ) break;
		if ( type == 1 )
		{
			clientCert = new Auth1Certificate( pay + off, len );
			gotCert = clientCert->IsValid() && clientCert->Verify( authPub );
			printf( "   cert: %s (userId=%lu)\n", gotCert ? "VERIFIED" : "verify FAILED", clientCert->GetUserId() );
		}
		else if ( type == 2 )
		{
			CryptKeyBase::CryptReturn pk = loginKey.Decrypt( pay + off, len );
			if ( pk.first ) { try { clientPriv = new EGPrivateKey( (unsigned short)pk.second, pk.first ); gotKey = true; } catch ( ... ) {} delete[] pk.first; }
			printf( "   client private key: %s\n", gotKey ? "decrypted + constructed" : "FAILED" );
		}
		off += len;
	}
	free( pay );

	if ( !gotCert || !gotKey )
	{
		printf( "\n=== AUTH HANDSHAKE FAILED ===\n" );
		return 1;
	}

	// - 4. Peer handshake (svc 203) ---
	b = 0;
	body[b++] = 2;
	body[b++] = 1;
	wr16( body + b, 0 ); b += 2;
	wr16( body + b, clientCert->GetRawLen() ); b += 2;
	memcpy( body + b, clientCert->GetRaw(), clientCert->GetRawLen() ); b += clientCert->GetRawLen();
	pay = Transact( 203, 50, body, b, &n, &replySvc, &replyMsg );
	if ( !pay || replySvc != 203 || replyMsg != 51 || n < 4 )
		{ printf( "FAIL: peer challenge1\n" ); return 1; }
	off = 0;
	encLen = rd16( pay + off ); off += 2;
	if ( off + encLen + 2 > n ) { printf( "FAIL: peer key length\n" ); return 1; }
	CryptKeyBase::CryptReturn peerSecret = clientPriv->Decrypt( pay + off, encLen );
	off += encLen;
	if ( !peerSecret.first || peerSecret.second < 3 ) { printf( "FAIL: peer key decrypt\n" ); return 1; }
	unsigned short channelLen = rd16( peerSecret.first );
	if ( channelLen + 2 != peerSecret.second ) { printf( "FAIL: peer key shape\n" ); return 1; }
	BFSymmetricKey channel;
	channel.Create( channelLen, peerSecret.first + 2 );
	delete[] peerSecret.first;

	unsigned short serverCertLen = rd16( pay + off ); off += 2;
	if ( off + serverCertLen > n ) { printf( "FAIL: peer cert length\n" ); return 1; }
	Auth1Certificate serverCert( pay + off, serverCertLen );
	if ( !serverCert.IsValid() || !serverCert.Verify( authPub ) )
		{ printf( "FAIL: peer cert verify\n" ); return 1; }
	free( pay );

	BFSymmetricKey challengeKey;
	challengeKey.Create( 8 );
	unsigned char peerPlain[128];
	int peerPlainLen = 0;
	wr16( peerPlain + peerPlainLen, channel.GetKeyLen() ); peerPlainLen += 2;
	memcpy( peerPlain + peerPlainLen, channel.GetKey(), channel.GetKeyLen() ); peerPlainLen += channel.GetKeyLen();
	memcpy( peerPlain + peerPlainLen, challengeKey.GetKey(), challengeKey.GetKeyLen() ); peerPlainLen += challengeKey.GetKeyLen();
	EGPublicKey serverPub( serverCert.GetPubKey() );
	CryptKeyBase::CryptReturn peerEncrypted = serverPub.Encrypt( peerPlain, peerPlainLen );
	if ( !peerEncrypted.first ) { printf( "FAIL: peer challenge2 encrypt\n" ); return 1; }
	b = 0;
	wr16( body + b, (unsigned short)peerEncrypted.second ); b += 2;
	memcpy( body + b, peerEncrypted.first, peerEncrypted.second ); b += (int)peerEncrypted.second;
	delete[] peerEncrypted.first;
	pay = Transact( 203, 52, body, b, &n, &replySvc, &replyMsg );
	if ( !pay || replySvc != 203 || replyMsg != 53 || n < 8 || rd16( pay ) != 0 )
		{ printf( "FAIL: peer complete\n" ); return 1; }
	encLen = rd16( pay + 2 );
	if ( 4 + encLen + 2 > n ) { printf( "FAIL: peer echo length\n" ); return 1; }
	CryptKeyBase::CryptReturn peerEcho = clientPriv->Decrypt( pay + 4, encLen );
	if ( !peerEcho.first || peerEcho.second != challengeKey.GetKeyLen() + 2
	  || rd16( peerEcho.first ) != challengeKey.GetKeyLen()
	  || memcmp( peerEcho.first + 2, challengeKey.GetKey(), challengeKey.GetKeyLen() ) )
		{ printf( "FAIL: peer echo verify\n" ); return 1; }
	delete[] peerEcho.first;
	unsigned short sessionId = rd16( pay + 4 + encLen );
	free( pay );
	printf( "OK: peer handshake established session %u\n", sessionId );

	// - 5. Resume the session on a fresh connection and fetch an encrypted directory ---
	closesocket( g_sock );
	g_sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( connect( g_sock, (sockaddr*)&a, sizeof a ) != 0 )
		{ printf( "FAIL: encrypted reconnect\n" ); return 1; }
	unsigned char inner[256];
	int innerLen = 0;
	wr16( inner + innerLen, 1 ); innerLen += 2;
	wr32( inner + innerLen, 30 ); innerLen += 4;
	wr32( inner + innerLen, 2 ); innerLen += 4;
	const char* dir = "/Half-Life/Public";
	unsigned short dirLen = (unsigned short)strlen( dir );
	wr16( inner + innerLen, dirLen ); innerLen += 2;
	for ( unsigned short i = 0; i < dirLen; i++ )
	{
		inner[innerLen++] = (unsigned char)dir[i];
		inner[innerLen++] = 0;
	}
	inner[innerLen++] = 0;
	CryptKeyBase::CryptReturn directoryCipher = channel.Encrypt( inner, innerLen );
	if ( !directoryCipher.first ) { printf( "FAIL: directory encrypt\n" ); return 1; }
	unsigned char envelope[1024];
	int envelopeLen = 7 + (int)directoryCipher.second;
	wr32( envelope, envelopeLen );
	envelope[4] = 2;
	wr16( envelope + 5, sessionId );
	memcpy( envelope + 7, directoryCipher.first, directoryCipher.second );
	delete[] directoryCipher.first;
	if ( !SendAll( envelope, envelopeLen ) ) { printf( "FAIL: directory send\n" ); return 1; }
	unsigned char lenBuf[4];
	if ( !RecvAll( lenBuf, 4 ) ) { printf( "FAIL: directory reply header\n" ); return 1; }
	unsigned long envelopeReplyLen = rd32( lenBuf );
	if ( envelopeReplyLen < 8 || envelopeReplyLen > sizeof( envelope ) )
		{ printf( "FAIL: directory reply size\n" ); return 1; }
	memcpy( envelope, lenBuf, 4 );
	if ( !RecvAll( envelope + 4, envelopeReplyLen - 4 ) || envelope[4] != 2
	  || rd16( envelope + 5 ) != sessionId )
		{ printf( "FAIL: directory reply envelope\n" ); return 1; }
	CryptKeyBase::CryptReturn directoryPlain = channel.Decrypt( envelope + 7, envelopeReplyLen - 7 );
	if ( !directoryPlain.first || directoryPlain.second < 14
	  || rd16( directoryPlain.first ) != 1 || rd32( directoryPlain.first + 2 ) != 30
	  || rd32( directoryPlain.first + 6 ) != 3 || rd16( directoryPlain.first + 10 ) != 0
	  || rd16( directoryPlain.first + 12 ) < 8 )
		{ printf( "FAIL: encrypted directory contents\n" ); return 1; }
	delete[] directoryPlain.first;
	delete clientCert;
	delete clientPriv;
	printf( "OK: encrypted session resume and directory reply\n" );
	printf( "\n=== AUTH + ENCRYPTED TRANSPORT OK ===\n" );
	return 0;
}
