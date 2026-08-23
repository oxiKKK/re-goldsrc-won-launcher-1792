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
// Purpose: declares the modal multi-select page (CMultiSelectDlg, IDD 220).
//
// $NoKeywords: $
//=============================================================================

#ifndef MULTISELECT_DLG_H
#define MULTISELECT_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "resource_dlg.h"

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg dialog
//
// The multiplayer hub: eight skinned rows, each a button and a help label.
// The top row is Quick start, or Resume + Disconnect while a game is running.

class CMultiSelectDlg : public CDlgBase
{
// Construction
public:
	CMultiSelectDlg( CWnd* pParent = NULL );

// Dialog Data
	//{{AFX_DATA(CMultiSelectDlg)
	enum { IDD = IDD_MULTISELECT };
	CODStatic	m_lblControlHelp;		// +224   IDC_CFG_CONTROLHELP
	CODBlendBtn	m_btnControls;			// +320   IDC_BTN_CONTROLS
	CODStatic	m_lblDoneHelp;			// +560   IDC_MULTI_DONEHELP
	CODBlendBtn	m_btnResume;			// +656   IDC_MULTI_RESUME
	CODBlendBtn	m_btnDisconnect;		// +896   IDC_MULTI_DISCONNECT
	CODBlendBtn	m_btnOK;				// +1136  IDOK
	CODStatic	m_lblQuickHelp;			// +1376  IDC_MAIN_QUICKHELP
	CODStatic	m_lblLan;				// +1472  IDC_MULTI_LAN
	CODStatic	m_lblCustomize;			// +1568  IDC_MULTI_CUSTOMIZE
	CODStatic	m_lblChat;				// +1664  IDC_MULTI_CHAT
	CODStatic	m_lblBrowse;			// +1760  IDC_MULTI_BROWSE
	CODStatic	m_lblSpectate;			// +1856  IDC_MULTI_SPECTATE
	CODStatic	m_lblResumeHelp;		// +1952  IDC_MULTI_RESUMEHELP
	CODStatic	m_lblDisconnectHelp;	// +2048  IDC_MULT_DISCONNECTHELP
	CODBlendBtn	m_btnQuick;				// +2144  IDC_BTN_QUICK
	CODBlendBtn	m_btnLan;				// +2384  IDC_BTN_LAN
	CODBlendBtn	m_btnCustomize;			// +2624  IDC_BTN_CUSTOMIZE
	CODBlendBtn	m_btnChat;				// +2864  IDC_BTN_CHAT
	CODBlendBtn	m_btnBrowse;			// +3104  IDC_BTN_BROWSE
	CODBlendBtn	m_btnSpectate;			// +3344  IDC_BTN_SPECTATE
	//}}AFX_DATA

// Attributes
public:
	// Re-slice every button out of the loaded strip.  Also driven from
	// CDlgConnectableBase when a skin change lands under the page.
	void	InitMembers();

// Overrides
	//{{AFX_VIRTUAL(CMultiSelectDlg)
	public:
	virtual ~CMultiSelectDlg();
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();
	virtual int		RMLPreIdle();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	Refresh();			// relayout + repaint + re-show
	void	RelayoutControls();	// the page's button/label layout pass

	HGLOBAL	m_headerLoaded;		// +3584  the "head_multi" button strip
	int		m_headerStride;		// +3588
	int		m_headerW;			// +3592  one strip cell
	int		m_headerH;			// +3596

	// Generated message map functions
	//{{AFX_MSG(CMultiSelectDlg)
	afx_msg void	OnBrowse();
	afx_msg void	OnSpectateBtn();
	afx_msg void	OnChat();
	afx_msg void	OnCustomize();
	afx_msg void	OnLan();
	afx_msg void	OnQuick();
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg void	OnDisconnect();
	afx_msg void	OnResume();
	afx_msg void	OnReadme();
	afx_msg void	OnShowWindow( BOOL bShow, UINT nStatus );
	afx_msg void	OnControls();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // MULTISELECT_DLG_H
