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
// Purpose: declares CPlayerInfoDlg, the server-browser "details" dialog.
//
// $NoKeywords: $
//=============================================================================

#ifndef PLAYERINFODLG_H
#define PLAYERINFODLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "ODStatic.h"
#include "ODButton.h"
#include "ServerInfo.h"
#include "odlistctrls.h"

class CPlayerInfoDlg : public CDlgBase
{
public:
	// (sic) the parent comes first: 0x452610 hands its second argument to
	// CDlgBase and stores the third at +1064.
	CPlayerInfoDlg( CWnd* pParent, CServerInfo* pServer );
	virtual ~CPlayerInfoDlg();										// 0x452790

	// Dialog data (binary byte offsets in comments).  Two pointer slots precede
	// the captions: the report lists OnInitDialog news in place.
	CODPlayerListCtrl*	m_pPlayerList;	// +224  (a1 dword +56)  built in OnInitDialog
	CODRuleListCtrl*	m_pRuleList;	// +228  (a1 dword +57)  built in OnInitDialog

	CODStatic			m_lblServerIp;		// +232   1194 IDC_PLAYERINFO_SERVERIP
	CODStatic			m_lblServerName;	// +328   1192 IDC_PLAYERINFO_SERVERNAME
	CODStatic			m_lblServerPing;	// +424   1196 IDC_PLAYERINFO_SERVERPING
	CODStatic			m_lblPing;			// +520   1112 IDC_PLAYERINFO_SERVER_PING
	CODStatic			m_lblName;			// +616   1110 IDC_PLAYERINFO_SERVER_NAME
	CODStatic			m_lblIp;			// +712   1111 IDC_PLAYERINFO_SERVER_IP
	CODBlendBtn			m_btnDone;			// +808   IDOK

	// Skin header strip (latched from the launcher header context in the ctor,
	// via the 0x452720 helper).
	HGLOBAL	m_headerLoaded;		// +1048
	int		m_headerStride;		// +1052
	int		m_headerW;			// +1056
	int		m_headerH;			// +1060

	CServerInfo*	m_pServer;	// +1064  the server whose details are shown

protected:
	virtual void	DoDataExchange( CDataExchange* pDX );	// 0x452850
	virtual BOOL	OnInitDialog();							// 0x4528F0

	void	LoadHeaderStrip();		// 0x452720 (latch header dims + button strip)

	afx_msg void	OnPaint();						// 0x412860
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );		// 0x412870
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );	// 0x406FE0
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );	// 0x453D00

	DECLARE_MESSAGE_MAP()
};

#endif // PLAYERINFODLG_H
