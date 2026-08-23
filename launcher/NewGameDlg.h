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
// Purpose: declares the "New Game" skill-select page (CNewGameDlg, IDD 204).
//
// $NoKeywords: $
//=============================================================================

#ifndef NEWGAME_DLG_H
#define NEWGAME_DLG_H
#pragma once

#include <afxwin.h>
#include "DlgBase.h"
#include "HLMainDlg.h"
#include "ODButton.h"
#include "ODStatic.h"

class CNewGameDlg : public CDlgBase
{
public:
	CNewGameDlg( CWnd* pParent = NULL );
	virtual ~CNewGameDlg();

	// Dialog data -- binary byte offsets in comments (labels then buttons).
	CODStatic	m_lblMediumHelp;	// +224  help for the Medium button
	CODStatic	m_lblEasyHelp;	// +320  help for the Easy button
	CODStatic	m_lblDifficultHelp;	// +416  help for the Hard button
	CODStatic	m_lblReturnHelp;	// +512  help for the Done button
	CODBlendBtn	m_btnEasy;	// +608  cmd 26   -- skill 1 (Easy)
	CODBlendBtn	m_btnMedium;	// +848  cmd 32   -- skill 2 (Medium)
	CODBlendBtn	m_btnDifficult;	// +1088 cmd 1157 -- skill 3 (Hard)
	CODBlendBtn	m_btnDone;	// +1328 cmd 2 (IDCANCEL) -- Done

	HGLOBAL	m_headerLoaded;		// +1568
	int		m_headerStride;		// +1572
	int		m_headerW;			// +1576
	int		m_headerH;			// +1580

protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();

	afx_msg void	OnSkillEasy();		// cmd 26 -> StartNewGame(1)
	afx_msg void	OnSkillMedium();		// cmd 32 -> StartNewGame(2)
	afx_msg void	OnSkillHard();		// cmd 1157 -> StartNewGame(3)

	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );

	void	StartNewGame( int skill );

	DECLARE_MESSAGE_MAP()
};

#endif // NEWGAME_DLG_H
