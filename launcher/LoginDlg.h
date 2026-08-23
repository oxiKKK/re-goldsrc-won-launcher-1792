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
// Purpose: declares CLoginDlg, the WON login dialog.
//
// $NoKeywords: $
//=============================================================================

#ifndef LOGIN_DLG_H
#define LOGIN_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "ODButton.h"
#include "ODStatic.h"
#include "resource_dlg.h"
#include "DlgPopupBase.h"

class CNetGameDlg;

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg dialog
//
// A modal progress popup: it drives the master-list request itself out of
// RMLPreIdle and closes as soon as the sheet reports a list.

class CLoginDlg : public CDlgPopupBase
{
// Construction
public:
	CLoginDlg( CNetGameDlg* pNetGame, CWnd* pParent = NULL );

// Dialog Data
protected:
	CNetGameDlg*	m_pNetGame;	// +100  the sheet the login talks to

	//{{AFX_DATA(CLoginDlg)
	enum { IDD = IDD_LOGIN };
	CODStatic		m_lblLine1;		// +104  IDC_LOGIN_LINE_LOWER
	CODStatic		m_lblLine2;		// +200  IDC_LOGIN_LINE_UPPER
	CODStatic		m_lblLogin;		// +296  IDC_LOGIN_TITLE
	CODBlendBtn		m_btnCancel;	// +392  IDCANCEL
	//}}AFX_DATA

// Overrides
	//{{AFX_VIRTUAL(CLoginDlg)
	public:
	virtual ~CLoginDlg();
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual int		RMLPreIdle();
	virtual void	OnOK();
	virtual void	OnCancel();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// One step of the master-list request: issue it, then watch the sheet for
	// the list to land.
	void	PollConnect();

	// printf-style status update: format into the shared buffer and push it
	// into the middle progress line.
	void	SetStatusLine( const char* pszFormat, ... );

	double	m_flStartTime;		// +632  latched in OnInitDialog
	int		m_nState;			// +640  tries left against the current master
	HGLOBAL	m_hStripBmp;		// +644  the loaded button strip
	int		m_nStripCount;		// +648
	int		m_nStripWidth;		// +652
	int		m_nStripHeight;		// +656
	CBrush	m_brush;			// +660  solid background brush (OnCtlColor)
	int		m_bDone;			// +668  connect-complete latch
	int		m_nConnectStage;	// +672  0 = not asked yet, 1 = request out
	double	m_flRequestTime;	// +680  when the current request went out

	// Generated message map functions
	//{{AFX_MSG(CLoginDlg)
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg HBRUSH	OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // LOGIN_DLG_H
