/******************************************************************************/
/*                                                                            */
/*                                 EASYTITANSOCKET.H                          */
/*                               WONMisc Socket Class                         */
/*                                   Include File                             */
/*                                                                            */
/******************************************************************************/

/*

  Class:             EasyTitanSocket

  Description:       This class implements methods for easilly sending and
                     receiving Titan messages.

  Author:            Brian Rothstein
  Last Modified:     28 Sept 98

  Base Classes:      EasySocket

  Contained Classes: none

  Friend Classes:    none

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

  Contained Data Types: none

  Private Data Members: none

  Protected Data Members: None

  Public Data Members: None

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

    Constructors:
    EasyTitanSocket(SocketType theType)
      This is the main constructor that you want to use since most of the
      time you know what kind of socket you want when you declare it.  The
      constructor doesn't do much.  It doesn't actually get a new socket
      descriptor.  It just initializes the type so that when a connect or
      sendto is performed, the correct kind of socket can be opened.

    EasyTitanSocket()
      The default constructor sets the socket's type to NO_TYPE.  Before
      using the socket you must call the setType method.  This constructor
      is useful if you don't initially know what kind of socket you need.

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

  Private Methods: none

  Protected Methods: None

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

  Public Methods:
    ES_ErrorType sendTMessage(TMessage *theMsg, int theTotalTime = 1000);
    ES_ErrorType recvTMessage(TMessage *theMsg, int theTotalTime = 1000);
      Send and receive Titan messages.  These functions provide convenient
      ways of sending and receiving Titan messages on connected sockets.
      They handle packing and unpacking of the message and receiving the
      header and assuring that the header types match and that the total
      length is received.  If a base class TMessage is passes to the
      recvTMessage method, then no check is performed for matching types
      and unpack is not called.

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

  Accessors: none
*/

#ifndef __EASYTITANSOCKET_H__
#define __EASYTITANSOCKET_H__

#include "ES_ErrorType.h"
#include "EasySocket.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// NOTE.
class EasyTitanSocket : public EasySocket
{
public:
  EasyTitanSocket( SocketType type, unsigned int maxMsgSize );
	virtual ~EasyTitanSocket();

	// Reset the internal message buffer size.
	void setMaxMsgSize( unsigned int maxMsgSize );
	// Receive a complete Titan message from the socket.
	ES_ErrorType recvTMessage( void *buffer, unsigned int *size, unsigned int *serviceType, unsigned int *messageType, unsigned int timeout = 0 );

private:
	unsigned char *mMessageBuf;		// Buffered TMessage data.
	unsigned int mMaxMsgSize;		// Maximum buffered message size.
	unsigned int mBufferedSize;		// Bytes buffered in mMessageBuf.
};

#if defined(_MSC_VER) && (_MSC_VER < 1600)
// VC6 has no static_assert keyword; use the negative-array compile-time check so the
// size-fidelity assert still fires under the MSVC 6.0 build (the ABI-faithful target).
typedef char EasyTitanSocket_SizeCheck[ sizeof(EasyTitanSocket) == 0x2C ? 1 : -1 ];
#else
static_assert(sizeof(EasyTitanSocket) == 0x2C, "EasyTitanSocket size mismatch");
#endif

#endif // __EASYTITANSOCKET_H__
