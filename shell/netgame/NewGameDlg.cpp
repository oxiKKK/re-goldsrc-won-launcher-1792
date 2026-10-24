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
// Purpose: the "New Game" skill-select page (CNewGameDlg, IDD 204).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/////////////////////////////////////////////////////////////////////////////
// CNewGameDlg::CNewGameDlg (0x43E030)

CNewGameDlg::CNewGameDlg( CWnd* pParent )
	: CDlgBase( IDD_NEWGAME, pParent )
{
	int	dims[2];

	m_pSelfWnd = this;
	LoadHeaderBitmap( "head_newgame", NULL );
	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnEasy.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_EASY, m_headerLoaded );
		m_btnMedium.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_MEDIUM, m_headerLoaded );
		m_btnDifficult.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DIFFICULT, m_headerLoaded );
		m_btnDone.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_BACK, m_headerLoaded );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNewGameDlg::~CNewGameDlg (0x43E1D0)

CNewGameDlg::~CNewGameDlg()
{
}

BEGIN_MESSAGE_MAP( CNewGameDlg, CDialog )
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_COMMAND( IDC_NEWGAME_EASY,      &CNewGameDlg::OnSkillEasy )
	ON_COMMAND( IDC_NEWGAME_DIFFICULT, &CNewGameDlg::OnSkillHard )
	ON_COMMAND( IDC_NEWGAME_MEDIUM,    &CNewGameDlg::OnSkillMedium )
	ON_WM_ACTIVATEAPP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewGameDlg::DoDataExchange (0x43E2A0)

void CNewGameDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_NEWGAME_MEDIUMHELP, m_lblMediumHelp );
	DDX_Control( pDX, IDC_NEWGAME_EASYHELP, m_lblEasyHelp );
	DDX_Control( pDX, IDC_NEWGAME_DIFFICULTHELP, m_lblDifficultHelp );
	DDX_Control( pDX, IDC_NEWGAME_RETURNHELP, m_lblReturnHelp );
	DDX_Control( pDX, IDC_NEWGAME_EASY,   m_btnEasy );
	DDX_Control( pDX, IDC_NEWGAME_MEDIUM,   m_btnMedium );
	DDX_Control( pDX, IDC_NEWGAME_DIFFICULT, m_btnDifficult );
	DDX_Control( pDX, IDCANCEL,    m_btnDone );	// cmd 2 = IDCANCEL ("Done")
}


/////////////////////////////////////////////////////////////////////////////
// CNewGameDlg::OnInitDialog (0x43E350)

BOOL CNewGameDlg::OnInitDialog()
{
	int	dims[2];

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	Launcher_HeaderSize( dims );
	int	w = dims[0], h = dims[1];
	int	ctlX = w + 60;
	int	rowW = ( g_nLauncherDefW - 10 ) - ctlX;

	m_btnEasy.MoveWindow( 50, 140, w, h, TRUE );
	m_btnEasy.SetWindowText( Launcher_LoadString( IDS_BTN_EASY ) );
	m_lblEasyHelp.MoveWindow( ctlX, 146, rowW, h - 6, TRUE );
	m_lblEasyHelp.SetTransparent( TRUE );
	m_lblEasyHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblEasyHelp.SetFontSize( 11, FW_NORMAL );
	m_lblEasyHelp.SetWindowText( Launcher_LoadString( IDS_NEWGAME_EASYHELP ) );

	m_btnMedium.MoveWindow( 50, 172, w, h, TRUE );
	m_btnMedium.SetWindowText( Launcher_LoadString( IDS_BTN_MEDIUM ) );
	m_lblMediumHelp.MoveWindow( ctlX, 178, rowW, h - 6, TRUE );
	m_lblMediumHelp.SetTransparent( TRUE );
	m_lblMediumHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblMediumHelp.SetFontSize( 11, FW_NORMAL );
	m_lblMediumHelp.SetWindowText( Launcher_LoadString( IDS_NEWGAME_MEDIUMHELP ) );

	m_btnDifficult.MoveWindow( 50, 204, w, h, TRUE );
	m_btnDifficult.SetWindowText( Launcher_LoadString( IDS_BTN_HARD ) );
	m_lblDifficultHelp.MoveWindow( ctlX, 210, rowW, h - 6, TRUE );
	m_lblDifficultHelp.SetTransparent( TRUE );
	m_lblDifficultHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblDifficultHelp.SetFontSize( 11, FW_NORMAL );
	m_lblDifficultHelp.SetWindowText( Launcher_LoadString( IDS_NEWGAME_DIFFICULTHELP ) );

	m_btnDone.MoveWindow( 50, 236,
		Launcher_StringHeight( IDS_NEWGAMEDLG_OFFSET, 0 ) + w, h, TRUE );
	m_btnDone.SetWindowText( Launcher_LoadString( IDS_BTN_CANCEL ) );
	m_lblReturnHelp.MoveWindow( ctlX, 242, rowW, h - 6, TRUE );
	m_lblReturnHelp.SetTransparent( TRUE );
	m_lblReturnHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblReturnHelp.SetFontSize( 11, FW_NORMAL );
	m_lblReturnHelp.SetWindowText( Launcher_LoadString( IDS_NEWGAME_RETURNHELP ) );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNewGameDlg::OnSkillEasy (0x43E630)

void CNewGameDlg::OnSkillEasy()
{
	StartNewGame( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CNewGameDlg::OnSkillMedium (0x43E640)

void CNewGameDlg::OnSkillMedium()
{
	StartNewGame( 2 );
}

/////////////////////////////////////////////////////////////////////////////
// CNewGameDlg::OnSkillHard (0x43E650)

void CNewGameDlg::OnSkillHard()
{
	StartNewGame( 3 );
}

/////////////////////////////////////////////////////////////////////////////
// CNewGameDlg::StartNewGame (0x43E660)

void CNewGameDlg::StartNewGame( int skill )
{
	GameInfo_t	gi;
	int			bDisconnect = 0;

	if ( engineapi.GetGameInfo( &gi, 0 ) && gi.state >= 2 )
	{
		// A game is already running: confirm a disconnect via the skinned
		// prompt (style 2 = OK + Cancel).  Bail unless OK is chosen.
		CPromptDlg	dlg( 2, NULL );
		dlg.SetMessage( Launcher_LoadString( IDS_NEWGAME_NEWPROMPT ) );
		if ( dlg.DoModal() != IDOK )
			return;
		bDisconnect = 1;
	}

	const char*	startmap = NULL;
	if ( g_pCurrentMod )
		startmap = g_pCurrentMod->GetKey( "startmap" );
	if ( !startmap )
		startmap = "c0a0";

	char	cmd[2048];
	if ( bDisconnect )
		sprintf( cmd, "disconnect\nskill %i\nmap %s\n", skill, startmap );
	else
		sprintf( cmd, "skill %i\nmap %s\n", skill, startmap );

	AFXSetTopLevelFrame( 1 );
	Launcher_RunMapCommand( cmd );
	OnOK();		// CDialog::OnOK -> EndDialog(IDOK); deferred launch loads the engine
}

/////////////////////////////////////////////////////////////////////////////
// CNewGameDlg::OnPaint (0x412860)

void CNewGameDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CNewGameDlg::OnEraseBkgnd (0x412870)

BOOL CNewGameDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNewGameDlg::OnActivateApp (0x406FE0)

void CNewGameDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}
