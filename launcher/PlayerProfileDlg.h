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
// Purpose: declares the player-identity / "Customize" page
//          (CPlayerProfileDlg).
//
// $NoKeywords: $
//=============================================================================

#ifndef PLAYERPROFILE_DLG_H
#define PLAYERPROFILE_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "HLMainDlg.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "ODComboBox.h"
#include "resource_dlg.h"
#include "AudioDlg.h"
#include "ODSlider.h"
#include "BorderlessEdit.h"

class CServerBrowser;
class CInfoDescription;

void	SetupLabel( CODStatic* pLbl, const RECT* prc, UINT uID );
CODBitmapButton*	SetupArrowButton( const RECT* prc, CWnd* pParent,
			const char* pszNormal, const char* pszDown,
			const char* pszFocus, UINT nID );

/////////////////////////////////////////////////////////////////////////////
// CPlayerProfileDlg dialog
//
// The page edits a private copy of the browser document and only writes it
// back on OK, so Cancel can restore the snapshot the constructor took.

class CPlayerProfileDlg : public CDlgBase
{
// Construction
public:
	CPlayerProfileDlg( CWnd* pParent = NULL );

// Dialog Data
	// The customize template reuses several IDC_AUDIO_* numeric ids for its own
	// labels and switches.
	//{{AFX_DATA(CPlayerProfileDlg)
	enum { IDD = IDD_PROFILE };
	CODStatic			m_lblMicVol;		// +224   32789
	CODStatic			m_lblMiles;			// +320   1224
	CODStatic			m_lblSpeakVol;		// +416   32790
	CODBlendCheckBox	m_chkVoice;			// +512   IDC_OPTS_VOCENABLE
	CODBlendBtn			m_btnSetInfo;		// +816   IDC_BTN_SETINFO
	CODBlendCheckBox	m_chkHiModels;		// +1056  IDC_OPTS_HIMODELS
	CODStatic			m_lblNickname;		// +1360  IDC_PROFILE_NICKNAME
	CODStatic			m_lblLogo;			// +1456  IDC_PROFILE_LOGO
	CODStatic			m_lblLogoColor;		// +1552  IDC_PROFILE_LOGOCOLOR
	CODBlendBtn			m_unk1648;			// +1648  constructed, never bound or used
	CODStatic			m_unk1888;			// +1888
	CODStatic			m_lblModel;			// +1984  IDC_PROFILE_MODEL
	CODStatic			m_lblColor;			// +2080  IDC_PROFILE_COLOR
	CODStatic			m_unk2176;			// +2176
	CODStatic			m_unk2272;			// +2272
	CODStatic			m_unk2368;			// +2368
	CODBlendBtn			m_btnModelPrev;		// +2464  IDC_PROFILE_LEFT
	CODBlendBtn			m_btnModelNext;		// +2704  IDC_PROFILE_RIGHT
	CODBlendBtn			m_btnLogoPrev;		// +2944  IDC_PROFILE_LOGO_PREV
	CODBlendBtn			m_btnLogoNext;		// +3184  IDC_PROFILE_LOGO_NEXT
	CODBlendBtn			m_btnDone;			// +3424  IDOK
	//}}AFX_DATA

// Attributes
public:
	float				m_flVoiceXmit;		// +3664  transmit slider value, 0..100
	float				m_flVoiceRecv;		// +3668  receive slider value, 0..100
	CODSlider*			m_pVoiceXmit;		// +3672
	CODSlider*			m_pVoiceRecv;		// +3676
	CODColorComboBox	m_colorCombo;		// +3680  built inline, not DDX-bound
	CBorderLessEdit*	m_pNameEdit;		// +3876  the nickname field
	int					m_bHiModels;		// +3880  latched m_chkHiModels state
	int					m_bVoiceEnable;		// +3884  latched m_chkVoice state
	int					m_unk3888;			// +3888  ctor-zeroed, never read
	CServerBrowser*		m_pSavedConfig;		// +3892  ctor snapshot of the document
	CBrush				m_brBlack;			// +3896  preview background brush
	HGLOBAL				m_headerLoaded;		// +3904  the loaded button strip
	int					m_headerStride;		// +3908
	int					m_headerW;			// +3912  one strip cell
	int					m_headerH;			// +3916
	CInfoDescription*	m_pUserDesc;		// +3920  user.scr model, live inside OnSetInfo
	int					m_bConfigChanged;	// +3924  set by ApplyToConfig
	HGLOBAL				m_hLogoDib;			// +3928  current logo preview DIB
	HGLOBAL				m_hModelDib;		// +3932  current model preview DIB
	char				m_szLogo[260];		// +3936  current spray-logo name
	int					m_logoIdx;			// +4196
	int					m_logoCount;		// +4200
	char*				m_pLogoNames;		// +4204  packed NUL-separated name blob
	int					m_nLogoNamesLen;	// +4208  byte length of that blob
	char				m_szModel[260];		// +4212  current model name (default "gordon")
	int					m_modelIdx;			// +4472
	int					m_modelCount;		// +4476
	DWORD				m_logoColorWord;	// +4480  the logo tint, as a COLORREF
	int					m_modelHue0;		// +4484  model band hue, rows 160..191
	int					m_modelHue1;		// +4488  model band hue, rows 192..223
	CODSlider*			m_pModelColor0;		// +4492
	CODSlider*			m_pModelColor1;		// +4496

// Overrides
	//{{AFX_VIRTUAL(CPlayerProfileDlg)
	public:
	virtual ~CPlayerProfileDlg();
	virtual BOOL	OnCommand( WPARAM wParam, LPARAM lParam );
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();
	virtual void	OnCancel();
	virtual int		RMLPreIdle();
	virtual void	DrawDialogOverlay( CDC* pDC, RECT* prc );
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	CacheHeaderMetrics();						// header strip -> the blend buttons
	void	LoadFromConfig( CServerBrowser* pCfg );

	// Model / logo enumeration.
	void	ScanModelList();		// rescan models/player, "gordon" first
	void	ScanLogoList();			// pack logos\*.bmp into m_pLogoNames
	int		LogoIndexByName();
	int		ModelIndexByName();		// also rewrites m_szModel to the full path

	void	SliderScrolled( int nSBCode, int nPos, CObject* pObj );

	const char*	ModelNameByIndex( int idx );
	const char*	LogoNameByIndex( int idx );

	// Model / logo preview cluster.
	HGLOBAL	LoadPreviewDib( int bModel, const char* pszName );	// BMP -> DIB + palette
	void	ApplyModelPalette();								// work palette -> model DIB
	void	RecolourLogoDib( COLORREF clr );					// HSV tint the logo DIB
	void	RefreshModelPreview();								// rebuild the palette + apply
	void	RefreshLogoPreview();								// recolour + invalidate
	void	PaintModelPreview( HDC hdc );						// centred model blit

	// Generated message map functions
	//{{AFX_MSG(CPlayerProfileDlg)
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT	OnSliderScroll( WPARAM wParam, LPARAM lParam );
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg HBRUSH	OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void	OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg void	OnHiModels();
	afx_msg void	OnVoiceEnable();
	afx_msg void	OnModelPrev();
	afx_msg void	OnModelNext();
	afx_msg void	OnLogoPrev();
	afx_msg void	OnLogoNext();
	afx_msg void	ApplyToConfig();
	afx_msg void	OnSetInfo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // PLAYERPROFILE_DLG_H
