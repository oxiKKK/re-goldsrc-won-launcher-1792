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
// Purpose: declares the multiplayer / advanced game-options page
//          (CGameOptionsDlg, IDD 175 = 0xAF, "head_advanced").
//
// $NoKeywords: $
//=============================================================================

#ifndef GAMEOPTS_DLG_H
#define GAMEOPTS_DLG_H
#pragma once

#include <afxwin.h>
#include "mod.h"
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "ODSlider.h"
#include "AudioDlg.h"

class CGameOptionsDlg : public CDlgBase
{
public:
	CGameOptionsDlg( CWnd* pParent = NULL );
	virtual ~CGameOptionsDlg();

// The mouse-sensitivity slider is built by hand in OnInitDialog (no template
// control); the binary never frees it.
	CODSlider*	m_pSensSlider;		// +224

// Dialog data -- 11 help labels then 10 owner-draw check boxes then Done.
// (Binary byte offset noted next to each control member.)
	CODStatic	m_lblConsole;		// +228   IDC 1174
	CODStatic	m_lblJLook;			// +324   IDC 1173
	CODStatic	m_lblSensHelp;		// +420   IDC 1096
	CODStatic	m_lblReverse;		// +516   IDC 1166
	CODStatic	m_lblMLook;			// +612   IDC 1168
	CODStatic	m_lblMFilter;		// +708   IDC 1171
	CODStatic	m_lblLookStrafe;	// +804   IDC 1170
	CODStatic	m_lblLookSpring;	// +900   IDC 1169
	CODStatic	m_lblCrosshair;		// +996   IDC 1165
	CODStatic	m_lblJoystick;		// +1092  IDC 1167
	CODStatic	m_lblAutoaim;		// +1188  IDC 1172

	CODBlendCheckBox	m_checkConsole;		// +1288  IDC 1067 (m_bChecked +1540)
	CODBlendCheckBox	m_checkJLook;		// +1592  IDC 1066 (+1844)
	CODBlendCheckBox	m_checkJoystick;	// +1896  IDC 1035 (+2148)
	CODBlendCheckBox	m_checkReverse;		// +2200  IDC 1065 (+2452)
	CODBlendCheckBox	m_checkMLook;		// +2504  IDC 1061 (+2756)
	CODBlendCheckBox	m_checkMFilter;		// +2808  IDC 1062 (+3060)
	CODBlendCheckBox	m_checkLookStrafe;	// +3112  IDC 1028 (+3364)
	CODBlendCheckBox	m_checkLookSpring;	// +3416  IDC 1027 (+3668)
	CODBlendCheckBox	m_checkCrosshair;	// +3720  IDC 1064 (+3972)
	CODBlendCheckBox	m_checkAutoaim;		// +4024  IDC 1063 (+4276)
	CODBlendBtn			m_btnDone;			// +4328  IDOK (strip slice 19)

// Working copies of the edited flags (binary byte offset in comments); seeded
// from the player config in OnInitDialog, read back in OnOK.
	int		m_bAutoaim;			// +4568  cfg[2569]
	int		m_bConsole;			// +4572  cfg[2570]
	int		m_bCrosshair;		// +4576  cfg[2563]
	int		m_bReverseMouse;	// +4580  cfg[2560] < 0
	int		m_bJoystick;		// +4584  cfg[2568]
	int		m_bMouseLook;		// +4588  cfg[2566] (gates the slider)
	int		m_bJoystickLook;	// +4592  cfg[2567]
	int		m_bLookSpring;		// +4596  cfg[2562]
	int		m_bLookStrafe;		// +4600  cfg[2561]
	int		m_bMouseFilter;		// +4604  cfg[2565]
	float	m_sensitivity;		// +4608  cfg[2573] (default 3.0)

// Skinned-dialog state mirrored from the binary ctor.
	HGLOBAL	m_headerLoaded;		// +4612
	int		m_headerStride;		// +4616
	int		m_headerW;			// +4620
	int		m_headerH;			// +4624

protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();

	// One per check box: copy the control's m_bChecked into the matching flag.
	afx_msg void	OnCrosshair();		// cmd 1064
	afx_msg void	OnReverse();		// cmd 1065
	afx_msg void	OnJoystick();		// cmd 1035
	afx_msg void	OnMouseLook();		// cmd 1061 (also enables slider)
	afx_msg void	OnLookSpring();		// cmd 1027
	afx_msg void	OnLookStrafe();		// cmd 1028
	afx_msg void	OnMouseFilter();		// cmd 1062
	afx_msg void	OnAutoaim();		// cmd 1063
	afx_msg void	OnJoystickLook();		// cmd 1066
	afx_msg void	OnConsole();		// cmd 1067

	// Mouse-look drives the forced highlight on the two "look" check boxes.
	void			HighlightLookOptions( BOOL bOn );

	afx_msg LRESULT	OnSliderScroll( WPARAM wParam, LPARAM lParam );
	void			SliderScrolled( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void	OnPaint();		// (shared skin-paint thunk)
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );		// (shared skin thunk)
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};

#endif // GAMEOPTS_DLG_H
