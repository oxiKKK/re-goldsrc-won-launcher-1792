//======================== reconstructed by oxi, 2026 ========================
//
// re-won-launcher-1792
// WON Half-Life launcher, build 1792
//
// This is a source-level reconstruction of hl.exe, the WON-era Half-Life
// launcher, build 1792 (Sep 20 2001), rebuilt from the retail binary.  It
// exists for educational and archival purposes.  It is non-commercial hobby
// work and is not affiliated with Valve.
//
// Purpose: declares CHLLanAsyncSocket, the LAN server-query socket.
//
// $NoKeywords: $
//=============================================================================

#ifndef HLLANASYNCSOCKET_H
#define HLLANASYNCSOCKET_H
#ifdef _WIN32
#pragma once
#endif

#include <afxsock.h>
#include "MessageBuffer.h"
#include "NetGame.h"

/////////////////////////////////////////////////////////////////////////////
// CHLLanAsyncSocket
//
// Broadcasts "infostring" across the query port range and publishes whatever
// answers into the browser document.  The IPX variant runs on a raw socket it
// binds itself, since MFC's Create cannot reach AF_IPX.

class CHLLanAsyncSocket : public CAsyncSocket
{
// Construction
public:
	CHLLanAsyncSocket( CNetGameDlg* pSheet );
	virtual ~CHLLanAsyncSocket();

// Overrides
	virtual void	OnReceive( int nErrorCode );
	virtual void	OnSend( int nErrorCode );

// Operations
	BOOL	Open();
	int		BroadcastQuery();
	void	PublishLanServer( SOCKADDR_IN* pFrom );

// Implementation
	SOCKET			m_hRawSocket;		// +8   raw winsock handle (m_bIpx path)
	BOOL			m_bIpx;				// +12  the IPX variant of the query socket
	BOOL			m_bOpen;			// +16  set once the socket is created & bound
	CMessageBuffer*	m_pBuffer;			// +20  8 KB read/write datagram buffer
	DWORD			m_unk24;			// +24
	DWORD			m_bQuerySent;		// +28  set once a query was broadcast
	CNetGameDlg*	m_pBrowserDoc;		// +32
	int				m_nLastResult;		// +36
	double			m_dSendTime;		// +40  last broadcast timestamp
};

#endif // HLLANASYNCSOCKET_H
