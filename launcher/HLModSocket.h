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
// Purpose: declares CHLModSocket, the mod-list query socket.
//
// $NoKeywords: $
//=============================================================================

#ifndef HLMODSOCKET_H
#define HLMODSOCKET_H
#ifdef _WIN32
#pragma once
#endif

#include <afxsock.h>
#include "MessageBuffer.h"
#include "mod.h"

/////////////////////////////////////////////////////////////////////////////
// CHLModSocket
//
// Pulls the custom-game list off one mod master server, a chunk per reply,
// and publishes the assembled chain through the caller's out-pointer.

class CHLModSocket : public CAsyncSocket
{
// Construction
public:
	CHLModSocket( mod_t** ppModListOut, const char* pszServer, short nPort );
	virtual ~CHLModSocket();

// Overrides
	virtual void	OnReceive( int nErrorCode );

// Operations
	BOOL	StartList();
	void	SendModListRequest();
	void	ParseModList();
	void	SendListRequest( long lArg );
	void	SendInstallNotify( long lArg );

// Implementation
	CMessageBuffer*	m_pMsg;				// +8    serialize buffer (0x800)
	BOOL			m_bDone;			// +12   set when the full list is parsed
	mod_t*			m_pModListHead;		// +16   head of the locally-built chain
	mod_t**			m_ppModListOut;		// +20   caller's out-pointer
	int				m_nBytesRead;		// +24   bytes from the last Receive
	char			m_szServer[256];	// +28   master-server address
	short			m_nPort;			// +284
};

#endif // HLMODSOCKET_H
