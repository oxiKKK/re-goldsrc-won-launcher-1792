// TitanRequest

#include <stdlib.h>
#include <string.h>

#include "AuthRequest.h"
#include "EasySocketError.h"
#include "EasyTitanSocket.h"
#include "ReadBuffer.h"
#include "TitanRequest.h"
#include "WriteBuffer.h"
#include "WON_AuthCertificate1.h"
#include "WON_AuthFactory.h"
#include "WON_BFSymmetricKey.h"
#include "WON_CryptFactory.h"
#include "WON_CryptKeyBase.h"
#include "msg/HeaderTypes.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define TITAN_REQUEST_OUTBOUND_BUF_SIZE				0x100
#define TITAN_REQUEST_CONNECT_TIMEOUT				5000
#define TITAN_REQUEST_SEND_TIMEOUT					1000
#define TITAN_REQUEST_RECV_TIMEOUT					8000
#define TITAN_REQUEST_LENGTH_FIELD_SIZE				4
#define TITAN_REQUEST_SEQUENCE_FIELD_SIZE			2
#define TITAN_REQUEST_INITIAL_SEQUENCE				1
#define TITAN_REQUEST_SEQUENCE_MODE_BASIC			1
#define TITAN_REQUEST_SEQUENCE_MODE_SESSION			2

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void ClearSessionKey( WON_BFSymmetricKey **sessionKey )
{
	if ( *sessionKey )
	{
		WON_CryptFactory::DeleteBFSymmetricKey( *sessionKey );
		*sessionKey = NULL;
	}
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// Initialize a Titan transport wrapper against a pre-resolved endpoint string.
TitanRequest::TitanRequest( const std::string &addrString, int port )
{
	initTitanRequest();
	mAddrString = addrString;
	mPort = port;
	mAddr = EasySocket::getAddrFromString( mAddrString );
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// C-string convenience constructor used by older call sites in the WON auth
// code.
TitanRequest::TitanRequest( const char *addrString, int port )
{
	initTitanRequest();

	if ( addrString && addrString[0] )
		mAddrString = addrString;

	mPort = port;
	mAddr = EasySocket::getAddrFromString( mAddrString );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
TitanRequest::~TitanRequest()
{
	ClearSessionKey( &mSessionKey );
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// Clear all per-connection Titan session state.
void TitanRequest::initTitanRequest( void )
{
	mAddr = 0;
	mUseSequence = FALSE;
	mUseSessionId = FALSE;
	mUseEncryption = FALSE;
	mSessionId = 0;
	mNextSendSeq = 0;
	mNextRecvSeq = 0;
	mSessionKey = NULL;
	mAuthContext = NULL;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// Establish or refresh the peer-auth session used by Titan messages (0x465930)
int TitanRequest::handlePeerLogin( unsigned long communityId, EasyTitanSocket *socket )
{
	WON_AuthCertificate1 *theCertP;
	int sessionId;
	int useSessionId;
	int useEncryption;
	char useSequence;

	// Reset session state before requesting a new session.
	useSequence = mUseSessionId ? TITAN_REQUEST_SEQUENCE_MODE_SESSION : TITAN_REQUEST_SEQUENCE_MODE_BASIC;
	useEncryption = mUseEncryption ? TRUE : FALSE;
	mNextRecvSeq = TITAN_REQUEST_INITIAL_SEQUENCE;
	mNextSendSeq = TITAN_REQUEST_INITIAL_SEQUENCE;

	ClearSessionKey( &mSessionKey );

	useSessionId = mUseSequence ? FALSE : TRUE;
	sessionId = 0;
	theCertP = NULL;

	if ( !mAuthContext->peerLogin(
		socket,
		mAddrString,
		mPort,
		useSequence,
		useEncryption,
		useSessionId,
		&theCertP,
		&mSessionKey,
		&sessionId,
		TRUE ) )
	{
		printf( "Unable to authenticate.\n" );
		return FALSE;
	}

	if ( theCertP->GetCommunityId() != communityId && communityId )
	{
		printf( "Invalid community returned by certificate.\n" );
		WON_AuthFactory::DeleteAuthCertificate1( theCertP );
		return FALSE;
	}

	WON_AuthFactory::DeleteAuthCertificate1( theCertP );
	mSessionId = sessionId;
	return TRUE;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// Wrap an outbound Titan payload with the optional auth/session header (0x465A30)
int TitanRequest::handleAuth( EasyTitanSocket *socket, void *outMsg, void *inMsg, unsigned long peerCommunityId )
{
	WriteBuffer *out;
	WriteBuffer *in;
	WON_CryptKeyBase::CryptReturn encrypted;
	int prefixLen;

	out = (WriteBuffer *)outMsg;
	in = (WriteBuffer *)inMsg;

	if ( !mAuthContext )
	{
		// No auth context.  Forward the message as-is.
		out->append( in->getBuffer(), in->getSize() );
		return TRUE;
	}

	if ( (!mUseSessionId || !mSessionKey) && !handlePeerLogin( peerCommunityId, socket ) )
		return FALSE;

	// Titan frames always start with a length field, followed by the optional
	// auth/session header consumed by the remote endpoint before message parsing.
	out->appendLong( 0 );
	if ( mUseEncryption )
		out->appendByte( WONMsg::EncryptedService );
	if ( mUseSessionId )
		out->appendShort( mSessionId );

	prefixLen = 0;
	if ( mUseSequence )
	{
		// Sequence-authenticated requests keep the first short in the caller's
		// buffer reserved for the rolling outbound sequence number.
		in->setShort( TITAN_REQUEST_SEQUENCE_FIELD_SIZE, mNextSendSeq );
		++mNextSendSeq;
		prefixLen = TITAN_REQUEST_SEQUENCE_FIELD_SIZE;
	}

	if ( mUseEncryption )
	{
		encrypted = mSessionKey->Encrypt( in->getBuffer() + prefixLen, in->getSize() - prefixLen );
		if ( !encrypted.GetData() )
		{
			printf( "Error encrypting: %s\n", mSessionKey->GetLastError() );
			return FALSE;
		}

		out->append( encrypted.GetData(), encrypted.GetLen() );
	}
	else
	{
		out->append( in->getBuffer() + TITAN_REQUEST_LENGTH_FIELD_SIZE, in->getSize() - TITAN_REQUEST_LENGTH_FIELD_SIZE );
	}

	out->setLong( 0, out->getSize() );
	return TRUE;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// Send one Titan request and decode the reply framing (0x465C50)
int TitanRequest::request( void *requestMsg, unsigned long serviceType, unsigned long messageType, void *replyMsg, EasyTitanSocket *socket, unsigned long peerCommunityId )
{
	WriteBuffer outbound( TITAN_REQUEST_OUTBOUND_BUF_SIZE );
	ReadBuffer reply;
	unsigned char headerByte;
	unsigned short seqValue;
	unsigned long serviceValue;
	unsigned long messageValue;
	WON_CryptKeyBase::CryptReturn decrypted;
	int error;
	unsigned int size;

	if ( !handleAuth( socket, &outbound, requestMsg, peerCommunityId ) )
		return FALSE;

	if ( socket->sendBuffer( outbound.getBuffer(), outbound.getSize(), 0, TITAN_REQUEST_SEND_TIMEOUT ) )
	{
		printf( "Error sending message to server.\n" );
		return FALSE;
	}

	size = sizeof( mRecvBuf );
	error = socket->recvTMessage( mRecvBuf, &size, 0, 0, TITAN_REQUEST_RECV_TIMEOUT );
	if ( error == ES_TIMED_OUT )
	{
		printf( "Timed out.\n" );
		return FALSE;
	}
	if ( error )
	{
		printf( "Socket error: %s\n", ES_ErrorTypeToString( error ).c_str() );
		return FALSE;
	}

	reply.setBuffer( (const char *)(mRecvBuf + TITAN_REQUEST_LENGTH_FIELD_SIZE), size - TITAN_REQUEST_LENGTH_FIELD_SIZE );
	if ( !reply.readUByte( &headerByte ) )
		return FALSE;

	if ( mUseSessionId )
	{
		if ( headerByte == 1 )
		{
			// Server rejected the current session.
			printf( "Session key has expired...\n" );
			ClearSessionKey( &mSessionKey );
			return FALSE;
		}
		if ( !reply.readUShort( &seqValue ) || seqValue != mSessionId )
		{
			printf( "Invalid session Id received.\n" );
			return FALSE;
		}
	}

	if ( headerByte == 2 )
	{
		// Encrypted replies replace the visible payload with the decrypted body;
		// later validation continues against that plaintext buffer.
		if ( !mUseEncryption )
		{
			printf( "Error: Received encrypted message on unencrypted channel.\n" );
			return FALSE;
		}

		decrypted = mSessionKey->Decrypt( reply.getDataPtr(), reply.getRemainingSize() );
		if ( !decrypted.GetData() )
		{
			printf( "Error decrypting message: %s\n", mSessionKey->GetLastError() );
			return FALSE;
		}

		memcpy( mRecvBuf, decrypted.GetData(), decrypted.GetLen() );
		reply.setBuffer( (const char *)mRecvBuf, decrypted.GetLen() );
	}
	else
	{
		reply.setBuffer( (const char *)(mRecvBuf + TITAN_REQUEST_LENGTH_FIELD_SIZE), size - TITAN_REQUEST_LENGTH_FIELD_SIZE );
	}

	if ( mUseSequence )
	{
		if ( !reply.readUShort( &seqValue ) || seqValue != mNextRecvSeq )
		{
			printf( "Invalid sequence number received.\n" );
			return FALSE;
		}
		++mNextRecvSeq;
	}

	if ( serviceType )
	{
		if ( !reply.readULong( &serviceValue ) )
		{
			printf( "Error: no service type.\n" );
			return FALSE;
		}
		if ( serviceValue != serviceType )
		{
			printf( "Invalid service type received from server.\n" );
			return FALSE;
		}
		if ( messageType )
		{
			if ( !reply.readULong( &messageValue ) )
			{
				printf( "Error: no message type.\n" );
				return FALSE;
			}
			if ( messageValue != messageType )
			{
				printf( "Invalid message type received from server.\n" );
				return FALSE;
			}
		}
	}

	*(ReadBuffer *)replyMsg = reply;
	return TRUE;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// Convenience overload for one-off Titan requests (0x465F90)
int TitanRequest::request( void *requestMsg, unsigned long serviceType, unsigned long messageType, void *replyMsg, unsigned long peerCommunityId )
{
	EasyTitanSocket socket( EasySocket::TCP, TITAN_REQUEST_RECV_BUF_SIZE );

	if ( socket.connect( mAddr, mPort, TITAN_REQUEST_CONNECT_TIMEOUT, 1 ) )
		return FALSE;

	return request( requestMsg, serviceType, messageType, replyMsg, &socket, peerCommunityId );
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// Install the auth context and transport policy bits that determine whether
// future Titan requests carry sequence numbers, encryption, and session ids.
void TitanRequest::setAuth( AuthRequest *authContext, int useSequence, int useEncryption, int useSessionId )
{
	mAuthContext = authContext;
	mUseSequence = useSequence;
	mUseEncryption = useEncryption;
	mUseSessionId = useSessionId;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void TitanRequest::setAddrPort( const std::string &addrString, int port )
{
	setAddrPort( addrString.c_str(), port );
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// Update the remote endpoint.
void TitanRequest::setAddrPort( const char *addrString, int port )
{
	// Reset session key if the remote endpoint changes.
	if ( (mAddrString.empty() && addrString && *addrString) ||
		 (!mAddrString.empty() && (!addrString || mAddrString != addrString)) ||
		 mPort != port )
	{
		ClearSessionKey( &mSessionKey );
	}

	if ( addrString && addrString[0] )
		mAddrString = addrString;
	else
		mAddrString.erase();

	mPort = port;
	mAddr = EasySocket::getAddrFromString( mAddrString );
}
