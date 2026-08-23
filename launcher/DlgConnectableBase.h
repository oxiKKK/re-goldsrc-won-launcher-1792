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
// Purpose: declares CDlgConnectableBase, the base for pages that can join a
//          server.
//
// $NoKeywords: $
//=============================================================================

#ifndef DLGCONNECTABLEBASE_H
#define DLGCONNECTABLEBASE_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"

class CServerInfo;
class CNetGameDlg;

/////////////////////////////////////////////////////////////////////////////
// CDlgConnectableBase dialog
//
// vftable 0x4AD90C -- adds the three connect slots the concrete page fills in.

class CDlgConnectableBase : public CDlgBase
{
// Construction
public:
	CDlgConnectableBase( UINT nIDTemplate, CWnd* pParent = NULL );

// Operations
public:
	void	ConnectToSelectedServer( CNetGameDlg* pTarget, CServerInfo* pInfo );

// Overrides
	//{{AFX_VIRTUAL(CDlgConnectableBase)
	public:
	virtual void	InitButtonStrips() = 0;
	virtual void	Relayout() = 0;

	// CDlgConnectableBase::OnConnectAbort (0x406950)
	virtual void	OnConnectAbort()	{}
	//}}AFX_VIRTUAL

// Implementation
protected:
	int		VerifyServerBeforeConnect( CNetGameDlg* pTarget, CServerInfo* pInfo );
	int		CheckModVersion( CServerInfo* pInfo, struct mod_s* pMod );
	void	RefreshAfterGameDirChange();
};

#endif // DLGCONNECTABLEBASE_H
