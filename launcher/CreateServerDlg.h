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
// Purpose: declares CCreateServerDlg, the Create Server page.
//
// $NoKeywords: $
//=============================================================================

#ifndef CREATESERVER_DLG_H
#define CREATESERVER_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "ODListBox.h"
#include "ODComboBox.h"
#include "BorderlessEdit.h"
#include "resource_dlg.h"
#include "AudioDlg.h"

class CServerDescription;
class CNetGameDlg;

/////////////////////////////////////////////////////////////////////////////
// CCreateServerDlg dialog
//
// The Create Server page: the caller reads the five gathered settings back
// after IDOK and turns them into either console commands or an hlds command
// line.

class CCreateServerDlg : public CDlgBase
{
// Construction
public:
	CCreateServerDlg( CNetGameDlg* pBrowser, CWnd* pParent = NULL );

// Attributes
public:
	// Gathered settings; the parent reads these after IDOK.
	char	m_szPassword[64];	// +2540
	char	m_szHostName[64];	// +2604
	char	m_szMap[32];		// +2668
	int		m_nMaxPlayers;		// +2700 (clamped 2..32)
	int		m_bDedicated;		// +2704

// Overrides
	//{{AFX_VIRTUAL(CCreateServerDlg)
	public:
	virtual ~CCreateServerDlg();
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual int		RMLPreIdle();			// CDlgBase frame slot 56
	//}}AFX_VIRTUAL

// Dialog Data
	//{{AFX_DATA(CCreateServerDlg)
	enum { IDD = IDD_CREATESERVER };
	// The four template ids are the STATIC labels; the three edits and the map
	// list are created in code, as OnInitDialog does.
	CODBlendCheckBox	m_btnDedicated;		// +232   IDC 1043
	CODStatic			m_lblPassword;		// +536   IDC 1115
	CStatic				m_unk632;			// +632   constructed, never bound
	CODBlendBtn			m_unk696;			// +696
	CODBlendBtn			m_unk936;			// +936
	CODBlendBtn			m_btnAdvanced;		// +1176  IDC 29
	CODStatic			m_lblName;			// +1416  IDC 1110  "Server Name:"
	CODStatic			m_lblMaxPlayers;	// +1512  IDC 1111  "Max. Players:"
	CODStatic			m_lblMap;			// +1608  IDC 1112  "Map:"
	CODBlendBtn			m_btnOK;			// +1704  IDOK
	CODBlendBtn			m_btnCancel;		// +1944  IDCANCEL
	//}}AFX_DATA

// Implementation
protected:
	void	LoadButtonStrips();		// re-slice the skin strip onto the buttons
	void	RefreshAndShow();		// reload strips, re-flow, re-activate + show
	void	PopulateMapList( const char* pszGameDir );
	void	LayoutHeaderButtons();	// re-flow the buttons under the banner

	CBorderLessEdit		m_editName;			// +2184
	CBorderLessEdit		m_editMaxPlayers;	// +2300
	// A child window with m_bAutoDelete set: it frees itself from
	// CODListBox::OnNcDestroy, so the page must never delete it.
	CODListBox*			m_pMapList;			// +2416  created with id 1006
	CBorderLessEdit		m_editPassword;		// +2420
	CString				m_strScratch;		// +2536  DoDataExchange read-back scratch

	CServerDescription*	m_pDescription;		// +2708  built by OnAdvanced
	HGLOBAL				m_bHeaderLoaded;	// +2712
	int					m_headerStride;		// +2716
	int					m_headerW;			// +2720
	int					m_headerH;			// +2724
	CBrush				m_bkBrush;			// +2728  OnCtlColor page brush
	// (sic) forwarded to CAdvancedMPDlg's own "context" slot, which that class
	// declares as an int; both really carry this pointer.
	CNetGameDlg*		m_pBrowser;			// +2736

	// Generated message map functions
	//{{AFX_MSG(CCreateServerDlg)
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg HBRUSH	OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg void	OnAdvanced();
	afx_msg void	OnMapListNotify();		// control 1042, empty body
	afx_msg void	OnMapListValidate();	// control 1006
	afx_msg void	OnDedicated();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // CREATESERVER_DLG_H
