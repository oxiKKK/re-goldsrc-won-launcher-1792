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
// Purpose: declares CAdvancedMPDlg, the advanced multiplayer options page,
//          with CServerDescription and the CD-key checksum.
//
// $NoKeywords: $
//=============================================================================

#ifndef ADVMP_DLG_H
#define ADVMP_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "scriptobject.h"
#include "resource_dlg.h"

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg dialog

class CAdvancedMPDlg : public CDlgBase
{
// Construction
public:
	CAdvancedMPDlg( CServerDescription* pDesc, int nContext, CWnd* pParent = NULL );

	enum { IDD = IDD_ADVANCEDMP };		// 231

	// One live control per option node; 20 bytes in the binary.
	struct OptCtrl
	{
		int				m_nType;	// +0   CScriptObject type
		CWnd*			m_pControl;	// +4   checkbox / combo / edit
		CODStatic*		m_pHelp;	// +8   help label
		CScriptObject*	m_pOption;	// +12  the description node it edits
		OptCtrl*		m_pNext;	// +16
	};

// Attributes
protected:
	int					m_nNumControls;		// +224  count built
	int					m_nPerPage;			// +228  controls per page (8)
	int					m_iPage;			// +232  current page
	OptCtrl*			m_pControls;		// +236  head of the live-control list
	CODBlendBtn			m_btnCancel;		// +240  DDX id 2,  strip idx 14
	CODStatic			m_lblPageInfo;		// +480  DDX id 1210
	CODBlendBtn			m_btnOK;			// +576  DDX id 1,  strip idx 19
	CODBitmapButton*	m_pPrevPage;		// +816  larrow* glyph, child id 124
	CODBitmapButton*	m_pNextPage;		// +820  rarrow* glyph, child id 123
	HGLOBAL				m_hHeader;			// +824  the header banner, NULL if absent
	int					m_headerStride;		// +828  banner row stride
	int					m_headerWH[2];		// +832  banner cell {w,h}
	CServerDescription*	m_pDesc;			// +840  the settings.scr model
	int					m_nContext;			// +844  caller context

// Operations
protected:
	void	BuildControls();
	void	DestroyControls();
	void	ApplyToDescription();
	void	ShowPage( int iPage );
	void	LoadButtonStrips();

// Overrides
	//{{AFX_VIRTUAL(CAdvancedMPDlg)
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();
	virtual int		RMLPreIdle();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAdvancedMPDlg();

	// Generated message map functions
protected:
	//{{AFX_MSG(CAdvancedMPDlg)
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnNextPage();
	afx_msg void	OnPrevPage();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // ADVMP_DLG_H
