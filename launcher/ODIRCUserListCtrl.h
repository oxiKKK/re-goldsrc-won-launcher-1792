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
// Purpose: declares CODIRCUserListCtrl, the chat user list.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODIRCUSERLISTCTRL_H
#define ODIRCUSERLISTCTRL_H
#ifdef _WIN32
#pragma once
#endif

#include "ODListCtrl.h"
#include "chatclient.h"

/////////////////////////////////////////////////////////////////////////////
// CODIRCUserListCtrl window
//
// The chat room's user roster, sorted by nick.

class CODIRCUserListCtrl : public CODListCtrl
{
// Construction
public:
	CODIRCUserListCtrl( CWnd* pOwner );

// Attributes
public:
	CWnd*	m_pOwnerDlg;	// +1908  the chat rooms page (ctor arg)

// Operations
public:
	void	AddRow( CChatUser* pUser );

// Overrides
public:
	virtual void	DrawRow( CDC* pDC, int iRow );

// Implementation
public:
	virtual ~CODIRCUserListCtrl();

protected:
	DECLARE_MESSAGE_MAP()
};

#endif // ODIRCUSERLISTCTRL_H
