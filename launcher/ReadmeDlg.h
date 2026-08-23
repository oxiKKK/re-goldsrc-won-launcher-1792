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
// Purpose: declares the Readme sub-dialog (CReadmeDlg, IDD 0xBE = 190).
//
// $NoKeywords: $
//=============================================================================

#ifndef README_DLG_H
#define README_DLG_H
#pragma once

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODEdit.h"
#include "resource_dlg.h"

/////////////////////////////////////////////////////////////////////////////
// CReadmeDlg dialog

class CReadmeDlg : public CDlgBase
{
// Construction
public:
	CReadmeDlg( CWnd* pParent = NULL );

// Dialog Data
	//{{AFX_DATA(CReadmeDlg)
	enum { IDD = IDD_README };
	CODBlendBtn	m_btnDone;		// +224  IDOK ("Done")
	CStatic		m_frame;		// +464  IDC_README_STATIC, the text panel's frame
	//}}AFX_DATA

// Overrides
	//{{AFX_VIRTUAL(CReadmeDlg)
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CReadmeDlg();

protected:
	HGLOBAL		m_headerLoaded;	// +524
	int			m_headerStride;	// +528
	int			m_headerW;		// +532
	int			m_headerH;		// +536
	CODEdit*	m_pRich;		// +540  fills the frame, built in OnInitDialog

	// Generated message map functions
	//{{AFX_MSG(CReadmeDlg)
	virtual BOOL	OnInitDialog();
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // README_DLG_H
