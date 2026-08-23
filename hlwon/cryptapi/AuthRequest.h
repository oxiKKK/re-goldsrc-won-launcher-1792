#ifndef _AuthRequest_H
#define _AuthRequest_H

// AuthRequest

#include "EasyTitanSocket.h"
#include "TitanRequest.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class WON_AuthPublicKeyBlock1;
class WON_AuthCertificate1;

class WON_AuthFamilyBuffer;
class WON_BFSymmetricKey;
class WON_EGPublicKey;
class WON_EGPrivateKey;
class ReadBuffer;

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// AuthRequest owns the WON authentication state for this client.
class AuthRequest : public TitanRequest
{
public:
	AuthRequest( const char *exeName, const char *guid, const char *addrString, int port );
	virtual ~AuthRequest();

	// error state and accessors
	int Error_Printf( int mask, char *fmt, ... );
	int InitError( void );
	char *GetLastError( void );
	int ReceivedResponse( void );
	WON_AuthCertificate1 *GetCertificate( void );
	WON_BFSymmetricKey *GetLoginKey( void );
	WON_EGPrivateKey *GetPrivateKey( void );

	// auth server login
	int verifyAuthStuff( WON_AuthFamilyBuffer *buffer );
	int getPublicKeys( EasyTitanSocket *socket );
	int getAuthVerifierKey( char *fileName );
	int handleLoginReply( ReadBuffer *reply, WON_BFSymmetricKey *replyKey );
	int getCertificate( int forceNewKey );

	// peer login and refresh
	int peerLogin( EasyTitanSocket *socket, const std::string &addrString, int port, char useSequence, int useEncryption, int useSessionId, WON_AuthCertificate1 **peerCert, WON_BFSymmetricKey **peerKey, int *sessionId, int refreshIfNeeded );
	void refreshCertificate( void );

	// utility helpers
	static int CreatePath( char *path );
	static char *HashPrint( const void *data, int len );

	// cached server key
	char *HashIP( int addr, unsigned short port, char *dest );
	void storeServerKey( void );
	int readServerKey( void );
	void HandleAuthRefresh( void );

	int setAddrAndPort( const char *addrString, int port );

	// utilities used by auth challenges
	static int ComputeSeededMD5andCRCForFileGroup( unsigned int *digest, char *source, const void *seed );

	char mExeName[256];                           // 0x8038
	char mGUID[100];                              // 0x8138
	int mRefreshState;                            // 0x819C
	int mAuthenticated;                           // 0x81A0
	WON_AuthPublicKeyBlock1 *mAuthPublicKeyBlock; // 0x81A4
	WON_AuthCertificate1 *mCertificate;           // 0x81A8
	WON_BFSymmetricKey *mServerKey;               // 0x81AC
	WON_BFSymmetricKey *mLoginKey;                // 0x81B0
	WON_EGPublicKey *mAuthVerifierKey;            // 0x81B4
	WON_EGPrivateKey *mPrivateKey;                // 0x81B8
	int mAdjustedExpireTime;                      // 0x81BC
	int mIssueTimeDelta;                          // 0x81C0
	int mNextRetryTime;                           // 0x81C4
	int mRetryCount;                              // 0x81C8
	unsigned char mRefreshNonce[8];               // 0x81CC
	EasyTitanSocket mSocket;                      // 0x81D4
	int mErrorFlag;                               // 0x8200
	int mErrorStatus;                             // 0x8204
	int mErrorCode;                               // 0x8208
	int mErrorMask;                               // 0x820C
	char mErrorString[256];                       // 0x8210
};

#endif // _AuthRequest_H
