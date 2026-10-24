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
// Purpose: declares CModReqDlg, the mod-list request popup.
//
// $NoKeywords: $
//=============================================================================

#ifndef MODREQ_DLG_H
#define MODREQ_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "mod.h"
#include "launcher.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "resource_dlg.h"
#include "DlgPopupBase.h"
#include "HLModSocket.h"
#include "ModInfoSocket.h"

/////////////////////////////////////////////////////////////////////////////
// CModReqDlg dialog
//
// Asks a master server for the custom-game list and shows progress while it
// arrives.  The two paths never run together: one talks to the custom-game
// masters over CHLModSocket, the other to the Half-Life master over
// CModInfoSocket.

class CModReqDlg : public CDlgPopupBase
{
// Construction
public:
	// bMode TRUE selects the custom-master path, FALSE the Half-Life master;
	// ppModList is the mod-list-head slot to feed and update.
	CModReqDlg( BOOL bMode, mod_t** ppModList, CWnd* pParent = NULL );

// Dialog Data
	//{{AFX_DATA(CModReqDlg)
	enum { IDD = IDD_MODREQ };
	CODBlendBtn		m_btnCancel;		// +104  IDC_MODREQ_CANCEL
	CODStatic		m_stStatus;			// +344  IDC_MODREQ_STATUS
	//}}AFX_DATA

// Overrides
	//{{AFX_VIRTUAL(CModReqDlg)
	public:
	virtual ~CModReqDlg();
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual int		RMLPreIdle();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	SetupButtons();
	void	StartCustomQuery();		// CFavorites list B -> CHLModSocket
	void	StartHLMasterQuery();	// CFavorites list A -> CModInfoSocket

	BOOL			m_bUseCustomMaster;	// +440  ctor mode
	mod_t**			m_ppModList;		// +444  mod-list-head slot
	CModInfoSocket*	m_pModInfoSocket;	// +448  Half-Life master path
	CHLModSocket*	m_pHLModSocket;		// +452  custom-master path
	double			m_flStartTime;		// +456  Sys_FloatTime at query start
	HGLOBAL			m_hStripBmp;		// +464  the loaded button strip
	int				m_nStripStride;		// +468
	int				m_nStripWidth;		// +472
	int				m_nStripHeight;		// +476

	// Generated message map functions
	//{{AFX_MSG(CModReqDlg)
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // MODREQ_DLG_H
