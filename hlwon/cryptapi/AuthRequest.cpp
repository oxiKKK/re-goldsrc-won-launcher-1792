// authrequest.cpp -- WON auth request handling

#include "AuthRequest.h"

#include <direct.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "EasySocketError.h"
#include "EasyTitanSocket.h"
#include "ReadBuffer.h"
#include "WriteBuffer.h"
#include "cryptapi.h"
#include "resource.h"
#include "crc.h"
#include "WON_AuthCertificate1.h"
#include "WON_AuthFactory.h"
#include "WON_AuthPublicKeyBlock1.h"
#include "WON_BFSymmetricKey.h"
#include "WON_CryptFactory.h"
#include "WON_CryptKeyBase.h"
#include "WON_EGPrivateKey.h"
#include "WON_EGPublicKey.h"
#include "msg/Auth/TMsgTypesAuth.h"
#include "msg/HeaderTypes.h"
#include "msg/ServerStatus.h"

extern WON_BFSymmetricKey *gConnectionKey;
extern char *(*Callback_GetLocalizedString)( unsigned int );

// buffer sizes
#define AUTH_ERROR_BUFFER_SIZE		1024
#define AUTH_SOCKET_BUFFER_SIZE		0x8000
#define AUTH_MESSAGE_BUFFER_SIZE	0x1000
#define AUTH_WRITE_BUFFER_SIZE		0x100

// timing and sizing
#define AUTH_CONNECT_TIMEOUT		5000
#define AUTH_RETRY_DELAY			5
#define AUTH_MIN_CERT_LIFE			30
#define AUTH_REFRESH_WINDOW			120
#define AUTH_REFRESH_RETRIES		4
#define AUTH_REFRESH_NONCE_LEN		6
#define AUTH_KEY_LEN				8
#define AUTH_DIGEST_LEN				0x10
#define AUTH_RECV_SIZE				4096
#define AUTH_CRC_BLOCK_SIZE			1024

// return types
#define LOGIN_REPLY_FAILED			0
#define LOGIN_REPLY_OK				1
#define LOGIN_REPLY_RETRY			2

// /////////////////////////////////////////////////////////////////////////////
// message ids
// /////////////////////////////////////////////////////////////////////////////

// login reply entry types
class Auth1LoginReplyHL
{
public:
	enum EntryType {
		LRCertificate      = 1,  // Certificate
		LRClientPrivateKey = 2,  // Client's Private Key
		LRPublicKeyBlock   = 3,  // AuthServer Public Key Block
		LRErrorInfo        = 4,  // Extended error info
		LRSecretConfirm    = 5,  // Client secret confirmation (encrypted)
	};
};

// auth state values
#define AUTH_NOT_AUTHENTICATED		0
#define AUTH_AUTHENTICATED			1
#define AUTH_STATUS_NONE			-1
#define AUTH_STATUS_LOGIN			0
#define AUTH_STATUS_CHALLENGE		1

// refresh state values
#define AUTH_REFRESH_IDLE			0
#define AUTH_REFRESH_CONNECTING	1
#define AUTH_REFRESH_WAITING		2

// error mask bits
#define AUTH_ERROR_NET				0x01
#define AUTH_ERROR_GEN				0x02
#define AUTH_ERROR_BADCD			0x04
#define AUTH_ERROR_CORRUPT			0x08
#define AUTH_ERROR_INUSE			0x10
#define AUTH_ERROR_BANNED			0x20

// /////////////////////////////////////////////////////////////////////////////
// Constructor
// /////////////////////////////////////////////////////////////////////////////
AuthRequest::AuthRequest( const char *exeName, const char *guid, const char *addrString, int port )
	: TitanRequest( addrString, port ),
	  mSocket( EasySocket::TCP, AUTH_SOCKET_BUFFER_SIZE )
{
	strcpy( mExeName, exeName );
	strcpy( mGUID, guid );

	mAuthPublicKeyBlock = NULL;
	mAuthVerifierKey = NULL;
	mPrivateKey = NULL;
	mCertificate = NULL;
	mLoginKey = NULL;
	mServerKey = NULL;
	mAuthenticated = AUTH_NOT_AUTHENTICATED;

	mSocket.setType( EasySocket::TCP );
	mSocket.setMaxMsgSize( AUTH_MESSAGE_BUFFER_SIZE );

	mRefreshState = AUTH_REFRESH_IDLE;
	mAdjustedExpireTime = -1;
	mIssueTimeDelta = 0;

	InitError();
	readServerKey();
}

// /////////////////////////////////////////////////////////////////////////////
// Destructor
// /////////////////////////////////////////////////////////////////////////////
AuthRequest::~AuthRequest()
{
	if ( mAuthVerifierKey )
		WON_CryptFactory::DeleteEGPublicKey( mAuthVerifierKey );
	if ( mPrivateKey )
		WON_CryptFactory::DeleteEGPrivateKey( mPrivateKey );
	if ( mCertificate )
		WON_AuthFactory::DeleteAuthCertificate1( mCertificate );
	if ( mAuthPublicKeyBlock )
		WON_AuthFactory::DeleteAuthPublicKeyBlock1( mAuthPublicKeyBlock );
	if ( mLoginKey )
		WON_CryptFactory::DeleteBFSymmetricKey( mLoginKey );
	if ( mServerKey )
		WON_CryptFactory::DeleteBFSymmetricKey( mServerKey );
}

// /////////////////////////////////////////////////////////////////////////////
// Error state and accessors
// /////////////////////////////////////////////////////////////////////////////
int AuthRequest::Error_Printf( int mask, char *fmt, ... )
{
	char string[AUTH_ERROR_BUFFER_SIZE];
	va_list argptr;

	va_start( argptr, fmt );
	mErrorMask |= mask;
	vsprintf( string, fmt, argptr );
	strcpy( mErrorString, string );
	va_end( argptr );
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int AuthRequest::InitError( void )
{
	sprintf( mErrorString, "No Error" );
	mErrorMask = 0;
	mErrorStatus = AUTH_STATUS_NONE;
	mErrorCode = -1;
	mErrorFlag = 0;
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
char *AuthRequest::GetLastError( void )
{
	return mErrorString;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int AuthRequest::ReceivedResponse( void )
{
	return mErrorFlag;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
WON_AuthCertificate1 *AuthRequest::GetCertificate( void )
{
	return mAuthenticated == AUTH_AUTHENTICATED ? mCertificate : NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
WON_BFSymmetricKey *AuthRequest::GetLoginKey( void )
{
	return mAuthenticated == AUTH_AUTHENTICATED ? mLoginKey : NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
WON_EGPrivateKey *AuthRequest::GetPrivateKey( void )
{
	return mAuthenticated == AUTH_AUTHENTICATED ? mPrivateKey : NULL;
}

// /////////////////////////////////////////////////////////////////////////////
// Auth server login
// /////////////////////////////////////////////////////////////////////////////
int AuthRequest::verifyAuthStuff( WON_AuthFamilyBuffer *buffer )
{
	int i;
	WON_AuthPublicKeyBlock1::PubKeyReturn key;

	if ( !mAuthPublicKeyBlock )
	{
		Error_Printf( AUTH_ERROR_GEN, "Need public key block." );
		return FALSE;
	}

	if ( !buffer->IsValid() )
	{
		Error_Printf( AUTH_ERROR_GEN, "Buffer is invalid." );
		return FALSE;
	}

	if ( mAuthPublicKeyBlock->GetNumKeys() <= 0 )
		return FALSE;

	for ( i = 0; i < mAuthPublicKeyBlock->GetNumKeys(); ++i )
	{
		key = i == 0 ? mAuthPublicKeyBlock->GetFirstKey() : mAuthPublicKeyBlock->GetNextKey();
		if ( buffer->Verify( key.mKeyP, key.mKeyLen ) )
			return TRUE;
	}

	return FALSE;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
int AuthRequest::getPublicKeys( EasyTitanSocket *socket )
{
	WriteBuffer request( AUTH_WRITE_BUFFER_SIZE );
	ReadBuffer reply;
	short status;
	short len;
	const unsigned char *data;
	unsigned long remaining;

	request.appendLong( 0 );
	request.appendLong( WONMsg::Auth1LoginHL );
	request.appendLong( WONMsg::Auth1GetPubKeys );
	request.setLong( 0, request.getSize() );

	if ( !TitanRequest::request( &request, WONMsg::Auth1LoginHL, WONMsg::Auth1GetPubKeysReply, &reply, socket, 0 ) )
		return FALSE;

	// Public-key replies are serialized as: status, raw-block length, then the
	// opaque key-block bytes that will later validate certs and key updates.
	if ( !reply.readShort( &status ) || !reply.readShort( &len ) )
	{
		Error_Printf( AUTH_ERROR_GEN, "Error in public key block message." );
		return FALSE;
	}

	data = reply.getTheRest( &remaining );

	if ( status || len < 0 || (unsigned short)len != remaining )
	{
		Error_Printf( AUTH_ERROR_GEN, "Error in public key block message." );
		return FALSE;
	}

	// Replace any older block in-place.  The server is allowed to rotate this
	// material and later replies may tell us to fetch and trust a newer block.
	if ( mAuthPublicKeyBlock )
		WON_AuthFactory::DeleteAuthPublicKeyBlock1( mAuthPublicKeyBlock );

	mAuthPublicKeyBlock = WON_AuthFactory::NewAuthPublicKeyBlock1( data, (unsigned short)len );
	if ( !mAuthVerifierKey )
	{
		Error_Printf( AUTH_ERROR_GEN, "Error: need auth verifier key." );
		return FALSE;
	}

	if ( !mAuthPublicKeyBlock->Verify( mAuthVerifierKey->GetKey(), mAuthVerifierKey->GetKeyLen() ) )
	{
		Error_Printf( AUTH_ERROR_GEN, "Verification of public key block failed." );
		return FALSE;
	}

	return TRUE;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
int AuthRequest::getAuthVerifierKey( char *fileName )
{
	FILE *fp;
	long len;
	char *base;
	char *data;
	WriteBuffer keyBuf( AUTH_WRITE_BUFFER_SIZE );

	fp = fopen( fileName, "rb" );
	if ( !fp )
		return FALSE;

	fseek( fp, 0, SEEK_END );
	len = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	data = (char *)malloc( len + 1 );
	if ( !data )
	{
		fclose( fp );
		return FALSE;
	}

	base = data;
	data[len] = 0;
	fread( data, len, 1, fp );
	fclose( fp );

	if ( mAuthVerifierKey )
	{
		WON_CryptFactory::DeleteEGPublicKey( mAuthVerifierKey );
		mAuthVerifierKey = NULL;
	}

	while ( len-- > 0 )
		keyBuf.appendByte( (unsigned char)*data++ );

	mAuthVerifierKey = WON_CryptFactory::NewEGPublicKey( (unsigned short)keyBuf.getSize(), keyBuf.getBuffer() );
	free( base );

	return (mAuthVerifierKey && !mAuthVerifierKey->GetLastError()) ? TRUE : FALSE;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
int AuthRequest::handleLoginReply( ReadBuffer *reply, WON_BFSymmetricKey *replyKey )
{
	short itemCount;
	int success;
	int havePublicKeyBlock;

	havePublicKeyBlock = FALSE;
	success = FALSE;

	if ( !reply->readShort( &itemCount ) )
		return LOGIN_REPLY_FAILED;

	// Login replies are a compact TLV (type-length-value) stream.
	while ( itemCount-- > 0 )
	{
		unsigned char type;
		short lenValue;
		unsigned short len;
		unsigned char *data;

		if ( !reply->readUByte( &type ) )
			return LOGIN_REPLY_FAILED;
		if ( !reply->readShort( &lenValue ) )
			return LOGIN_REPLY_FAILED;
		len = (unsigned short)lenValue;
		data = reply->getDataPtr();
		if ( !reply->skipBytes( len ) )
			return LOGIN_REPLY_FAILED;

		switch ( type )
		{
		case Auth1LoginReplyHL::LRCertificate:
			// Item 1 is the certificate that proves our current auth identity.
			if ( mCertificate )
				WON_AuthFactory::DeleteAuthCertificate1( mCertificate );
			mCertificate = WON_AuthFactory::NewAuthCertificate1( data, len );
			if ( !verifyAuthStuff( mCertificate ) )
			{
				Error_Printf( AUTH_ERROR_GEN, "Verification of certificate failed." );
				return LOGIN_REPLY_FAILED;
			}
			mErrorStatus = AUTH_STATUS_NONE;
			mErrorCode = -1;
			break;

		case Auth1LoginReplyHL::LRClientPrivateKey:
			{
				if ( !replyKey )
					break;

				// The private key is wrapped with the negotiated login key,
				// so the caller must supply that key before we can materialize it.
				WON_CryptKeyBase::CryptReturn decrypted = replyKey->Decrypt( data, len );
				if ( !decrypted.GetData() )
				{
					Error_Printf( AUTH_ERROR_GEN, "Error decrypting private key." );
					return LOGIN_REPLY_FAILED;
				}

				if ( mPrivateKey )
					WON_CryptFactory::DeleteEGPrivateKey( mPrivateKey );

				mPrivateKey = WON_CryptFactory::NewEGPrivateKey( (unsigned short)decrypted.GetLen(), decrypted.GetData() );
				if ( mPrivateKey && mPrivateKey->GetLastError() )
				{
					Error_Printf( AUTH_ERROR_GEN, "Error constructing private key: %s", mPrivateKey->GetLastError() );
					return LOGIN_REPLY_FAILED;
				}
			}
			break;

		case Auth1LoginReplyHL::LRPublicKeyBlock:
			// This replaces the public-key block mid-login.  Returning 2 tells
			// the caller that the login must be reissued with the fresh key block.
			if ( !mAuthVerifierKey )
			{
				Error_Printf( AUTH_ERROR_GEN, "Error: need auth verifier key." );
				return LOGIN_REPLY_FAILED;
			}
			if ( mAuthPublicKeyBlock )
				WON_AuthFactory::DeleteAuthPublicKeyBlock1( mAuthPublicKeyBlock );
			mAuthPublicKeyBlock = WON_AuthFactory::NewAuthPublicKeyBlock1( data, len );
			if ( !mAuthPublicKeyBlock->Verify( mAuthVerifierKey->GetKey(), mAuthVerifierKey->GetKeyLen() ) )
			{
				Error_Printf( AUTH_ERROR_GEN, "Verification of public key block failed." );
				return LOGIN_REPLY_FAILED;
			}
			havePublicKeyBlock = TRUE;
			break;

		case Auth1LoginReplyHL::LRErrorInfo:
			{
				char errorText[256];

				// The raw server error text exists, but the retail client maps the
				// numeric auth code to localized UI strings whenever possible.
				mErrorFlag = 1;
				if ( len >= sizeof( errorText ) )
					len = sizeof( errorText ) - 1;
				memcpy( errorText, data, len );
				errorText[len] = 0;
				if ( Callback_GetLocalizedString )
				{
					// The server can return a numeric error code and status that map to localized client strings.
					// If the code is unknown, fall back to a generic failure message.
					switch ( mErrorCode )
					{
					case WONMsg::StatusAuth_InvalidCDKey:
						Error_Printf( AUTH_ERROR_BADCD, Callback_GetLocalizedString( IDS_CDKEY_BAD ) );
						break;

					case WONMsg::StatusAuth_CRCFailed:
						Error_Printf( AUTH_ERROR_CORRUPT, Callback_GetLocalizedString( IDS_NET_CORRUPT ) );
						break;

					case WONMsg::StatusAuth_KeyInUse:
						Error_Printf( AUTH_ERROR_INUSE, Callback_GetLocalizedString( IDS_WON_CDINUSE ) );
						break;

					case WONMsg::StatusAuth_LockedOut:
						Error_Printf( AUTH_ERROR_BANNED, Callback_GetLocalizedString( IDS_WON_BANNED ) );
						break;

					case WONMsg::StatusAuth_VerifyFailed:
						if ( mErrorStatus == AUTH_STATUS_LOGIN )
							Error_Printf( AUTH_ERROR_BADCD, Callback_GetLocalizedString( IDS_CDKEY_BAD ) );
						else if ( mErrorStatus == AUTH_STATUS_CHALLENGE )
							Error_Printf( AUTH_ERROR_CORRUPT, Callback_GetLocalizedString( IDS_NET_CORRUPT ) );
						else
							Error_Printf( AUTH_ERROR_GEN, Callback_GetLocalizedString( IDS_WON_AUTHFAILURE ) );
						break;

					default:
						Error_Printf( AUTH_ERROR_GEN, Callback_GetLocalizedString( IDS_WON_AUTHFAILURE ) );
						break;
					}
				}
				break;
			}
		}

		success = TRUE;
	}

	if ( !success )
		return LOGIN_REPLY_FAILED;

	return havePublicKeyBlock ? LOGIN_REPLY_RETRY : LOGIN_REPLY_OK;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
int AuthRequest::getCertificate( int forceNewKey )
{
	EasyTitanSocket socket(EasySocket::TCP, AUTH_SOCKET_BUFFER_SIZE);
	WriteBuffer request(AUTH_WRITE_BUFFER_SIZE);
	WriteBuffer loginBlob(AUTH_WRITE_BUFFER_SIZE);
	ReadBuffer reply;
	WON_CryptKeyBase::CryptReturn encrypted;
	WON_EGPublicKey* publicKey;
	WON_AuthPublicKeyBlock1::PubKeyReturn keyInfo;
	unsigned int digest[4];
	unsigned int crcDigest[4];
	char path[512];
	ES_ErrorType error;

	mErrorFlag = 0;

	error = socket.connect(mAddrString, mPort, AUTH_CONNECT_TIMEOUT, 1);
	if (error)
	{
		Error_Printf(AUTH_ERROR_NET, "Error connecting to Auth server: %s", ES_ErrorTypeToString(error).c_str());
		return FALSE;
	}

	if (!mAuthPublicKeyBlock && !getPublicKeys(&socket))
	{
		Error_Printf(AUTH_ERROR_NET, "Could not obtain pkey block");
		return FALSE;
	}

	if (mLoginKey)
	{
		WON_CryptFactory::DeleteBFSymmetricKey(mLoginKey);
		mLoginKey = NULL;
	}

	// Start with either the cached server key or a fresh random session key.
	if (mServerKey)
		mLoginKey = WON_CryptFactory::NewBFSymmetricKey(mServerKey->GetKeyLen(), mServerKey->GetKey());
	else
		mLoginKey = WON_CryptFactory::NewBFSymmetricKey(AUTH_KEY_LEN, 0);

	// A cached server key means WON can resume from the prior challenge state
	// instead of always forcing a cold login handshake.

	if (!mLoginKey || mLoginKey->GetLastError())
	{
		Error_Printf(AUTH_ERROR_GEN, "Unable to generate WON session key");
		if (mLoginKey)
		{
			WON_CryptFactory::DeleteBFSymmetricKey(mLoginKey);
			mLoginKey = NULL;
		}
		return FALSE;
	}

	request.appendLong(0);
	request.appendLong(WONMsg::Auth1LoginHL);
	request.appendLong(WONMsg::Auth1LoginRequestHL);
	request.appendShort(mAuthPublicKeyBlock->GetBlockId());
	request.appendByte(mPrivateKey == NULL);

	// The encrypted login blob carries the proposed session key plus the game GUID.
	// If the server trusts the cached state it can answer with a direct login reply.
	loginBlob.appendShort(mAuthPublicKeyBlock->GetBlockId());
	loginBlob.appendShort(mLoginKey->GetKeyLen());
	loginBlob.append(mLoginKey->GetKey(), mLoginKey->GetKeyLen());
	loginBlob.appendString(mGUID);

	keyInfo = mAuthPublicKeyBlock->GetFirstKey();
	publicKey = WON_CryptFactory::NewEGPublicKey(keyInfo.mKeyLen, keyInfo.mKeyP);
	encrypted = publicKey->Encrypt(loginBlob.getBuffer(), loginBlob.getSize());
	WON_CryptFactory::DeleteEGPublicKey(publicKey);
	if (!encrypted.GetData())
	{
		Error_Printf(AUTH_ERROR_GEN, "Error encrypting login data.");
		return FALSE;
	}

	request.appendShort((unsigned short)encrypted.GetLen());
	request.append(encrypted.GetData(), (int)encrypted.GetLen());
	request.setLong(0, request.getSize());

	if (!TitanRequest::request(&request, WONMsg::Auth1LoginHL, 0, &reply, &socket, 0))
	{
		Error_Printf(AUTH_ERROR_NET, "Could not communicate with WON, check your internet connection");
		return FALSE;
	}

	unsigned long messageType;

	if ( !reply.readULong( &messageType ) )
	{
		Error_Printf( AUTH_ERROR_GEN, "Invalid message received from Auth server." );
		return FALSE;
	}

	// The server either accepts the proposed session state immediately or forces
	// the checksum challenge path to re-prove local file integrity.
	switch (messageType)
	{
	case WONMsg::Auth1LoginReplyHL:
		{
			short errorCode;

			// A direct login reply usually means the server accepted the current
			// session proposal without forcing the checksum challenge branch.
			if (!reply.readShort(&errorCode) || !errorCode)
			{
				Error_Printf(AUTH_ERROR_GEN, "Invalid message received from Auth server.");
				return FALSE;
			}

			mErrorStatus = AUTH_STATUS_LOGIN;
			mErrorCode = errorCode;
			if (handleLoginReply(&reply, mLoginKey) == LOGIN_REPLY_RETRY)
			{
				// A return value of 2 means the server sent a replacement key block.
				return getCertificate(FALSE);
			}

			return FALSE;
		}

	case WONMsg::Auth1LoginChallengeHL:
		{
			unsigned long remaining;
			const unsigned char* seedData;
			WON_CryptKeyBase::CryptReturn loginReturn;
			short encryptedLen;

			// The challenge seed is reused twice: first as the persisted server key,
			// then as salt for the seeded file-integrity response.
			if (!reply.readShort(&encryptedLen))
			{
				Error_Printf(AUTH_ERROR_GEN, "Invalid message received from Auth server.");
				return FALSE;
			}

			seedData = reply.getTheRest(&remaining);
			if (encryptedLen < 0 || (unsigned short)encryptedLen != remaining)
			{
				Error_Printf(AUTH_ERROR_GEN, "Invalid message received from Auth server.");
				return FALSE;
			}

			loginReturn = mLoginKey->Decrypt(seedData, (unsigned short)encryptedLen);
			if (!loginReturn.GetData() || loginReturn.GetLen() < 8)
			{
				Error_Printf(AUTH_ERROR_GEN, "Error in challenge seed encryption.");
				return FALSE;
			}

			if (mServerKey)
				WON_CryptFactory::DeleteBFSymmetricKey(mServerKey);
			mServerKey = WON_CryptFactory::NewBFSymmetricKey(AUTH_KEY_LEN, loginReturn.GetData());
			storeServerKey();

			// Once the challenge seed is decrypted, it becomes the cached server key and
			// also drives the seeded checksum calculation for this login attempt.
			if (g_authIsServer)
				sprintf(path, "sw.dll");
			else
				sprintf(path, "%s;sw.dll;hw.dll", mExeName);

			// WON validates both the aggregate file set and a seeded digest derived
			// from CRCs of fixed-size chunks across the same file group.

			if (!MD5_Hash_File((unsigned char*)digest, path, FALSE, NULL))
			{
				Error_Printf(AUTH_ERROR_GEN, "Validation failure - 2.");
				return FALSE;
			}

			if (!ComputeSeededMD5andCRCForFileGroup(crcDigest, path, loginReturn.GetData()))
			{
				Error_Printf(AUTH_ERROR_GEN, "Validation failure - 3.");
				return FALSE;
			}

			WriteBuffer checksumBuf(AUTH_WRITE_BUFFER_SIZE);
			WON_CryptKeyBase::CryptReturn checksumReply;

			// The checksum reply is the final proof step in the login exchange.
			checksumBuf.append(digest, AUTH_DIGEST_LEN);
			checksumBuf.append(crcDigest, AUTH_DIGEST_LEN);

			request.rewind();
			request.appendLong(0);
			request.appendLong(WONMsg::Auth1LoginHL);
			request.appendLong(WONMsg::Auth1LoginConfirmHL);

			checksumReply = mLoginKey->Encrypt(checksumBuf.getBuffer(), checksumBuf.getSize());
			if (!checksumReply.GetData())
			{
				Error_Printf(AUTH_ERROR_GEN, "Error encrypting checksum response.");
				return FALSE;
			}

			request.appendShort((unsigned short)checksumReply.GetLen());
			request.append(checksumReply.GetData(), (unsigned int)checksumReply.GetLen());
			request.setLong(0, request.getSize());

			if (!TitanRequest::request(&request, WONMsg::Auth1LoginHL, WONMsg::Auth1LoginReplyHL, &reply, &socket, 0))
			{
				Error_Printf(AUTH_ERROR_NET, "Did not receive a response from WON Authentication server, please check your internet connection.");
				return FALSE;
			}

			{
				short errorCode;

				if (!reply.readShort(&errorCode))
				{
					Error_Printf(AUTH_ERROR_GEN, "Error in challenge message.");
					return FALSE;
				}

				mErrorCode = errorCode;
				mErrorStatus = AUTH_STATUS_CHALLENGE;
				if (!handleLoginReply(&reply, mLoginKey) || errorCode)
					return FALSE;

				mAuthenticated = AUTH_AUTHENTICATED;
				mAdjustedExpireTime = (int)(mCertificate->GetExpireTime() + (int)(time(NULL) - mCertificate->GetIssueTime()));
				mIssueTimeDelta = (int)(mCertificate->GetIssueTime() - time(NULL));
				return TRUE;
			}
		}
	}

	Error_Printf(AUTH_ERROR_GEN, "Invalid message received from Auth server.");
	return FALSE;
}

// /////////////////////////////////////////////////////////////////////////////
// Peer login
// /////////////////////////////////////////////////////////////////////////////
int AuthRequest::peerLogin( EasyTitanSocket *socket, const std::string &addrString, int port, char useSequence, int useEncryption, int useSessionId, WON_AuthCertificate1 **peerCert, WON_BFSymmetricKey **peerKey, int *sessionId, int refreshIfNeeded )
{
	WriteBuffer request( AUTH_WRITE_BUFFER_SIZE );
	WriteBuffer payload( AUTH_WRITE_BUFFER_SIZE );
	ReadBuffer reply;
	WON_CryptKeyBase::CryptReturn secret;
	WON_CryptKeyBase::CryptReturn responseSecret;
	WON_BFSymmetricKey *challengeKey;
	WON_EGPublicKey *peerPublicKey;
	WON_AuthCertificate1 *thePeerCert;
	WON_BFSymmetricKey *thePeerKey;
	unsigned short encLen;
	unsigned short certLen;
	unsigned short status;
	const unsigned char *encData;
	const unsigned char *certData;
	time_t now;

	thePeerCert = NULL;
	thePeerKey = NULL;

	if ( peerCert )
		*peerCert = NULL;
	if ( peerKey )
		*peerKey = NULL;
	if ( sessionId )
		*sessionId = 0;

	now = time( NULL );
	if ( !mCertificate || now > mAdjustedExpireTime || (mAdjustedExpireTime - now) < AUTH_MIN_CERT_LIFE )
	{
		// Peer auth depends on a still-valid local WON certificate.  If ours is
		// near expiry, refresh it first so the peer never sees stale credentials.
		if ( !getCertificate( TRUE ) )
			return 0;
	}

	request.appendLong( 0 );
	request.appendLong( WONMsg::Auth1PeerToPeer );
	request.appendLong( WONMsg::Auth1Request );
	request.appendByte( useSequence );
	request.appendByte( useEncryption );
	request.appendShort( (unsigned short)useSessionId );
	request.appendShort( mCertificate->GetRawLen() );
	request.append( mCertificate->GetRaw(), mCertificate->GetRawLen() );
	request.setLong( 0, request.getSize() );

	if ( !TitanRequest::request( &request, WONMsg::Auth1PeerToPeer, 0, &reply, socket, 0 ) )
		return FALSE;

	{
		unsigned long messageType;

		// The peer either accepts the presented certificate flow or asks us to refresh
		// our WON cert before it will continue the handshake.
		if ( !reply.readULong( &messageType ) )
			return FALSE;

		switch ( messageType )
		{
		case WONMsg::Auth1Complete:
			{
				short ignoredValue;

				if ( !reply.readShort( &ignoredValue ) )
					return FALSE;
				if ( refreshIfNeeded && getCertificate( TRUE ) )
					return peerLogin( socket, addrString, port, useSequence, useEncryption, useSessionId, peerCert, peerKey, sessionId, 0 );
				return FALSE;
			}

		case WONMsg::Auth1Challenge1:
			break;

		default:
			Error_Printf( AUTH_ERROR_GEN, "Invalid message type received from peer." );
			return FALSE;
		}
	}

	if ( !reply.readUShort( &encLen ) )
	{
		Error_Printf( AUTH_ERROR_GEN, "Invalid message received from peer." );
		return FALSE;
	}

	// The first encrypted peer blob contains its proposed symmetric key material
	// plus enough structure to prove the decrypt succeeded cleanly.
	encData = reply.getDataPtr();
	if ( !reply.skipBytes( encLen ) )
	{
		Error_Printf( AUTH_ERROR_GEN, "Invalid message received from peer." );
		return FALSE;
	}

	secret = mPrivateKey->Decrypt( encData, encLen );
	if ( !secret.GetData() || secret.GetLen() < 2 || *(unsigned short *)secret.GetData() != secret.GetLen() - 2 )
	{
		Error_Printf( AUTH_ERROR_GEN, "Invalid message received from peer." );
		return FALSE;
	}

	// The decrypted peer blob starts with a length-prefixed symmetric key that
	// becomes the shared connection key if the rest of the exchange succeeds.
	if ( peerKey )
	{
		thePeerKey = WON_CryptFactory::NewBFSymmetricKey( (unsigned short)(secret.GetLen() - 2), secret.GetData() + 2 );
		if ( !thePeerKey || thePeerKey->GetLastError() )
		{
			if ( thePeerKey )
			{
				WON_CryptFactory::DeleteBFSymmetricKey( thePeerKey );
				thePeerKey = NULL;
			}
			Error_Printf( AUTH_ERROR_GEN, "Invalid message received from peer." );
			return FALSE;
		}
	}

	if ( !reply.readUShort( &certLen ) )
	{
		Error_Printf( AUTH_ERROR_GEN, "Invalid message received from peer." );
		goto failure;
	}

	certData = reply.getDataPtr();
	if ( !reply.skipBytes( certLen ) )
	{
		Error_Printf( AUTH_ERROR_GEN, "Invalid message received from peer." );
		goto failure;
	}

	thePeerCert = WON_AuthFactory::NewAuthCertificate1( certData, certLen );
	if ( !verifyAuthStuff( thePeerCert ) )
	{
		WON_AuthFactory::DeleteAuthCertificate1( thePeerCert );
		thePeerCert = NULL;
		goto failure;
	}

	// From here on the peer certificate is trusted enough to use its embedded
	// public key for the challenge round-trip that proves private-key ownership.
	if ( (thePeerCert->GetExpireTime() - mIssueTimeDelta) < time( NULL ) )
		Error_Printf( AUTH_ERROR_GEN, "The server's certificate has expired." );

	// Challenge step two: encrypt the peer's proposed secret plus our random
	// challenge key under the peer public key and require it back untouched.
	request.rewind();
	request.appendLong( 0 );
	request.appendLong( WONMsg::Auth1PeerToPeer );
	request.appendLong( WONMsg::Auth1Challenge2 );

	challengeKey = WON_CryptFactory::NewBFSymmetricKey( AUTH_KEY_LEN, NULL );
	payload.appendShort( (unsigned short)(secret.GetLen() - 2) );
	payload.append( secret.GetData() + 2, (int)(secret.GetLen() - 2) );
	payload.append( challengeKey->GetKey(), challengeKey->GetKeyLen() );

	peerPublicKey = WON_CryptFactory::NewEGPublicKey( thePeerCert->GetPubKeyLen(), thePeerCert->GetPubKey() );
	if ( peerPublicKey->GetLastError() )
	{
		Error_Printf( AUTH_ERROR_GEN, "Error in Client B's public key: %s", peerPublicKey->GetLastError() );
		WON_CryptFactory::DeleteEGPublicKey( peerPublicKey );
		goto challenge_failure;
	}

	responseSecret = peerPublicKey->Encrypt( payload.getBuffer(), payload.getSize() );
	if ( !responseSecret.GetData() )
	{
		Error_Printf( AUTH_ERROR_GEN, "Error Encrypting secret data: %s", peerPublicKey->GetLastError() );
		WON_CryptFactory::DeleteEGPublicKey( peerPublicKey );
		goto challenge_failure;
	}
	WON_CryptFactory::DeleteEGPublicKey( peerPublicKey );

	request.appendShort( (unsigned short)responseSecret.GetLen() );
	request.append( responseSecret.GetData(), (int)responseSecret.GetLen() );
	request.setLong( 0, request.getSize() );

	if ( !TitanRequest::request( &request, WONMsg::Auth1PeerToPeer, WONMsg::Auth1Complete, &reply, socket, 0 ) )
		goto challenge_failure;

	// The peer must echo our random challenge key back inside its encrypted reply.
	if ( !reply.readUShort( &status ) )
	{
		Error_Printf( AUTH_ERROR_GEN, "Error in message received from peer." );
		goto challenge_failure;
	}

	if ( (short)status < 0 )
	{
		Error_Printf( AUTH_ERROR_GEN, "Error status on authentication: %d", (short)status );
		goto challenge_failure;
	}

	if ( !reply.readUShort( &encLen ) )
	{
		Error_Printf( AUTH_ERROR_GEN, "Error in message received from peer." );
		goto challenge_failure;
	}

	// A successful peer must return our challenge key inside its encrypted reply.
	// That proves it could decrypt the earlier message with its private key.
	encData = reply.getDataPtr();
	if ( !reply.skipBytes( encLen ) )
	{
		Error_Printf( AUTH_ERROR_GEN, "Error in message received from peer." );
		goto challenge_failure;
	}

	if ( sessionId )
	{
		unsigned short peerSessionId;

		if ( !reply.readUShort( &peerSessionId ) )
		{
			Error_Printf( AUTH_ERROR_GEN, "Error in message received from peer." );
			goto challenge_failure;
		}

		*sessionId = peerSessionId;
	}

	responseSecret = mPrivateKey->Decrypt( encData, encLen );
	if ( !responseSecret.GetData() ||
		 responseSecret.GetLen() < 2 ||
		 *(unsigned short *)responseSecret.GetData() != challengeKey->GetKeyLen() ||
		 memcmp( responseSecret.GetData() + 2, challengeKey->GetKey(), challengeKey->GetKeyLen() ) )
	{
		Error_Printf( AUTH_ERROR_GEN, "Invalid secret returned by peer." );
		goto challenge_failure;
	}

	WON_CryptFactory::DeleteBFSymmetricKey( challengeKey );
	if ( peerCert )
		*peerCert = thePeerCert;
	else if ( thePeerCert )
		WON_AuthFactory::DeleteAuthCertificate1( thePeerCert );

	if ( peerKey )
		*peerKey = thePeerKey;

	return TRUE;
///////////////////////////////////////////////////////////////////////////////

challenge_failure:
	WON_CryptFactory::DeleteBFSymmetricKey( challengeKey );

failure:
	if ( thePeerCert )
	{
		WON_AuthFactory::DeleteAuthCertificate1( thePeerCert );
		thePeerCert = NULL;
	}
	if ( thePeerKey )
	{
		WON_CryptFactory::DeleteBFSymmetricKey( thePeerKey );
		thePeerKey = NULL;
	}
	if ( peerCert )
		*peerCert = NULL;
	if ( peerKey )
		*peerKey = NULL;
	return FALSE;
}

// /////////////////////////////////////////////////////////////////////////////
// Certificate refresh and server-key cache
// /////////////////////////////////////////////////////////////////////////////
void AuthRequest::refreshCertificate( void )
{
	WriteBuffer request( AUTH_WRITE_BUFFER_SIZE );
	WriteBuffer payload( AUTH_WRITE_BUFFER_SIZE );
	WON_CryptKeyBase::CryptReturn encrypted;
	int i;

	request.appendLong( 0 );
	request.appendLong( WONMsg::Auth1LoginHL );
	request.appendLong( WONMsg::Auth1RefreshHL );
	request.appendLong( mCertificate->GetUserId() );

	// The refresh nonce lets the server bind its reply to this specific refresh
	// attempt instead of any earlier in-flight retry.
	for ( i = 0; i < AUTH_REFRESH_NONCE_LEN; ++i )
	{
		mRefreshNonce[i] = (unsigned char)(rand() % 255);
		payload.appendByte( mRefreshNonce[i] );
	}
	payload.appendLong( mCertificate->GetUserId() );

	encrypted = mLoginKey->Encrypt( payload.getBuffer(), payload.getSize() );
	if ( !encrypted.GetData() )
	{
		mRetryCount = -1;
		return;
	}

	request.append( encrypted.GetData(), (int)encrypted.GetLen() );
	request.setLong( 0, request.getSize() );

	if ( mSocket.sendBuffer( request.getBuffer(), request.getSize(), (int*)0, (unsigned int)0 ) )
	{
		// A successful send moves the refresh logic back into async connect mode.
		mRefreshState = AUTH_REFRESH_CONNECTING;
		mNextRetryTime = (int)time( NULL ) + AUTH_RETRY_DELAY;
		--mRetryCount;
		mSocket.connect( mAddr, mPort, 0, 1 );
	}
	else
	{
		mRefreshState = AUTH_REFRESH_WAITING;
	}
}

// /////////////////////////////////////////////////////////////////////////////
// Utility helpers
// /////////////////////////////////////////////////////////////////////////////
int AuthRequest::CreatePath( char *path )
{
	int result;
	char *scan;

	// Build any missing intermediate directories in-place so callers can pass
	// a full relative file path instead of pre-creating the auth cache tree.
	result = 0;
	for ( scan = path + 1; *scan; ++scan )
	{
		if ( *scan == '\\' || *scan == '/' )
		{
			char saved = *scan;

			*scan = 0;
			result = _mkdir( path );
			*scan = saved;
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
char *AuthRequest::HashPrint( const void *data, int len )
{
	static char hash[32];
	char buffer[8];
	int i;

	memset( hash, 0, sizeof( hash ) );
	for ( i = 0; i < len; ++i )
	{
		sprintf( buffer, "%02x", ((const unsigned char *)data)[i] );
		strcat( hash, buffer );
	}

	return hash;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
char *AuthRequest::HashIP( int addr, unsigned short port, char *dest )
{
	unsigned short hash;
	unsigned char *scan;
	int i;

	hash = 0;
	scan = (unsigned char *)&addr;
	for ( i = 0; i < 4; ++i )
	{
		hash ^= scan[i];
		hash += scan[i];
	}

	scan = (unsigned char *)&port;
	for ( i = 0; i < 2; ++i )
	{
		hash ^= scan[i];
		hash += scan[i];
	}

	strcpy( dest, HashPrint( &hash, 2 ) );
	dest[4] = 0;
	return dest;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
void AuthRequest::storeServerKey( void )
{
	char suffix[256];
	char fileName[512];
	const char *prefix;
	FILE *fp;

	HashIP( mAddr, (unsigned short)mPort, suffix );
	prefix = g_authIsServer ? "ksv-" : "kcl-";
	// Split cache files by auth mode and hashed endpoint so each server keeps
	// its own persisted challenge key without colliding with other targets.
	sprintf( fileName, "auth\\%s%s.dat", prefix, suffix );

	if ( !mServerKey )
		return;

	CreatePath( fileName );
	fp = fopen( fileName, "wb" );
	if ( !fp )
		return;

	fwrite( mServerKey->GetKey(), 1, mServerKey->GetKeyLen(), fp );
	fflush( fp );
	fclose( fp );
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
int AuthRequest::readServerKey( void )
{
	char suffix[256];
	char fileName[512];
	char *prefix;
	FILE *fp;
	long len;
	char *base;
	char *data;
	WriteBuffer keyBuf( AUTH_WRITE_BUFFER_SIZE );

	HashIP( mAddr, (unsigned short)mPort, suffix );
	prefix = g_authIsServer ? "ksv-" : "kcl-";
	// ksv- and kcl- mirror the two auth modes; the hashed suffix keeps the key
	// cache stable per address/port without writing raw endpoint strings to disk.
	sprintf( fileName, "auth\\%s%s.dat", prefix, suffix );
	CreatePath( fileName );

	fp = fopen( fileName, "rb" );
	if ( !fp )
		return FALSE;

	fseek( fp, 0, SEEK_END );
	len = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	data = (char *)malloc( len + 1 );
	if ( !data )
	{
		fclose( fp );
		return FALSE;
	}

	base = data;
	data[len] = 0;
	fread( data, len, 1, fp );
	fclose( fp );

	// Rebuild the serialized key byte-for-byte into a WriteBuffer because the WON
	// factory here expects a contiguous buffer with an explicit short length.
	if ( mServerKey )
	{
		WON_CryptFactory::DeleteBFSymmetricKey( mServerKey );
		mServerKey = NULL;
	}

	while ( len-- > 0 )
		keyBuf.appendByte( (unsigned char)*data++ );

	mServerKey = WON_CryptFactory::NewBFSymmetricKey( (unsigned short)keyBuf.getSize(), keyBuf.getBuffer() );
	free( base );

	if ( !mServerKey || mServerKey->GetLastError() )
	{
		if ( mServerKey )
		{
			WON_CryptFactory::DeleteBFSymmetricKey( mServerKey );
			mServerKey = NULL;
		}
		return FALSE;
	}

	return TRUE;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
void AuthRequest::HandleAuthRefresh( void )
{
	time_t now;
	ReadBuffer reply;
	unsigned int serviceType;
	unsigned int messageType;
	unsigned int size;

	if ( !mCertificate || !mServerKey )
		return;

	// Refresh runs as a small state machine over the persistent auth socket:
	// 0 = idle, 1 = connecting/retrying, 2 = waiting for the login reply.
	now = time( NULL );
	if ( mRefreshState == AUTH_REFRESH_IDLE )
	{
		// Idle state.
		if ( now > mAdjustedExpireTime || (mAdjustedExpireTime - now) < AUTH_REFRESH_WINDOW )
		{
			mRetryCount = AUTH_REFRESH_RETRIES;
			mRefreshState = AUTH_REFRESH_CONNECTING;
			mNextRetryTime = (int)now + AUTH_RETRY_DELAY;
			mSocket.connect( mAddr, mPort, 0, 1 );
			return;
		}
		return;
	}

	if ( mRefreshState == AUTH_REFRESH_CONNECTING )
	{
		if ( mRetryCount < 0 )
			return;
		// Connect/retry state.
		if ( mSocket.checkAsynchConnect( 0 ) )
			refreshCertificate();
		else if ( now > mNextRetryTime )
		{
			--mRetryCount;
			mNextRetryTime = (int)now + AUTH_RETRY_DELAY;
			mSocket.connect( mAddr, mPort, 0, 1 );
		}
		return;
	}

	if ( mRefreshState == AUTH_REFRESH_WAITING )
	{
		size = AUTH_RECV_SIZE;
		serviceType = WONMsg::Auth1LoginHL;
		messageType = WONMsg::Auth1LoginReplyHL;
		// Reply-wait state.
		ES_ErrorType error = mSocket.recvTMessage( mRecvBuf, &size, &serviceType, &messageType, 0 );
		if ( error )
		{
			if ( error != ES_TIMED_OUT )
			{
				mRefreshState = AUTH_REFRESH_CONNECTING;
				mNextRetryTime = (int)time( NULL ) + AUTH_RETRY_DELAY;
				--mRetryCount;
				mSocket.connect( mAddr, mPort, 0, 1 );
			}

			return;
		}

		reply.setBuffer( (const char *)mRecvBuf, size );
		mErrorStatus = AUTH_STATUS_NONE;
		mErrorCode = -1;
		if ( handleLoginReply( &reply, NULL ) )
			mRefreshState = AUTH_REFRESH_IDLE;
		else
		{
			mRefreshState = AUTH_REFRESH_CONNECTING;
			mNextRetryTime = (int)time( NULL ) + AUTH_RETRY_DELAY;
			--mRetryCount;
			mSocket.connect( mAddr, mPort, 0, 1 );
		}
	}
}

// /////////////////////////////////////////////////////////////////////////////
// Endpoint update
// /////////////////////////////////////////////////////////////////////////////
int AuthRequest::setAddrAndPort( const char *addrString, int port )
{
	// Switching endpoints changes which cached server key file applies, so
	// reload that state immediately after the TitanRequest address update.
	TitanRequest::setAddrPort( addrString, port );
	return readServerKey();
}

// /////////////////////////////////////////////////////////////////////////////
// Checksum helpers
// /////////////////////////////////////////////////////////////////////////////
int AuthRequest::ComputeSeededMD5andCRCForFileGroup( unsigned int *digest, char *source, const void *seed )
{
	char destination[260];
	char *cur;
	char *end;
	int total;
	int size;
	unsigned char *data;
	char *crcList;
	int crcCount;
	unsigned char *scan;
	char *crcOut;
	CRC32_t crc;

	total = 0;
	cur = source;
	// source is a semicolon-delimited file group.  The original auth scheme
	// treats that group as one logical blob before chunking it into CRC blocks.
	if ( cur )
	{
		while ( *cur )
		{
			if ( *cur == ';' )
				++cur;
			end = cur;
			while ( *end && *end != ';' )
				++end;
			strncpy( destination, cur, end - cur );
			destination[end - cur] = 0;
			cur = end;

			{
				FILE *fp = fopen( destination, "rb" );
				if ( fp )
				{
					fseek( fp, 0, SEEK_END );
					total += (int)ftell( fp );
					fclose( fp );
				}
			}
		}
	}

	data = (unsigned char *)new unsigned char[total + 1];
	crcCount = (total + (AUTH_CRC_BLOCK_SIZE - 1)) / AUTH_CRC_BLOCK_SIZE;
	crcList = (char *)new char[crcCount * sizeof( unsigned int ) + sizeof( unsigned int )];

	// First pass measures the flattened file size; second pass fills the buffer so
	// CRCs can be taken over fixed offsets regardless of original file splits.
	cur = source;
	scan = data;
	if ( cur )
	{
		while ( *cur )
		{
			FILE *fp;
			long fileLen;

			if ( *cur == ';' )
				++cur;
			end = cur;
			while ( *end && *end != ';' )
				++end;
			strncpy( destination, cur, end - cur );
			destination[end - cur] = 0;
			cur = end;

			fp = fopen( destination, "rb" );
			if ( !fp )
				continue;
			fseek( fp, 0, SEEK_END );
			fileLen = ftell( fp );
			fseek( fp, 0, SEEK_SET );
			fread( scan, fileLen, 1, fp );
			fclose( fp );
			scan += fileLen;
		}
	}

	scan = data;
	size = total;
	crcOut = crcList;

	// The CRC list becomes the salted input to MD5_Hash_CachedFile, which ties
	// local file integrity to the server-provided challenge seed for this login.
	while ( size >= AUTH_CRC_BLOCK_SIZE )
	{
		CRC32_Init( &crc );
		CRC32_ProcessBuffer( &crc, scan, AUTH_CRC_BLOCK_SIZE );
		*(unsigned int *)crcOut = CRC32_Final( crc );
		crcOut += sizeof( unsigned int );
		scan += AUTH_CRC_BLOCK_SIZE;
		size -= AUTH_CRC_BLOCK_SIZE;
	}

	if ( size )
	{
		CRC32_Init( &crc );
		CRC32_ProcessBuffer( &crc, scan, size );
		*(unsigned int *)crcOut = CRC32_Final( crc );
	}

	delete[] data;
	size = MD5_Hash_CachedFile( (unsigned char *)digest, (unsigned char *)crcList, crcCount, 1, (unsigned int *)seed );
	delete[] crcList;
	return size;
}

// CryptApi_AuthHasError  (0x47B8D0)
int CryptApi_AuthHasError( void* pAuth )
{
	return pAuth ? ( (AuthRequest*)pAuth )->mErrorFlag : 0;
}

// CryptApi_AuthErrorState  (0x47B8E0)
int CryptApi_AuthErrorState( void* pAuth )
{
	return pAuth ? ( (AuthRequest*)pAuth )->mErrorStatus : 0;
}

// CryptApi_AuthErrorCode  (0x47B8F0)
int CryptApi_AuthErrorCode( void* pAuth )
{
	return pAuth ? ( (AuthRequest*)pAuth )->mErrorCode : 0;
}

// CryptApi_AuthErrorString  (0x47B8C0)
char* CryptApi_AuthErrorString( void* pAuth )
{
	static char	szNone[1] = "";
	return pAuth ? ( (AuthRequest*)pAuth )->mErrorString : szNone;
}
