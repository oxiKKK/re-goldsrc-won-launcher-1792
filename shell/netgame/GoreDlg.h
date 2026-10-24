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
// Purpose: declares the gore / violence-lock dialog (CGoreDlg, IDD_GORE =
//          232).
//
// $NoKeywords: $
//=============================================================================

#ifndef GORE_DLG_H
#define GORE_DLG_H
#pragma once

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "resource_dlg.h"
#include "AudioDlg.h"

class CGoreDlg : public CDlgBase
{
public:
	CGoreDlg( CWnd* pParent = NULL );
	virtual ~CGoreDlg();

// Dialog data (the binary byte offset is noted next to each control member).
	CODBlendCheckBox	m_checkGore;	// +224  IDC_GORE_CHECKBOX (30); m_bChecked at +476
	CODStatic			m_help;			// +528  IDC_GORE_HELP (1209)
	CODBlendBtn			m_btnDone;		// +624  IDOK (strip slice 19)

// Gore-lock password.
	CString		m_userToken;		// +864

// Skinned-dialog state mirrored from the binary ctor.
	HGLOBAL	m_headerLoaded;			// +868
	int		m_headerStride;			// +872
	int		m_headerW;				// +876
	int		m_headerH;				// +880

protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();
	virtual int		RMLPreIdle();		// (slot 56)

	afx_msg void	OnGoreCheck();		// IDC_GORE_CHECKBOX clicked

	void	SetupButtonStrip();		// (header geometry + Done strip)

	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );

	DECLARE_MESSAGE_MAP()
};

#endif // GORE_DLG_H
