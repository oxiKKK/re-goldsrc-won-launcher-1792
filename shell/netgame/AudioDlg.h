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
// Purpose: declares the Audio options page (CAudioDlg, IDD 162, "head_audio").
//
// $NoKeywords: $
//=============================================================================

#ifndef AUDIO_DLG_H
#define AUDIO_DLG_H
#pragma once

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "ODSlider.h"
#include "resource_dlg.h"

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg dialog

class CAudioDlg : public CDlgBase
{
// Construction
public:
	CAudioDlg( CWnd* pParent = NULL );

// Attributes
public:
	// Settings being edited: loaded from the player config in OnInitDialog,
	// written back (and saved) by OnOK.
	float	m_volume;			// +224  game volume, 0..100 (config keeps 0..1)
	float	m_suitVolume;		// +228  HEV suit volume, 0..100
	int		m_bHighQuality;		// +232  "hisound" -- high quality sound
	int		m_bCDMusic;			// +236  play CD music
	int		m_bA3D;				// +240  A3D hardware support
	int		m_bEAX;				// +244  EAX hardware support
	int		m_savedVidRestart;	// +248  force_mode_set snapshot (ctor)

	// The two sliders are built by hand in OnInitDialog; the binary never
	// frees them.
	CODSlider*	m_pVolumeSlider;	// +252
	CODSlider*	m_pSuitVolSlider;	// +256
	BYTE		m_pad260[4];		// +260

	//{{AFX_DATA(CAudioDlg)
	CODBlendCheckBox	m_checkEAX;			// +264   IDC_AUDIO_EAX (1028)
	CODStatic			m_lblCDHint;		// +568   IDC_AUDIO_CDHINT (1212)
	CODBlendCheckBox	m_checkA3D;			// +664   IDC_AUDIO_A3D (1027)
	CODStatic			m_lblVolume;		// +968   IDC_AUDIO_VOLUME (1090)
	CODStatic			m_lblSuitVol;		// +1064  IDC_AUDIO_SUITVOL (1079)
	CODBlendCheckBox	m_checkCDMusic;		// +1160  IDC_AUDIO_USECD (1026)
	CODBlendCheckBox	m_checkHighQuality;	// +1464  IDC_AUDIO_HIGHQUALITY (1025)
	CODBlendBtn			m_btnDone;			// +1768  IDOK
	//}}AFX_DATA

// Operations
protected:
	// HL_WM_SCROLL worker: commits a slider position into the value members.
	void	SliderScrolled( int nSBCode, int nPos, CObject* pObj );

// Overrides
	//{{AFX_VIRTUAL(CAudioDlg)
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();				// commit + save
	virtual void	OnCancel();			// restore the restart flag, then OnOK
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAudioDlg();

	// Generated message map functions
protected:
	//{{AFX_MSG(CAudioDlg)
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT	OnSliderScroll( WPARAM wParam, LPARAM lParam );
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnHighQuality();	// forces a restart
	afx_msg void	OnUseCD();
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg void	OnA3D();			// forces a restart
	afx_msg void	OnEAX();			// forces a restart
	//}}AFX_MSG

	HGLOBAL	m_headerLoaded;	// +2008
	int		m_headerStride;	// +2012
	int		m_headerW;		// +2016
	int		m_headerH;		// +2020

	DECLARE_MESSAGE_MAP()
};

// The slider notification message, RegisterWindowMessage( "HL_WM_SCROLL" ).
extern UINT	g_uiScrollMsg;

#endif // AUDIO_DLG_H
