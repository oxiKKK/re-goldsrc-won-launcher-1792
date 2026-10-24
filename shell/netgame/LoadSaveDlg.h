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
// Purpose: declares the Load or Save Game page (CLoadSaveDlg).
//
// $NoKeywords: $
//=============================================================================

#ifndef LOADSAVE_DLG_H
#define LOADSAVE_DLG_H
#pragma once

#include <afxwin.h>
#include "DlgBase.h"
#include "HLMainDlg.h"
#include "ODButton.h"
#include "ODStatic.h"

class CLoadSaveDlg : public CDlgBase
{
public:
	CLoadSaveDlg( CWnd* pParent = NULL );
	virtual ~CLoadSaveDlg();

	CODStatic	m_lblRow2;	// +224   IDC 1150  value row 2
	CODStatic	m_lblRow1;	// +320   IDC 1163  value row 1
	CODStatic	m_field;	// +416   IDC 1080  player-name field
	CODStatic	m_lblRow0;	// +512   IDC 1162  value row 0
	CODBlendBtn	m_btnBack;	// +608   IDCANCEL  (Back)
	CODBlendBtn	m_btnRow1;	// +848   IDC 33    caption row 1
	CODBlendBtn	m_btnRow0;	// +1088  IDC 27    caption row 0

protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();

	afx_msg void	OnLoadGame();
	afx_msg void	OnSaveGame();
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );

	HGLOBAL		m_headerLoaded;	// +1328
	int			m_headerStride;	// +1332
	int			m_headerW;		// +1336
	int			m_headerH;		// +1340

	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );

	DECLARE_MESSAGE_MAP()
};

#endif // LOADSAVE_DLG_H
