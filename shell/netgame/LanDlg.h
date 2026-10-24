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
// Purpose: declares CLan, the LAN games page.
//
// $NoKeywords: $
//=============================================================================

#ifndef LAN_DLG_H
#define LAN_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "ODButton.h"
#include "ODHLListCtrl.h"
#include "DlgConnectableBase.h"
#include "resource_dlg.h"

class CNetGameDlg;

class CLan : public CDlgConnectableBase
{
public:
	CLan( CWnd* pParent = NULL );		// 0x420390
	virtual ~CLan();					// 0x420B80

	enum { IDD = IDD_LAN };				// 223 (0xDF)

	// The report list joins the double-clicked row through its host page.
	friend class CODHLListCtrl;

protected:
	virtual void	DoDataExchange( CDataExchange* pDX );	// 0x420490
	virtual BOOL	OnInitDialog();							// 0x420530
	virtual int		RMLPreIdle();							// 0x420C50 (frame-protocol slot 56)
	virtual void	Relayout();								// 0x420EB0 (slot 62)
	virtual void	OnOK();									// 0x4213A0 (slot 49, the Done button)

	// Relayout the seven skinned buttons + the server list against the current
	// header strip size and the running-game state.
	void	UpdateLayout();			// 0x4213C0

	virtual void	InitButtonStrips();		// 0x420A40 (slot 61)
	void	PopulateList();			// 0x420F00

	afx_msg void	OnConnect();	// 1200  0x420FB0
	afx_msg void	OnRefresh();	// 1203  0x420FF0
	afx_msg void	OnCreateGame();	// 1205  0x421050
	afx_msg void	OnLeaveGame();	// 137   0x4212A0
	afx_msg void	OnSpectate();	// 136   0x421320
	afx_msg void	OnServerInfo();	// 1204  0x421660

	CODBlendBtn		m_btnServerInfo;// +248   id 1204
	CODBlendBtn		m_btnSpectate;	// +488   id 136
	CODBlendBtn		m_btnLeave;		// +728   id 137
	CODBlendBtn		m_btnOK;		// +968   id 1
	CODBlendBtn		m_btnCreate;	// +1208  id 1205
	CODBlendBtn		m_btnRefresh;	// +1448  id 1203
	CODBlendBtn		m_btnConnect;	// +1688  id 1200
	CODHLListCtrl*	m_pServerList;	// +1928  the server list control
	HGLOBAL			m_hStripDib;	// +1932  btns_main strip DIB (Launcher_HeaderLoaded)
	int				m_nStripStride;	// +1936  strip row stride  (Launcher_HeaderStride)
	int				m_stripWH[2];	// +1940  strip cell {w,h}  (Launcher_HeaderSize)
	BYTE			m_bProxySelected;	// +1948  last connect-button cell (proxy => spectate)

	double			m_flQueryStart;	// +224   Sys_FloatTime when the LAN broadcast went out
	double			m_flQueryTick;	// +232   Sys_FloatTime of the last idle pass
	int				m_bQuerying;	// +240   a LAN enumeration is in flight
	CNetGameDlg*	m_pBrowser;		// +244   hosted property-sheet browser engine

	virtual void	OnConnectAbort();	// slot 63

	// Generated message map functions
	//{{AFX_MSG(CLan)
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // LAN_DLG_H
