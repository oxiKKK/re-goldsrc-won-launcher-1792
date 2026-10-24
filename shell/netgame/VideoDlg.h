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
// Purpose: declares CVideoDlg, the Video options page (IDD 0xA1 = 161).
//
// $NoKeywords: $
//=============================================================================

#ifndef VIDEO_DLG_H
#define VIDEO_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "ODSlider.h"
#include "resource_dlg.h"

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg dialog

class CVideoDlg : public CDlgBase
{
// Construction
public:
	CVideoDlg( CWnd* pParent = NULL );

// Attributes
public:
	// Settings being edited: loaded from the player config in OnInitDialog,
	// written back (and saved) by OnOK.
	int		m_bSpriteSkip;		// +224   "skip sprites" flag

	// The three sliders are built by hand in OnInitDialog (no dialog-template
	// controls); the binary never frees them.
	CODSlider*	m_pScreenSizeSlider;	// +228
	CODSlider*	m_pGammaSlider;			// +232
	CODSlider*	m_pGlareSlider;			// +236

	// gfx/shell/gamma.bmp (the preview image) and the 0..255 ramp it is pushed
	// through, rebuilt by BuildGammaRamp whenever gamma or glare move.
	RECT	m_rcGamma;			// +240  {0, 0, w, h} of gamma.bmp
	HGLOBAL	m_hGammaDib;		// +256

	HGLOBAL	m_headerLoaded;		// +260
	int		m_headerStride;		// +264
	int		m_headerW;			// +268
	int		m_headerH;			// +272

	BYTE	m_gammaTable[256];	// +276
	BYTE	m_pad532[4];		// +532

	//{{AFX_DATA(CVideoDlg)
	CODBlendCheckBox	m_checkSpriteSkip;	// +536   IDC_SKIP_SPRITE (1043)
	CODStatic			m_lblGammaHelp;		// +840   IDC_VIDEO_GAMMAHELP (1168)
	CODStatic			m_lblGlareHelp;		// +936   IDC_VIDEO_GLAREHELP (1169)
	CODStatic			m_lblScreenSize;	// +1032  IDC_VIDEO_SCREENSIZE (1101)
	CODStatic			m_lblGamma;			// +1128  IDC_VIDEO_GAMMA (1099)
	CODStatic			m_lblGlare;			// +1224  IDC_VIDEO_GLARE (1100)
	CStatic				m_imgGamma;			// +1320  IDC_VIDEO_GAMMAIMAGE
	CODBlendBtn			m_btnDone;			// +1384  IDOK
	//}}AFX_DATA

	float	m_screenSize;		// +1624  screen size, 3..12
	float	m_gamma;			// +1628  gamma, 18..30
	float	m_glare;			// +1632  glare reduction, 0..10

// Operations
protected:
	// HL_WM_SCROLL worker: live gamma preview + commit on release.
	void	SliderScrolled( int nSBCode, int nPos, CObject* pObj );

// Gamma preview pipeline
	void	BuildGammaRamp();
	void	DrawGammaDib( HGLOBAL hDib, CDC* pDC, RECT* prcDest );
	void	DrawGammaPreview( CDC* pDC );
	void	RedrawGammaImage();

// Overrides
	//{{AFX_VIRTUAL(CVideoDlg)
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();				// commit + save
	// The skin overdraw hook (CDlgBase slot 54) drops the rect and forwards to
	// DrawGammaPreview, which is what puts the preview on screen.
	virtual void	DrawDialogOverlay( CDC* pDC, RECT* prc );
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVideoDlg();			// frees the gamma DIB

	// Generated message map functions
protected:
	//{{AFX_MSG(CVideoDlg)
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT	OnSliderScroll( WPARAM wParam, LPARAM lParam );
	afx_msg void	OnSpriteSkipCheck();
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // VIDEO_DLG_H
