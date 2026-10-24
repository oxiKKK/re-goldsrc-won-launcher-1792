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
// Purpose: declares the Configuration sub-dialog (CConfigureDlg,
//          IDD_CONFIGURE = 160).
//
// $NoKeywords: $
//=============================================================================

#ifndef CONFIG_DLG_H
#define CONFIG_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "HLMainDlg.h"
#include "ODButton.h"
#include "ODStatic.h"

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg dialog
//
// The Configuration page: six caption rows, each flying its sub-page out of
// its own button.

class CConfigureDlg : public CDlgBase
{
// Construction
public:
	CConfigureDlg( CWnd* pParent = NULL );

// Attributes
public:
	//{{AFX_DATA(CConfigureDlg)
	CODStatic	m_lblCustomize;			// +224   IDC 1199  (hidden)
	CODBlendBtn	m_btnCustomize;			// +320   IDC 1198  (hidden)
	CODStatic	m_lblAutopatchHelp;		// +560   IDC 1152
	CODStatic	m_lblGore;				// +656   IDC 1151
	CODBlendBtn	m_btnAutopatch;			// +752   IDC 41
	CODBlendBtn	m_btnGore;				// +992   IDC 30
	CODStatic	m_lblReturnToMain;		// +1232  IDC 1150
	CODStatic	m_lblVidHelp;			// +1328  IDC 1147
	CODStatic	m_lblUnused;			// +1424  constructed, not wired
	CODStatic	m_lblControlHelp;		// +1520  IDC 1149
	CODStatic	m_lblAudioHelp;			// +1616  IDC 1148
	CODBlendBtn	m_btnDone;				// +1712  IDOK
	CODBlendBtn	m_btnVideo;				// +1952  IDC 31
	CODBlendBtn	m_btnControls;			// +2192  IDC 39
	CODBlendBtn	m_btnAudio;				// +2432  IDC 36
	//}}AFX_DATA

// Overrides
	//{{AFX_VIRTUAL(CConfigureDlg)
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CConfigureDlg();

protected:
	HGLOBAL		m_headerLoaded;			// +2672
	int			m_headerStride;			// +2676
	int			m_headerW;				// +2680
	int			m_headerH;				// +2684

	// Generated message map functions
protected:
	//{{AFX_MSG(CConfigureDlg)
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg void	OnVideo();
	afx_msg void	OnAudio();
	afx_msg void	OnControls();
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg void	OnAutopatch();
	afx_msg void	OnGore();
	afx_msg void	OnCustomize();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // CONFIG_DLG_H
