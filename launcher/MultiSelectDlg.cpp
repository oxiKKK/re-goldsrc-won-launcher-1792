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
// Purpose: the modal multi-select page (CMultiSelectDlg, IDD 220).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The hub's reentrancy/refresh latch.  Also read by the browser pages, which
// arm it when they hand control back.
int	g_bHubNeedsRefresh;

// Entries at 0x4B0110, base map 0x4B4398 = CDialog.
BEGIN_MESSAGE_MAP( CMultiSelectDlg, CDialog )
	//{{AFX_MSG_MAP(CMultiSelectDlg)
	ON_BN_CLICKED( IDC_BTN_BROWSE,       OnBrowse )
	ON_BN_CLICKED( IDC_BTN_SPECTATE,     OnSpectateBtn )
	ON_BN_CLICKED( IDC_BTN_CHAT,         OnChat )
	ON_BN_CLICKED( IDC_BTN_CUSTOMIZE,    OnCustomize )
	ON_BN_CLICKED( IDC_BTN_LAN,          OnLan )
	ON_BN_CLICKED( IDC_BTN_QUICK,        OnQuick )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
	ON_BN_CLICKED( IDC_MULTI_DISCONNECT, OnDisconnect )
	ON_BN_CLICKED( IDC_MULTI_RESUME,     OnResume )
	ON_BN_CLICKED( IDC_BTN_WON,          OnReadme )
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED( IDC_BTN_CONTROLS,     OnControls )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::CMultiSelectDlg (0x430370)

CMultiSelectDlg::CMultiSelectDlg( CWnd* pParent )
	: CDlgBase( IDD_MULTISELECT, pParent )
{
	m_pSelfWnd = this;		// gates the slide transition
	InitMembers();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::InitMembers (0x430530)
//
// Every face is freed before it is re-sliced, so a re-entry after a skin
// change picks up the new strip rather than the old blend.

void CMultiSelectDlg::InitMembers()
{
	int	wh[2];

	LoadHeaderBitmap( "head_multi", NULL );

	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( wh );
	m_headerW      = wh[0];
	m_headerH      = wh[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		CSize	cell( m_headerW, m_headerH );

		m_btnQuick.FreeSkinBitmaps();
		m_btnQuick.SetDIBData( cell, BTNSTRIP_QUICK_START, m_headerLoaded );
		// (sic) Resume alone re-slices before it frees, so its first paint
		// after a skin change still uses the previous face.
		m_btnResume.SetDIBData( cell, BTNSTRIP_RESUME_GAME, m_headerLoaded );
		m_btnResume.FreeSkinBitmaps();
		m_btnDisconnect.FreeSkinBitmaps();
		m_btnDisconnect.SetDIBData( cell, BTNSTRIP_DISCONNECT, m_headerLoaded );
		m_btnBrowse.FreeSkinBitmaps();
		m_btnBrowse.SetDIBData( cell, BTNSTRIP_INTERNET_GAMES, m_headerLoaded );
		m_btnChat.FreeSkinBitmaps();
		m_btnChat.SetDIBData( cell, BTNSTRIP_CHAT_ROOMS, m_headerLoaded );
		m_btnLan.FreeSkinBitmaps();
		m_btnLan.SetDIBData( cell, BTNSTRIP_LAN_GAMES, m_headerLoaded );
		m_btnCustomize.FreeSkinBitmaps();
		m_btnCustomize.SetDIBData( cell, BTNSTRIP_CUSTOMIZE, m_headerLoaded );
		m_btnOK.FreeSkinBitmaps();
		m_btnOK.SetDIBData( cell, BTNSTRIP_DONE, m_headerLoaded );
		m_btnControls.FreeSkinBitmaps();
		m_btnControls.SetDIBData( cell, BTNSTRIP_CONTROLS, m_headerLoaded );
		m_btnSpectate.FreeSkinBitmaps();
		m_btnSpectate.SetDIBData( cell, BTNSTRIP_SPECTATE_WIDE, m_headerLoaded );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::DoDataExchange (0x4306D0)

void CMultiSelectDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_CFG_CONTROLHELP,     m_lblControlHelp );
	DDX_Control( pDX, IDC_BTN_CONTROLS,        m_btnControls );
	DDX_Control( pDX, IDC_MULTI_DONEHELP,      m_lblDoneHelp );
	DDX_Control( pDX, IDC_MULTI_RESUME,        m_btnResume );
	DDX_Control( pDX, IDC_MULTI_DISCONNECT,    m_btnDisconnect );
	DDX_Control( pDX, IDOK,                    m_btnOK );
	DDX_Control( pDX, IDC_MAIN_QUICKHELP,      m_lblQuickHelp );
	DDX_Control( pDX, IDC_MULTI_LAN,           m_lblLan );
	DDX_Control( pDX, IDC_MULTI_CUSTOMIZE,     m_lblCustomize );
	DDX_Control( pDX, IDC_MULTI_CHAT,          m_lblChat );
	DDX_Control( pDX, IDC_MULTI_BROWSE,        m_lblBrowse );
	DDX_Control( pDX, IDC_MULTI_SPECTATE,      m_lblSpectate );
	DDX_Control( pDX, IDC_MULTI_RESUMEHELP,    m_lblResumeHelp );
	DDX_Control( pDX, IDC_MULT_DISCONNECTHELP, m_lblDisconnectHelp );
	DDX_Control( pDX, IDC_BTN_QUICK,           m_btnQuick );
	DDX_Control( pDX, IDC_BTN_LAN,             m_btnLan );
	DDX_Control( pDX, IDC_BTN_CUSTOMIZE,       m_btnCustomize );
	DDX_Control( pDX, IDC_BTN_CHAT,            m_btnChat );
	DDX_Control( pDX, IDC_BTN_BROWSE,          m_btnBrowse );
	DDX_Control( pDX, IDC_BTN_SPECTATE,        m_btnSpectate );
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::Refresh (0x430860)

void CMultiSelectDlg::Refresh()
{
	InitMembers();
	RelayoutControls();
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
	ShowWindow( SW_SHOW );
	IN_HideMouse();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnBrowse (0x4308A0)

void CMultiSelectDlg::OnBrowse()
{
	CServerBrowserDlg	dlg( 1, NULL );		// internet mode

	InitChildDialog( &dlg, &m_btnBrowse );
	if ( dlg.Run() )
	{
		dlg.DoModal();
		Refresh();
	}
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnSpectateBtn (0x430940)

void CMultiSelectDlg::OnSpectateBtn()
{
	CSpecGameDlg	dlg( 1, NULL );

	InitChildDialog( &dlg, &m_btnSpectate );
	if ( dlg.Run() )
	{
		dlg.DoModal();
		Refresh();
	}
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnChat (0x4309E0)

void CMultiSelectDlg::OnChat()
{
	CServerBrowserDlg	dlg( 0, NULL );		// chat mode

	InitChildDialog( &dlg, &m_btnChat );
	if ( dlg.Run() )
	{
		dlg.DoModal();
		Refresh();
	}
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnCustomize (0x430A80)

void CMultiSelectDlg::OnCustomize()
{
	CPlayerProfileDlg	dlg( this );

	InitChildDialog( &dlg, &m_btnCustomize );
	dlg.DoModal();
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnLan (0x430B10)

void CMultiSelectDlg::OnLan()
{
	CLan	dlg( NULL );

	InitChildDialog( &dlg, &m_btnLan );
	dlg.DoModal();
	RestoreAfterModal();
	Refresh();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnQuick (0x430BA0)

void CMultiSelectDlg::OnQuick()
{
	g_bEnforceServerCap = 1;

	CNetGameDlg*	pBrowser = new CNetGameDlg( NULL, 0 );

	if ( g_bWonLoginRequired )
	{
		CLoginDlg	login( pBrowser, NULL );
		if ( login.DoModal() != IDOK )
		{
			delete pBrowser;
			g_bEnforceServerCap = 0;
			return;
		}
	}

	if ( !Launcher_ConnectAndLaunch( NULL, NULL ) )
	{
		VID_HideEngineWindow();
		Refresh();
		Launcher_HandleConnectFailure();
	}

	delete pBrowser;
	g_bEnforceServerCap = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnOK (0x430DF0)
//
// The "Done" path: drop any running game, unload the engine and close.

void CMultiSelectDlg::OnOK()
{
	if ( Eng_ShouldReload() )
	{
		CPromptDlg	prompt( 2, NULL );		// OK + Cancel

		prompt.SetMessage( Launcher_LoadString( IDS_MULTISELECT_EXITGAMEPROMPT ) );
		if ( prompt.DoModal() != IDOK )
			return;

		resumeOnSwitch = 0;		// do not resume a CD track for the menu map
		if ( engineapi.Cbuf_AddText )
			engineapi.Cbuf_AddText( "disconnect\n" );
		Eng_Frame( 0 );
	}

	gBackground   = 0;
	Eng_Load( 0, 0 );
	gDLLState     = 0;
	gDLLStateInfo = 0;
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::~CMultiSelectDlg (0x431060)

CMultiSelectDlg::~CMultiSelectDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnActivateApp (0x4311F0)

void CMultiSelectDlg::OnActivateApp( BOOL bActive, DWORD /*dwThreadID*/ )
{
	Default();
	ActiveApp = bActive;
	if ( bActive && gDLLState == DLL_ACTIVE && gEngineVidType == VT_Direct3D )
		AppActivate( bActive, 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::RelayoutControls (0x431230)
//
// Eight rows, 32px apart from y=140: the top one is Quick start, or Resume
// plus Disconnect while a game is running.

void CMultiSelectDlg::RelayoutControls()
{
	int			wh[2];
	GameInfo_t	gi;

	Launcher_HeaderSize( wh );
	Launcher_HeaderSize( wh );

	int	nWidth  = wh[0];
	int	nHeight = wh[1];
	if ( Launcher_StringHeight( IDS_SPANISH, 0 ) )
		nHeight += 10;

	int	nLabelX     = nWidth + 60;
	int	nLabelRight = g_nLauncherDefW - 10;

	int	bInGame = 0;
	if ( engineapi.GetGameInfo( &gi, 0 ) && gi.state == ca_active )
		bInGame = ( gi.signon != 0 );

	::LockWindowUpdate( m_hWnd );

	int	nLabelW = nLabelRight - nLabelX;
	int	nBtnTop = 140;

	if ( bInGame )
	{
		m_btnResume.ShowWindow( SW_SHOW );
		m_lblResumeHelp.ShowWindow( SW_SHOW );
		m_btnDisconnect.ShowWindow( SW_SHOW );
		m_lblDisconnectHelp.ShowWindow( SW_SHOW );
		m_btnQuick.ShowWindow( SW_HIDE );
		m_lblQuickHelp.ShowWindow( SW_HIDE );

		m_btnResume.MoveWindow( 50, 140, nWidth, nHeight, TRUE );
		m_lblResumeHelp.MoveWindow( nLabelX, 146, nLabelW, nHeight - 6, TRUE );
		nBtnTop = 172;
		m_btnDisconnect.MoveWindow( 50, 172, nWidth, nHeight, TRUE );
		m_lblDisconnectHelp.MoveWindow( nLabelX, 178, nLabelW, nHeight - 6, TRUE );
	}
	else
	{
		m_btnResume.ShowWindow( SW_HIDE );
		m_lblResumeHelp.ShowWindow( SW_HIDE );
		m_btnDisconnect.ShowWindow( SW_HIDE );
		m_lblDisconnectHelp.ShowWindow( SW_HIDE );
		m_btnQuick.ShowWindow( SW_SHOW );
		m_lblQuickHelp.ShowWindow( SW_SHOW );

		m_btnQuick.MoveWindow( 50, 140, nWidth, nHeight, TRUE );
		m_lblQuickHelp.MoveWindow( nLabelX, 146, nLabelW, nHeight - 6, TRUE );
	}

	// The browse and spectate blurbs are the two that wrap, so they carry the
	// locale's extra lines.
	int	y = nBtnTop + 32;

	m_btnBrowse.MoveWindow( 50, y, nWidth, nHeight, TRUE );
	int	nLines = Launcher_StringHeight( IDS_GERMAN, 0 );
	m_lblBrowse.MoveWindow( nLabelX, y + 6, nLabelW, nHeight + y + 7 * nLines - ( y + 6 ), TRUE );

	y += 32;
	m_btnSpectate.MoveWindow( 50, y, nWidth, nHeight, TRUE );
	nLines = Launcher_StringHeight( IDS_GERMAN, 0 );
	m_lblSpectate.MoveWindow( nLabelX, y + 6, nLabelW, nHeight + y + 7 * nLines - ( y + 6 ), TRUE );

	y += 32;
	m_btnChat.MoveWindow( 50, y, nWidth, nHeight, TRUE );
	m_lblChat.MoveWindow( nLabelX, y + 6, nLabelW, nHeight - 6, TRUE );

	y += 32;
	m_btnLan.MoveWindow( 50, y, nWidth, nHeight, TRUE );
	m_lblLan.MoveWindow( nLabelX, y + 6, nLabelW, nHeight - 6, TRUE );

	y += 32;
	m_btnCustomize.MoveWindow( 50, y, nWidth, nHeight, TRUE );
	m_lblCustomize.MoveWindow( nLabelX, y + 6, nLabelW, nHeight - 6, TRUE );

	y += 32;
	m_btnControls.MoveWindow( 50, y, nWidth, nHeight, TRUE );
	m_lblControlHelp.MoveWindow( nLabelX, y + 6, nLabelW, nHeight - 6, TRUE );

	y += 32;
	int	nBottom = y + nHeight;
	m_btnOK.MoveWindow( 50, y, nWidth, nBottom - y, TRUE );
	m_lblDoneHelp.MoveWindow( nLabelX, y + 6, nLabelW, nBottom - ( y + 6 ), TRUE );

	InitMembers();
	::LockWindowUpdate( NULL );
	::InvalidateRect( m_hWnd, NULL, TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnInitDialog (0x431640)

BOOL CMultiSelectDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	// Quick start
	m_btnQuick.SetWindowText( Launcher_LoadString( IDS_BTN_QUICK ) );
	m_lblQuickHelp.SetTransparent( TRUE );
	m_lblQuickHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblQuickHelp.SetFontSize( 11, FW_NORMAL );
	m_lblQuickHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_QUICKHELP ) );

	// Resume
	m_btnResume.SetWindowText( Launcher_LoadString( IDS_BTN_RESUME ) );
	m_lblResumeHelp.SetTransparent( TRUE );
	m_lblResumeHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblResumeHelp.SetFontSize( 11, FW_NORMAL );
	m_lblResumeHelp.SetWindowText( Launcher_LoadString( IDS_MULTI_RESUMEHELP ) );

	// Disconnect
	m_btnDisconnect.SetWindowText( Launcher_LoadString( IDS_BTN_DISCONNECT ) );
	m_lblDisconnectHelp.SetTransparent( TRUE );
	m_lblDisconnectHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblDisconnectHelp.SetFontSize( 11, FW_NORMAL );
	m_lblDisconnectHelp.SetWindowText( Launcher_LoadString( IDS_MULTI_DISCONNECTHELP ) );

	// Internet games
	m_btnBrowse.SetWindowText( Launcher_LoadString( IDS_BTN_BROWSE ) );
	m_lblBrowse.SetTransparent( TRUE );
	m_lblBrowse.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblBrowse.SetFontSize( 11, FW_NORMAL );
	m_lblBrowse.SetWindowText( Launcher_LoadString( IDS_MULTI_BROWSEHELP ) );

	// Spectate
	m_btnSpectate.SetWindowText( Launcher_LoadString( IDS_BTN_SPECTATE ) );
	m_lblSpectate.SetTransparent( TRUE );
	m_lblSpectate.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblSpectate.SetFontSize( 11, FW_NORMAL );
	m_lblSpectate.SetWindowText( Launcher_LoadString( IDS_SPECTATE_HELP ) );

	// Chat rooms
	m_btnChat.SetWindowText( Launcher_LoadString( IDS_BTN_CHAT ) );
	m_lblChat.SetTransparent( TRUE );
	m_lblChat.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblChat.SetFontSize( 11, FW_NORMAL );
	m_lblChat.SetWindowText( Launcher_LoadString( IDS_MULTI_CHATHELP ) );

	// LAN games
	m_btnLan.SetWindowText( Launcher_LoadString( IDS_BTN_LAN ) );
	m_lblLan.SetTransparent( TRUE );
	m_lblLan.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblLan.SetFontSize( 11, FW_NORMAL );
	m_lblLan.SetWindowText( Launcher_LoadString( IDS_MULTI_LANHELP ) );

	// Customize
	m_btnCustomize.SetWindowText( Launcher_LoadString( IDS_BTN_CUSTOMIZE ) );
	m_lblCustomize.SetTransparent( TRUE );
	m_lblCustomize.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblCustomize.SetFontSize( 11, FW_NORMAL );
	m_lblCustomize.SetWindowText( Launcher_LoadString( IDS_MULTI_CUSTOMIZEHELP ) );

	// Controls
	m_btnControls.SetWindowText( Launcher_LoadString( IDS_BTN_CONTROLS ) );
	m_lblControlHelp.SetTransparent( TRUE );
	m_lblControlHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblControlHelp.SetFontSize( 11, FW_NORMAL );
	m_lblControlHelp.SetWindowText( Launcher_LoadString( IDS_CFG_CONTROLHELP ) );

	// Done
	m_btnOK.SetWindowText( Launcher_LoadString( IDS_BTN_DONE ) );
	m_lblDoneHelp.SetTransparent( TRUE );
	m_lblDoneHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblDoneHelp.SetFontSize( 11, FW_NORMAL );
	m_lblDoneHelp.SetWindowText( Launcher_LoadString( IDS_MULTI_DONEHELP ) );

	RelayoutControls();
	ShowWindow( SW_RESTORE );
	::UpdateWindow( m_hWnd );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::RMLPreIdle (0x431A40)

int CMultiSelectDlg::RMLPreIdle()
{
	Launcher_SyncEngineWindow( this );

	if ( Eng_Frame( gBackground ) && !gBackground )
	{
		// The engine ran a frame in the foreground: arm a refresh for when
		// focus returns, then idle this pass.
		if ( ActiveApp )
		{
			g_bHubNeedsRefresh = 1;
			return 1;
		}
		return 0;
	}

	if ( ActiveApp )
	{
		if ( g_bHubNeedsRefresh )
		{
			::InvalidateRect( m_hWnd, NULL, TRUE );
			RelayoutControls();
			::SetActiveWindow( m_hWnd );
			SetForegroundWindow();
			ShowWindow( SW_SHOWNORMAL );
			::ShowWindow( mainwindow, SW_HIDE );
			IN_HideMouse();
			g_bHubNeedsRefresh = 0;
			if ( gDLLState == DLL_ACTIVE || gDLLState == DLL_PAUSED )
				gBackground = 1;
		}
		if ( Launcher_GetRestartFlag() )
			OnOK();
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnDisconnect (0x431B20)

void CMultiSelectDlg::OnDisconnect()
{
	int			bInGame = 0;
	GameInfo_t	gi;

	if ( engineapi.GetGameInfo( &gi, 0 ) && gi.state == ca_active )
		bInGame = ( gi.signon != 0 );

	gBackground = 1;
	if ( bInGame )
	{
		resumeOnSwitch = 0;		// do not resume a CD track for the menu map
		engineapi.Cbuf_AddText( "disconnect\n" );
		Eng_Frame( 1 );
	}
	gBackground = 0;
	RelayoutControls();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnResume (0x431BA0)

void CMultiSelectDlg::OnResume()
{
	int			bInGame = 0;
	GameInfo_t	gi;

	if ( engineapi.GetGameInfo( &gi, 0 ) && gi.state == ca_active )
		bInGame = ( gi.signon != 0 );

	if ( !bInGame )
	{
		RelayoutControls();
		return;
	}

	Launcher_StartEngine( 0 );
	gBackground = 0;
	Rate_ApplyFromConfig();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnReadme (0x431C00)

void CMultiSelectDlg::OnReadme()
{
	char	szFile[MAX_PATH];

	if ( Launcher_LoadStringInto( szFile, IDS_WON_URL ) )
	{
		if ( (INT_PTR)::ShellExecuteA( NULL, "open", szFile, NULL, NULL, SW_SHOWNORMAL )
			<= (INT_PTR)HINSTANCE_ERROR )
			Launcher_ShowMessageByIdEx( NULL, IDS_URL_BROWSERFAIL, szFile );
	}
	else
	{
		::GetLastError();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnShowWindow (0x431C60)

void CMultiSelectDlg::OnShowWindow( BOOL /*bShow*/, UINT /*nStatus*/ )
{
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnControls (0x431C70)

void CMultiSelectDlg::OnControls()
{
	ShowWindow( SW_RESTORE );

	CKeyboardDlg	dlg( NULL );

	InitChildDialog( &dlg, &m_btnControls );
	dlg.DoModal();
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnPaint (0x412860)

void CMultiSelectDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectDlg::OnEraseBkgnd (0x412870)

BOOL CMultiSelectDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}
