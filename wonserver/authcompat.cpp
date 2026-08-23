// authcompat.cpp -- feed our packed Auth1 public-key block to the GENUINE
// WONAuth.dll and report whether it parses and verifies.
//
// The vanilla launcher reaches WONAuth.dll through WONAuth_FetchPubKeyBlock
// (0x47ba10): it reads [u16 status][u16 len][len bytes] off svc 202 / msg 2, calls
// WON_AuthFactory::NewAuthPublicKeyBlock1( data, len ), then verifies the result
// against the key it loaded from kver.kp.  A block the real DLL cannot parse throws
// a C++ exception straight through hl.exe, which is the "abnormal program
// termination" abort.  This harness closes that loop without the UI.

#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "crypt/EGPrivateKey.h"
#include "crypt/EGPublicKey.h"
#include "crypt/BFSymmetricKey.h"
#include "auth/Auth1PublicKeyBlock.h"

using namespace WONCrypt;
using namespace WONAuth;

// -- the two WONAuth.dll entry points the launcher uses on this path -------------
typedef void* ( __cdecl *NewBlockFn )( const unsigned char* p, unsigned short n );
typedef void ( __cdecl *DeleteBlockFn )( void* p );
typedef int ( __thiscall *VerifyFn )( void* self, const unsigned char* key, unsigned short keyLen );
typedef int ( __thiscall *GetNumKeysFn )( void* self );
typedef unsigned short ( __thiscall *GetBlockIdFn )( void* self );

static NewBlockFn		g_New		= NULL;
static DeleteBlockFn	g_Delete	= NULL;
static VerifyFn			g_Verify	= NULL;
static GetNumKeysFn		g_GetNumKeys= NULL;
static GetBlockIdFn		g_GetBlockId= NULL;

static int LoadRealWonAuth( const char* pszDir )
{
	char path[MAX_PATH];
	_snprintf( path, sizeof( path ) - 1, "%s\\WONAuth.dll", pszDir );
	path[sizeof( path ) - 1] = 0;

	HMODULE h = LoadLibraryA( path );
	if ( !h )
	{
		printf( "FAIL: LoadLibrary(%s) -> %lu\n", path, GetLastError() );
		return 0;
	}
	printf( "loaded %s\n", path );

	g_New        = (NewBlockFn)GetProcAddress( h, "?NewAuthPublicKeyBlock1@WON_AuthFactory@@SAPAVWON_AuthPublicKeyBlock1@@PBEG@Z" );
	g_Delete     = (DeleteBlockFn)GetProcAddress( h, "?DeleteAuthPublicKeyBlock1@WON_AuthFactory@@SAXPAVWON_AuthPublicKeyBlock1@@@Z" );
	g_Verify     = (VerifyFn)GetProcAddress( h, "?Verify@WON_AuthFamilyBuffer@@QBEHPBEG@Z" );
	g_GetNumKeys = (GetNumKeysFn)GetProcAddress( h, "?GetNumKeys@WON_AuthPublicKeyBlock1@@QBEHXZ" );
	g_GetBlockId = (GetBlockIdFn)GetProcAddress( h, "?GetBlockId@WON_AuthPublicKeyBlock1@@QBEGXZ" );

	printf( "  New=%p Delete=%p Verify=%p GetNumKeys=%p GetBlockId=%p\n",
			g_New, g_Delete, g_Verify, g_GetNumKeys, g_GetBlockId );
	return ( g_New && g_Verify ) ? 1 : 0;
}

// Pack a block the way wonserver_auth.cpp's BuildPubKeyReply does, at a chosen
// EG modulus size, then hand the raw bytes to the real DLL.
static void TryKeyLen( unsigned int nKeyLen )
{
	printf( "\n=== EG modulus %u bytes (%u-bit) ===\n", nKeyLen, nKeyLen * 8 );

	EGPrivateKey*	pVerifier = NULL;
	EGPrivateKey*	pAuth     = NULL;
	try
	{
		pVerifier = new EGPrivateKey( nKeyLen );
		pAuth     = new EGPrivateKey( nKeyLen );
	}
	catch ( ... )
	{
		printf( "  key generation threw\n" );
		return;
	}

	const unsigned char*	pRaw = NULL;
	unsigned short			nRaw = 0;
	Auth1PublicKeyBlock		block( 1 );
	try
	{
		block.SetLifespan( time( NULL ), 365 * 24 * 3600 );
		block.KeyList().push_back( (const EGPublicKey&)pAuth->GetPublicKey() );
		if ( !block.Pack( *pVerifier ) )
		{
			printf( "  our Pack() failed\n" );
			delete pVerifier; delete pAuth;
			return;
		}
		pRaw = (const unsigned char*)block.GetRaw();
		nRaw = (unsigned short)block.GetRawLen();
	}
	catch ( ... )
	{
		printf( "  our Pack() threw\n" );
		delete pVerifier; delete pAuth;
		return;
	}

	printf( "  packed %u bytes; first 16:", nRaw );
	for ( int i = 0; i < 16 && i < nRaw; i++ )
		printf( " %02X", pRaw[i] );
	printf( "\n" );

	void*	pBlock = NULL;
	try
	{
		pBlock = g_New( pRaw, nRaw );
	}
	catch ( ... )
	{
		printf( "  >>> real NewAuthPublicKeyBlock1 THREW (this is the launcher abort)\n" );
		delete pVerifier; delete pAuth;
		return;
	}

	if ( !pBlock )
	{
		printf( "  real NewAuthPublicKeyBlock1 returned NULL\n" );
		delete pVerifier; delete pAuth;
		return;
	}

	printf( "  real NewAuthPublicKeyBlock1 OK -> %p", pBlock );
	if ( g_GetBlockId )		printf( ", blockId=%u", g_GetBlockId( pBlock ) );
	if ( g_GetNumKeys )		printf( ", numKeys=%d", g_GetNumKeys( pBlock ) );
	printf( "\n" );

	// Verify against the matching public verifier key -- what kver.kp supplies.
	try
	{
		const EGPublicKey&	pub = (const EGPublicKey&)pVerifier->GetPublicKey();
		int ok = g_Verify( pBlock, (const unsigned char*)pub.GetKey(),
						   (unsigned short)pub.GetKeyLen() );
		printf( "  real Verify -> %d %s\n", ok, ok ? "(SIGNATURE OK)" : "(rejected)" );
	}
	catch ( ... )
	{
		printf( "  real Verify THREW\n" );
	}

	if ( g_Delete )
		g_Delete( pBlock );
	delete pVerifier;
	delete pAuth;
}

// Does OUR Blowfish encrypt round-trip through the DLL the launcher actually calls?
// Decrypting the client's traffic proves their-encrypt -> our-decrypt; the reply
// direction needs our-encrypt -> their-decrypt, which is what this checks.
typedef void* ( __cdecl *NewBFFn )( unsigned short len, const unsigned char* key );
typedef void ( __thiscall *BFDecFn )( void* self, void* ret, const unsigned char* p, unsigned long n );

static void TryBlowfishInterop( const char* pszDir )
{
	printf( "\n=== our Blowfish encrypt -> real DLL decrypt ===\n" );

	char path[MAX_PATH];
	_snprintf( path, sizeof( path ) - 1, "%s\\WONCrypt.dll", pszDir );
	path[sizeof( path ) - 1] = 0;
	HMODULE h = LoadLibraryA( path );
	if ( !h ) { printf( "  no WONCrypt.dll\n" ); return; }

	NewBFFn	pNew = (NewBFFn)GetProcAddress( h,
		"?NewBFSymmetricKey@WON_CryptFactory@@SAPAVWON_BFSymmetricKey@@GPBE@Z" );
	BFDecFn	pDec = (BFDecFn)GetProcAddress( h,
		"?Decrypt@WON_BFSymmetricKey@@QAE?AVCryptReturn@WON_CryptKeyBase@@PBEK@Z" );
	printf( "  NewBFSymmetricKey=%p Decrypt=%p\n", pNew, pDec );
	if ( !pNew || !pDec ) return;

	unsigned char rgKey[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	// 488 bytes: the size of a real directory reply's inner buffer.  A short test
	// buffer would not catch a block-chaining bug past the first block.
	unsigned char rgMsg[488];
	for ( int i = 0; i < (int)sizeof( rgMsg ); i++ )
		rgMsg[i] = (unsigned char)( i * 7 + 1 );

	WONCrypt::BFSymmetricKey ours;
	ours.Create( sizeof( rgKey ), rgKey );
	WONCrypt::CryptKeyBase::CryptReturn enc = ours.Encrypt( rgMsg, sizeof( rgMsg ) );
	if ( !enc.first ) { printf( "  our Encrypt failed\n" ); return; }
	printf( "  plain %d -> cipher %lu\n", (int)sizeof( rgMsg ), (unsigned long)enc.second );

	void* pTheirs = pNew( sizeof( rgKey ), rgKey );
	if ( !pTheirs ) { printf( "  their NewBFSymmetricKey failed\n" ); delete[] enc.first; return; }

	// CryptReturn is returned by value and is wider than a bare {ptr,len}; give the
	// hidden return slot room to spare rather than smashing the stack.
	unsigned char	retBuf[64];
	memset( retBuf, 0, sizeof( retBuf ) );
	pDec( pTheirs, retBuf, enc.first, (unsigned long)enc.second );

	unsigned char*	pOut = *(unsigned char**)retBuf;
	unsigned long	nOut = *(unsigned long*)( retBuf + 4 );
	if ( !pOut )
		printf( "  >>> their Decrypt returned NULL -- our ciphertext is not readable\n" );
	else if ( nOut != sizeof( rgMsg ) || memcmp( pOut, rgMsg, sizeof( rgMsg ) ) != 0 )
		printf( "  >>> their Decrypt gave %lu bytes, MISMATCH\n", nOut );
	else
		printf( "  round-trip OK (%lu bytes match)\n", nOut );

	delete[] enc.first;
}

int main( int argc, char** argv )
{
	setvbuf( stdout, NULL, _IONBF, 0 );		// crashes must not swallow the trace

	const char* pszDir = ( argc > 1 )
		? argv[1]
		: "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Half-Life 1792";

	TryBlowfishInterop( pszDir );

	if ( !LoadRealWonAuth( pszDir ) )
		return 1;

	// 512-bit is what wonserver_auth.cpp generates today; walk up to the sizes the
	// original WON keys plausibly used.
	// Repeat one size: the packed length varies per signing, and we want to know
	// whether the real parser accepts every length our packer can emit.
	unsigned int rgLens[] = { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 };
	for ( int i = 0; i < sizeof( rgLens ) / sizeof( rgLens[0] ); i++ )
		TryKeyLen( rgLens[i] );


	printf( "\ndone\n" );
	return 0;
}
