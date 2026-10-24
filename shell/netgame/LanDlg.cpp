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
// Purpose: CLan, the LAN games page.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"


// Set when the engine ran a frame in the foreground; the next idle pass
// relayouts the page and re-broadcasts. (4E2CB8)
static int	g_bLanPageDirty;

// Entries at 0x4AEF98, base map 0x4B4398 = CDialog.
BEGIN_MESSAGE_MAP( CLan, CDialog )
	//{{AFX_MSG_MAP(CLan)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED( 1200, OnConnect )
	ON_BN_CLICKED( 1203, OnRefresh )
	ON_BN_CLICKED( 1205, OnCreateGame )
	ON_BN_CLICKED( 137,  OnLeaveGame )
	ON_BN_CLICKED( 136,  OnSpectate )
	ON_BN_CLICKED( 1204, OnServerInfo )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CLan::CLan (0x420390)
CLan::CLan( CWnd* pParent )
	: CDlgConnectableBase( IDD, pParent )
{
	m_pSelfWnd     = this;
	m_bQuerying      = 0;
	m_pBrowser       = NULL;
	m_bProxySelected = 0;
	InitButtonStrips();
}

// CLan::DoDataExchange (0x420490)
void CLan::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_LAN_INFO, m_btnServerInfo );
	DDX_Control( pDX, IDC_MULTI_RESUME,  m_btnSpectate );
	DDX_Control( pDX, IDC_DLG223_IDC_BTN_DISCONNECT,  m_btnLeave );
	DDX_Control( pDX, IDOK,    m_btnOK );
	DDX_Control( pDX, IDC_LAN_STARTGAME, m_btnCreate );
	DDX_Control( pDX, IDC_LAN_REFRESH, m_btnRefresh );
	DDX_Control( pDX, IDC_LAN_JOINGAME, m_btnConnect );
}

// CLan::InitButtonStrips (0x420A40)
void CLan::InitButtonStrips()
{
	LoadHeaderBitmap( "head_lan", 0 );
	m_hStripDib    = Launcher_HeaderLoaded();
	Launcher_HeaderSize( m_stripWH );
	m_nStripStride = Launcher_HeaderStride();
	if ( m_hStripDib )
	{
		m_btnConnect.FreeSkinBitmaps();
		m_btnConnect.SetDIBData( CSize( m_stripWH[0], m_stripWH[1] ), BTNSTRIP_CONNECT, m_hStripDib );
		m_btnSpectate.SetDIBData( CSize( m_stripWH[0], m_stripWH[1] ), BTNSTRIP_RESUME_GAME, m_hStripDib );
		m_btnSpectate.FreeSkinBitmaps();
		m_btnLeave.FreeSkinBitmaps();
		m_btnLeave.SetDIBData( CSize( m_stripWH[0], m_stripWH[1] ), BTNSTRIP_DISCONNECT, m_hStripDib );
		m_btnServerInfo.FreeSkinBitmaps();
		m_btnServerInfo.SetDIBData( CSize( m_stripWH[0], m_stripWH[1] ), BTNSTRIP_SPECTATE, m_hStripDib );
		m_btnCreate.FreeSkinBitmaps();
		m_btnCreate.SetDIBData( CSize( m_stripWH[0], m_stripWH[1] ), BTNSTRIP_CREATE, m_hStripDib );
		m_btnRefresh.FreeSkinBitmaps();
		m_btnRefresh.SetDIBData( CSize( m_stripWH[0], m_stripWH[1] ), BTNSTRIP_REFRESH, m_hStripDib );
		m_btnOK.FreeSkinBitmaps();
		m_btnOK.SetDIBData( CSize( m_stripWH[0], m_stripWH[1] ), BTNSTRIP_DONE, m_hStripDib );
	}
}

// CLan::~CLan (0x420B80)
CLan::~CLan()
{
	delete m_pBrowser;
}

// CLan::OnInitDialog (0x420530)
BOOL CLan::OnInitDialog()
{
	odcolumn_t	col;
	char		szTitles[9][32];
	char		szBuf[128];
	int			widths[9];
	int			wh[2];
	RECT		rc;
	int			i;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	m_btnConnect.SetWindowText( Launcher_LoadString( IDS_BTN_JOIN ) );
	m_btnCreate.SetWindowText( Launcher_LoadString( IDS_BTN_CREATESV ) );
	m_btnRefresh.SetWindowText( Launcher_LoadString( IDS_BTN_REFRESH ) );
	m_btnSpectate.SetWindowText( Launcher_LoadString( IDS_BTN_RESUME ) );
	m_btnLeave.SetWindowText( Launcher_LoadString( IDS_BTN_DISCONNECT ) );
	m_btnOK.SetWindowText( Launcher_LoadString( IDS_BTN_DONE ) );
	m_btnServerInfo.SetWindowText( Launcher_LoadString( IDS_BTN_INFO ) );

	// The LAN page owns its sort order (the internet page passes 0 here).
	m_pServerList = new CODHLListCtrl( this, 1 );
	if ( !m_pServerList )
	{
		Launcher_ShowMessageById( 0, IDS_MULTI_NOSERVERLIST );
		return -1;
	}

	Launcher_HeaderSize( wh );
	rc.left   = wh[0] + 30;
	rc.top    = 140;
	rc.right  = g_nLauncherDefW - 10;
	rc.bottom = g_nLauncherDefH - 30;
	m_pServerList->Create( WS_CHILD | WS_VISIBLE | WS_TABSTOP, rc, this,
						   IDC_NET_SERVER_LIST );
	m_pServerList->SetRowHeight( 16 );
	m_pServerList->SetHeaderFont( 10, FW_NORMAL );

	// Nine report columns: four unlabelled glyph columns, then the text ones.
	sprintf( szBuf, "" );
	strcpy( szTitles[0], szBuf );
	sprintf( szBuf, "" );
	strcpy( szTitles[1], szBuf );
	sprintf( szBuf, "" );
	strcpy( szTitles[2], szBuf );
	sprintf( szBuf, "" );
	strcpy( szTitles[3], szBuf );
	Launcher_LoadStringInto( szBuf, IDS_SERVER_GAMESERVER );
	strcpy( szTitles[4], szBuf );
	Launcher_LoadStringInto( szBuf, IDS_SERVER_SPEED );
	strcpy( szTitles[5], szBuf );
	Launcher_LoadStringInto( szBuf, IDS_SERVER_MAP );
	strcpy( szTitles[6], szBuf );
	Launcher_LoadStringInto( szBuf, IDS_SERVER_GAME );
	strcpy( szTitles[7], szBuf );
	Launcher_LoadStringInto( szBuf, IDS_SERVER_PLAYERS );
	strcpy( szTitles[8], szBuf );

	// The five text columns take their widths from the localised offset string.
	widths[0] = 16;
	widths[1] = 16;
	widths[2] = 16;
	widths[3] = 16;
	widths[4] = Launcher_StringHeight( IDS_LAN_OFFSET, 0 );
	widths[5] = Launcher_StringHeight( IDS_LAN_OFFSET, 1 );
	widths[6] = Launcher_StringHeight( IDS_LAN_OFFSET, 2 );
	widths[7] = Launcher_StringHeight( IDS_LAN_OFFSET, 3 );
	widths[8] = Launcher_StringHeight( IDS_LAN_OFFSET, 4 );

	m_pServerList->SetSortKey( "LAN Sort Order" );

	for ( i = 0; i < 9; i++ )
	{
		sprintf( col.title, szTitles[i] );
		col.width = widths[i];
		m_pServerList->AddColumn( &col );
	}

	// The latch tells the CNetGameDlg ctor it was opened from a games page.
	g_bNetGameSheetOpened = 1;
	m_pBrowser = new CNetGameDlg( (CServerBrowser*)1, 1 );
	g_bNetGameSheetOpened = 0;

	UpdateLayout();
	ShowWindow( SW_SHOW );
	::UpdateWindow( m_hWnd );
	OnRefresh();
	return TRUE;
}

// CLan::RMLPreIdle (0x420C50, frame-protocol slot 56)
int CLan::RMLPreIdle()
{
	// The connect button becomes "spectate" while a proxy row is selected.
	CServerInfo*	pSel = m_pServerList->GetSelectedServer();
	if ( pSel )
	{
		BYTE	bProxy = pSel->m_bProxy;
		if ( bProxy != m_bProxySelected )
		{
			m_bProxySelected = bProxy;
			m_btnConnect.SetDIBData( CSize( m_stripWH[0], m_stripWH[1] ),
				bProxy ? 69 : BTNSTRIP_CONNECT, m_hStripDib );	// 69: the proxy cell
			::InvalidateRect( m_btnConnect.m_hWnd, NULL, TRUE );
		}
	}

	Launcher_SyncEngineWindow( this );

	if ( Eng_Frame( gBackground ) && !gBackground )
	{
		g_bLanPageDirty = 1;
		return 1;
	}

	if ( m_bQuerying )
	{
		double	flNow = engineapi.Sys_FloatTime();
		m_flQueryTick = flNow;

		// The broadcast gets two seconds to collect replies; then the refresh
		// dialog pings whatever answered.
		if ( flNow - m_flQueryStart > 2.0 )
		{
			m_bQuerying = 0;
			m_pBrowser->StopLanQuery();

			RefreshCriteria_t	crit;
			memset( &crit, 0, sizeof( crit ) );
			crit.m_nMaxOutstanding = m_pBrowser->m_nMaxSockets;
			crit.m_nMaxRetries     = m_pBrowser->m_nRetries;
			crit.m_dStateTimeout   = m_pBrowser->m_dTimeout;
			crit.m_nPhaseMask      = 2;

			CRefreshDlg	dlg( &crit, m_pBrowser->m_pServerListHead, NULL );
			dlg.DoModal();
			PopulateList();
		}
	}

	if ( g_bLanPageDirty )
	{
		Relayout();
		g_bLanPageDirty = 0;
		if ( gDLLState == DLL_ACTIVE || gDLLState == DLL_PAUSED )
			gBackground = 1;
		OnRefresh();
	}

	if ( Launcher_GetRestartFlag() )
		OnOK();

	return 0;
}

// CLan::Relayout (0x420EB0, slot 62)
void CLan::Relayout()
{
	InitButtonStrips();		UpdateLayout();
	SetActiveWindow();
	SetFocus();
	ShowWindow( SW_SHOWNORMAL );
	::InvalidateRect( m_hWnd, NULL, TRUE );
}

// CLan::PopulateList (0x420F00)
void CLan::PopulateList()
{
	if ( !m_pServerList )
		return;

	m_pServerList->ResetContent();

	int	iRow = 0;
	for ( CServerInfo* p = m_pBrowser->m_pServerListHead; p; p = p->m_pNext )
	{
		if ( !p->GetFiltered() )
			m_pServerList->InsertRecord( p, iRow++ );
	}

	::InvalidateRect( m_pServerList->m_hWnd, NULL, TRUE );
	m_pServerList->SortRows( (odrowcmp_t)ServerBrowser_CompareInfo, -1 );
	m_pServerList->SelectItem( 0, 1 );
	m_pServerList->RefitScrollbar();
	m_pServerList->UpdateScrollbar( 1 );
}

// CLan::OnConnect (0x420FB0)
void CLan::OnConnect()
{
	if ( !m_pServerList )
		return;

	int	nSel = m_pServerList->GetCurSel();
	if ( nSel == -1 )
		return;

	CServerInfo*	pInfo = (CServerInfo*)m_pServerList->GetItemData( nSel );
	if ( pInfo )
		ConnectToSelectedServer( m_pBrowser, pInfo );
}

// CLan::OnRefresh (0x420FF0)
void CLan::OnRefresh()
{
	if ( !m_pBrowser )
		return;

	// Empty the visible list before the fresh enumeration repopulates it.
	if ( m_pServerList )
		m_pServerList->ResetContent();			// 0x44B9D0

	m_pBrowser->ClearServers( 0 );
	m_pBrowser->StartLanQuery( 1 );

	m_flQueryTick = m_flQueryStart = engineapi.Sys_FloatTime();
	m_bQuerying   = 1;
}

// CLan::OnCreateGame (0x421050)
void CLan::OnCreateGame()
{
	if ( !m_pBrowser )
		return;

	g_bResumeOnSwitch = FALSE;
	if ( engineapi.Cbuf_AddText )
		engineapi.Cbuf_AddText( "disconnect\n" );

	CCreateServerDlg	dlg( m_pBrowser, this );

	InitChildDialog( &dlg, &m_btnCreate );

	int	nResult = dlg.DoModal();

	RestoreAfterModal();

	if ( nResult == IDOK )
	{
		if ( dlg.m_bDedicated )
		{
			// A dedicated server is another process; this one goes away.
			char	szCmd[1024];

			sprintf( szCmd, "hlds.exe +sv_lan 1 +maxplayers %i +sv_password \"%s\" +hostname \"%s\" +map %s",
				dlg.m_nMaxPlayers, dlg.m_szPassword, dlg.m_szHostName, dlg.m_szMap );
			if ( g_pCurrentMod && g_pCurrentMod != g_pValveMod )
			{
				strcat( szCmd, " -game " );
				strcat( szCmd, g_pCurrentMod->GetKeyString( "gamedir" ) );
			}
			Eng_Shutdown();
			WinExec( szCmd, SW_RESTORE );
			PostQuitMessage( 0 );
			return;
		}

		char	szCbuf[1024];

		sprintf( szCbuf, "disconnect\nsv_lan 1\nsetmaster disable\nmaxplayers %i\nsv_password \"%s\"\nhostname \"%s\"\nmap %s\n",
			dlg.m_nMaxPlayers, dlg.m_szPassword, dlg.m_szHostName, dlg.m_szMap );
		if ( engineapi.Cbuf_AddText )
			engineapi.Cbuf_AddText( szCbuf );
		Eng_Frame( 1 );

		if ( !Launcher_StartEngine( 0 ) )
		{
			gDLLState = 0;
			Eng_Load( 0, 0 );
			VID_HideEngineWindow();
			return;			// the failure path skips the repaint
		}

		if ( engineapi.Cbuf_AddText )
			engineapi.Cbuf_AddText( szCbuf );
	}

	// Cancelling and a started engine both land here.
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

// CLan::OnLeaveGame (0x4212A0)
void CLan::OnLeaveGame()
{
	int			bInGame = 0;
	GameInfo_t	gi;

	if ( engineapi.GetGameInfo( &gi, 0 ) && gi.state == ca_active && gi.active )
		bInGame = 1;

	gBackground = 1;
	if ( bInGame )
	{
		g_bResumeOnSwitch = FALSE;
		engineapi.Cbuf_AddText( "disconnect\n" );
		Eng_Frame( 1 );
	}
	gBackground = 0;
	UpdateLayout();
}

// CLan::OnSpectate (0x421320)
void CLan::OnSpectate()
{
	int		bInGame = 0;
	GameInfo_t	gi;
	if ( engineapi.GetGameInfo( &gi, 0 ) && gi.state == ca_active )
		bInGame = ( gi.signon != 0 );

	if ( !bInGame )
	{
		UpdateLayout();
		return;
	}

	Launcher_StartEngine( 0 );

	::ShowWindow( ::GetParent( m_hWnd ), SW_HIDE );
	::ShowWindow( gLauncherWnd, SW_HIDE );
	gBackground = 0;

	Rate_ApplyFromConfig();
}

// CLan::OnOK (0x4213A0)
void CLan::OnOK()
{
	::ShowWindow( ::GetParent( m_hWnd ), SW_SHOW );
	CDialog::OnOK();
}

// CLan::UpdateLayout (0x4213C0)
void CLan::UpdateLayout()
{
	int		nLeft   = 30;
	int		wh[2];
	Launcher_HeaderSize( wh );
	int		nRight  = wh[0] - 40;
	int		nHeight = wh[1];
	int		nListLeft = wh[0] + 40;

	int		nBtnTop = 140;
	int		bInfoWide = Launcher_StringHeight( IDS_FRENCH, 0 );

	char*	pszLang = Launcher_LoadString( IDS_LANGUAGE );
	int		bItalian = ( _strcmpi( pszLang, "Italiano" ) == 0 );

	int		nColRight;
	if ( bItalian )
	{
		nColRight = nRight + 34;
		nLeft     = 20;
	}
	else
	{
		nColRight = nRight + 10;
	}

	int		bInGame = 0;
	GameInfo_t	gi;
	if ( engineapi.GetGameInfo( &gi, 0 ) && gi.state == ca_active )
		bInGame = ( gi.signon != 0 );

	::LockWindowUpdate( m_hWnd );
	int		nWidth = nColRight;
	if ( bInGame )
	{
		m_btnSpectate.ShowWindow( SW_SHOW );
		m_btnLeave.ShowWindow( SW_SHOW );
		m_btnConnect.ShowWindow( SW_HIDE );
		m_btnSpectate.MoveWindow( nLeft, 140, nWidth, nHeight, TRUE );
		nBtnTop = 172;
		m_btnLeave.MoveWindow( nLeft, 172, nWidth, nHeight, TRUE );
	}
	else
	{
		m_btnSpectate.ShowWindow( SW_HIDE );
		m_btnLeave.ShowWindow( SW_HIDE );
		m_btnConnect.ShowWindow( SW_SHOW );
		m_btnConnect.MoveWindow( nLeft, 140, nWidth, nHeight, TRUE );
	}

	int		y = nBtnTop + 32;
	m_btnCreate.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
	y += 32;

	// Only this button gets the extra 15px, and only where the caption fits.
	int		bInfoPad = 1;
	if ( bItalian || bInfoWide )
		bInfoPad = 0;
	m_btnServerInfo.MoveWindow( nLeft, y, nWidth + 15 * bInfoPad, nHeight, TRUE );
	y += 32;
	m_btnRefresh.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
	m_btnOK.MoveWindow( nLeft, y + 32, nWidth, nHeight, TRUE );

	if ( bInfoWide )
		nListLeft -= 30;
	else if ( bItalian )
		nListLeft -= 20;
	if ( m_pServerList )
		m_pServerList->MoveWindow( nListLeft, 125, g_nLauncherDefW - 10 - nListLeft,
			g_nLauncherDefH - 155, TRUE );

	::LockWindowUpdate( NULL );

	InitButtonStrips();

	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

// CLan::OnServerInfo (0x421660)
void CLan::OnServerInfo()
{
	if ( !m_pServerList )
		return;

	CServerInfo*	pInfo = m_pServerList->GetSelectedServer();
	if ( !pInfo )
		return;

	CWaitCursor	wait;

	// Re-query just this server, then show its detail page.
	RefreshCriteria_t	criteria;
	memset( &criteria, 0, sizeof( criteria ) );
	criteria.m_nMaxOutstanding = m_pBrowser->m_nMaxSockets;
	criteria.m_nMaxRetries     = m_pBrowser->m_nRetries;
	criteria.m_dStateTimeout   = m_pBrowser->m_dTimeout;
	criteria.m_nPhaseMask      = 14;			// info + players + rules

	// Unlink the record so the refresh pass sees it alone.
	CServerInfo*	pSavedNext = pInfo->m_pNext;

	pInfo->m_pNext = NULL;

	CRefreshDlg	refresh( &criteria, pInfo, NULL );
	refresh.DoModal();

	pInfo->m_pNext = pSavedNext;

	CPlayerInfoDlg	info( this, pInfo );
	InitChildDialog( &info, &m_btnServerInfo );
	info.DoModal();
	RestoreAfterModal();

	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

// CLan::OnConnectAbort (0x421840) -- CDlgConnectableBase slot 63
void CLan::OnConnectAbort()
{
	OnRefresh();
}

// CLan::OnPaint (0x412860)
void CLan::OnPaint()
{
	PaintSkinnedDialog();
}

// CLan::OnEraseBkgnd (0x412870)
BOOL CLan::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

