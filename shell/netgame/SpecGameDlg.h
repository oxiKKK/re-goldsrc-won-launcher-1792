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
// Purpose: declares CSpecGameDlg, the spectate variant of the Internet Games page.
//
// $NoKeywords: $
//=============================================================================

#ifndef SPECGAME_DLG_H
#define SPECGAME_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "NetGame.h"

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg dialog
//
// The spectate variant of the browser page: the same construction, a Join
// button wearing a different strip cell, and a filter pinned to proxies.
// RTTI ".?AVCSpecGameDlg@@", vftable 0x4B39A0.

class CSpecGameDlg : public CServerBrowserDlg
{
// Construction
public:
	CSpecGameDlg( int nMode, CWnd* pParent = NULL );

// Overrides
	//{{AFX_VIRTUAL(CSpecGameDlg)
	public:
	virtual ~CSpecGameDlg();
	protected:
	virtual BOOL	HasCreateGameButton();					// slot +256
	virtual void	Relayout();								// slot +248
	virtual void	ApplyFilter( netfilter_t* pFilter );	// slot +260
	virtual BOOL	OnInitDialog();							// slot +188
	virtual const char*	GetSettingsSection();				// slot +268
	virtual void	LoadFilter( netfilter_t* pFilter );		// slot +264
	virtual const char*	GetHeaderBitmap();					// slot +272
	virtual UINT	GetJoinCaptionId();						// slot +276
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Not an override: the base's InitButtonStrips (slot +244) is inherited
	// unchanged, and this runs from the constructor and from Relayout.
	void	SetupJoinButtonStrip();
};

#endif // SPECGAME_DLG_H
