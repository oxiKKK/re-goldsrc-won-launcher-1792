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
// Purpose: CRefreshDlg, the server-refresh progress dialog.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Pass-wide timestamps shared with the async socket / ResetCounters.
static double	s_flPumpStamp;			// dbl_4F8C20 -- this pump call's entry time
static double	s_flPollStamp;			// unk_4F8C28 -- last GetGameInfo poll

BEGIN_MESSAGE_MAP( CRefreshDlg, CDialog )
	//{{AFX_MSG_MAP(CRefreshDlg)  -- map @0x4B2FD0
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CRefreshDlg::CRefreshDlg (0x45B1F0)
CRefreshDlg::CRefreshDlg( RefreshCriteria_t* pCriteria, CServerInfo* pServerList, CWnd* pParent )
	: CDlgPopupBase( CRefreshDlg::IDD, pParent )
{
	int	wh[2];

	m_pServerList    = pServerList;
	SetPaintWnd( this );			// 0x45B285 -- OnPaint binds its CPaintDC to this
	m_nConsidered    = 0;			// +0x2EC
	m_nDone          = 0;			// +0x320
	m_pCriteria      = pCriteria;

	// default the whole-pass timeout if the caller left it zero
	if ( m_pCriteria->m_flOverallTimeout == 0.0 )
		m_pCriteria->m_flOverallTimeout = 7.5;

	// size the skinned cancel button from the loaded button strip
	m_nButtonStrips = Launcher_HeaderLoaded();
	Launcher_HeaderSize( wh );
	m_szButtonStrip[0] = wh[0];
	m_szButtonStrip[1] = wh[1];
	m_nStripCount      = Launcher_HeaderStride();
	if ( m_nButtonStrips )
		m_odCancel.SetDIBData( CSize( m_szButtonStrip[0], m_szButtonStrip[1] ), BTNSTRIP_BACK, m_nButtonStrips );

	SetModalProgressPopup( 1 );		// 0x40BDE0
}

// CRefreshDlg::DoDataExchange (0x45B350)
void CRefreshDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_REFRESH_STATUS,      m_odStatusLine );
	DDX_Control( pDX, IDC_REFRESH_PERCENT,      m_odPercent );
	DDX_Control( pDX, IDCANCEL,  m_odCancel );
	DDX_Control( pDX, IDC_REFRESH_TITLE,      m_odTitle );
	DDX_Control( pDX, IDC_REFRESH_BODY,      m_odBody );
}

// CRefreshDlg::StampServerRecords (0x45B3D0)
//
// Put every queryable record back into the queued state for a fresh pass; what
// gets cleared depends on which phases this refresh is running.

void CRefreshDlg::StampServerRecords( CServerInfo* pHead )
{
	double	flNow = engineapi.Sys_FloatTime();

	for ( CServerInfo* p = pHead; p; p = p->m_pNext )
	{
		if ( p->m_bLan || p->GetFiltered() )
			continue;

		if ( p->m_pSocket )
			p->CloseSocket();

		if ( m_pCriteria->m_nPhaseMask & 4 )
			p->ClearPlayers();

		if ( m_pCriteria->m_nPhaseMask & 1 )
			p->m_dSvPing = 0.0;

		p->m_dSendTime   = flNow;
		p->m_bNoResponse = 0;
		p->m_nStatus     = SVQ_QUEUED;
		p->m_nRetry      = 0;
	}
}

// CRefreshDlg::CountPingable (0x45B470)
//
// Also re-stamps each record's send time, so the caller's count and the clock
// the pass measures against are taken together.

int CRefreshDlg::CountPingable( CServerInfo* pHead )
{
	int		n     = 0;
	double	flNow = engineapi.Sys_FloatTime();

	for ( CServerInfo* p = pHead; p; p = p->m_pNext )
	{
		if ( !p->m_bLan && !p->GetFiltered() )
		{
			p->m_dSendTime = flNow;
			++n;
		}
	}
	return n;
}

// CRefreshDlg::OnCancel (0x45B4D0)
void CRefreshDlg::OnCancel()
{
	CDialog::OnCancel();
}

// CRefreshDlg::RMLPreIdle (0x45B4F0, slot 55)
int CRefreshDlg::RMLPreIdle()
{
	DrivePass();
	OnEngineFrame();
	PollGameInfo();
	return 0;
}

// CRefreshDlg::UpdateStatus (0x45B520) -- flSeconds is elapsed pass time, not a
// percentage; the three labels are body / elapsed / servers-remaining.
void CRefreshDlg::UpdateStatus( float flSeconds, const char* pszFmt, ... )
{
	char	szText[1024];
	va_list	args;

	va_start( args, pszFmt );
	vsprintf( szText, pszFmt, args );
	va_end( args );

	if ( m_odBody.GetSafeHwnd() )
	{
		m_odBody.SetWindowText( szText );
		m_odBody.Invalidate( TRUE );
		m_odBody.UpdateWindow();
	}

	if ( m_odPercent.GetSafeHwnd() )
	{
		if ( flSeconds == 0.0f )
			sprintf( szText, "" );
		else
			sprintf( szText, Launcher_LoadString( IDS_MOD_TIME ), flSeconds );	// 0x206
		m_odPercent.SetWindowText( szText );
		m_odPercent.Invalidate( TRUE );
		m_odPercent.UpdateWindow();
	}

	if ( m_odStatusLine.GetSafeHwnd() )
	{
		if ( m_nConsidered <= 0 )
			strcpy( szText, "" );
		else
			sprintf( szText, Launcher_LoadString( IDS_REFRESH_SERVERS ), m_nConsidered );	// 0x228
		m_odStatusLine.SetWindowText( szText );
		m_odStatusLine.Invalidate( TRUE );
		m_odStatusLine.UpdateWindow();
	}
}

// CRefreshDlg::PollGameInfo (0x45B6B0)
void CRefreshDlg::PollGameInfo()
{
	double	t = engineapi.Sys_FloatTime();
	if ( t - s_flPollStamp >= 0.5 )
	{
		s_flPollStamp = t;
		GameInfo_t	gi;
		engineapi.GetGameInfo( &gi, 0 );
	}
}

// CRefreshDlg::OnInitDialog (0x45B700)
BOOL CRefreshDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 300x200, centred on the primary display (raw screen metrics, as the binary).
	MoveWindow( 0, 0, 300, 200, FALSE );
	CRect	rcWnd;
	GetWindowRect( &rcWnd );
#ifdef LAUNCHER_FIXES
	Dlg_CenterPopup( this, rcWnd.Width(), rcWnd.Height() );
#else
	MoveWindow( ( GetSystemMetrics( SM_CXSCREEN ) - rcWnd.Width() ) / 2,
				( GetSystemMetrics( SM_CYSCREEN ) - rcWnd.Height() ) / 2,
				rcWnd.Width(), rcWnd.Height(), TRUE );
#endif

	int	wh[2] = { 0, 0 };
	Launcher_HeaderSize( wh );
	int	cyButton = wh[1];

	CRect	rc;
	GetClientRect( &rc );
	int	cx = rc.Width();
	int	cy = rc.Height();

	// The labels are opaque CODStatics filling REFRESH_BG_COLOR; left transparent
	// they show the page skin behind the popup.
	m_odTitle.MoveWindow( 10, 10, cx - 20, 40, TRUE );
	m_odTitle.SetTextColor( Scheme_GetColor( "REFRESH_TITLE_COLOR" ) );
	m_odTitle.m_clrBgnd = Scheme_GetColor( "REFRESH_BG_COLOR" );
	m_odTitle.SetFontSize( 16, FW_HEAVY );
	m_odTitle.SetTransparent( 0 );
	m_odTitle.SetCentered( TRUE );
	m_odTitle.SetWindowText( Launcher_LoadString( IDS_MULTI_REFRESH ) );		// 0x1B4

	m_odBody.MoveWindow( 25, 60, cx - 50, cy - cyButton - 80, TRUE );
	m_odBody.SetTextColor( Scheme_GetColor( "REFRESH_TEXT_COLOR" ) );
	m_odBody.m_clrBgnd = Scheme_GetColor( "REFRESH_BG_COLOR" );
	m_odBody.SetFontSize( 14, FW_HEAVY );
	m_odBody.SetTransparent( 0 );
	m_odBody.SetCentered( TRUE );

	int	xCancel = cx - 110;
	int	yButton = cy - cyButton - 10;
	m_odCancel.MoveWindow( xCancel, yButton, 100, cyButton, TRUE );
	m_odCancel.SetTransparent( 0 );
	m_odCancel.m_bHasArrow = 0;
	m_odCancel.m_clrBg     = Scheme_GetColor( "REFRESH_BG_COLOR" );
	m_odCancel.m_clrDown   = RGB( 255, 180, 0 );		// 0xB4FF

	// Both status lines share the strip left of Cancel; the elapsed-time line sits
	// one row above the server counter.
	m_odPercent.MoveWindow( 10, yButton - 20, xCancel - 20, cyButton, TRUE );
	m_odPercent.SetTextColor( Scheme_GetColor( "REFRESH_TEXT_COLOR" ) );
	m_odPercent.m_clrBgnd = Scheme_GetColor( "REFRESH_BG_COLOR" );
	m_odPercent.SetFontSize( 12, FW_HEAVY );
	m_odPercent.SetTransparent( 0 );

	m_odStatusLine.MoveWindow( 10, yButton, xCancel - 20, cyButton, TRUE );
	m_odStatusLine.SetTextColor( Scheme_GetColor( "REFRESH_TEXT_COLOR" ) );
	m_odStatusLine.SetBgColor( Scheme_GetColor( "REFRESH_BG_COLOR" ) );
	m_odStatusLine.SetFontSize( 12, FW_HEAVY );
	m_odStatusLine.SetTransparent( 0 );

	ShowWindow( SW_RESTORE );
	UpdateWindow();

	UpdateStatus( 0.0f, "" );
	ResetCounters();
	return TRUE;
}

// CRefreshDlg::DrivePass (0x45BA90)
//
// The popup accepts itself once the pass stops, so a completed refresh reads as
// success to whoever ran it.

void CRefreshDlg::DrivePass()
{
	if ( !Pump() )
		OnOK();
}

// CRefreshDlg::ResetCounters (0x45BAB0)
void CRefreshDlg::ResetCounters()
{
	UpdateStatus( 0.0f, "" );

	m_nDone         = 0;
	m_nConsidered   = 0;
	m_nInfoRequests = 0;
	for ( int i = 0; i < 4; i++ )
		m_nRetriesPhase[i] = 0;

	StampServerRecords( m_pServerList );
	m_nPingable = CountPingable( m_pServerList );

	m_flStartTime  = engineapi.Sys_FloatTime();
	m_flLastUpdate = m_flStartTime;
	sprintf( m_szLastError, "" );

	g_flLastReceiveTime = (float)engineapi.Sys_FloatTime();		// pass-start activity stamp
}

// CRefreshDlg::Pump (0x45BB60)
BOOL CRefreshDlg::Pump()					// 0x45BB60
{
	CRefreshDlg*	pDlg = this;

	pDlg->m_nConsidered = 0;
	s_flPumpStamp = engineapi.Sys_FloatTime();

	for ( CServerInfo* p = pDlg->m_pServerList; p; p = p->m_pNext )
	{
		// Skip LAN, filtered and IPX records.  0x45BB94 tests [p+0x40] -- m_bLan --
		// not the socket at +452: skipping records that hold a socket would strand
		// every server the moment OpenConnection gave it one, so its slot would never
		// be released and the pass could never finish.
		if ( p->m_bLan || p->GetFiltered() || p->m_bIpx )
			goto next;

		{
			double	flNow = engineapi.Sys_FloatTime();

			// Throttled (>= 0.1s) status-line refresh.
			if ( flNow - pDlg->m_flLastUpdate >= 0.1 )
			{
				pDlg->m_flLastUpdate = flNow;
				pDlg->UpdateStatus( (float)( flNow - pDlg->m_flStartTime ), "%s", pDlg->m_szLastError );
			}

			int	nStatus = p->m_nStatus;
			if ( nStatus == SVQ_IDLE || nStatus == SVQ_DEAD )
			{
				// Idle / dead record: tear down any socket and decrement the
				// outstanding-connection count.
				if ( p->m_pSocket )
				{
					--pDlg->m_nInfoRequests;
					p->CloseSocket();
				}
				goto next;
			}

			++pDlg->m_nConsidered;

			double	flSince = flNow - p->m_dSendTime;
			if ( flSince >= pDlg->m_pCriteria->m_dStateTimeout )
			{
				// - per-state timeout: retransmit or expire ---
				int	nState = p->m_nStatus;
				if ( nState == SVQ_PING_SENT )
				{
					// pinging: one fewer ping outstanding
					if ( p->m_nNumPings-- < 0 )
					{
						// out of pings -> finalise stats, go ask for info
						p->ComputePingStats();
						p->m_dSendTime = engineapi.Sys_FloatTime();
						p->SendInfoRequest();
					}
					else
					{
						// drop this slot's sample, resend ping
						p->m_rgPing[p->m_nNumPings] = 0.0;
						p->m_dSendTime = engineapi.Sys_FloatTime();
						p->SendPingRequest();
					}
					goto next;
				}

				if ( p->m_nRetry++ >= pDlg->m_pCriteria->m_nMaxRetries )
				{
					// retries exhausted: before the info reply (< SVQ_INFO_DONE) the
					// row never responded; after it, mark the record dead.
					if ( nState < SVQ_INFO_DONE )
					{
						p->m_nStatus = SVQ_IDLE;
						p->m_bNoResponse = 1;
					}
					else
					{
						p->m_nStatus = SVQ_DEAD;
					}
					goto next;
				}

				// retransmit the current phase's request
				switch ( nState )
				{
				case SVQ_CONNECT_RETRY:
					p->m_dSendTime = engineapi.Sys_FloatTime();
					p->Connect();
					++pDlg->m_nRetriesPhase[0];
					break;
				case SVQ_INFO_SENT:
					p->m_dSendTime = engineapi.Sys_FloatTime();
					p->SendInfoRequest();
					++pDlg->m_nRetriesPhase[1];
					break;
				case SVQ_PLAYERS_SENT:
					p->m_dSendTime = engineapi.Sys_FloatTime();
					p->SendPlayersRequest();
					++pDlg->m_nRetriesPhase[2];
					break;
				case SVQ_RULES_SENT:
					p->m_dSendTime = engineapi.Sys_FloatTime();
					p->SendRulesRequest();
					++pDlg->m_nRetriesPhase[3];
					break;
				default:
					break;
				}
				goto next;
			}

			// - not timed out: drive each phase to the next ---
			if ( p->m_nStatus == SVQ_QUEUED )
			{
				// querying: cache the (first) server name into the status line,
				// then open a connection if we have a free slot.
				if ( pDlg->m_pCriteria->m_nPhaseMask && !strlen( pDlg->m_szLastError ) )
				{
					Launcher_LoadStringInto( pDlg->m_szLastError, 0xE6, (LPCTSTR)p->m_strName );
					pDlg->m_flLastUpdate = -1.0;		// force the next status refresh
				}

				if ( pDlg->m_nInfoRequests >= pDlg->m_pCriteria->m_nMaxOutstanding )
				{
					p->m_dSendTime = engineapi.Sys_FloatTime();
					goto next;
				}
				if ( !p->OpenConnection() )
					goto next;
				++pDlg->m_nInfoRequests;
				p->ResetRetry();
			}

			if ( p->m_nStatus == SVQ_SOCKET_OPEN )
			{
				if ( !p->Connect() )
					goto next;
				p->m_dSendTime = engineapi.Sys_FloatTime();
				p->ResetRetry();
			}

			if ( p->m_nStatus != SVQ_CONNECT_RETRY )
			{
				if ( p->m_nStatus == SVQ_CONNECTED )
				{
					// ping-init: clear the sample array, prime the countdown.
					memset( p->m_rgPing, 0, sizeof( double ) * g_nNumPings );
					p->m_nNumPings = g_nNumPings;
					if ( ( pDlg->m_pCriteria->m_nPhaseMask & 1 ) == 0 )
					{
						// Pinging disabled -> fall straight through to the info phase.
						// This must NOT "goto next": the record owns a socket from
						// OpenConnection above, and the top-of-loop test skips every
						// record that has one, so it would never be driven again and
						// would sit at PING_DONE forever (the browser list stayed
						// empty).  -1 samples makes the PING_DONE block below go
						// straight to ComputePingStats + SendInfoRequest.
						p->m_nStatus   = SVQ_PING_DONE;
						p->m_nNumPings = -1;
					}
					else
					{
					p->m_dSendTime = engineapi.Sys_FloatTime();
					p->SendPingRequest();
					}
				}

				if ( p->m_nStatus != SVQ_PING_SENT )
				{
					if ( p->m_nStatus == SVQ_PING_DONE )
					{
						// a ping reply arrived: stash the measured RTT (held in
						// m_dSvPing) into the next sample slot, resend until done.
						if ( p->m_nNumPings-- >= 0 )
						{
							p->m_rgPing[p->m_nNumPings] = p->m_dSvPing;
							p->m_dSvPing = 0.0;
							p->m_dSendTime = engineapi.Sys_FloatTime();
							p->SendPingRequest();
							goto next;
						}
						// all samples collected -> finalise + move to info phase
						p->ComputePingStats();
						if ( ( pDlg->m_pCriteria->m_nPhaseMask & 2 ) == 0 )
							p->m_nStatus = SVQ_INFO_DONE;		// info disabled -> skip
						p->m_dSendTime = engineapi.Sys_FloatTime();
						p->SendInfoRequest();
					}

					if ( p->m_nStatus != SVQ_INFO_SENT )
					{
						if ( p->m_nStatus == SVQ_INFO_DONE )
						{
							// info reply arrived: optionally begin the player query.
							if ( ( pDlg->m_pCriteria->m_nPhaseMask & 4 ) == 0 )
							{
								p->m_nStatus = SVQ_PLAYERS_DONE;	// players disabled -> skip
								goto next;
							}
							if ( !p->BeginPlayerQuery() )
							{
								p->m_nStatus = SVQ_PLAYERS_DONE;
								goto next;
							}
							p->SendPlayersRequest();
							p->m_dSendTime = engineapi.Sys_FloatTime();
							p->ResetRetry();
						}

						if ( p->m_nStatus != SVQ_PLAYERS_SENT )
						{
							if ( p->m_nStatus == SVQ_PLAYERS_DONE )
							{
								// players done: optionally ask for rules.
								if ( ( pDlg->m_pCriteria->m_nPhaseMask & 8 ) != 0 )
								{
									p->m_dSendTime = engineapi.Sys_FloatTime();
									p->SendRulesRequest();
								}
								else
								{
									p->m_nStatus = SVQ_RULES_DONE;	// rules disabled -> done
								}
							}

							if ( p->m_nStatus != SVQ_RULES_SENT )
							{
								// fully done -> mark dead and record the "complete"
								// status line.
								p->m_nStatus = SVQ_DEAD;
								Launcher_LoadStringInto( pDlg->m_szLastError, 0x7E, (LPCTSTR)p->m_strName );
							}
						}
					}
				}
			}
		}

next:
		;
	}

	// - pass continuation test ---
	pDlg->m_nDone = pDlg->m_nConsidered;
	if ( !pDlg->m_nConsidered )
		return FALSE;
	if ( pDlg->m_nConsidered >= pDlg->m_pCriteria->m_nMaxOutstanding )
		return TRUE;
	return engineapi.Sys_FloatTime() - g_flLastReceiveTime <= pDlg->m_pCriteria->m_flOverallTimeout;
}

// CRefreshDlg::RMLIdle (0x45B510) -- popup slot 56, folded with RMLPump
void CRefreshDlg::RMLIdle()
{
	OnEngineFrame();
}

// CRefreshDlg::RMLPump (0x45B510) -- popup slot 58, the same folded body
void CRefreshDlg::RMLPump()
{
	OnEngineFrame();
}

// CRefreshDlg::OnEraseBkgnd (0x4112E0)
BOOL CRefreshDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	CDlgPopupBase::OnPaint();
	return TRUE;
}

// CRefreshDlg::OnPaint (0x4113F0)
void CRefreshDlg::OnPaint()
{
	CDlgPopupBase::OnPaint();
}

