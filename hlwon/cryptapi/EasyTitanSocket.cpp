// EasyTitanSocket

#include <new>
#include <string.h>

#include "EasyTitanSocket.h"

#define EASY_TITAN_MESSAGE_HEADER_SIZE	4 // size field only
#define EASY_TITAN_MESSAGE_MIN_SIZE		12 // size + service id + message id

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasyTitanSocket::EasyTitanSocket (0x40E230)
EasyTitanSocket::EasyTitanSocket( SocketType type, unsigned int maxMsgSize )
	: EasySocket( type )
{
	mMessageBuf = new unsigned char[maxMsgSize];
	mMaxMsgSize = maxMsgSize;
	mBufferedSize = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasyTitanSocket::~EasyTitanSocket (0x40E2C0)
EasyTitanSocket::~EasyTitanSocket()
{
	delete[] mMessageBuf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasyTitanSocket::setMaxMsgSize (0x40E1F0)
void EasyTitanSocket::setMaxMsgSize( unsigned int maxMsgSize )
{
	delete[] mMessageBuf;

	mMessageBuf = new unsigned char[maxMsgSize];
	mMaxMsgSize = maxMsgSize;
	mBufferedSize = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// EasyTitanSocket::recvTMessage (0x40E2E0)
ES_ErrorType EasyTitanSocket::recvTMessage( void *buffer, unsigned int *size, unsigned int *serviceType, unsigned int *messageType, unsigned int timeout )
{
	int 			received;
	unsigned int 	elapsed, remaining;
	int 			result;
	unsigned long 	startTime, msgSize;

	startTime = EasySocket::GetTickCount();

	// Read the fixed-size Titan header first.
	if ( mBufferedSize < EASY_TITAN_MESSAGE_HEADER_SIZE )
	{
		result = recvBuffer(
			mMessageBuf + mBufferedSize,
			EASY_TITAN_MESSAGE_HEADER_SIZE - mBufferedSize,
			&received,
			timeout );

		if ( result )
		{
			if ( result == ES_INCOMPLETE_RECV )
			{
				mBufferedSize += received;
				return ES_TIMED_OUT;
			}
			return (ES_ErrorType)result;
		}

		mBufferedSize += received;
	}

	elapsed = EasySocket::GetTickCount() - startTime;
	remaining = timeout;
	if ( elapsed >= timeout )
		remaining = 0;

	msgSize = *(unsigned int *)mMessageBuf;

	// Titan messages are length-prefixed and include service/message ids.
	if ( msgSize > mMaxMsgSize )
		return ES_TMSG_TOO_LARGE;
	if ( msgSize < EASY_TITAN_MESSAGE_MIN_SIZE )
		return ES_INVALID_TMSG;

	// The body read reuses remaining - elapsed, which underflows once the budget
	// is spent (sic).
	result = recvBuffer( mMessageBuf + mBufferedSize, msgSize - mBufferedSize, &received, remaining - elapsed );
	if ( result == ES_INCOMPLETE_RECV )
	{
		mBufferedSize += received;
		return ES_TIMED_OUT;
	}
	if ( result )
		return (ES_ErrorType)result;

	if ( size )
		*size = msgSize;
	if ( serviceType )
		*serviceType = *(unsigned int *)(mMessageBuf + 4);
	if ( messageType )
		*messageType = *(unsigned int *)(mMessageBuf + 8);

	// Copy out the complete message and reset the buffered state.
	memcpy( buffer, mMessageBuf, msgSize );
	mBufferedSize = 0;
	return ES_NO_ERROR;
}
