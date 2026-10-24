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
// Purpose: declares the Video Modes dialog (CVideoModeDlg, IDD 0xD0 = 208) and
//          CODVideoList.
//
// $NoKeywords: $
//=============================================================================

#ifndef VIDEOMODE_DLG_H
#define VIDEOMODE_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "launcher.h"
#include "engine.h"
#include "vid.h"
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "ODListCtrl.h"
#include "ODComboBox.h"
#include "ODTabCtrl.h"
#include "resource_dlg.h"

/////////////////////////////////////////////////////////////////////////////
// CODVideoList window
//
// The display-mode report list: each row record is the vmode_t itself.

class CODVideoList : public CODListCtrl
{
public:
	virtual void	DrawRow( CDC* pDC, int iRow );
};

// One row of a CVideoModeDlg picker table (label + help/detail text).
typedef struct vidmodedesc_s
{
	char	label[32];		// +0   the list row text
	char	help[128];		// +32  detail text shown for the row
} vidmodedesc_t;			// 160 bytes (0xA0)

// Child-control IDs the page creates by hand; they have no generated IDC_ symbol.
#define IDC_VIDMODE_TABS		119		// renderer tab strip
#define IDC_VIDMODE_GLCOMBO		120		// OpenGL driver combo
#define IDC_VIDMODE_D3DCOMBO	122		// Direct3D device combo
#define IDC_VIDMODE_MODELIST	1005	// display-mode report list

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg dialog

class CVideoModeDlg : public CDlgBase
{
// Construction
public:
	CVideoModeDlg( CWnd* pParent = NULL );

// Saved state snapshots (binary byte offsets in comments).
	int		m_savedVidRestart;		// +224    force_mode_set snapshot (ctor)
	int		m_bWasWindowed;			// +228    gEngineModeWindowed at dialog entry

// The two driver tables backing the combos (filled in OnInitDialog).
	vidmodedesc_t	m_glDrivers[128];	// +232    GL-driver rows (gldrv\drvmap.txt)
	int				m_nGLDrivers;		// +20712
	vidmodedesc_t	m_d3dDevices[128];	// +20716  D3D-device rows
	int				m_nD3DDevices;		// +41196

// The settings being edited (loaded from g_EngineMode, applied by OnOK).
	int		m_bWindowedMouse;		// +41200  mouse captured while windowed
	char	m_szGLDriver[128];		// +41204  current GL driver ("Default" / "3dfxgl.dll")
	char	m_szD3DDevice[128];		// +41332  current D3D device ("Default")

// The four hand-built controls (created in OnInitDialog; the binary never frees
// them).
	CODVideoList*		m_pModeList;		// +41460  IDC 1005 (display-mode report list)
	CODTabCtrl*			m_pRendererTabs;	// +41464  IDC 119  (&Software / Open&GL / &Direct3D)
	CODDriverComboBox*	m_pGLDriverCombo;	// +41468  IDC 120
	CODDriverComboBox*	m_pD3DDeviceCombo;	// +41472  IDC 122

	int			m_unk41476;			// +41476  (ctor 0)
	int			m_nMode;			// +41480  current display-mode ordinal
	int			m_nWidth;			// +41484
	int			m_nHeight;			// +41488
	int			m_nBPP;				// +41492
	int			m_bWindowed;		// +41496
	int			m_b3DWarning;		// +41500  "3DWarning" profile int (persisted by the dtor)
	vidtype_t	m_nVidType;			// +41504  renderer being edited (ctor VT_Software)
	vidtype_t	m_nEngineVidType;	// +41508  gEngineVidType at dialog entry
	int			m_bOpenGLAvail;		// +41512
	int			m_bD3DAvail;		// +41516
	CBitmap		m_bmp;				// +41520  (ctor-zeroed; no reader in the image)

// Skinned-dialog state mirrored from the binary ctor.
	HGLOBAL	m_headerLoaded;			// +41528
	int		m_headerStride;			// +41532
	int		m_headerW;				// +41536
	int		m_headerH;				// +41540

// Dialog data (the binary byte offset is noted next to each control member).
	CODBlendBtn			m_btn3DInfoSite;	// +41544  IDC_VIDMODE_3D_INFO_SITE (1216, strip slice 61)
	CODStatic			m_lblHint;			// +41784  IDC_VIDMODE_HINT (1173, HELP_COLOR help text)
	CODBlendCheckBox	m_checkWindowed;	// +41880  IDC_VIDMODE_WINDOWED (1043)
	CODBlendCheckBox	m_checkMouse;		// +42184  IDC_VIDMODE_MOUSE (1044)
	CODStatic			m_lblAdvanced;		// +42488  IDC_VIDMODE_ADVANCED (1172)
	CODBlendBtn			m_btnCancel;		// +42584  IDC_VIDMODE_CANCEL (1170, strip slice 14)
	CODBlendBtn			m_btnOK;			// +42824  IDOK (strip slice 22)

// Operations
protected:
	void	AddModeColumns( RECT* prcList );
	void	SetRenderer( vidtype_t type );
	// Refill the mode list from modelist and re-select wCur x hCur.
	void	RebuildModeList( int wCur, int hCur );
	void	LoadGLDrivers();
	// Show the driver combo + advanced text for tab 0 sw / 1 gl / 2 d3d.
	void	UpdateForRenderer( int iTab );
	// Grey the mouse-capture box while fullscreen.
	void	UpdateMouseCheck( int bWindowed );
	static void	AddDriverRow( CODDriverComboBox* pCombo, vidmodedesc_t* pRows,
					int* pnRows, const char* pszLabel, const char* pszDesc );

// Overrides
	//{{AFX_VIRTUAL(CVideoModeDlg)
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();				// validate + persist + apply
	virtual void	OnCancel();			// restore the entry mode
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVideoModeDlg();			// persists "3DWarning"

	// Generated message map functions
protected:
	//{{AFX_MSG(CVideoModeDlg)
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg void	OnAdvanced();
	afx_msg void	OnMouseCheck();
	afx_msg void	OnWindowedCheck();
	afx_msg void	OnSelectRenderer();		// renderer tab pick, IDC 119
	afx_msg void	OnSelectMode();
	afx_msg void	OnSelectGLDriver();
	afx_msg void	On3DInfoSite();
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // VIDEOMODE_DLG_H
