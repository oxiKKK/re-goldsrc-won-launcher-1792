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
// Purpose: declares the Custom Game / mod chooser page (CModDlg, IDD 234).
//
// $NoKeywords: $
//=============================================================================

#ifndef MOD_DLG_H
#define MOD_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "HLMainDlg.h"
#include "ODButton.h"
#include "ODListCtrl.h"
#include "mod.h"
#include "resource_dlg.h"

/////////////////////////////////////////////////////////////////////////////
// CModDlg dialog
//
// The mod list the page shows is its own copy, not g_pModList: a refresh
// merges what the masters report over that copy, and only an install folds
// anything back into the global list.

class CModDlg : public CDlgBase
{
// Construction
public:
	CModDlg( CWnd* pParent = NULL );

// Dialog Data
	//{{AFX_DATA(CModDlg)
	enum { IDD = IDD_CUSTOMGAME };
	CODBlendBtn		m_btnVisitModSite;	// +224   IDC_CUSTOMGAME_VIST_MOD_SITE
	CODBlendBtn		m_btnDeactivate;	// +464   IDC_CUSTOMGAME_DEACTIVATE
	CODBlendBtn		m_btnActivate;		// +704   IDC_CUSTOMGAME_ACTIVATE
	CODBlendBtn		m_btnRefreshList;	// +944   IDC_CUSTOMGAME_REFRESH_LIST
	CODBlendBtn		m_btnInstall;		// +1184  IDC_CUSTOMGAME_IINSTALL
	CODBlendBtn		m_btnDone;			// +1424  IDOK
	//}}AFX_DATA

// Attributes
public:
	CODModListCtrl*	m_pList;			// +1664  created in OnInitDialog
	HGLOBAL			m_hHeaderDIB;		// +1668  the "head_custom" button strip
	int				m_headerStride;		// +1672
	int				m_headerW;			// +1676  one strip cell
	int				m_headerH;			// +1680
	int				m_bReady;			// +1684  layout-ready flag
	mod_t*			m_pMods;			// +1688  the page's copy of the mod list

// Overrides
	//{{AFX_VIRTUAL(CModDlg)
	public:
	virtual ~CModDlg();
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	InitMembers();
	void	FreeMods();			// free the page's private mod copy
	void	PopulateList();
	void	RefreshList();		// re-query both masters and rebuild m_pMods
	void	UpdateButtons();
	void	SwitchToMod( mod_t* pMod );		// make pMod the active game
	void	UpdateButtonStates();			// enable/disable from the selected mod's URLs

	// Generated message map functions
	//{{AFX_MSG(CModDlg)
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnRefreshList();
	afx_msg void	OnActivate();
	afx_msg void	OnInstall();
	afx_msg void	OnDeactivate();
	afx_msg void	OnVisitModSite();
	afx_msg void	OnSelChangeList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // MOD_DLG_H
