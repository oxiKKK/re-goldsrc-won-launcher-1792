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
// Purpose: declares the Save Game dialog (CSaveDlg, IDD 203, "head_save").
//
// $NoKeywords: $
//=============================================================================

#ifndef SAVE_DLG_H
#define SAVE_DLG_H
#pragma once

#include <afxwin.h>
#include <stdio.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODListCtrl.h"
#include "resource.h"
#include "resource_dlg.h"
#include "LoadDlg.h"

class CSaveDlg : public CDlgBase
{
public:
	CSaveDlg( CWnd* pParent = NULL );
	virtual ~CSaveDlg();

	enum { IDD = IDD_SAVE };					// 203

	void	PopulateSaves();

// Dialog data -- binary byte offsets in comments.
	CODBlendBtn	m_btnDelete;	// +224  IDC 1021, IDS_BTN_DELETE, strip slice 27
	CODBlendBtn	m_btnBack;		// +464  IDCANCEL, IDS_BTN_CANCEL, strip slice 14
	CODBlendBtn	m_btnSave;		// +704  IDC 1019, IDS_BTN_SAVE, strip slice 12

	CODSaveGameListCtrl*	m_pList;	// +960  heap list (0x774 bytes)
	CBrush		m_bkBrush;		// +964  RGB( 63, 63, 63 ) row background
	COLORREF	m_clrBk;		// +972  0x3F3F3F
	COLORREF	m_clrText;		// +976  0xFFFFFF
	savegame_t*	m_pSaves;		// +980  row records (row 0 = the new-save slot)
	int			m_nSaves;		// +984  rows (incl. the new-save slot)

	HGLOBAL		m_headerLoaded;	// btns_main strip state (ctor)
	int			m_headerW;		// strip cell width
	int			m_headerH;		// strip cell height
	int			m_headerStride;	// strip rows

protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();

	afx_msg void	OnSave();		// list-activate + "Save" button
	afx_msg void	OnDelete();		// "Delete" button
	afx_msg void	UpdateButtonStates();		// list selection-changed -> dim/enable

	afx_msg void	OnNcPaint();
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};

// Shared Save/Load helpers, also used by CLoadDlg.
char*		Save_GetSaveDir();
savegame_t*	Save_ClearRecord( savegame_t* pRec );
BOOL		Save_ParseGameHeader( FILE* fp, char* pszMapName, char* pszComment );
BOOL		Save_ReadFileInfo( LPCSTR lpFileName, const char* pszDisplayName, savegame_t* pRec );

#endif // SAVE_DLG_H
