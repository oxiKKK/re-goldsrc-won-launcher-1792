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
// Purpose: declares CSetInfoDlg, the user.scr advanced-options page, and the
//          CInfoDescription model it writes.
//
// $NoKeywords: $
//=============================================================================

#ifndef SETINFO_DLG_H
#define SETINFO_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "BorderlessEdit.h"
#include "scriptobject.h"
#include "resource_dlg.h"

// Concrete INFO_OPTIONS model -- the player-info ("setinfo") sibling of
// CServerDescription; same machinery, "INFO_OPTIONS" description name.
class CInfoDescription : public CDescription
{
public:
	CInfoDescription();							// 0x463710
	// No dtor of its own: ~CDescription (0x460200) frees both strings below.

	virtual int	WriteScriptHeader( FILE* fp );
	virtual int	WriteFileHeader( FILE* fp );
};

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg dialog

class CSetInfoDlg : public CDlgBase
{
// Construction
public:
	CSetInfoDlg( CDescription* pDesc, CWnd* pParent = NULL );

	enum { IDD = IDD_SETINFO };		// 239

	// One live control per option node (new 0x14 in the binary).
	struct OptCtrl
	{
		int				m_nType;	// +0   CScriptObject type
		CWnd*			m_pControl;	// +4   checkbox / edit / combo
		CODStatic*		m_pHelp;	// +8   help label
		CScriptObject*	m_pOption;	// +12  the description node it edits
		OptCtrl*		m_pNext;	// +16
	};

// Attributes
protected:
	int					m_nNumControls;		// +224  controls built
	int					m_nPerPage;			// +228  controls per page (8)
	int					m_iPage;			// +232  current page
	CBorderLessEdit*	m_pSensEdit;		// +236  the fixed "sensitivity" field
	OptCtrl*			m_pControls;		// +240  head of the live-control list
	CODStatic			m_lblPageInfo;		// +244  DDX id 1210
	CODBlendBtn			m_btnCancel;		// +344  DDX id 2,  strip idx 14
	CODBlendBtn			m_btnOK;			// +584  DDX id 1,  strip idx 19
	CODStatic			m_lblSensHelp;		// +824  DDX id 32792
	CODBitmapButton*	m_pPrevPage;		// +920  larrow* glyph, child id 124
	CODBitmapButton*	m_pNextPage;		// +924  rarrow* glyph, child id 123
	HGLOBAL				m_hHeader;			// +928  the header banner, NULL if absent
	int					m_headerStride;		// +932  banner row stride
	int					m_headerWH[2];		// +936  banner cell {w,h}
	CDescription*		m_pDesc;			// +944  the user.scr model

// Operations
protected:
	void	LoadButtonStrips();
	void	ApplyToDescription();
	void	BuildControls();
	void	DestroyControls();
	void	ShowPage( int iPage );

// Overrides
	//{{AFX_VIRTUAL(CSetInfoDlg)
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();
	virtual int		RMLPreIdle();		// frame-protocol slot 56
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSetInfoDlg();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSetInfoDlg)
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnNextPage();
	afx_msg void	OnPrevPage();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // SETINFO_DLG_H
