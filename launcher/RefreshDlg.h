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
// Purpose: declares CRefreshDlg, the server-refresh progress dialog.
//
// $NoKeywords: $
//=============================================================================

#ifndef REFRESH_DLG_H
#define REFRESH_DLG_H

#include <afxwin.h>
#include "NetGame.h"
#include "DlgPopupBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "resource_dlg.h"

class CServerInfo;

// Refresh criteria/config block handed in by the server browser (timeouts +
// retry caps that bound the ping state machine).
struct RefreshCriteria_t
{
	int		m_nMaxOutstanding;	// +0   simultaneous queries
	int		m_nMaxRetries;		// +4   retransmits per state
	double	m_dStateTimeout;	// +8   per-state timeout (0x45BC44 reads it as a qword)
	int		m_nPhaseMask;		// +16  which query phases are enabled
	int		m_reserved20;		// +20
	double	m_flOverallTimeout;	// +24  whole-pass timeout (defaults to 7.5)
	int		m_bReportErrors;	// +32  0x45BDC9 -- load IDS 0xE6 into m_szLastError
	int		m_reserved36;		// +36
};

class CRefreshDlg : public CDlgPopupBase
{
// Construction
public:
	CRefreshDlg( RefreshCriteria_t* pCriteria, CServerInfo* pServerList, CWnd* pParent );	// 0x45B1F0

// Dialog Data
	//{{AFX_DATA(CRefreshDlg)
	enum { IDD = IDD_REFRESH };		// 218
	CODStatic	m_odStatusLine;		// IDC 1183  per-server status line
	CODStatic	m_odPercent;		// IDC 1182  percent text
	CODBlendBtn	m_odCancel;			// IDCANCEL  skinned cancel button
	CODStatic	m_odTitle;			// IDC 1180  "Refreshing..."
	CODStatic	m_odBody;			// IDC 1181  body / help text
	//}}AFX_DATA

// Overrides
	//{{AFX_VIRTUAL(CRefreshDlg)
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );		// 0x45B350
	virtual BOOL	OnInitDialog();								// 0x45B700
	virtual int		RMLPreIdle();		// slot 55
	virtual void	OnCancel();			// slot 50
	virtual void	RMLIdle();			// slot 56
	virtual void	RMLPump();			// slot 58
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	UpdateStatus( float flSeconds, const char* pszFmt, ... );	// 0x45B520
	void	ResetCounters();										// 0x45BAB0
	void	PollGameInfo();											// 0x45B6B0 (throttled GetGameInfo)
	// CServerBrowser-coupled ping machinery (forward-deps on the server-browser
	// query verbs sub_4621B0/461F30.. ; declared here, driven from RMLPreIdle):
	void	StampServerRecords( CServerInfo* pHead );			// 0x45B3D0
	int		CountPingable( CServerInfo* pHead );				// 0x45B470
	BOOL	Pump();												// 0x45BB60  -> TRUE while running
	void	DrivePass();

	//{{AFX_MSG(CRefreshDlg)  -- map @0x4B2FD0
	afx_msg void	OnPaint();									// 0x4113F0
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );					// 0x4112E0
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// - members (field-set faithful; the binary lays them past the CDlgBase base) ---
	RefreshCriteria_t*	m_pCriteria;		// +1084  timeouts/retry caps
	CServerInfo*		m_pServerList;		// +728   head of the engine server records
	HGLOBAL				m_nButtonStrips;	// +732  btns_main strip DIB (HeaderLoaded)
	int					m_nStripCount;		// +736  strip rows (HeaderStride)
	int					m_szButtonStrip[2];	// +740  strip cell width/height
	int					m_nConsidered;		// +748  servers visited this pass
	int					m_nPingable;		// initial pingable count
	int					m_nDone;			// servers completed (-> percent)
	int					m_nInfoRequests;	// detailed-info queries outstanding
	int					m_nRetriesPhase[4];	// per-phase retransmit counters
	double				m_flStartTime;		// pass start (Sys_FloatTime)
	double				m_flLastUpdate;		// last UI refresh time
	char				m_szLastError[256];	// last server message
};

#endif // REFRESH_DLG_H
