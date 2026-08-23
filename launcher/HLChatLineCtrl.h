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
// Purpose: declares CHLChatLineCtrl, the internet-games chat input line.
//
// $NoKeywords: $
//=============================================================================

#ifndef HLCHATLINECTRL_H
#define HLCHATLINECTRL_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "BorderlessEdit.h"

class CServerBrowserDlg;

/////////////////////////////////////////////////////////////////////////////
// CHLChatLineCtrl window
//
// vftable 0x4AE264 -- the internet-games page's chat input line; Submit
// parses the leading '/' commands itself.

class CHLChatLineCtrl : public CBorderLessEdit
{
// Construction
public:
	CHLChatLineCtrl( CWnd* pOwner );

// Operations
public:
	void	Submit( const char* pszWhisperTarget );
	void	ClearEditSelection();

// Overrides
	// Enter submits the line instead of reaching the dialog manager, which would
	// otherwise fire the default button (IDOK) and close the page.
	//{{AFX_VIRTUAL(CHLChatLineCtrl)
	virtual BOOL	PreTranslateMessage( MSG* pMsg );
	//}}AFX_VIRTUAL

// Attributes
public:
	CServerBrowserDlg*	m_pOwner;		// +116  the hosting page
	COLORREF			m_clrChatBk;	// +120  RGB( 63, 63, 63 )

// Implementation
public:
	virtual ~CHLChatLineCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHLChatLineCtrl)
	afx_msg void	OnChar( UINT nChar, UINT nRepCnt, UINT nFlags );
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // HLCHATLINECTRL_H
