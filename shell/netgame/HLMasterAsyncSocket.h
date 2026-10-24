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
// Purpose: declares CHLMasterAsyncSocket, the master-server query socket.
//
// $NoKeywords: $
//=============================================================================

#ifndef HLMASTERASYNCSOCKET_H
#define HLMASTERASYNCSOCKET_H
#ifdef _WIN32
#pragma once
#endif

#include <afxsock.h>
#include "MessageBuffer.h"
#include "NetGame.h"

// Nonzero => apply the unregistered-server cap.
extern int	g_bEnforceServerCap;	// 0x4EA8F0

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket
//
// sizeof 0x428.  Pages the master server's list one batch at a time, resuming
// from the token each reply carries, and handles the 'w' reply that carries
// the auth/titan/master server groups.

class CHLMasterAsyncSocket : public CAsyncSocket
{
// Construction
public:
	CHLMasterAsyncSocket( CNetGameDlg* pSheet );
	virtual ~CHLMasterAsyncSocket();

// Overrides
	virtual void	OnReceive( int nErrorCode );

// Operations
	void	ParseServerList();
	void	ParseVersionReply();
	int		RequestServerBatch( long lLastAddr );
	void	SetListDone( DWORD dwDone );
	void	SetFilter( const char* pszFilter );
	void	BeginFetch();
	void	DecTries();
	BOOL	HasTries();
	void	FlushSend();

	// CHLMasterAsyncSocket::Reset (0x41A9A0)
	void	Reset()		{ m_nTries = 1; }

// Implementation
	CMessageBuffer*	m_pBuffer;			// +8   8 KB
	DWORD			m_dwListDone;		// +12  set when the master sends a 0 resume token
	CNetGameDlg*	m_pBrowserDoc;		// +16  the doc servers are added to
	int				m_nLastResult;		// +20  last Receive byte count
	double			m_flLastSend;		// +24  Sys_FloatTime of the last request
	int				m_nServers;			// +32  running parsed count
	int				m_nTries;			// +36  resend budget
	char			m_szFilter[1024];	// +40  infostring criteria; empty => plain 'e'
};

#endif // HLMASTERASYNCSOCKET_H
