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
// Purpose: CHLMainDlg, the main-menu dialog (IDD_MAIN).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The logo animation control is driven with raw ACM_* messages; GetSafeHwnd
// yields NULL -- a harmless no-op SendMessage -- when its window isn't up.
#ifndef ACM_OPENA
#define ACM_OPENA	(WM_USER + 100)
#define ACM_PLAY	(WM_USER + 101)
#endif

// Command the launcher posts to the menu to flush a queued error message
// (WM_COMMAND 121); handled by OnReportError.
#define ID_LAUNCHER_REPORTERROR		121
#define IDI_LAUNCHER				138

HHOOK	g_hKeyboardHook = NULL;
int		g_bConsoleMode  = 0;
int		g_bNoPrompt     = 0;
int		g_bNoFly        = 0;
DWORD	WindowStyle = 0;	// (4E2148) engine-window style (AdjustWindowRectEx)
static int		g_bPageRestart = 0;	// (4E215C) engine asked to restart after the page
static int		g_bInGameCached;	// (4E1B3C) cached in-game state (InGame fallback)

// the main version
static int		g_nLauncherVersion = LAUNCHER_VERSION;	// (4CF8AC) build/version number shown in the corner

// Deferred menu actions requested by the engine during a frame, serviced on the
// next tick.  Set only while a game is live.
static int	g_bRelayout1Pending = 0;	// (4E2164) DLL_INFO_RELAYOUT_1 (Eng_DeferRelayout1)
static int	g_bRelayout2Pending = 0;	// (4E2168) DLL_INFO_RELAYOUT_2 (Eng_DeferRelayout2)
static int	g_bOpenManualPending = 0;	// (4E216C) Eng_DeferOpenManual
static int	g_bMenuButtonPending = 0;	// (4E2144) Eng_DeferMenuButton (replay a menu button)
static int	g_bMenuLoopReady = 0;	// (4E2160) the deferred loop has run at least once
static int	g_bEngineTopmost = 0;	// (4CF254)

/*
==================
Eng_DeferRelayout1 (0x415D70)
==================
*/
void Eng_DeferRelayout1( int a )
{
	g_bRelayout1Pending = a;
}

/*
==================
Eng_DeferOpenManual (0x415D80)
==================
*/
void Eng_DeferOpenManual( int a )
{
	g_bOpenManualPending = a;
}

/*
==================
Eng_DeferRelayout2 (0x415D90)
==================
*/
void Eng_DeferRelayout2( int a )
{
	g_bRelayout2Pending = a;
}

/*
==================
Launcher_OpenManual (0x415DA0)
==================
*/
static void Launcher_OpenManual( void )
{
	char	file[260];
	HINSTANCE	hResult;

	if ( !Launcher_BuildResourcePath( IDS_ENDDEMO_URL, file ) )
		return;

	// ShellExecute returns a value <= 32 (HINSTANCE_ERROR) when it fails to launch.
	hResult = ShellExecuteA( gLauncherWnd, "open", file, NULL, NULL, SW_SHOWMAXIMIZED );
	if ( (int)hResult <= HINSTANCE_ERROR )
		Launcher_ShowMessageByIdEx( 0, IDS_URL_BROWSERFAIL, file );
}

/*
==================
Eng_ClearDeferRelayout1 (0x415E70)

A dedicated 0-argument clear, separate from Eng_DeferRelayout1( 0 ).  RMLPostPump
calls both.
==================
*/
void Eng_ClearDeferRelayout1( void )
{
	g_bRelayout1Pending = 0;
}

/*
==================
Eng_GetDeferOpenManual (0x415E80)
==================
*/
int Eng_GetDeferOpenManual( void )
{
	return g_bOpenManualPending;
}

/*
==================
Eng_GetDeferRelayout2 (0x415E90)
==================
*/
int Eng_GetDeferRelayout2( void )
{
	return g_bRelayout2Pending;
}

/*
==================
Eng_GetDeferRelayout1 (0x415EA0)
==================
*/
int Eng_GetDeferRelayout1( void )
{
	return g_bRelayout1Pending;
}

/*
==================
Launcher_AppOwnsForeground (0x415EB0)
==================
*/
int Launcher_AppOwnsForeground( void )
{
	HWND	fg = GetForegroundWindow();
	if ( !fg )
		return 0;

	if ( fg == gLauncherWnd || fg == mainwindow )
		return 1;

	for ( HWND p = GetParent( fg ); p; p = GetParent( p ) )
	{
		if ( p == gLauncherWnd )
		return 1;
	}

	return 0;
}

/*
==================
Launcher_StartEngine (0x415F10)
==================
*/
int Launcher_StartEngine( int bBackground )
{
	AFX_MANAGE_STATE( AfxGetModuleState() );

	const char*	pszDll;
	int			err;

	gBackground = bBackground;
	if ( engineapi.GameSetBackground )
		engineapi.GameSetBackground( bBackground );

	g_bChangingVideoModes = 1;

	pszDll = ( g_bVidD3D || g_bVidGL ) ? "hw.dll" : "sw.dll";
	err = Eng_Load( pszDll, ENG_NORMAL );
	if ( err )
	{
		VID_HideEngineWindow();
		Launcher_ShowMessageById( 0, err );
		g_bChangingVideoModes = 0;
		return 0;
	}

	Eng_GameSetState( DLL_ACTIVE );
	if ( engineapi.Cbuf_AddText )
		engineapi.Cbuf_AddText( "hideconsole\n" );
	if ( !gBackground && !IsWindowVisible( mainwindow ) )
		VID_ShowEngineWindow( 1 );
	engineapi.ForceReloadProfile();
	g_bRestartPending     = 0;
	g_bChangingVideoModes = 0;
	return 1;
}

/*
==================
Launcher_StartEngineFg (0x416050)
==================
*/
void Launcher_StartEngineFg( void )
{
	Launcher_StartEngine( 0 );
}

/*
==================
Launcher_SetEngineTopmost (0x416060)
==================
*/
static void Launcher_SetEngineTopmost( int bTop )
{
	if ( !mainwindow )
		return;

	if ( bTop )
	{
		if ( g_bEngineTopmost != 1 )
		{
			g_bEngineTopmost = 1;
			SetWindowPos( mainwindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
			if ( CheckParm( "-notopmost", NULL ) )
				SetWindowPos( mainwindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
		}
	}
	else if ( g_bEngineTopmost )
	{
		g_bEngineTopmost = 0;
		SetWindowPos( mainwindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
	}
}

/*
==================
Launcher_SyncEngineWindow (0x416100)
==================
*/
void Launcher_SyncEngineWindow( CWnd* pDlg )
{
#ifdef LAUNCHER_RE
	// Not in the original.  This is the one place every page's idle path funnels
	// through while the engine owns the screen, so the in-game build marker is
	// driven from here -- ahead of the early-outs below, since it has to be told
	// to hide in exactly the cases they bail on.
	Launcher_UpdateGameOverlay();
#endif

	if ( gBackground )
		return;

	if ( !( ActiveApp || gEngineModeWindowed ) 
	                  || gDLLState != DLL_ACTIVE )
		return;

	::ShowWindow( mainwindow, SW_SHOW );
	if ( pDlg )
		pDlg->ShowWindow( SW_HIDE );

	if ( Launcher_AppOwnsForeground() )
	{
		if ( engineapi.GetPauseState() )
			engineapi.SetPauseState( 0 );

		Launcher_SetEngineTopmost( g_bConsoleMode == 0 );

		if ( windowed_mouse )
		{
			RECT	rc;
			GetWindowRect( mainwindow, &rc );
			ClipCursor( &rc );
		}

		SetFocus( mainwindow );

		if ( GetCapture() != mainwindow )
		{
			ReleaseCapture();
			if ( gEngineModeWindowed && windowed_mouse )
				SetCapture( mainwindow );
		}
	}
	else
	{
		if ( !engineapi.GetPauseState() )
			engineapi.SetPauseState( 1 );

		Launcher_SetEngineTopmost( 0 );
		ClipCursor( NULL );
		ReleaseCapture();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::RMLPreIdle (0x416250)
//
// slot 56

int CHLMainDlg::RMLPreIdle()
{
	if ( Launcher_GetRestartFlag() != 0 )
		OnOK();

	Launcher_SyncEngineWindow( this );

	if ( Eng_Frame( 0 ) && !gBackground )
	{
		m_bEngineWasActive = 1;
		return 1;
	}

	if ( !Launcher_MainButtonsLoaded() )
	{
		Launcher_LoadMainButtonsBitmap();
		::RedrawWindow( m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
	}

	if ( ActiveApp && Launcher_AppOwnsForeground() )
	{
		ShowWindow( SW_SHOWNORMAL );		// raise the launcher dialog
		::ShowWindow( mainwindow, SW_HIDE );		// hide the engine window
	}

	NullStub( 1 );
	ClipCursor( NULL );

	if ( !gBackground && ActiveApp )
	{
		if ( m_bEngineWasActive )
		{
			// Returned from in-game: hand focus back to the menu and queue a
			// refresh (the latch is cleared in RMLPostPump).
			SetActiveWindow();
			SetFocus();
			m_bMenuRefreshPending = 1;
			m_lastActivity = GetTickCount();
			RefreshAfterConfig();
			return 0;
		}

		if ( m_bMenuRefreshPending )
		{
			m_bMenuRefreshPending = 0;
			if ( m_bLogoOpened )
			{
				ShowMenuPage( 1 );
				::SendMessageA( m_pLogoAnim->GetSafeHwnd(), ACM_PLAY, (WPARAM)-1, (LPARAM)0xFFFF0000 );
			}
		}
	}
	return 0;
}

/*
==================
Eng_DeferMenuButton (0x4163A0)
==================
*/
void Eng_DeferMenuButton( int a )
{
	g_bMenuButtonPending = a;
}

/*
==================
Eng_GetDeferMenuButton (0x4163B0)
==================
*/
int Eng_GetDeferMenuButton( void )
{
	return g_bMenuButtonPending;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::RMLPostPump (0x4163C0)
//
// slot 60

void CHLMainDlg::RMLPostPump()
{
	if ( m_bEngineWasActive && ActiveApp )
	{
		m_bEngineWasActive = 0;
		LayoutMainMenu( InGame(), 1 );
	}

	if ( AFXGetTopLevelFrame() )	// straight launch (New Game / Training / +map / gamegauge)
	{
		AFXSetTopLevelFrame( 0 );

		// Replay the pending "skill N\nmap X\n" command stored by the menu action
		LaunchGameCmd( 0, 2, Launcher_GetPendingMap() );
		Launcher_RunMapCommand( "" );
	}

	// Every flag is read through its getter and cleared through its setter.
	if ( Eng_GetDeferOpenManual() )
	{
		Launcher_OpenManual();
		LayoutMainMenu( 0, 1 );
		ShowMenuPage( 1 );
		Eng_DeferOpenManual( 0 );
	}
	if ( Eng_GetDeferRelayout2() )
	{
		NullStub();
		LayoutMainMenu( 0, 1 );
		ShowMenuPage( 1 );
		Eng_DeferRelayout2( 0 );
	}
	if ( Eng_GetDeferRelayout1() )
	{
		// Both, in this order: the dedicated clear and then the setter.
		Eng_ClearDeferRelayout1();
		Eng_DeferRelayout1( 0 );
		LayoutMainMenu( 0, 1 );
		ShowMenuPage( 1 );
	}

	if ( Eng_GetDeferMenuButton() )	// engine asked the menu to replay an OK click
	{
		Eng_DeferMenuButton( 0 );
		::PostMessageA( m_hWnd, WM_COMMAND, MAKEWPARAM( IDOK, BN_CLICKED ),
			(LPARAM)m_btnOK.GetSafeHwnd() );
	}

	g_bMenuLoopReady = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::CHLMainDlg (0x4164F0)

CHLMainDlg::CHLMainDlg( CWnd* pParent )
	: CDlgBase( IDD_MAIN, pParent )
{
	m_pSelfWnd				= this;
	m_bEngineWasActive		= 0;
	m_bMenuRefreshPending	= 0;
	m_pLogoAnim				= NULL;
	m_bLogoOpened			= 0;
	m_bLogoPlaying			= 0;
	m_bReady				= 0;
	m_bInGameMenu			= 0;
	m_defW					= g_nLauncherDefW;
	m_defH					= g_nLauncherDefH;
	m_unk6980				= 0;

	RefreshDialogSkin();
	g_hKeyboardHook = SetWindowsHookExA( WH_KEYBOARD, LauncherKeyboardHook, NULL, GetCurrentThreadId() );
}

BEGIN_MESSAGE_MAP( CHLMainDlg, CDialog )
	//{{AFX_MSG_MAP(CHLMainDlg)
	ON_COMMAND( IDC_MAIN_MULTIPLAYER, OnLaunchModalPage )
	ON_MESSAGE( WM_DISPLAYCHANGE, OnDisplayChange )
	ON_COMMAND( IDC_MAIN_NEW_GAME, OnNewGame )
	ON_COMMAND( IDC_MAIN_HAZARD_COURSE, OnTraining )
	ON_COMMAND( IDC_MAIN_LOAD_OR_SAVE_GAME, OnCreateGame )
	ON_COMMAND( IDC_MAIN_LOAD_GAME, OnLoadGame )
	ON_COMMAND( IDC_MAIN_CONFIGURE_HALF_LIFE, OnConfiguration )
	ON_WM_PAINT()
	ON_COMMAND( IDC_MAIN_VIEW_README, OnReadmeDialog )
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
	ON_COMMAND( IDC_MAIN_RETURN_TO_GAME, OnMultiplayer )
	ON_WM_ACTIVATE()
	ON_COMMAND( IDC_MAIN_CONSOLE, OnConsole )
	ON_WM_SHOWWINDOW()
	ON_WM_SYSCOMMAND()
	ON_WM_SIZE()
	ON_COMMAND( IDC_MAIN_MINIMIZE, OnMinimizeButton )
	ON_COMMAND( IDC_MAIN_CLOSE, OnCloseButton )
	ON_COMMAND( IDC_MAIN_ORDER_HALF_LIFE, OnUnusedButton )
	ON_COMMAND( IDC_BTN_PREVIEWS, OnPreviews )
	ON_WM_SYSKEYUP()
	ON_COMMAND( IDC_MAIN_CUSTOM_GAME, OnCustomGame )
	ON_COMMAND( IDC_MAIN_FRIENDS, OnFriends )
	ON_COMMAND( ID_LAUNCHER_REPORTERROR, OnReportError )
	ON_COMMAND( IDC_MAIN_CHAT_ROOMS, OnUnusedButton )
	ON_COMMAND( IDC_MAIN_INTERNET_GAMES, OnUnusedButton )
	ON_COMMAND( IDC_MAIN_LAN_GAMES, OnUnusedButton )
	ON_COMMAND( IDC_MAIN_QUICK_START, OnUnusedButton )
	ON_COMMAND( IDC_MAIN_TFC_MANUAL, OnTfcManual )
	//}}AFX_MSG_MAP
#ifdef LAUNCHER_FIXES
	ON_WM_EXITSIZEMOVE()
#endif
END_MESSAGE_MAP()

#ifdef LAUNCHER_FIXES
/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnExitSizeMove (LAUNCHER_FIXES)
//
// The drag is over: remember where the launcher was put.

void CHLMainDlg::OnExitSizeMove()
{
	Default();
	Dlg_SaveWindowPos( this );
}

#endif	// LAUNCHER_FIXES

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::LauncherKeyboardHook (0x416830)

LRESULT CALLBACK CHLMainDlg::LauncherKeyboardHook( int code, WPARAM wParam, LPARAM lParam )
{
	HWND	hTarget;
	int		bTransitionUp;

	if ( code == HC_ACTION )
	{
		hTarget = g_bEngineWindowUp ? mainwindow : gLauncherWnd;
		bTransitionUp = ( HIWORD( lParam ) & KF_UP );	// key-up transition bit

		if ( ActiveApp )
		{
			if ( wParam == VK_TAB )
			{
				if ( ( HIWORD( lParam ) & KF_ALTDOWN ) && bTransitionUp )	// Alt down
					::PostMessageA( hTarget, WM_SYSKEYDOWN, VK_TAB, lParam );
			}
			else if ( wParam == VK_ESCAPE )
			{
				// Alt+Esc, or Ctrl+Esc with the up-bit set.
				if ( ( HIWORD( lParam ) & KF_ALTDOWN ) )
				{
					if ( bTransitionUp )
						::PostMessageA( hTarget, WM_SYSKEYDOWN, VK_ESCAPE, lParam );
				}
				else if ( bTransitionUp && ( GetKeyState( VK_CONTROL ) & 0x8000 ) )
				{
					::PostMessageA( hTarget, WM_SYSKEYDOWN, VK_ESCAPE, lParam );
				}
			}
		}
	}

	return CallNextHookEx( g_hKeyboardHook, code, wParam, lParam );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::RefreshDialogSkin (0x4168F0)

void CHLMainDlg::RefreshDialogSkin( void )
{
	int		wh[2] = { 0, 0 };

	m_hMenuStrip = Launcher_HeaderLoaded();

	// The 8-byte local is only the out-buffer for the query; the cell size then
	// lives in the member, and it is the member's address every SetDIBData below
	// receives.
	Launcher_HeaderSize( wh );							// {w,h}
	m_menuCell.cx    = wh[0];
	m_menuCell.cy    = wh[1];
	m_nMenuStripRows = Launcher_HeaderStride();

	if ( m_hMenuStrip )
	{
		// Each button's slice index into btns_main (from the strip layout).
		m_btnNewGame.SetDIBData( m_menuCell, BTNSTRIP_NEW_GAME, m_hMenuStrip );
		m_btnReturnToGame.SetDIBData( m_menuCell, BTNSTRIP_RESUME_GAME, m_hMenuStrip );
		m_btnHazardCourse.SetDIBData( m_menuCell, BTNSTRIP_HAZARD_COURSE, m_hMenuStrip );
		m_btnLoadGame.SetDIBData( m_menuCell, BTNSTRIP_LOAD_GAME, m_hMenuStrip );
		m_btnLoadOrSaveGame.SetDIBData( m_menuCell, BTNSTRIP_LOAD_SAVE_GAME, m_hMenuStrip );
		m_btnConfigureHalfLife.SetDIBData( m_menuCell, BTNSTRIP_CONFIGURATION, m_hMenuStrip );
		m_btnQuickStart.SetDIBData( m_menuCell, BTNSTRIP_QUICK_START, m_hMenuStrip );
		m_btnChatRooms.SetDIBData( m_menuCell, BTNSTRIP_CHAT_ROOMS, m_hMenuStrip );
		m_btnInternetGames.SetDIBData( m_menuCell, BTNSTRIP_INTERNET_GAMES, m_hMenuStrip );
		m_btnLanGames.SetDIBData( m_menuCell, BTNSTRIP_LAN_GAMES, m_hMenuStrip );
		m_btnViewReadme.SetDIBData( m_menuCell, BTNSTRIP_VIEW_README, m_hMenuStrip );
		m_btnOK.SetDIBData( m_menuCell, BTNSTRIP_OK_NARROW, m_hMenuStrip );
		m_btnMultiplayer.SetDIBData( m_menuCell, BTNSTRIP_MULTIPLAYER, m_hMenuStrip );
		m_btnPreviews.SetDIBData( m_menuCell, BTNSTRIP_PREVIEWS, m_hMenuStrip );
		m_btnConsole.SetDIBData( m_menuCell, BTNSTRIP_CONSOLE, m_hMenuStrip );
		m_btnCustomGame.SetDIBData( m_menuCell, BTNSTRIP_CUSTOM_GAME, m_hMenuStrip );
	}

	if ( m_hWnd )
	{
		CreateLogoAnim();

		const char*	pszType = g_pCurrentMod ? g_pCurrentMod->GetKey( "type" ) : NULL;

			// The four buttons are highlighted in a different order per branch.
		if ( pszType && *pszType && !_stricmp( pszType, "multiplayer_only" ) )
		{
			m_btnHazardCourse.SetHighlight( 1 );
			m_btnNewGame.SetHighlight( 1 );
			m_btnLoadGame.SetHighlight( 1 );
			m_btnLoadOrSaveGame.SetHighlight( 1 );
		}
		else
		{
			m_btnNewGame.SetHighlight( 0 );
			m_btnHazardCourse.SetHighlight( 0 );
			m_btnLoadGame.SetHighlight( 0 );
			m_btnLoadOrSaveGame.SetHighlight( 0 );
		}

		m_stcClose.SetSkin( "cls_n", "cls_d", "cls_f" );
		m_stcMin.SetSkin( "min_n", "min_d", "min_f" );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::~CHLMainDlg (0x416B70)

CHLMainDlg::~CHLMainDlg()
{
	delete m_pLogoAnim;
	m_pLogoAnim = NULL;

	UnhookWindowsHookEx( g_hKeyboardHook );

#ifdef LAUNCHER_RE
	// Not in the original: the marker window outlives the engine window unless
	// it is torn down here, and it is topmost.
	Launcher_DestroyGameOverlay();
#endif

	if ( mainwindow )
	{
		::DestroyWindow( mainwindow );
		mainwindow = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::DoDataExchange (0x416E80)

void CHLMainDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_MAIN_QUICK_START, m_btnQuickStart );
	DDX_Control( pDX, IDC_MAIN_CHAT_ROOMS, m_btnChatRooms );
	DDX_Control( pDX, IDC_MAIN_INTERNET_GAMES, m_btnInternetGames );
	DDX_Control( pDX, IDC_MAIN_LAN_GAMES, m_btnLanGames );
	DDX_Control( pDX, IDC_QUICKSTART, m_lblQuickStart );
	DDX_Control( pDX, IDC_LAN_GAMES, m_lblLanGames );
	DDX_Control( pDX, IDC_INTERNET_GAMES, m_lblInternetGames );
	DDX_Control( pDX, IDC_CHAT_ROOMS, m_lblChatRooms );
	DDX_Control( pDX, IDC_MAIN_FRIENDS, m_btnFriends );
	DDX_Control( pDX, IDC_MAIN_CUSTOMHELP, m_lblCustomHelp );
	DDX_Control( pDX, IDC_MAIN_PREVIEWSHELP, m_lblPreviewsHelp );
	DDX_Control( pDX, IDC_BTN_PREVIEWS, m_btnPreviews );
	DDX_Control( pDX, IDC_MAIN_MINIMIZE, m_stcMin );
	DDX_Control( pDX, IDC_MAIN_CLOSE, m_stcClose );
	DDX_Control( pDX, IDC_MAIN_ORDERHELP, m_lblOrderHelp );
	DDX_Control( pDX, IDC_MAIN_ORDER_HALF_LIFE, m_btnOrderHalfLife );
	DDX_Control( pDX, IDC_MAIN_CONSOLE, m_btnConsole );
	DDX_Control( pDX, IDC_MAIN_READMEHELP, m_lblReadmeHelp );
	DDX_Control( pDX, IDC_MAIN_QUITHELP, m_lblQuitHelp );
	DDX_Control( pDX, IDC_MAIN_RETURNHELP, m_lblReturnHelp );
	DDX_Control( pDX, IDC_MAIN_NEWGAMEHELP, m_lblNewGameHelp );
	DDX_Control( pDX, IDC_MAIN_TRAININGHELP, m_lblTrainingHelp );
	DDX_Control( pDX, IDC_MAIN_LOADHELP, m_lblLoadHelp );
	DDX_Control( pDX, IDC_MAIN_LOADSAVEHELP, m_lblLoadSaveHelp );
	DDX_Control( pDX, IDC_MAIN_MULTIPLAYERHELP, m_lblMultiplayerHelp );
	DDX_Control( pDX, IDC_MAIN_CONFIGUREHELP, m_lblConfigureHelp );
	DDX_Control( pDX, IDC_MAIN_RETURN_TO_GAME, m_btnReturnToGame );
	DDX_Control( pDX, IDC_MAIN_NEW_GAME, m_btnNewGame );
	DDX_Control( pDX, IDC_MAIN_HAZARD_COURSE, m_btnHazardCourse );
	DDX_Control( pDX, IDC_MAIN_LOAD_GAME, m_btnLoadGame );
	DDX_Control( pDX, IDC_MAIN_LOAD_OR_SAVE_GAME, m_btnLoadOrSaveGame );
	DDX_Control( pDX, IDC_MAIN_CONFIGURE_HALF_LIFE, m_btnConfigureHalfLife );
	DDX_Control( pDX, IDC_MAIN_VIEW_README, m_btnViewReadme );
	DDX_Control( pDX, IDOK,    m_btnOK );
	DDX_Control( pDX, IDC_MAIN_MULTIPLAYER, m_btnMultiplayer );
	DDX_Control( pDX, IDC_MAIN_CUSTOM_GAME, m_btnCustomGame );
	DDX_Control( pDX, IDC_MAIN_TFC_MANUAL, m_btnTfcManual );
	DDX_Control( pDX, IDC_MAIN_MANUALHELP, m_lblManualHelp );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnDisplayChange (0x417150)

LRESULT CHLMainDlg::OnDisplayChange( WPARAM, LPARAM )
{
	if ( gDLLState != DLL_ACTIVE )
		Dlg_CenterWindow( this );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnLaunchModalPage (0x417170)

void CHLMainDlg::OnLaunchModalPage()
{
	CString	strToken = Launcher_GetProfileString( "Settings", "User Token 2", "" );

	if ( strToken.GetLength() )
	{
		Launcher_ShowMessageById( 0, IDS_CONTENT_NOMULTIPLAYER );
		return;
	}

	// If a game is live, confirm the disconnect first.
	GameInfo_t	gi;
	if ( engineapi.GetGameInfo && engineapi.GetGameInfo( &gi, 0 ) && gi.state == ca_active && gi.active )
	{
		CPromptDlg	prompt( 2, NULL );		// OK + Cancel

		prompt.SetMessage( Launcher_LoadString( IDS_MAIN_EXITMULTIPLAYERPROMPT ) );
		if ( prompt.DoModal() != IDOK )
			return;
		if ( engineapi.Cbuf_AddText )
		{
			gBackground = 1;
			engineapi.Cbuf_AddText( "disconnect\n" );
			Eng_Frame( 1 );
		}
	}

	ShowMenuPage( 0 );

	CMultiSelectDlg	page( NULL );
	InitChildDialog( &page, &m_btnMultiplayer );
	Eng_Load( NULL, 0 );
	gDLLState     = DLL_INACTIVE;
	gDLLStateInfo = 0;
	gbConsoleMode = 1;
	if ( !gEngineModeWindowed )
		DDraw_SetDisplayMode( -1 );

	page.DoModal();

	Sleep( 100 );
	if ( !gEngineModeWindowed )
		DDraw_SetDisplayMode( -1 );

	if ( g_bPageRestart )
	{
		Eng_Load( NULL, 0 );			// engine restart requested
		OnOK();
	}
	else
	{
		LayoutMainMenu( 0, 1 );
		gbConsoleMode  = 0;
		m_lastActivity = GetTickCount();
		RestoreAfterModal();
		ShowMenuPage( 1 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::PlayLogo (0x417510)

void CHLMainDlg::PlayLogo()
{
	CLogoDlg	logo( this );
	logo.DoModal();
}

/*
==================
Launcher_CreateEngineWindow (0x417570)
==================
*/
HWND  Launcher_CreateEngineWindow( HWND hParent, HINSTANCE hInst )
{
	CWinThread*	pThread;
	RECT		rc = { 0, 0, 1000, 1000 };
	WNDCLASSA	wc;
	const char*	pszTitle;
	char		szTitle[256];
	char*		p;
	HWND		hWnd;

	pThread = AfxGetThread();
	if ( pThread )
		pThread->GetMainWnd();

	wc.style         = CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc   = MainWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInst;
	wc.hIcon         = LoadIconA( AfxGetResourceHandle(),
		MAKEINTRESOURCEA( IDI_LAUNCHER ) );
	wc.hCursor       = LoadCursorA( NULL, IDC_ARROW );
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "Half-Life";
	RegisterClassA( &wc );

	WindowStyle = gEngineModeWindowed
		? (WS_POPUP|WS_CLIPSIBLINGS|WS_CAPTION)		// windowed  (0x84C00000)
		: (WS_POPUP|WS_CLIPSIBLINGS);					// fullscreen
	AdjustWindowRectEx( &rc, WindowStyle, FALSE, 0 );

	pszTitle = g_pCurrentMod ? g_pCurrentMod->GetKeyString( "game" ) : NULL;
	if ( !pszTitle || !*pszTitle )
		pszTitle = "Half-Life";
	strncpy( szTitle, pszTitle, sizeof( szTitle ) - 1 );
	szTitle[sizeof( szTitle ) - 1] = 0;

	p = szTitle + strlen( szTitle ) - 1;
	while ( p > szTitle && *p == ' ' )
		*p-- = 0;

	hWnd = CreateWindowExA(
		0, 						// dwExStyle
		"Half-Life", 			// lpClassName
		szTitle, 				// lpWindowName
		WindowStyle, 			// dwStyle
		0, 0,
		GetSystemMetrics( SM_CXSCREEN ), // nWidth
		GetSystemMetrics( SM_CYSCREEN ), // nHeight
		hParent,  NULL, 
		hInst,    NULL );

	return hWnd;
}

/*
==================
Launcher_RestoreAfterEngine (0x4176B0)
==================
*/
HWND  Launcher_RestoreAfterEngine( int windowed )
{
	HWND	hWnd = mainwindow;

	// VID_DestroyDIB is idempotent (it null-checks each object).
	VID_DestroyDIB();

	if ( mainwindow )
	{
		DestroyWindow( mainwindow );
		UpdateWindow( mainwindow );
		UnregisterClassA( "Half-Life", gLauncherHandle );
		hWnd = mainwindow;
	}

	if ( windowed && lpDD )
	{
		DDraw_Shutdown();
		hWnd = mainwindow;
	}

	gEngineModeWindowed = windowed;

#ifdef LAUNCHER_FIXES
	// The video page can flip this at runtime: the shell moves between the
	// desktop mode and its own, and is resized to whichever it now owns.
	if ( windowed )
		Shell_LeaveFullscreen();
	else
		Shell_ComputeMetrics();

	if ( gLauncherWnd && ::IsWindow( gLauncherWnd ) )
	{
		CWnd*	pShell = CWnd::FromHandle( gLauncherWnd );

		Dlg_CenterWindow( pShell );
		if ( !windowed && !Shell_HasFullscreenMode() )
			Shell_ShowBackdrop( 1 );
	}
#endif

	if ( hWnd )
	{
		DestroyWindow( hWnd );
		mainwindow = NULL;
	}

	mainwindow = Launcher_CreateEngineWindow( gLauncherWnd, gLauncherHandle );
	return mainwindow;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::CreateLogoAnim (0x417750)

void CHLMainDlg::CreateLogoAnim()
{
	if ( m_pLogoAnim )
		delete m_pLogoAnim;

	m_pLogoAnim = new CAnimateCtrl();
	if ( !m_pLogoAnim )
	{
		m_bLogoOpened  = 0;
		m_bLogoPlaying = 0;
		return;
	}

	CRect	rc( 0, 70, g_nLauncherDefW, 170 );
	m_pLogoAnim->Create(
		WS_CHILD | WS_VISIBLE | ACS_TRANSPARENT | ACS_AUTOPLAY | 0x80, 
		rc, this, 0
	);

	if ( !::SendMessageA( m_pLogoAnim->GetSafeHwnd(), ACM_OPENA, 0, (LPARAM)COM_FindPath( "media\\logo.avi" ) ) )
	{
		delete m_pLogoAnim;
		m_pLogoAnim    = NULL;
		m_bLogoOpened  = 0;
		m_bLogoPlaying = 0;
		return;
	}
	m_bLogoOpened = 1;

	if ( !::SendMessageA( m_pLogoAnim->GetSafeHwnd(), ACM_PLAY, (WPARAM)-1, (LPARAM)0xFFFF0000 ) )		// whole clip, infinite loop
	{
		m_bLogoPlaying = 0;
		return;
	}
	m_bLogoPlaying = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnInitDialog (0x4178C0)

BOOL CHLMainDlg::OnInitDialog()
{
	HWND	hDlg;
	CDC*	pDC;
	RECT	rc;
	int		bNotRestart;
	int		x0, x1, x2;
	MSG		msg;
	char	szArgs[256];
	char*	pszValue;

	LOG_ENTER();
	CDialog::OnInitDialog();
	NullStub( 0 );

	// Black out the client area and flush any queued mouse/key messages.
	pDC = CDC::FromHandle( ::GetDC( m_hWnd ) );
	PatBlt( pDC->m_hDC, 0, 0, g_nLauncherDefW, g_nLauncherDefH, BLACKNESS );
	::ReleaseDC( m_hWnd, pDC->m_hDC );
	m_btnFriends.ShowWindow( SW_HIDE );

	while ( PeekMessageA( &msg, NULL, WM_MOUSEFIRST, WM_MBUTTONDBLCLK, PM_REMOVE ) )
		;
	while ( PeekMessageA( &msg, NULL, WM_KEYFIRST, WM_SYSDEADCHAR, PM_REMOVE ) )
		;

	SetWindowText( "Half-Life" );
	hDlg = GetSafeHwnd();
	gLauncherHandle = AfxGetInstanceHandle();

	// Skin + position the close and minimise buttons.
	m_stcClose.SetSkin( "cls_n", "cls_d", "cls_f" );
	m_stcMin.SetSkin( "min_n", "min_d", "min_f" );
	x0 = g_nLauncherDefW - 48;
	x1 = x0 + 19;
	x2 = x0 + 38;
	m_stcMin.MoveWindow( x0, 10, x1 - x0, 19, TRUE );
	m_stcClose.MoveWindow( x1, 10, x2 - x1, 19, TRUE );

	g_bConsoleMode = ( CheckParm( "-console", NULL ) != NULL );
	if ( g_bConsoleMode )
		CFG_SetTokenProfile( g_szConfigName, "console", "1.0", 0 );
	g_bNoPrompt = ( CheckParm( "-noprompt", NULL ) != NULL );

#ifdef LAUNCHER_FIXES
	// Publish the launcher window before the first Dlg_CenterWindow call, so
	// that call knows this is the main window and not one of its pages.
	gLauncherWnd = m_hWnd;
#endif
	Dlg_CenterWindow( this );

	// Give the launcher window a system menu + minimise box; fullscreen also
	// gets WS_EX_TOPMOST.
	SetWindowLongA( hDlg, GWL_STYLE, GetWindowLongA( hDlg, GWL_STYLE ) | WS_SYSMENU | WS_MINIMIZEBOX );
	if ( !gEngineModeWindowed )
		SetWindowLongA( hDlg, GWL_EXSTYLE, GetWindowLongA( hDlg, GWL_EXSTYLE ) | WS_EX_TOPMOST );

#ifdef LAUNCHER_FIXES
	// LAUNCHER_FIXES: a caption bar with the window's title in it, sized so the
	// skin's client area stays exactly where it was.  See Dlg_ApplyTitleBar.
	Dlg_ApplyTitleBar( this, "Half-Life" );

	// ...and the launcher's own icon in that caption, the taskbar and Alt+Tab.
	// It is the same IDI_LAUNCHER the engine window class registers.
	{
		HICON	hIconBig = (HICON)LoadImageA( AfxGetResourceHandle(),
			MAKEINTRESOURCEA( IDI_LAUNCHER ), IMAGE_ICON,
			GetSystemMetrics( SM_CXICON ), GetSystemMetrics( SM_CYICON ), 0 );
		HICON	hIconSmall = (HICON)LoadImageA( AfxGetResourceHandle(),
			MAKEINTRESOURCEA( IDI_LAUNCHER ), IMAGE_ICON,
			GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );

		if ( hIconBig )
			SetIcon( hIconBig, TRUE );
		if ( hIconSmall )
			SetIcon( hIconSmall, FALSE );
	}
#endif

	bNotRestart  = ( Launcher_GetRestartFlag() == 0 );
	gLauncherWnd = m_hWnd;

	LOG( "preloading engine (Eng_Load NULL,1); windowed=%d", gEngineModeWindowed );
	Eng_Load( NULL, 1 );			// preload the engine for the background map

	if ( !gEngineModeWindowed )
	{
		LOG( "fullscreen path: checking DirectDraw mode availability" );
		if ( !DDraw_IsModeAvailable() )
		{
			LOG( "DDraw mode unavailable -- IDS_DDRAW_REQUIRED" );
			Launcher_ShowMessageById( 0, IDS_DDRAW_REQUIRED );
			OnCancel();
		}
		DDraw_Init( 1, 0 );
	}

	// Intro logo, unless a command line wants to go straight in.
	if ( bNotRestart
	  && ( ( !CheckParm( "-nosierra", NULL ) 
	      && !CheckParm( "-toconsole", NULL )
	      && !CheckParm( "-console", NULL ) 
		  && !CheckParm( "-dev", NULL )
	      && !CheckParm( "-gamegauge", NULL ) 
		  && !g_bConsoleMode
	      && !CheckParm( "+map", NULL ) 
		  && !CheckParm( "+connect", NULL ) )
	    || CheckParm( "-logo", NULL ) ) )
	{
		LOG( "playing intro logo" );
		ShowCursor( FALSE );
		PlayLogo();
		ShowCursor( TRUE );
		LOG( "intro logo done" );
	}
	else
	{
		LOG( "skipping intro logo (cmdline/restart)" );
	}

	// Fullscreen needs the desktop mode restored to draw the menu through GDI.
	if ( gEngineModeWindowed )
	{
		if ( lpDD )
			lpDD->Release();
		lpDD = NULL;
	}
	else
	{
		if ( !lpDD )
		{
			Launcher_ShowMessageById( 0, IDS_DDRAW_REQUIRED );
			OnCancel();
		}
		DDraw_SetDisplayMode( -1 );

#ifdef LAUNCHER_FIXES
		// No mode small enough to switch to: fill the desktop behind the shell
		// and put the shell in the middle of it.  See Shell_ShowBackdrop.
		if ( Shell_FullscreenActive() && !Shell_HasFullscreenMode() )
		{
			Shell_ShowBackdrop( 1 );
			Dlg_CenterWindow( this );
		}
#endif
	}

	// Paint the menu, then create the engine child window.
	CClientDC	dc( this );

	::GetClientRect( m_hWnd, &rc );
	::FillRect( dc.m_hDC, &rc, CBrush( RGB( 0, 0, 0 ) ) );
	LOG( "laying out main menu" );
	LayoutMainMenu( 0, 1 );
	::GetClientRect( m_hWnd, &rc );
	::FillRect( dc.m_hDC, &rc, CBrush( RGB( 0, 0, 0 ) ) );
	ShowWindow( SW_SHOWNORMAL );
	::UpdateWindow( m_hWnd );
	Launcher_SetRestartFlag( 0 );

	if ( mainwindow )
	{
		::DestroyWindow( mainwindow );
		mainwindow = NULL;
	}
	LOG( "creating engine child window" );
	mainwindow = Launcher_CreateEngineWindow( m_hWnd, gLauncherHandle );
	LOG( "engine child window = %p", mainwindow );

	// Low-spec machines skip the menu fly-in.
	if ( CheckParm( "-nofly", NULL ) || ef.iCPUMhz < 195 )
		g_bNoFly = 1;
	if ( CheckParm( "-fly", NULL ) )
		g_bNoFly = 0;

	::GetClientRect( m_hWnd, &rc );
	::FillRect( dc.m_hDC, &rc, CBrush( RGB( 0, 0, 0 ) ) );
	m_bReady = 1;					// dialog fully laid out

	if ( !g_bConsoleMode )
		m_btnConsole.ShowWindow( SW_HIDE );
	m_btnReturnToGame.ShowWindow( SW_HIDE );

	CreateLogoAnim();

	if ( CheckParm( "-toconsole", NULL ) )
	{
		AFXSetTopLevelFrame( 1 );
	}
	else if ( CheckParm( "-gamegauge", &pszValue ) )
	{
		if ( !pszValue )
			pszValue = "gg";
		AFXSetTopLevelFrame( 1 );
		// (sic) forward slashes, not newlines -- the bytes at 0x4CF2E4 are
		// "/ngg %s/n", so the shipping build never terminated this command.
		sprintf( szArgs, "/ngg %s/n", pszValue );
		Launcher_RunMapCommand( szArgs );
	}
	else if ( CheckParm( "+connect", NULL ) || CheckParm( "+map", NULL ) )
	{
		AFXSetTopLevelFrame( 1 );
	}
	else
	{
		// Normal path: show the menu page and bring the launcher forward.
		LOG( "showing menu page + bringing launcher to foreground" );
		ShowMenuPage( 1 );
		LayoutMainMenu( 0, 1 );
		NullStub( 1 );
		::SetForegroundWindow( m_hWnd );
	}

	LOG( "OnInitDialog complete -- menu is up" );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnNewGame (0x417F00)
//
// cmd 1016

void CHLMainDlg::OnNewGame()
{
	ShowMenuPage( 0 );
	CNewGameDlg	page( this );
	InitChildDialog( &page, &m_btnNewGame );
	page.DoModal();
	m_lastActivity = GetTickCount();
	RestoreAfterModal();
	ShowMenuPage( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnTraining (0x417FA0)
//
// cmd 1223

void CHLMainDlg::OnTraining()
{
	const char*	pszMap;
	char		szCmd[260];

	pszMap = NULL;
	if ( g_pCurrentMod )
		pszMap = g_pCurrentMod->GetKey( "trainmap" );
	if ( !pszMap )
		pszMap = "t0a0";

	sprintf( szCmd, "map %s\n", pszMap );
	AFXSetTopLevelFrame( 1 );
	Launcher_RunMapCommand( szCmd );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnLoadGame (0x418240)
//
// cmd 1021

void CHLMainDlg::OnLoadGame()
{
	ShowMenuPage( 0 );
	CLoadDlg	page( this );
	InitChildDialog( &page, &m_btnLoadGame );
	page.DoModal();
	RestoreAfterModal();
	ShowMenuPage( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnCreateGame (0x4182E0)
//
// cmd 1022

void CHLMainDlg::OnCreateGame()
{
	ShowMenuPage( 0 );
	CLoadSaveDlg	page( this );
	InitChildDialog( &page, &m_btnLoadOrSaveGame );
	page.DoModal();
	RestoreAfterModal();
	ShowMenuPage( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnConfiguration (0x418380)
//
// cmd 1017

void CHLMainDlg::OnConfiguration()
{
	ShowMenuPage( 0 );
	CConfigureDlg	page( this );
	InitChildDialog( &page, &m_btnConfigureHalfLife );
	page.DoModal();
	m_lastActivity = GetTickCount();
	RestoreAfterModal();
	ShowMenuPage( 1 );
	RefreshAfterConfig();	// settings may have changed the renderer
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::DrawDialogOverlay (0x418430)
//
// CDlgBase paint slot 54

void CHLMainDlg::DrawDialogOverlay( CDC* pDC, RECT* prc )
{
	if ( !m_bReady )
	{
		CBrush	black( RGB( 0, 0, 0 ) );
		pDC->FillRect( prc, &black );
		return;
	}

	CFont	font;
	font.CreateFont( -10, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, VARIABLE_PITCH, "Arial" );

	char	ver[64];
	sprintf( ver, "v%i/%s", g_nLauncherVersion, g_szPatchVersion );

	CRect	vr( g_nLauncherDefW - 75, g_nLauncherDefH - 20,
	            g_nLauncherDefW - 5,  g_nLauncherDefH - 5 );
	CFont*		pOldFont = pDC->SelectObject( &font );
	int			oldBk    = pDC->SetBkMode( TRANSPARENT );
	COLORREF	oldClr   = pDC->SetTextColor( RGB( 120, 90, 31 ) );
	pDC->DrawText( ver, -1, &vr, DT_NOPREFIX );

	pDC->SetTextColor( oldClr );
	pDC->SetBkMode( oldBk );
	pDC->SelectObject( pOldFont );

#ifdef LAUNCHER_RE
	// Top-left, as on every other page (the v%i/%s string above is the original's).
	Launcher_DrawBuildMarker( pDC );
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::PreTranslateMessage (0x4185D0)

BOOL CHLMainDlg::PreTranslateMessage( MSG* pMsg )
{
	// While the engine is live and unpaused the menu is not really up: hand the
	// message straight to the engine's window procedure.
	if ( gDLLState != DLL_INACTIVE && gDLLState != DLL_PAUSED )
	{
		return (BOOL)::CallWindowProc( MainWndProc, m_hWnd, pMsg->message,
									   pMsg->wParam, pMsg->lParam );
	}

	if ( pMsg->message != WM_KEYDOWN )
		return CDialog::PreTranslateMessage( pMsg );

	if ( pMsg->wParam == VK_RETURN )
	{
		// ENTER presses the focused menu button rather than firing IDOK.
		CButton*	pBtn = (CButton*)AfxDynamicDownCast( RUNTIME_CLASS( CButton ),
								CWnd::FromHandle( ::GetFocus() ) );
		if ( !pBtn )
			return CWnd::PreTranslateMessage( pMsg );

		::SendMessage( m_hWnd, WM_COMMAND, pBtn->GetDlgCtrlID(),
					   (LPARAM)pBtn->m_hWnd );
		return TRUE;
	}

	if ( pMsg->wParam != VK_ESCAPE )
		return CDialog::PreTranslateMessage( pMsg );

	// ESC before the deferred loop has run once is swallowed.
	if ( !g_bMenuLoopReady )
		return TRUE;

	if ( m_bInGameMenu )
		OnMultiplayer();
	else
		OnOK();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnOK (0x4186E0)
//
// slot 49
// -- the Quit button, ESC and the title-bar
// close glyph all land here.

void CHLMainDlg::OnOK()
{
	if ( !g_bMenuLoopReady )
		return;

	// Is a game live? Selects the prompt text and whether a pending restart may
	// skip the prompt.
	int	bInGame = 0;
	if ( gDLLState != DLL_CLOSE && gDLLState != DLL_INACTIVE )
	{
		GameInfo_t	gi;
		bInGame = 1;
		if ( !engineapi.GetGameInfo( &gi, 0 ) || gi.state == ca_disconnected )
			bInGame = 0;
	}
	if ( g_pCurrentMod )
	{
		const char*	pszType = g_pCurrentMod->GetKey( "type" );
		if ( pszType && !_stricmp( pszType, "multiplayer_only" ) )
			bInGame = 0;
	}

	// A console/dedicated engine has no skinned UI -- nothing to do.
	if ( gbConsoleMode )
		return;

	if ( CheckParm( "-gamegauge", NULL ) )
		g_bNoPrompt = 1;

	CPromptDlg	prompt( 2, NULL );		// OK + Cancel
	prompt.SetMessage( Launcher_LoadString( bInGame ? IDS_MAIN_QUITPROMPTINGAME : IDS_MAIN_QUITPROMPT ) );

	if ( g_bNoPrompt
	  || ( Launcher_GetRestartFlag() && !bInGame )
	  || prompt.DoModal() == IDOK )
	{
		// Confirmed: quit the engine, tear it down and end the dialog.
		ShowMenuPage( 0 );
		if ( gDLLState != DLL_INACTIVE )
		{
			if ( gDLLState != DLL_CLOSE && engineapi.Cbuf_AddText )
				engineapi.Cbuf_AddText( "quit\n" );
			Eng_GameSetState( DLL_CLOSE );
		}
		Eng_Load( NULL, 0 );
		::DestroyWindow( mainwindow );
		mainwindow = NULL;
		CDialog::OnOK();
	}
	else
	{
		// Cancelled: drop any pending restart so RMLPreIdle stops spinning on it.
		Launcher_SetRestartFlag( 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnReadmeDialog (0x418940)
//
// cmd 1018

void CHLMainDlg::OnReadmeDialog()
{
	ShowMenuPage( 0 );
	CReadmeDlg	page( this );
	InitChildDialog( &page, &m_btnViewReadme );
	page.DoModal();
	m_lastActivity = GetTickCount();
	RestoreAfterModal();
	ShowMenuPage( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnActivateApp (0x4189E0)

void    CHLMainDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	Default();
	ActiveApp = bActive;

	if ( !g_bChangingVideoModes 
		&& bActive 
		&& gDLLState == DLL_ACTIVE 
		&& gEngineVidType == VT_Direct3D )
		{ 
			AppActivate( bActive, 0 );
		}
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnMultiplayer (0x418A20)
//
// cmd 1019/1167

void CHLMainDlg::OnMultiplayer()
{
	ShowMultiplayerPage( 0, 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnActivate (0x418A30)
//
// Re-centres the dialog when it is activated while the engine does not own the
// screen; the same test OnDisplayChange makes.

void CHLMainDlg::OnActivate( UINT nState, CWnd* pOther, BOOL bMin )
{
	if ( gDLLState != DLL_ACTIVE && nState )
		Dlg_CenterWindow( this );

	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::ShowMenuPage (0x418A60)

void CHLMainDlg::ShowMenuPage( int bShow )
{
	if ( !GetSafeHwnd() )
		return;

	if ( bShow )
	{
		if ( m_bLogoOpened && m_pLogoAnim )
			::ShowWindow( m_pLogoAnim->GetSafeHwnd(), SW_RESTORE );

		if ( !m_bLogoPlaying || !m_bLogoOpened )
		{
			m_bLogoPlaying = ( m_bLogoOpened && m_pLogoAnim
				&& ::SendMessageA( m_pLogoAnim->GetSafeHwnd(), ACM_OPENA, 0, (LPARAM)COM_FindPath( "media\\logo.avi" ) )
				&& ::SendMessageA( m_pLogoAnim->GetSafeHwnd(), ACM_PLAY, (WPARAM)-1, (LPARAM)0xFFFF0000 ) );
		}
	}
	else
	{
		m_bLogoPlaying = 0;
		if ( m_bLogoOpened && m_pLogoAnim )
		{
			::ShowWindow( m_pLogoAnim->GetSafeHwnd(), SW_HIDE );
			::SendMessageA( m_pLogoAnim->GetSafeHwnd(), ACM_OPENA, 0, 0 );		// ACM_OPEN(NULL) = close
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::LayoutMainMenu (0x418B70)

void CHLMainDlg::LayoutMainMenu( int bInGame, int bForce )
{
	// Button-strip cell size (cell width/height of one menu button).
	int		hdr[2] = { 0, 0 };
	Launcher_HeaderSize( hdr );
	int		nWidth  = hdr[0];
	int		nHeight = hdr[1];

	int		nLeft   = Launcher_StringHeight( IDS_HLMAIN_OFFSET, 0 ) + 70;	// column left edge
	int		nRight  = g_nLauncherDefW - 10;						// help-label right edge
	int		nHelpX  = nLeft + nWidth + 10;						// help-label left edge
	int		nStep   = g_bConsoleMode ? 28 : 32;					// row pitch
	int		nWidthH = nRight - nHelpX;							// help-label width
	int		y       = 180;										// running row top

	// Early-out when the requested variant is already laid out (unless forced).
	if ( !bForce )
	{
		if ( bInGame )	{ if ( g_nMenuState == 1 ) return; }
		else			{ if ( !g_nMenuState ) return; }
	}

	::LockWindowUpdate( m_hWnd );
	g_nMenuState     = ( bInGame != 0 );
	m_bInGameMenu = bInGame;

	if ( g_bConsoleMode )
	{
		m_btnConsole.MoveWindow( nLeft, 180, nWidth, nHeight, TRUE );
		y = nStep + 180;
	}

	// - Multiplayer header (in-game only): current-game shortcut + help. ---
	//
	// Every conditional row in this ladder pairs its show with an EnableWindow;
	// the "show" is SW_RESTORE throughout, never SW_SHOWNORMAL.
	if ( bInGame )
	{
		m_btnReturnToGame.EnableWindow( TRUE );
		m_btnReturnToGame.ShowWindow( SW_RESTORE );
		m_lblReturnHelp.EnableWindow( TRUE );
		m_lblReturnHelp.ShowWindow( SW_RESTORE );

		m_btnReturnToGame.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
		SetWindowTextSafe( &m_btnReturnToGame, Launcher_LoadString( IDS_BTN_RETURN ) );
		m_lblReturnHelp.MoveWindow( nHelpX, y + 6, nRight - nHelpX, nHeight - 6, TRUE );
		m_lblReturnHelp.SetTransparent( 1 );
		m_lblReturnHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
		m_lblReturnHelp.SetFontSize( 11, FW_NORMAL );
		m_lblReturnHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_RETURNHELP ) );
		y += nStep;
	}
	else
	{
		m_btnReturnToGame.EnableWindow( FALSE );
		m_btnReturnToGame.ShowWindow( SW_HIDE );
		m_lblReturnHelp.EnableWindow( FALSE );
		m_lblReturnHelp.ShowWindow( SW_HIDE );
	}

	// The unconditional show/hide run.  It shows the readme row and the Order
	// button here and hides them again further down (sic) -- the binary really
	// does both.
	m_btnMultiplayer.ShowWindow( SW_RESTORE );
	m_lblMultiplayerHelp.ShowWindow( SW_RESTORE );
	m_btnOrderHalfLife.ShowWindow( SW_RESTORE );
	m_btnHazardCourse.ShowWindow( SW_RESTORE );
	m_lblTrainingHelp.ShowWindow( SW_RESTORE );
	m_btnCustomGame.ShowWindow( SW_RESTORE );
	m_lblCustomHelp.ShowWindow( SW_RESTORE );

	// The TFC manual is hidden here, not at the end of the ladder.
	m_btnTfcManual.ShowWindow( SW_HIDE );
	m_lblManualHelp.ShowWindow( SW_HIDE );

	m_btnConfigureHalfLife.ShowWindow( SW_RESTORE );
	m_lblConfigureHelp.ShowWindow( SW_RESTORE );

	// The top-row tab buttons and their labels are unused in this layout: hidden
	// and then disabled, in two runs of eight.
	m_btnQuickStart.ShowWindow( SW_HIDE );
	m_btnChatRooms.ShowWindow( SW_HIDE );
	m_btnInternetGames.ShowWindow( SW_HIDE );
	m_btnLanGames.ShowWindow( SW_HIDE );
	m_lblQuickStart.ShowWindow( SW_HIDE );
	m_lblLanGames.ShowWindow( SW_HIDE );
	m_lblInternetGames.ShowWindow( SW_HIDE );
	m_lblChatRooms.ShowWindow( SW_HIDE );

	m_btnQuickStart.EnableWindow( FALSE );
	m_btnChatRooms.EnableWindow( FALSE );
	m_btnInternetGames.EnableWindow( FALSE );
	m_btnLanGames.EnableWindow( FALSE );
	m_lblQuickStart.EnableWindow( FALSE );
	m_lblLanGames.EnableWindow( FALSE );
	m_lblInternetGames.EnableWindow( FALSE );
	m_lblChatRooms.EnableWindow( FALSE );

	m_btnViewReadme.ShowWindow( SW_RESTORE );
	m_lblReadmeHelp.ShowWindow( SW_RESTORE );
	m_btnPreviews.ShowWindow( SW_RESTORE );
	m_lblPreviewsHelp.ShowWindow( SW_RESTORE );
	m_btnOK.ShowWindow( SW_RESTORE );
	m_lblQuitHelp.ShowWindow( SW_RESTORE );

	// - New game ---
	m_btnNewGame.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
	SetWindowTextSafe( &m_btnNewGame, Launcher_LoadString( IDS_BTN_NEWGAME ) );
	m_lblNewGameHelp.MoveWindow( nHelpX, y + 6, nRight - nHelpX, nHeight - 6, TRUE );
	m_lblNewGameHelp.SetTransparent( 1 );
	m_lblNewGameHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblNewGameHelp.SetFontSize( 11, FW_NORMAL );
	m_lblNewGameHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_NEWGAMEHELP ) );
	y += nStep;

	// - Training ---
	m_btnHazardCourse.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
	SetWindowTextSafe( &m_btnHazardCourse, Launcher_LoadString( IDS_BTN_TRAINING ) );
	m_lblTrainingHelp.MoveWindow( nHelpX, y + 6, nWidthH, nHeight - 6, TRUE );
	m_lblTrainingHelp.SetTransparent( 1 );
	m_lblTrainingHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblTrainingHelp.SetFontSize( 11, FW_NORMAL );
	m_lblTrainingHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_TRAININGHELP ) );
	y += nStep;

	// - Configuration ---
	m_btnConfigureHalfLife.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
	SetWindowTextSafe( &m_btnConfigureHalfLife, Launcher_LoadString( IDS_BTN_CONFIGURE ) );
	m_lblConfigureHelp.MoveWindow( nHelpX, y + 6, nWidthH, nHeight - 6, TRUE );
	m_lblConfigureHelp.SetTransparent( 1 );
	m_lblConfigureHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblConfigureHelp.SetFontSize( 11, FW_NORMAL );
	m_lblConfigureHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_CONFIGUREHELP ) );
	y += nStep;

	// - Load game (front-end) / Save game (in-game): one shared row. ---
	if ( bInGame )
	{
		m_btnLoadGame.EnableWindow( FALSE );			// hide "Load game"
		m_btnLoadGame.ShowWindow( SW_HIDE );
		m_lblLoadHelp.EnableWindow( FALSE );
		m_lblLoadHelp.ShowWindow( SW_HIDE );

		m_btnLoadOrSaveGame.EnableWindow( TRUE );	// show "Save game"
		m_btnLoadOrSaveGame.ShowWindow( SW_RESTORE );
		m_lblLoadSaveHelp.EnableWindow( TRUE );
		m_lblLoadSaveHelp.ShowWindow( SW_RESTORE );

		m_btnLoadOrSaveGame.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
		SetWindowTextSafe( &m_btnLoadOrSaveGame, Launcher_LoadString( IDS_BTN_LOADSAVE ) );
		m_lblLoadSaveHelp.MoveWindow( nHelpX, y + 6, nWidthH, nHeight - 6, TRUE );
		m_lblLoadSaveHelp.SetTransparent( 1 );
		m_lblLoadSaveHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
		m_lblLoadSaveHelp.SetFontSize( 11, FW_NORMAL );
		m_lblLoadSaveHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_LOADSAVEHELP ) );
	}
	else
	{
		m_btnLoadOrSaveGame.EnableWindow( FALSE );	// hide "Save game"
		m_btnLoadOrSaveGame.ShowWindow( SW_HIDE );
		m_lblLoadSaveHelp.EnableWindow( FALSE );
		m_lblLoadSaveHelp.ShowWindow( SW_HIDE );

		m_btnLoadGame.EnableWindow( TRUE );			// show "Load game"
		m_btnLoadGame.ShowWindow( SW_RESTORE );
		m_lblLoadHelp.EnableWindow( TRUE );
		m_lblLoadHelp.ShowWindow( SW_RESTORE );

		m_btnLoadGame.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
		SetWindowTextSafe( &m_btnLoadGame, Launcher_LoadString( IDS_BTN_LOAD ) );
		m_lblLoadHelp.MoveWindow( nHelpX, y + 6, nWidthH, nHeight - 6, TRUE );
		m_lblLoadHelp.SetTransparent( 1 );
		m_lblLoadHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
		m_lblLoadHelp.SetFontSize( 11, FW_NORMAL );
		m_lblLoadHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_LOADHELP ) );
	}
	y += nStep;

	// - Multiplayer (taller help: 4 extra text lines). ---
	m_btnMultiplayer.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
	SetWindowTextSafe( &m_btnMultiplayer, Launcher_LoadString( IDS_BTN_MULTIPLAYER ) );
	m_lblMultiplayerHelp.MoveWindow( nHelpX, y + 6, nWidthH, 4 * Launcher_StringHeight( IDS_GERMAN, 0 ) + nHeight, TRUE );
	m_lblMultiplayerHelp.SetTransparent( 1 );
	m_lblMultiplayerHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblMultiplayerHelp.SetFontSize( 11, FW_NORMAL );
	m_lblMultiplayerHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_MULTIPLAYERHELP ) );
	y += nStep;

	// - Custom game (opens the mod list / "find updates"). ---
	m_btnCustomGame.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
	SetWindowTextSafe( &m_btnCustomGame, Launcher_LoadString( IDS_BTN_CUSTOMGAME ) );
	m_lblCustomHelp.MoveWindow( nHelpX, y + 6, nWidthH, nHeight + 2, TRUE );
	m_lblCustomHelp.SetTransparent( 1 );
	m_lblCustomHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblCustomHelp.SetFontSize( 11, FW_NORMAL );
	m_lblCustomHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_CUSTOMHELP ) );
	y += nStep;

	// The Order button was shown in the run above; it is taken back out here.
	m_btnOrderHalfLife.EnableWindow( FALSE );
	m_btnOrderHalfLife.ShowWindow( SW_HIDE );
	m_lblOrderHelp.EnableWindow( FALSE );
	m_lblOrderHelp.ShowWindow( SW_HIDE );

	// - Readme row (front-end only). ---
	if ( bInGame )
	{
		m_btnViewReadme.EnableWindow( FALSE );
		m_btnViewReadme.ShowWindow( SW_HIDE );
		m_lblReadmeHelp.EnableWindow( FALSE );
		m_lblReadmeHelp.ShowWindow( SW_HIDE );
	}
	else
	{
		m_btnViewReadme.EnableWindow( TRUE );
		m_btnViewReadme.ShowWindow( SW_RESTORE );
		m_lblReadmeHelp.EnableWindow( TRUE );
		m_lblReadmeHelp.ShowWindow( SW_RESTORE );

		m_btnViewReadme.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
		SetWindowTextSafe( &m_btnViewReadme, Launcher_LoadString( IDS_BTN_README ) );
		m_lblReadmeHelp.MoveWindow( nHelpX, y + 6, nWidthH, nHeight - 6, TRUE );
		m_lblReadmeHelp.SetTransparent( 1 );
		m_lblReadmeHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
		m_lblReadmeHelp.SetFontSize( 11, FW_NORMAL );
		m_lblReadmeHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_READMEHELP ) );
		y += nStep;
	}

	// - Previews (media URL). ---
	m_btnPreviews.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
	SetWindowTextSafe( &m_btnPreviews, Launcher_LoadString( IDS_BTN_PREVIEWS ) );
	m_lblPreviewsHelp.MoveWindow( nHelpX, y + 6, nWidthH, nHeight - 6, TRUE );
	m_lblPreviewsHelp.SetTransparent( 1 );
	m_lblPreviewsHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblPreviewsHelp.SetFontSize( 11, FW_NORMAL );
	m_lblPreviewsHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_PREVIEWSHELP ) );
	y += nStep;

	// - Quit (IDOK). ---
	m_btnOK.MoveWindow( nLeft, y, nWidth, nHeight, TRUE );
	SetWindowTextSafe( &m_btnOK, Launcher_LoadString( IDS_BTN_QUIT ) );
	// The Quit help's right edge is a hardcoded 472 in the binary, not nRight. (sic)
	m_lblQuitHelp.MoveWindow( nHelpX, y + 6, 472 - nHelpX, nHeight - 6, TRUE );
	m_lblQuitHelp.SetTransparent( 1 );
	m_lblQuitHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblQuitHelp.SetFontSize( 11, FW_NORMAL );
	m_lblQuitHelp.SetWindowText( Launcher_LoadString( IDS_MAIN_QUITHELP ) );

	// Refresh button captions/enables, then disable the single-player actions
	// (New game / Training / Load / Save) for multiplayer-only mods.
	if ( m_hWnd )
	{
		CreateLogoAnim();

		const char*	pszType = g_pCurrentMod ? g_pCurrentMod->GetKey( "type" ) : NULL;

		if ( pszType && *pszType && !_stricmp( pszType, "multiplayer_only" ) )
		{
			m_btnHazardCourse.SetHighlight( 1 );
			m_btnNewGame.SetHighlight( 1 );
			m_btnLoadGame.SetHighlight( 1 );
			m_btnLoadOrSaveGame.SetHighlight( 1 );
		}
		else
		{
			m_btnNewGame.SetHighlight( 0 );
			m_btnHazardCourse.SetHighlight( 0 );
			m_btnLoadGame.SetHighlight( 0 );
			m_btnLoadOrSaveGame.SetHighlight( 0 );
		}
	}

	::InvalidateRect( m_hWnd, NULL, TRUE );
	::LockWindowUpdate( NULL );
	::RedrawWindow( m_hWnd, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::LaunchGameCmd (0x4198A0)

void CHLMainDlg::LaunchGameCmd( int bSetState, int nReserved, const char* pszFmt, ... )
{
	if ( m_bLogoOpened && m_pLogoAnim )
	{
		::ShowWindow( m_pLogoAnim->GetSafeHwnd(), SW_HIDE );
		::UpdateWindow( m_pLogoAnim->GetSafeHwnd() );
	}

	if ( Launcher_StartEngine( 0 ) )
	{
		if ( pszFmt && *pszFmt )
		{
			char	buf[1024];
			va_list	va;
			va_start( va, pszFmt );
			vsprintf( buf, pszFmt, va );
			va_end( va );
			strcat( buf, "\n" );

			LOG( "LaunchGameCmd: gDLLState=%d Cbuf_AddText=%p cmd='%s'",
				gDLLState, (void*)engineapi.Cbuf_AddText, buf );

			if ( gDLLState != DLL_CLOSE && engineapi.Cbuf_AddText )
				engineapi.Cbuf_AddText( buf );
		}
		if ( bSetState )
		{
			Eng_Frame( 1 );
			Eng_GameSetState( DLL_PAUSED );
		}
	}
	else
	{
		gDLLState = DLL_INACTIVE;
		Eng_Load( NULL, 0 );
		VID_HideEngineWindow();
		if ( m_bLogoOpened && m_pLogoAnim )
		{
			ShowMenuPage( 1 );
			::SendMessageA( m_pLogoAnim->GetSafeHwnd(), ACM_PLAY, (WPARAM)-1, (LPARAM)0xFFFF0000 );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::ShowMultiplayerPage (0x4199D0)

int  CHLMainDlg::ShowMultiplayerPage( int bSetState, int nReserved )
{
	LaunchGameCmd( bSetState, nReserved, NULL );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnConsole (0x4199F0)
//
// cmd 1167 / IDC_MAIN_CONSOLE

void CHLMainDlg::OnConsole()
{
	OnMultiplayer();
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnShowWindow (0x419A00)

void CHLMainDlg::OnShowWindow( BOOL bShow, UINT nStatus )
{
	int		wh[2];

	if ( nStatus )
	{
		Default();
		return;
	}

	LockWindowUpdate();
	if ( bShow )
	{
		Dlg_CenterWindow( this );
		LayoutMainMenu( m_bInGameMenu, 0 );
		Launcher_LoadMainButtonsBitmap();
		ShowMenuPage( 1 );
		Launcher_LoadSplashBitmap();			// reload gfx/shell/splash.bmp behind the menu
		// The strip work is inlined here rather than delegated to
		// RefreshDialogSkin, which also creates the logo animation and reskins the
		// title-bar glyphs -- neither of which this handler does.
		m_hMenuStrip = Launcher_HeaderLoaded();
		Launcher_HeaderSize( wh );
		m_menuCell.cx    = wh[0];
		m_menuCell.cy    = wh[1];
		m_nMenuStripRows = Launcher_HeaderStride();

		if ( m_hMenuStrip )
		{
			m_btnNewGame.SetDIBData( m_menuCell, BTNSTRIP_NEW_GAME, m_hMenuStrip );
			m_btnReturnToGame.SetDIBData( m_menuCell, BTNSTRIP_RESUME_GAME, m_hMenuStrip );
			m_btnHazardCourse.SetDIBData( m_menuCell, BTNSTRIP_HAZARD_COURSE, m_hMenuStrip );
			m_btnLoadGame.SetDIBData( m_menuCell, BTNSTRIP_LOAD_GAME, m_hMenuStrip );
			m_btnLoadOrSaveGame.SetDIBData( m_menuCell, BTNSTRIP_LOAD_SAVE_GAME, m_hMenuStrip );
			m_btnConfigureHalfLife.SetDIBData( m_menuCell, BTNSTRIP_CONFIGURATION, m_hMenuStrip );
			m_btnQuickStart.SetDIBData( m_menuCell, BTNSTRIP_QUICK_START, m_hMenuStrip );
			m_btnChatRooms.SetDIBData( m_menuCell, BTNSTRIP_CHAT_ROOMS, m_hMenuStrip );
			m_btnInternetGames.SetDIBData( m_menuCell, BTNSTRIP_INTERNET_GAMES, m_hMenuStrip );
			m_btnLanGames.SetDIBData( m_menuCell, BTNSTRIP_LAN_GAMES, m_hMenuStrip );
			m_btnViewReadme.SetDIBData( m_menuCell, BTNSTRIP_VIEW_README, m_hMenuStrip );
			m_btnPreviews.SetDIBData( m_menuCell, BTNSTRIP_PREVIEWS, m_hMenuStrip );
			m_btnOK.SetDIBData( m_menuCell, BTNSTRIP_OK_NARROW, m_hMenuStrip );
			m_btnMultiplayer.SetDIBData( m_menuCell, BTNSTRIP_MULTIPLAYER, m_hMenuStrip );
			m_btnConsole.SetDIBData( m_menuCell, BTNSTRIP_CONSOLE, m_hMenuStrip );
			m_btnCustomGame.SetDIBData( m_menuCell, BTNSTRIP_CUSTOM_GAME, m_hMenuStrip );
		}

		// Three different orders, none of them RefreshDialogSkin's. (sic)
		m_btnConsole.FreeSkinBitmaps();
		m_btnNewGame.FreeSkinBitmaps();
		m_btnReturnToGame.FreeSkinBitmaps();
		m_btnHazardCourse.FreeSkinBitmaps();
		m_btnLoadGame.FreeSkinBitmaps();
		m_btnLoadOrSaveGame.FreeSkinBitmaps();
		m_btnConfigureHalfLife.FreeSkinBitmaps();
		m_btnQuickStart.FreeSkinBitmaps();
		m_btnChatRooms.FreeSkinBitmaps();
		m_btnInternetGames.FreeSkinBitmaps();
		m_btnLanGames.FreeSkinBitmaps();
		m_btnViewReadme.FreeSkinBitmaps();
		m_btnPreviews.FreeSkinBitmaps();
		m_btnTfcManual.FreeSkinBitmaps();
		m_btnOK.FreeSkinBitmaps();
		m_btnMultiplayer.FreeSkinBitmaps();

		::RedrawWindow( m_btnConsole.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnNewGame.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnReturnToGame.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnHazardCourse.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnLoadGame.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnLoadOrSaveGame.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnConfigureHalfLife.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnViewReadme.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnQuickStart.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnChatRooms.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnInternetGames.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnLanGames.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnPreviews.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnTfcManual.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnOK.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
		::RedrawWindow( m_btnMultiplayer.m_hWnd, NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );
	}
	else
	{
		Launcher_FreeMainButtonsBitmap();							// free the button strip
		ShowMenuPage( 0 );
		Launcher_FreeSplashBitmap();								// drop the background while hidden
	}
	UnlockWindowUpdate();
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnReportError (0x419E20)
//
// cmd 121

void CHLMainDlg::OnReportError()
{
	if ( Launcher_GetErrorState() == 1 )
	{
		Launcher_ErrorMessageBox( 0, Launcher_GetErrorMessage() );
		exit( -1 );
	}

	Launcher_GetErrorState();		// unused call

	Launcher_ErrorMessageBox( 0, Launcher_GetErrorMessage() );
	Launcher_SetErrorMessage( "" );
	Launcher_SetErrorState( 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::InGame (0x419E70)

int CHLMainDlg::InGame()
{
	int			bActiveOrPaused = ( gDLLState == DLL_PAUSED || gDLLState == DLL_ACTIVE );
	GameInfo_t	gi;

	// probably not in game when these conditions are met.
	if ( ( gDLLState == DLL_CLOSE || gDLLState == DLL_INACTIVE
	    || ( engineapi.GetGameInfo( &gi, 0 )  && gi.state != ca_disconnected ) )
	    && bActiveOrPaused )
		return bActiveOrPaused;

	return g_bInGameCached;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnSysCommand (0x419EE0)
//
// The screensaver and the window move are both swallowed outright; the close box
// goes through the Quit prompt instead of tearing the dialog down.

void CHLMainDlg::OnSysCommand( UINT nID, LPARAM lParam )
{
	if ( nID == SC_SCREENSAVE )
		return;

	if ( nID == SC_CLOSE )
	{
		OnOK();				// through slot 49, not EndDialog
		return;
	}

	if ( nID == SC_MOVE )
		return;

	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnMinimizeButton (0x419F10)
//
// cmd 1173

void CHLMainDlg::OnMinimizeButton()
{
	ShowWindow( SW_MINIMIZE );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnPreviews (0x419F20)
//
// cmd 1024

void CHLMainDlg::OnPreviews()
{
	char	szFile[260];
	HINSTANCE	hResult;

	if ( !Launcher_BuildResourcePath( IDS_MEDIA_PREVIEWURL, szFile ) )
		return;

	hResult = ShellExecuteA( ::GetFocus(), "open", szFile, NULL, NULL, SW_SHOWNORMAL );
	if ( (int)hResult <= HINSTANCE_ERROR )
		Launcher_ShowMessageByIdEx( 0, IDS_URL_BROWSERFAIL, szFile );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnSysKeyUp (0x419FA0)
//
// Alt+Tab and Alt+Esc out of a fullscreen game: the counterpart to the
// WM_SYSKEYDOWN that LauncherKeyboardHook posts.  Windowed mode is left alone,
// since the shell can switch away from it by itself.

void CHLMainDlg::OnSysKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( ( nChar == VK_TAB || nChar == VK_ESCAPE )
	  && ( nFlags & KF_ALTDOWN )
	  && ( nFlags & KF_UP ) )
	{
		if ( !gEngineModeWindowed )
			ShowWindow( SW_MINIMIZE );
	}

	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnCustomGame (0x41A000)
//
// cmd 1025

void CHLMainDlg::OnCustomGame()
{
	ShowMenuPage( 0 );
	CModDlg	page( this );
	InitChildDialog( &page, &m_btnCustomGame );
	page.DoModal();
	m_lastActivity = GetTickCount();
	RestoreAfterModal();
	ShowMenuPage( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnFriends (0x41A0A0)
//
// cmd 1221
// The whole body is one call to the stripped no-op.

void CHLMainDlg::OnFriends()
{
	NullStub( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::RefreshAfterConfig (0x41A0B0)

void CHLMainDlg::RefreshAfterConfig()
{
	RefreshDialogSkin();
	LayoutMainMenu( InGame(), 1 );
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
	NullStub( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnTfcManual (0x41A100)
//
// cmd 1029

void CHLMainDlg::OnTfcManual()
{
	char	file[260];
	HINSTANCE	hResult;

	if ( !Launcher_BuildResourcePath( IDS_TFCMANUAL_URL, file ) )
		return;

	hResult = ShellExecuteA( ::GetFocus(), "open", file, NULL, NULL, SW_SHOWMAXIMIZED );
	if ( (int)hResult <= HINSTANCE_ERROR )
		Launcher_ShowMessageByIdEx( 0, IDS_URL_BROWSERFAIL, file );

	ShowWindow( SW_MINIMIZE );
}

#ifdef LAUNCHER_RE
/*
==================
Launcher_UpdateShellAfterEngine

Not original.
==================
*/
void Launcher_UpdateShellAfterEngine( void )
{
	CWinThread*	pThread = AfxGetThread();

	if ( pThread )
	{
		CWnd*	pMainWnd = pThread->GetMainWnd();

		if ( pMainWnd )
		{
			CHLMainDlg*	pMainDlg = (CHLMainDlg*)pMainWnd;
			pMainDlg->LayoutMainMenu( CHLMainDlg::InGame(), 1 );
			if ( pMainDlg->m_pLogoAnim )
				pMainDlg->m_pLogoAnim->ShowWindow( SW_HIDE );
		}
	}
}
#endif

/*
==================
NullStub (0x40E460)

The 0-argument and 1-argument forms are two of the several distinct empty
functions the linker folded onto this address.
==================
*/
void NullStub( void )
{
}

/*
==================
NullStub (0x40E460)

The 1-argument form is folded onto the same address.
==================
*/
void NullStub( int /*flag*/ )
{
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnUnusedButton (0x40E460)
//
// cmds 1020/1023/1026/1027/1070

void CHLMainDlg::OnUnusedButton()
{
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnPaint (0x412860)

void CHLMainDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnEraseBkgnd (0x412870)

BOOL CHLMainDlg::OnEraseBkgnd( CDC* pDC )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnSize (0x455E00)
//
// Folded across seven handler slots in the binary; the body is `Default()`, not
// a base call.

void CHLMainDlg::OnSize( UINT nType, int cx, int cy )
{
#ifdef LAUNCHER_FIXES
	// A fullscreen shell owns the display mode, so minimising has to give it
	// back -- otherwise the desktop stays at 640x480 behind the user's back --
	// and coming back has to take it again and lay out over it.
	if ( Shell_FullscreenActive() && m_bReady )
	{
		if ( nType == SIZE_MINIMIZED )
		{
			Shell_LeaveFullscreen();
			Shell_ShowBackdrop( 0 );
		}
		else if ( nType == SIZE_RESTORED )
		{
			Shell_EnterFullscreen();
			if ( !Shell_HasFullscreenMode() )
				Shell_ShowBackdrop( 1 );
			Dlg_CenterWindow( this );
			LayoutMainMenu( InGame(), 1 );
		}
	}
#endif

	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg::OnCloseButton (0x45D6A0)
//
// cmd 1174

void CHLMainDlg::OnCloseButton()
{
	OnOK();
}
