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
// Purpose: declares CVidSelectDlg, the Video hub page (IDD 0xD1 = 209).
//
// $NoKeywords: $
//=============================================================================

#ifndef VIDSELECT_DLG_H
#define VIDSELECT_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "resource_dlg.h"

/////////////////////////////////////////////////////////////////////////////
// CVidSelectDlg dialog

class CVidSelectDlg : public CDlgBase
{
// Construction
public:
	CVidSelectDlg( CWnd* pParent = NULL );

// Dialog data (the binary byte offset is noted next to each control member).
	CODStatic	m_lblOptionsHelp;	// +224  IDC_VIDSELECT_OPTIONSHELP (1146)
	CODStatic	m_lblModesHelp;		// +320  IDC_VIDSELECT_MODESHELP (1147)
	CODStatic	m_lblReturnHelp;	// +416  IDC_VIDSELECT_RETURNHELP (1150)
	CODBlendBtn	m_btnReturn;		// +512  IDCANCEL (strip slice 19, "Done")
	CODBlendBtn	m_btnOptions;		// +752  IDC_VIDSELECT_VIDEO_OPTIONS (1164, slice 23)
	CODBlendBtn	m_btnModes;			// +992  IDC_VIDSELECT_VIDEO_MODES (1165, slice 24)

// Overrides
	//{{AFX_VIRTUAL(CVidSelectDlg)
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVidSelectDlg();

	// Generated message map functions
protected:
	//{{AFX_MSG(CVidSelectDlg)
	afx_msg void	OnVideoModes();		// opens CVideoModeDlg
	afx_msg void	OnVideoOptions();	// opens CVideoDlg
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	//}}AFX_MSG

// Skinned-dialog state mirrored from the binary ctor.
	HGLOBAL	m_headerLoaded;		// +1232
	int		m_headerStride;		// +1236
	int		m_headerW;			// +1240
	int		m_headerH;			// +1244

	DECLARE_MESSAGE_MAP()
};

#endif // VIDSELECT_DLG_H
