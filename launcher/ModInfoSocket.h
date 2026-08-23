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
// Purpose: declares CModInfoSocket, the mod-info query socket.
//
// $NoKeywords: $
//=============================================================================

#ifndef MODINFOSOCKET_H
#define MODINFOSOCKET_H
#ifdef _WIN32
#pragma once
#endif

#include <afxsock.h>
#include "MessageBuffer.h"
#include "mod.h"

// CModInfoSocket::m_nState
#define MODINFO_QUERYING	0
#define MODINFO_ERROR		1
#define MODINFO_DONE		2

// Retry budget and the initial backoff, which doubles on every resend.
#define MODINFO_RETRIES		3
#define MODINFO_TIMEOUT		1.0f

/////////////////////////////////////////////////////////////////////////////
// CModInfoSocket
//
// sizeof 0x2C.  Walks the master's per-mod server/player counts a page at a
// time and folds them into the mod list it was handed.

class CModInfoSocket : public CAsyncSocket
{
// Construction
public:
	CModInfoSocket( mod_t* pModList );
	virtual ~CModInfoSocket();

// Overrides
	virtual void	OnReceive( int nErrorCode );

// Operations
	void	StartList();
	BOOL	Pump();

// Implementation
protected:
	void	SendRequest( const char* pszToken );
	void	ParseModList();

	CMessageBuffer*	m_pMsg;			// +8   serialize buffer (0x2000)
	int				m_unk12;		// +12
	mod_t*			m_pModList;		// +16  list whose counts get updated
	int				m_nBytesRead;	// +20  bytes from the last Receive
	int				m_nState;		// +24  MODINFO_*
	int				m_unk28;		// +28
	int				m_nRetriesLeft;	// +32
	float			m_flTimeout;	// +36  seconds before retry
	float			m_flLastSend;	// +40  Sys_FloatTime of last Send
};

#endif // MODINFOSOCKET_H
