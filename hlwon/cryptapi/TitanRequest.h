#ifndef _TitanRequest_H
#define _TitanRequest_H

// TitanRequest

#include <string>

class AuthRequest;
class WON_AuthCertificate1;
class WON_BFSymmetricKey;
class EasyTitanSocket;

#define TITAN_REQUEST_RECV_BUF_SIZE 0x8000

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class TitanRequest
{
public:
	TitanRequest( const std::string &addrString, int port );
	TitanRequest( const char *addrString, int port );
	virtual ~TitanRequest();

	void initTitanRequest( void );

	int handlePeerLogin( unsigned long communityId, EasyTitanSocket *socket );
	int handleAuth( EasyTitanSocket *socket, void *outMsg, void *inMsg, unsigned long peerCommunityId );

	int request( void *requestMsg, unsigned long serviceType, unsigned long messageType, void *replyMsg, EasyTitanSocket *socket, unsigned long peerCommunityId );
	int request( void *requestMsg, unsigned long serviceType, unsigned long messageType, void *replyMsg, unsigned long peerCommunityId );

	// Set authentication context and session behavior.
	void setAuth( AuthRequest *authContext, int useSequence, int useEncryption, int useSessionId );
	void setAddrPort( const std::string &addrString, int port );
	void setAddrPort( const char *addrString, int port );

	// NOTE(ox): in the old vc60 decomp. std::string is 0x10 bytes. Now its 0x1c.
#ifdef IDACLANG
	char				mAddrString[0x10];
#else
	std::string			mAddrString;
#endif
	int					mPort;
	unsigned char		mRecvBuf[TITAN_REQUEST_RECV_BUF_SIZE];	// Reply buffer.
	int					mAddr;

	int					mUseSequence;
	int					mUseSessionId;
	int					mUseEncryption;

	unsigned short		mSessionId;
	unsigned short		mNextSendSeq;
	unsigned short		mNextRecvSeq;

	WON_BFSymmetricKey *mSessionKey;	// Current session key.
	AuthRequest*		mAuthContext;	// Used for peer login.
};

#endif // _TitanRequest_H
