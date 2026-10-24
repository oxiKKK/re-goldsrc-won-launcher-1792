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
// Purpose: declares CHLAsyncSocket, the launcher's async socket base.
//
// $NoKeywords: $
//=============================================================================

#ifndef HLASYNCSOCKET_H
#define HLASYNCSOCKET_H
#ifdef _WIN32
#pragma once
#endif

#include <afxsock.h>
#include "MessageBuffer.h"
#include "serverconnection.h"

extern float	g_flLastReceiveTime;	// 0x4E3914

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket
//
// The UDP socket that drives one server record through the query sequence;
// each protocol step parses into the CServerConnection it was built for.

class CHLAsyncSocket : public CAsyncSocket
{
// Construction
public:
	CHLAsyncSocket( CServerConnection* pConn );
	virtual ~CHLAsyncSocket();

// Overrides
	virtual void	OnReceive( int nErrorCode );
	virtual void	OnConnect( int nErrorCode );

// Operations
	void	ParsePingReply( CServerConnection* pCnx );
	void	ParseInfoResponse( CServerConnection* pCnx );
	void	ParsePlayerList( CServerConnection* pCnx );
	void	ParseRules( CServerConnection* pCnx );

// Implementation
	CMessageBuffer*		m_pBuffer;		// +8   8 KB read buffer
	CServerConnection*	m_pConnection;	// +12  the record this socket drives
};

#endif // HLASYNCSOCKET_H
