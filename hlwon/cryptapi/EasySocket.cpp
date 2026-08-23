#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "EasySocket.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define ES_SELECT_WIDTH					64
#define ES_WINSOCK_VERSION				0x101
#define ES_NONBLOCKING_IOCTL			0x8004667E
#define ES_SOCKET_TCP_OPT_LEVEL			0xFFFF
#define ES_SOCKET_TCP_OPT_NAME			0xFFFFFF7F

#ifndef SD_SEND
#define SD_SEND						1
#endif

#ifdef WIN32
// Keep Winsock alive for the lifetime of the module.
static InitWinsock gEasySocketWinsock;
#endif // WIN32

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int EasySocket::ESGetLastError()
{
	return WSAGetLastError();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::GetTickCount (0x40CA60)
unsigned long EasySocket::GetTickCount()
{
	return ::GetTickCount();
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::EasySocket (0x40C110)
EasySocket::EasySocket( SocketType theType )
{
	mSocket = INVALID_SOCKET;
	mType = theType;
	mConnected = false;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// Default construction leaves the socket untyped so older call sites can pick
// TCP or UDP later through setType.
EasySocket::EasySocket()
{
	mSocket = INVALID_SOCKET;
	mType = NO_TYPE;
	mConnected = false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::~EasySocket (0x40C130)
EasySocket::~EasySocket()
{
	close();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::setType (0x40C240)
void EasySocket::setType( SocketType theType )
{
	if ( mType != theType )
		close();

	mType = theType;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::getNewDescriptor (0x40C260)
ES_ErrorType EasySocket::getNewDescriptor( void )
{
	int optVal;
	u_long argp;

	// Descriptor creation always starts from a closed socket.
	if ( !isInvalid() )
		close();

	if ( mType == NO_TYPE )
		return ES_ERROR_NO_TYPE;

	switch ( mType )
	{
	case TCP:
		mSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		break;

	case UDP:
		mSocket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
		break;

	default:
		// This binary only has concrete setup for TCP and UDP here.
		break;
	}

	if ( mSocket == INVALID_SOCKET )
		return WSAErrorToEnum( ESGetLastError() );

	if ( mType == TCP )
	{
		// Match the original TCP setup before switching to nonblocking mode.
		optVal = 1;
		if ( setsockopt( mSocket, ES_SOCKET_TCP_OPT_LEVEL, ES_SOCKET_TCP_OPT_NAME, (const char *)&optVal, 4 ) == SOCKET_ERROR )
			return WSAErrorToEnum( ESGetLastError() );
	}

	// The binary forces all descriptors into nonblocking mode here.
	argp = 1;
	if ( ioctlsocket( mSocket, ES_NONBLOCKING_IOCTL, &argp ) == SOCKET_ERROR )
		return WSAErrorToEnum( ESGetLastError() );

	return ES_NO_ERROR;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// Bind the socket to a local port, optionally enabling address reuse first.
ES_ErrorType EasySocket::bind( int thePort, bool allowReuse )
{
	SOCKADDR_IN aSockAddr;
	int optVal;
	ES_ErrorType result;

	if ( isInvalid() )
	{
		result = getNewDescriptor();
		if ( result != ES_NO_ERROR )
			return result;
	}

	if ( allowReuse )
	{
		optVal = 1;
		setsockopt( mSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&optVal, sizeof( optVal ) );
	}

	memset( &aSockAddr, 0, sizeof( aSockAddr ) );
	aSockAddr.sin_family = AF_INET;
	aSockAddr.sin_addr.s_addr = INADDR_ANY;
	aSockAddr.sin_port = htons( (unsigned short)thePort );

	if ( ::bind( mSocket, (SOCKADDR *)&aSockAddr, sizeof( aSockAddr ) ) == SOCKET_ERROR )
		return WSAErrorToEnum( ESGetLastError() );

	return ES_NO_ERROR;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::connect (0x40C340)
ES_ErrorType EasySocket::connect( const SOCKADDR &theSockAddr, int theWaitTime )
{
	ES_ErrorType result;

	// TCP always reconnects through a fresh descriptor.
	if ( mType == TCP )
		disconnect();

	if ( isInvalid() )
	{
		result = getNewDescriptor();
		if ( result != ES_NO_ERROR )
			return result;
	}

	if ( ::connect( mSocket, &theSockAddr, sizeof( theSockAddr ) ) == SOCKET_ERROR )
	{
		result = WSAErrorToEnum( ESGetLastError() );
		if ( result != ES_WSAEWOULDBLOCK )
			return result;
	}

	mConnected = true;
	mDestAddr = theSockAddr;

	// A zero timeout leaves the connect in its async state.
	if ( theWaitTime && mType == TCP && !waitForWrite( theWaitTime ) )
	{
		disconnect();
		return ES_TIMED_OUT;
	}

	return ES_NO_ERROR;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::connect (0x40C3E0) -- the wait time doubles on every retry.
ES_ErrorType EasySocket::connect( const SOCKADDR &theSockAddr, int theWaitTime, int theNumTries )
{
	int tries;
	ES_ErrorType result;

	tries = 0;
	if ( theNumTries <= 0 )
		return (ES_ErrorType)theNumTries;

	for ( ; ; )
	{
		result = connect( theSockAddr, theWaitTime );
		if ( result == ES_NO_ERROR )
			return ES_NO_ERROR;

		theWaitTime *= 2;
		if ( ++tries >= theNumTries )
			return result;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::connect (0x40C430)
ES_ErrorType EasySocket::connect( long theAddress, int thePort, int theWaitTime, int theNumTries )
{
	SOCKADDR_IN aSockAddr;

	memset( &aSockAddr, 0, sizeof( aSockAddr ) );
	aSockAddr.sin_family = AF_INET;
	aSockAddr.sin_port = htons( (unsigned short)thePort );
	aSockAddr.sin_addr.s_addr = theAddress;

	return connect( (SOCKADDR &)aSockAddr, theWaitTime, theNumTries );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_IPX
ES_ErrorType EasySocket::connect( unsigned char theAddress[6], int thePort, int theWaitTime, int theNumTries )
{
	SOCKADDR_IPX aSockAddr;
	ES_ErrorType result;

	result = getSockAddrIpx( aSockAddr, theAddress, thePort );
	if ( result != ES_NO_ERROR )
		return result;

	return connect( (SOCKADDR &)aSockAddr, theWaitTime, theNumTries );
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::connect (0x40C490)
ES_ErrorType EasySocket::connect( const string &theAddress, int thePort, int theWaitTime, int theNumTries )
{
	SOCKADDR_IN aSockAddr;
	ES_ErrorType result;

	result = getSockAddrIn( aSockAddr, theAddress, thePort );
	if ( result != ES_NO_ERROR )
		return result;

	return connect( (SOCKADDR &)aSockAddr, theWaitTime, theNumTries );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ES_ErrorType EasySocket::listen( int theBacklog )
{
	ES_ErrorType result;

	if ( mType != TCP )
		return ES_ERROR_STREAM_NOT_ALLOWED;

	if ( isInvalid() )
	{
		result = getNewDescriptor();
		if ( result != ES_NO_ERROR )
			return result;
	}

	if ( ::listen( mSocket, theBacklog ) == SOCKET_ERROR )
		return WSAErrorToEnum( ESGetLastError() );

	return ES_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ES_ErrorType EasySocket::accept( EasySocket *theEasySocket, int theWaitTime )
{
	SOCKADDR aSockAddr;
	int addrLen;
	SOCKET newSocket;

	if ( !theEasySocket )
		return ES_INVALID_SOCKET;

	if ( !waitForRead( theWaitTime ) )
		return ES_TIMED_OUT;

	addrLen = sizeof( aSockAddr );
	newSocket = ::accept( mSocket, &aSockAddr, &addrLen );
	if ( newSocket == INVALID_SOCKET )
		return WSAErrorToEnum( ESGetLastError() );

	theEasySocket->close();
	theEasySocket->mSocket = newSocket;
	theEasySocket->mType = mType;
	theEasySocket->mDestAddr = aSockAddr;
	theEasySocket->mConnected = true;
	return ES_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::checkAsynchConnect (0x40C4D0)
bool EasySocket::checkAsynchConnect( int theWaitTime )
{
	return !isInvalid() && waitForWrite( theWaitTime );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool EasySocket::waitForAccept( int theWaitTime )
{
	return waitForRead( theWaitTime );
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::waitForWrite (0x40C500) -- also the nonblocking-connect success test.
bool EasySocket::waitForWrite( int theWaitTime )
{
	fd_set aWriteSet;
	TIMEVAL aTimeout;
	int waitTime;

	if ( isInvalid() )
		return false;

	waitTime = theWaitTime;
	if ( waitTime < 0 )
		waitTime = 0;

	FD_ZERO( &aWriteSet );
	FD_SET( mSocket, &aWriteSet );

	aTimeout.tv_sec = waitTime / 1000;
	aTimeout.tv_usec = 1000 * (waitTime % 1000);

	return select( ES_SELECT_WIDTH, 0, &aWriteSet, 0, &aTimeout ) == 1;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::waitForRead (0x40C590)
bool EasySocket::waitForRead( int theWaitTime )
{
	fd_set aReadSet;
	TIMEVAL aTimeout;
	int waitTime;

	if ( isInvalid() )
		return false;

	waitTime = theWaitTime;
	if ( waitTime < 0 )
		waitTime = 0;

	FD_ZERO( &aReadSet );
	FD_SET( mSocket, &aReadSet );

	aTimeout.tv_sec = waitTime / 1000;
	aTimeout.tv_usec = 1000 * (waitTime % 1000);

	return select( ES_SELECT_WIDTH, &aReadSet, 0, 0, &aTimeout ) == 1;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::sendBuffer (0x40C620) -- one timeout budget covers the whole buffer.
ES_ErrorType EasySocket::sendBuffer( const void *theBuf, int theLen, int *theSentLen, int theTotalTime )
{
	int sentLen;
	unsigned long startTick;
	int done;
	int remainingTime;
	int result;

	if ( isInvalid() )
		return ES_INVALID_SOCKET;

	sentLen = 0;
	startTick = GetTickCount();
	done = 0;

	do
	{
		unsigned long now;

		if ( sentLen >= theLen )
			break;

		now = GetTickCount();
		if ( now - startTick < (unsigned long)theTotalTime )
			remainingTime = theTotalTime + startTick - now;
		else
		{
			remainingTime = 0;
			done = 1;
		}

		if ( waitForWrite( remainingTime ) )
		{
			result = send( mSocket, (const char *)theBuf + sentLen, theLen - sentLen, 0 );
			if ( result < 0 )
				return WSAErrorToEnum( ESGetLastError() );

			// Keep sending until the whole buffer is flushed or the timeout expires.
			sentLen += result;
		}
	}
	while ( !done );

	if ( theSentLen )
		*theSentLen = sentLen;

	return sentLen >= theLen ? ES_NO_ERROR : ES_INCOMPLETE_SEND;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::recvBuffer (0x40C6F0) -- stops on a full buffer, a shutdown, one
// datagram, or the timeout budget.
ES_ErrorType EasySocket::recvBuffer( void *theBuf, int theLen, int *theRecvLen, int theTotalTime )
{
	int done;
	int receivedAny;
	unsigned long startTick;
	int recvLen;

	if ( isInvalid() )
		return ES_INVALID_SOCKET;

	done = 0;
	receivedAny = 0;
	startTick = GetTickCount();
	recvLen = 0;

	do
	{
		unsigned long now;
		int remainingTime;
		int result;

		if ( recvLen >= theLen )
			break;

		now = GetTickCount();
		if ( now - startTick < (unsigned long)theTotalTime )
			remainingTime = theTotalTime + startTick - now;
		else
		{
			remainingTime = 0;
			done = 1;
		}

		if ( waitForRead( remainingTime ) )
		{
			result = recv( mSocket, (char *)theBuf + recvLen, theLen - recvLen, 0 );
			if ( result < 0 )
				return WSAErrorToEnum( ESGetLastError() );

			if ( !result )
				done = 1;

			recvLen += result;
			receivedAny = 1;

			// Datagram sockets stop after the first packet.
			if ( mType != TCP )
				break;
		}
	}
	while ( !done );

	if ( theRecvLen )
		*theRecvLen = recvLen;

	if ( !receivedAny )
		return ES_TIMED_OUT;

	if ( mType != TCP )
		return ES_NO_ERROR;

	if ( recvLen )
		return recvLen >= theLen ? ES_NO_ERROR : ES_INCOMPLETE_RECV;

	return ES_SHUTDOWN;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ES_ErrorType EasySocket::broadcastBuffer( const void *theBuf, int theLen, int thePort, int theTotalTime )
{
	SOCKADDR_IN aSockAddr;
	int optVal;

	if ( mType != UDP )
		return ES_ERROR_STREAM_NOT_ALLOWED;

	optVal = 1;
	setsockopt( mSocket, SOL_SOCKET, SO_BROADCAST, (const char *)&optVal, sizeof( optVal ) );

	getBroadcastSockAddrIn( aSockAddr, thePort );
	return sendBufferTo( theBuf, theLen, (SOCKADDR &)aSockAddr, theTotalTime );
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::sendBufferTo (0x40C800)
ES_ErrorType EasySocket::sendBufferTo( const void *theBuf, int theLen, const SOCKADDR &theSockAddr, int theTotalTime )
{
	int result;

	if ( !waitForWrite( theTotalTime ) )
		return ES_INCOMPLETE_SEND;

	result = sendto( mSocket, (const char *)theBuf, theLen, 0, &theSockAddr, sizeof( theSockAddr ) );
	if ( result == SOCKET_ERROR )
		return WSAErrorToEnum( ESGetLastError() );

	return result == theLen ? ES_NO_ERROR : ES_PARTIAL_SENDTO;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ES_ErrorType EasySocket::sendBufferTo( const void *theBuf, int theLen, long theAddr, int thePort, int theTotalTime )
{
	SOCKADDR_IN aSockAddr;
	ES_ErrorType result;

	result = getSockAddrInFast( aSockAddr, getAddrFromLong( theAddr ), thePort );
	if ( result != ES_NO_ERROR )
		return result;

	return sendBufferTo( theBuf, theLen, (SOCKADDR &)aSockAddr, theTotalTime );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_IPX
ES_ErrorType EasySocket::sendBufferTo( const void *theBuf, int theLen, unsigned char theAddr[6], int thePort, int theTotalTime )
{
	SOCKADDR_IPX aSockAddr;
	ES_ErrorType result;

	result = getSockAddrIpx( aSockAddr, theAddr, thePort );
	if ( result != ES_NO_ERROR )
		return result;

	return sendBufferTo( theBuf, theLen, (SOCKADDR &)aSockAddr, theTotalTime );
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::sendBufferTo (0x40C880)
ES_ErrorType EasySocket::sendBufferTo( const void *theBuf, int theLen, const string &theAddr, int thePort, int theTotalTime )
{
	SOCKADDR_IN aSockAddr;
	ES_ErrorType result;

	result = getSockAddrIn( aSockAddr, theAddr, thePort );
	if ( result != ES_NO_ERROR )
		return result;

	return sendBufferTo( theBuf, theLen, (SOCKADDR &)aSockAddr, theTotalTime );
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::recvBufferFrom (0x40C8D0)
ES_ErrorType EasySocket::recvBufferFrom( void *theBuf, int theLen, SOCKADDR *theSockAddr, int *theRecvLen, int theTotalTime )
{
	int addrLen;
	int result;

	if ( theRecvLen )
		*theRecvLen = 0;

	if ( !waitForRead( theTotalTime ) )
		return ES_INCOMPLETE_RECV;

	// Source-style datagram receive helper layered on top of the same wait logic.
	addrLen = sizeof( *theSockAddr );
	result = recvfrom( mSocket, (char *)theBuf, theLen, 0, theSockAddr, &addrLen );
	if ( result == SOCKET_ERROR )
		return WSAErrorToEnum( ESGetLastError() );

	if ( theRecvLen )
		*theRecvLen = result;

	return ES_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::recvBufferFrom (0x40C980)
ES_ErrorType EasySocket::recvBufferFrom( void *theBuf, int theLen, long *theAddr, int *thePort, int *theRecvLen, int theTotalTime )
{
	SOCKADDR_IN aSockAddr;
	ES_ErrorType result;

	result = recvBufferFrom( theBuf, theLen, (SOCKADDR *)&aSockAddr, theRecvLen, theTotalTime );
	if ( result != ES_NO_ERROR )
		return result;

	if ( theAddr )
		*theAddr = aSockAddr.sin_addr.s_addr;
	if ( thePort )
		*thePort = ntohs( aSockAddr.sin_port );

	return ES_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_IPX
ES_ErrorType EasySocket::recvBufferFrom( void *theBuf, int theLen, unsigned char theAddr[6], int *thePort, int *theRecvLen, int theTotalTime )
{
	SOCKADDR_IPX aSockAddr;
	ES_ErrorType result;

	result = recvBufferFrom( theBuf, theLen, (SOCKADDR *)&aSockAddr, theRecvLen, theTotalTime );
	if ( result != ES_NO_ERROR )
		return result;

	memcpy( theAddr, aSockAddr.sa_nodenum, 6 );
	if ( thePort )
		*thePort = ntohs( aSockAddr.sa_socket );

	return ES_NO_ERROR;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ES_ErrorType EasySocket::recvBufferFrom( void *theBuf, int theLen, string *theAddrString, int *thePort, int *theRecvLen, int theTotalTime )
{
	long aAddr;
	ES_ErrorType result;

	result = recvBufferFrom( theBuf, theLen, &aAddr, thePort, theRecvLen, theTotalTime );
	if ( result != ES_NO_ERROR )
		return result;

	if ( theAddrString )
		*theAddrString = getAddrFromLong( aAddr );

	return ES_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ES_ErrorType EasySocket::shutdown( int theHow )
{
	if ( isInvalid() )
		return ES_INVALID_SOCKET;

	if ( ::shutdown( mSocket, theHow ) == SOCKET_ERROR )
		return WSAErrorToEnum( ESGetLastError() );

	return ES_NO_ERROR;
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::disconnect (0x40C9E0)
void EasySocket::disconnect()
{
	SOCKADDR name;

	mConnected = false;

	if ( mType == TCP )
	{
		close();
		return;
	}

	// UDP-style sockets disconnect by connecting to a null address.
	memset( &name, 0, sizeof( name ) );
	if ( ::connect( mSocket, &name, sizeof( name ) ) != 0 )
		close();
}

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// Shut down the send side, optionally wait for the peer to drain/close, then
// tear down the descriptor using the normal close path.
ES_ErrorType EasySocket::gracefulClose( int theWaitTime )
{
	if ( !isInvalid() )
		shutdown( SD_SEND );

	if ( theWaitTime > 0 )
		waitForRead( theWaitTime );

	close();
	return ES_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::close (0x40CA30)
void EasySocket::close()
{
	if ( !isInvalid() )
		closesocket( mSocket );

	mSocket = INVALID_SOCKET;
	mConnected = false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::isInvalid (0x40C140)
bool EasySocket::isInvalid( void )
{
	return mSocket == INVALID_SOCKET;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void EasySocket::getDestAddr( SOCKADDR *theSockAddr )
{
	if ( theSockAddr )
		*theSockAddr = mDestAddr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
long EasySocket::getDestAddr( void )
{
	if ( !mConnected )
		return 0;

	return ((SOCKADDR_IN *)&mDestAddr)->sin_addr.s_addr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_IPX
void EasySocket::getDestAddr( unsigned char theAddr[6] )
{
	if ( theAddr )
		memcpy( theAddr, ((SOCKADDR_IPX *)&mDestAddr)->sa_nodenum, 6 );
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
string EasySocket::getDestAddrString( void )
{
	return getAddrFromLong( getDestAddr() );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int EasySocket::getDestPort( void )
{
	if ( !mConnected )
		return 0;

	return ntohs( ((SOCKADDR_IN *)&mDestAddr)->sin_port );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
long EasySocket::getLocalAddr( void )
{
	SOCKADDR_IN aSockAddr;
	int addrLen;

	if ( isInvalid() )
		return 0;

	addrLen = sizeof( aSockAddr );
	memset( &aSockAddr, 0, sizeof( aSockAddr ) );
	if ( getsockname( mSocket, (SOCKADDR *)&aSockAddr, &addrLen ) == SOCKET_ERROR )
		return 0;

	return aSockAddr.sin_addr.s_addr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_IPX
void EasySocket::getLocalAddr( unsigned char theAddr[6] )
{
	memset( theAddr, 0, 6 );
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
string EasySocket::getLocalAddrString( void )
{
	return getAddrFromLong( getLocalAddr() );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int EasySocket::getLocalPort( void )
{
	SOCKADDR_IN aSockAddr;
	int addrLen;

	if ( isInvalid() )
		return 0;

	addrLen = sizeof( aSockAddr );
	memset( &aSockAddr, 0, sizeof( aSockAddr ) );
	if ( getsockname( mSocket, (SOCKADDR *)&aSockAddr, &addrLen ) == SOCKET_ERROR )
		return 0;

	return ntohs( aSockAddr.sin_port );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
EasySocket::SocketType EasySocket::getType( void )
{
	return mType;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
ES_ErrorType EasySocket::asyncSelect( HWND hWnd, unsigned int wMsg, long lEvent )
{
	if ( isInvalid() )
		return ES_INVALID_SOCKET;

	return WSAAsyncSelect( mSocket, hWnd, wMsg, lEvent ) == SOCKET_ERROR ? WSAErrorToEnum( ESGetLastError() ) : ES_NO_ERROR;
}
#endif // WIN32

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int EasySocket::setOption( int level, int optname, const char* optval, int optlen )
{
	return setsockopt( mSocket, level, optname, optval, optlen );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int EasySocket::getOption( int level, int optname, char* optval, int* optlen ) const
{
	return getsockopt( mSocket, level, optname, optval, optlen );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::startWinsock (0x40C150)
ES_ErrorType EasySocket::startWinsock( void )
{
	WSADATA wsaData;

	if ( !WSAStartup( ES_WINSOCK_VERSION, &wsaData ) )
		return ES_NO_ERROR;

	return WSAErrorToEnum( ESGetLastError() );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::stopWinsock (0x40C190)
ES_ErrorType EasySocket::stopWinsock( void )
{
	if ( !WSACleanup() )
		return ES_NO_ERROR;

	return WSAErrorToEnum( ESGetLastError() );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::WSAErrorToEnum (0x40CE60)
ES_ErrorType EasySocket::WSAErrorToEnum( int theError )
{
	switch ( theError )
	{
	case 10004: return ES_WSAEINTR;
	case 10009: return ES_WSAEBADF;
	case 10013: return ES_WSAEACCES;
	case 10014: return ES_WSAEFAULT;
	case 10022: return ES_WSAEINVAL;
	case 10024: return ES_WSAEMFILE;
	case 10035: return ES_WSAEWOULDBLOCK;
	case 10036: return ES_WSAEINPROGRESS;
	case 10037: return ES_WSAEALREADY;
	case 10038: return ES_WSAENOTSOCK;
	case 10039: return ES_WSAEDESTADDRREQ;
	case 10040: return ES_WSAEMSGSIZE;
	case 10041: return ES_WSAEPROTOTYPE;
	case 10042: return ES_WSAENOPROTOOPT;
	case 10043: return ES_WSAEPROTONOSUPPORT;
	case 10044: return ES_WSAESOCKTNOSUPPORT;
	case 10045: return ES_WSAEOPNOTSUPP;
	case 10046: return ES_WSAEPFNOSUPPORT;
	case 10047: return ES_WSAEAFNOSUPPORT;
	case 10048: return ES_WSAEADDRINUSE;
	case 10049: return ES_WSAEADDRNOTAVAIL;
	case 10050: return ES_WSAENETDOWN;
	case 10051: return ES_WSAENETUNREACH;
	case 10052: return ES_WSAENETRESET;
	case 10053: return ES_WSAECONNABORTED;
	case 10054: return ES_WSAECONNRESET;
	case 10055: return ES_WSAENOBUFS;
	case 10056: return ES_WSAEISCONN;
	case 10057: return ES_WSAENOTCONN;
	case 10058: return ES_WSAESHUTDOWN;
	case 10059: return ES_WSAETOOMANYREFS;
	case 10060: return ES_WSAETIMEDOUT;
	case 10061: return ES_WSAECONNREFUSED;
	case 10062: return ES_WSAELOOP;
	case 10063: return ES_WSAENAMETOOLONG;
	case 10064: return ES_WSAEHOSTDOWN;
	case 10065: return ES_WSAEHOSTUNREACH;
	case 10066: return ES_WSAENOTEMPTY;
	case 10067: return ES_WSAEPROCLIM;
	case 10068: return ES_WSAEUSERS;
	case 10069: return ES_WSAEDQUOT;
	case 10070: return ES_WSAESTALE;
	case 10071: return ES_WSAEREMOTE;
	case 10091: return ES_WSASYSNOTREADY;
	case 10092: return ES_WSAVERNOTSUPPORTED;
	case 10093: return ES_WSANOTINITIALISED;
	case 10101: return ES_WSAEDISCON;
	default: return ES_NO_ERROR;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasySocket::getSockAddrIn (0x40C1B0)
ES_ErrorType EasySocket::getSockAddrIn( SOCKADDR_IN &theSockAddr, const string &theAddress, int thePort )
{
	memset( &theSockAddr, 0, sizeof( theSockAddr ) );

	theSockAddr.sin_family = AF_INET;
	theSockAddr.sin_port = htons( (unsigned short)thePort );
	theSockAddr.sin_addr.s_addr = getAddrFromString( theAddress );

	return theSockAddr.sin_addr.s_addr ? ES_NO_ERROR : ES_INVALID_ADDR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ES_ErrorType EasySocket::getSockAddrIn( SOCKADDR_IN &theSockAddr, const string &theAddressAndPort )
{
	string::size_type sepPos;
	string address;
	int port;

	sepPos = theAddressAndPort.find( ':' );
	if ( sepPos == string::npos )
		return ES_INVALID_ADDR;

	address = theAddressAndPort.substr( 0, sepPos );
	port = atoi( theAddressAndPort.c_str() + sepPos + 1 );
	return getSockAddrIn( theSockAddr, address, port );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_IPX
ES_ErrorType EasySocket::getSockAddrIpx( SOCKADDR_IPX &theSockAddr, const unsigned char theAddress[6], int thePort )
{
	memset( &theSockAddr, 0, sizeof( theSockAddr ) );
	theSockAddr.sa_family = AF_IPX;
	theSockAddr.sa_socket = htons( (unsigned short)thePort );
	memcpy( theSockAddr.sa_nodenum, theAddress, 6 );
	return ES_NO_ERROR;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ES_ErrorType EasySocket::getSockAddrInFast( SOCKADDR_IN &theSockAddr, const string &theAddress, int thePort )
{
	memset( &theSockAddr, 0, sizeof( theSockAddr ) );
	theSockAddr.sin_family = AF_INET;
	theSockAddr.sin_port = htons( (unsigned short)thePort );
	theSockAddr.sin_addr.s_addr = inet_addr( theAddress.c_str() );

	return theSockAddr.sin_addr.s_addr == INADDR_NONE ? ES_INVALID_ADDR : ES_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ES_ErrorType EasySocket::getSockAddrInFast( SOCKADDR_IN &theSockAddr, const string &theAddressAndPort )
{
	string::size_type sepPos;
	string address;
	int port;

	sepPos = theAddressAndPort.find( ':' );
	if ( sepPos == string::npos )
		return ES_INVALID_ADDR;

	address = theAddressAndPort.substr( 0, sepPos );
	port = atoi( theAddressAndPort.c_str() + sepPos + 1 );
	return getSockAddrInFast( theSockAddr, address, port );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void EasySocket::getBroadcastSockAddrIn( SOCKADDR_IN &theSockAddr, int thePort )
{
	memset( &theSockAddr, 0, sizeof( theSockAddr ) );
	theSockAddr.sin_family = AF_INET;
	theSockAddr.sin_port = htons( (unsigned short)thePort );
	theSockAddr.sin_addr.s_addr = INADDR_BROADCAST;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_IPX
void EasySocket::getBroadcastSockAddrIpx( SOCKADDR_IPX &theSockAddr, int thePort )
{
	memset( &theSockAddr, 0, sizeof( theSockAddr ) );
	theSockAddr.sa_family = AF_IPX;
	theSockAddr.sa_socket = htons( (unsigned short)thePort );
	memset( theSockAddr.sa_nodenum, 0xFF, 6 );
}
#endif

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// EasySocket::getAddrFromString (0x40C200)
long EasySocket::getAddrFromString( const string &theAddress )
{
	unsigned long addr;
	HOSTENT *host;

	addr = inet_addr( theAddress.c_str() );
	if ( addr == INADDR_NONE )
	{
		// Fall back to name resolution when the input is not a raw dotted address.
		host = gethostbyname( theAddress.c_str() );
		if ( host )
			return **(long **)host->h_addr_list;
	}

	return (long)addr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
string EasySocket::getAddrFromLong( long theAddress )
{
	IN_ADDR anAddr;
	const char *text;

	anAddr.s_addr = theAddress;
	text = inet_ntoa( anAddr );
	return text ? text : "";
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_IPX
void EasySocket::getAddrFromString( unsigned char theAddr[6], const string &theAddress )
{
	memset( theAddr, 0, 6 );
	(void)theAddress;
}

string EasySocket::getAddrFromNodeNum( unsigned const char theNodeNum[6] )
{
	char aBuf[32];

	sprintf( aBuf, "%02X%02X%02X%02X%02X%02X",
		theNodeNum[0], theNodeNum[1], theNodeNum[2],
		theNodeNum[3], theNodeNum[4], theNodeNum[5] );
	return aBuf;
}
#endif
