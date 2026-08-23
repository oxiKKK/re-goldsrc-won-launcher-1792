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
// Purpose: declares the Load Game dialog (CLoadDlg, IDD 0x9F = 159).
//
// $NoKeywords: $
//=============================================================================

#ifndef LOAD_DLG_H
#define LOAD_DLG_H
#pragma once

#include <afxwin.h>
#include "DlgBase.h"
#include "HLMainDlg.h"
#include "ODButton.h"
#include "ODListCtrl.h"

// One parsed save slot (the binary's 452-byte per-save record, sub_426020).
typedef struct savegame_s
{
	char		filename[260];	// +0    the .sav file name
	char		mapname[32];	// +260  internal map name (from the GameHeader)
	char		comment[80];	// +292  description (trailing elapsed time stripped)
	char		elapsed[32];	// +372  elapsed-time string (last token of the comment)
	char		date[32];		// +404  formatted last-write date
	FILETIME	fileTime;		// +436  last-write time (sort key)
	int			bQuicksave;	// +444  name contains "quick"
	int			bAutosave;	// +448  name contains "autosave"
} savegame_t;					// 452 bytes (0x1C4)

// Save-file helpers (bodies in LoadDlg.cpp).
int		Launcher_ParseSaveFile( char* lpFileName, const char* pszSaveName, savegame_t* rec );
int		Launcher_GetPlayerName( struct cfg_keybind_t* pBindings, char* pSaveKey, char* pLoadKey );

class CLoadDlg : public CDlgBase
{
public:
	CLoadDlg( CWnd* pParent = NULL );
	virtual ~CLoadDlg();

// Dialog data (the binary byte offset is noted next to each control member).
	CODBlendBtn				m_btnDelete;	// +224  IDC 1021 (Delete)
	CODBlendBtn				m_btnBack;		// +464  IDCANCEL
	CODBlendBtn				m_btnLoad;		// +704  IDC 1020 (Load)
	CODSaveGameListCtrl*	m_pList;		// +960
	savegame_t*				m_pSaves;		// +980  parsed save array
	int						m_nSaves;		// +984  save count

protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();

	// vftable +152 -- the CWnd slot is overridden but only forwards.
	virtual LRESULT	WindowProc( UINT message, WPARAM wParam, LPARAM lParam );

	void	PopulateSaves();

	afx_msg void	OnLoad();
	afx_msg void	OnDelete();
	afx_msg void	UpdateButtonStates();
	afx_msg void	OnNcPaint();
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );

// Skinned-dialog state mirrored from the binary ctor.
	CBrush		m_bkBrush;		// +964
	HGLOBAL		m_headerLoaded;	// +944
	int			m_headerStride;	// +948
	int			m_headerW;		// +952
	int			m_headerH;		// +956
	COLORREF	m_clrText;		// +976
	COLORREF	m_clrBk;		// +972

	DECLARE_MESSAGE_MAP()
};

#endif // LOAD_DLG_H
