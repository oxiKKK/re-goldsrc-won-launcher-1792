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
// Purpose: the Load or Save Game page (CLoadSaveDlg).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveDlg::CLoadSaveDlg (0x426D70)

CLoadSaveDlg::CLoadSaveDlg( CWnd* pParent )
	: CDlgBase( IDD_LOADSAVE, pParent )
{
	int	dims[2];

	m_pSelfWnd = this;		// +204 -- gates the slide transition
	LoadHeaderBitmap( "head_saveload", 0 );
	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnRow1.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_ROW_ODD, m_headerLoaded );
		m_btnRow0.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_ROW_EVEN, m_headerLoaded );
		m_btnBack.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DONE, m_headerLoaded );
	}
}

BEGIN_MESSAGE_MAP( CLoadSaveDlg, CDialog )
	ON_COMMAND( IDC_LOADSAVE_LOAD_GAME, OnLoadGame )
	ON_COMMAND( IDC_LOADSAVE_SAVE_GAME, OnSaveGame )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveDlg::DoDataExchange (0x426FA0)

void CLoadSaveDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_LOADSAVE_RETURN, m_lblRow2 );
	DDX_Control( pDX, IDC_LOADSAVE_SAVEHELP, m_lblRow1 );
	DDX_Control( pDX, IDC_LOADSAVE_HINT, m_field );
	DDX_Control( pDX, IDC_LOADSAVE_LOADHELP, m_lblRow0 );
	DDX_Control( pDX, IDCANCEL,    m_btnBack );
	DDX_Control( pDX, IDC_LOADSAVE_SAVE_GAME,   m_btnRow1 );
	DDX_Control( pDX, IDC_LOADSAVE_LOAD_GAME,   m_btnRow0 );
}

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveDlg::OnLoadGame (0x427040)

void CLoadSaveDlg::OnLoadGame()
{
	ShowWindow( SW_RESTORE );

	CLoadDlg	page;
	InitChildDialog( &page, &m_btnRow0 );
	page.DoModal();
	RestoreAfterModal();

	// A straight launch out of the load page leaves the engine owning the screen,
	// so this page closes behind it instead of coming back up.
	if ( gTopLevelFrame )
		OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveDlg::OnSaveGame (0x4270E0)

void CLoadSaveDlg::OnSaveGame()
{
	ShowWindow( SW_RESTORE );

	CSaveDlg	page;
	InitChildDialog( &page, &m_btnRow1 );
	page.DoModal();
	RestoreAfterModal();

	if ( gTopLevelFrame )
		OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CLoadSaveDlg::OnInitDialog (0x427180)

BOOL CLoadSaveDlg::OnInitDialog()
{
	GameInfo_t	gi;
	int		dims[2];
	int		w, h, ctlX, ctlW, right;
	char	szName[260];

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	Launcher_HeaderSize( dims );		// the shared button-cell size {w,h}
	w = dims[0];
	h = dims[1];
	ctlX = w + 60;
	right = g_nLauncherDefW - 10;
	ctlW = right - ctlX;

	// Three caption/value rows stacked under the header.
	m_btnRow0.MoveWindow( 50, 140, w, h, TRUE );
	m_btnRow0.SetWindowText( Launcher_LoadString( IDS_BTN_LOAD ) );
	m_lblRow0.MoveWindow( ctlX, 146, right - ctlX, h - 6, TRUE );
	m_lblRow0.SetTransparent( TRUE );
	m_lblRow0.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblRow0.SetFontSize( 11, FW_NORMAL );
	m_lblRow0.SetWindowText( Launcher_LoadString( IDS_LOADSAVE_LOADHELP ) );

	m_btnRow1.MoveWindow( 50, 172, w, h, TRUE );
	m_btnRow1.SetWindowText( Launcher_LoadString( IDS_BTN_SAVE ) );
	m_lblRow1.MoveWindow( ctlX, 178, ctlW, h - 6, TRUE );
	m_lblRow1.SetTransparent( TRUE );
	m_lblRow1.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblRow1.SetFontSize( 11, FW_NORMAL );
	m_lblRow1.SetWindowText( Launcher_LoadString( IDS_LOADSAVE_SAVEHELP ) );

	m_btnBack.MoveWindow( 50, 204, w, h, TRUE );
	m_btnBack.SetWindowText( Launcher_LoadString( IDS_BTN_DONE ) );
	m_lblRow2.MoveWindow( ctlX, 210, ctlW, h - 6, TRUE );
	m_lblRow2.SetTransparent( TRUE );
	m_lblRow2.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblRow2.SetFontSize( 11, FW_NORMAL );
	m_lblRow2.SetWindowText( Launcher_LoadString( IDS_LOADSAVE_RETURN ) );

	// The multi-line player-name field (double height, no caption).
	m_field.MoveWindow( ctlX, 242, ctlW, 2 * h - 6, TRUE );
	m_field.SetTransparent( 1 );
	m_field.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_field.SetFontSize( 11, FW_NORMAL );

	// Show which keys are bound to quicksave/quickload in the field help text
	// (Launcher_GetPlayerName fills the two key-name buffers; str 0x4D formats them).
	char	saveKey[64], loadKey[64];
	if ( Launcher_GetPlayerName( g_pServerBrowser->m_playerConfig.m_binds, saveKey, loadKey ) )
	{
		Launcher_LoadStringInto( szName, IDS_LOADSAVE_HINT, saveKey, loadKey );
		m_field.SetWindowText( szName );
	}

	// The Save row is dimmed unless a game is actually running.
	if ( engineapi.GetGameInfo( &gi, 0 ) && ( gi.state != ca_active || !gi.active ) )
		m_btnRow1.SetHighlight( 1 );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveDlg::~CLoadSaveDlg (0x426EE0)

CLoadSaveDlg::~CLoadSaveDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveDlg::OnActivateApp (0x406FE0)

void CLoadSaveDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveDlg::OnPaint (0x412860)

void CLoadSaveDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CLoadSaveDlg::OnEraseBkgnd (0x412870)

BOOL CLoadSaveDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}
