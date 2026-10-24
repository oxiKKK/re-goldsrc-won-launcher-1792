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
// Purpose: the Configuration sub-dialog (CConfigureDlg, IDD_CONFIGURE = 160).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

BEGIN_MESSAGE_MAP( CConfigureDlg, CDialog )
	//{{AFX_MSG_MAP(CConfigureDlg)
	ON_MESSAGE( WM_DISPLAYCHANGE, OnDisplayChange )
	ON_COMMAND( IDC_CONFIGURE_VIDEO, OnVideo )
	ON_COMMAND( IDC_CONFIGURE_AUDIO, OnAudio )
	ON_COMMAND( IDC_CONFIGURE_CONTROLS, OnControls )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
	ON_COMMAND( IDC_BTN_AUTOPATCH, OnAutopatch )
	ON_COMMAND( IDC_BTN_GORE, OnGore )
	ON_COMMAND( IDC_CONFIGURE_CUSTOMIZE, OnCustomize )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::CConfigureDlg (0x404840)

CConfigureDlg::CConfigureDlg( CWnd* pParent )
	: CDlgBase( IDD_CONFIGURE, pParent )
{
	int	dims[2];

	m_pSelfWnd = this;
	LoadHeaderBitmap( "head_config", NULL );
	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnVideo.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_VIDEO, m_headerLoaded );
		m_btnAudio.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_AUDIO, m_headerLoaded );
		m_btnControls.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_CONTROLS, m_headerLoaded );
		m_btnDone.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DONE, m_headerLoaded );
		m_btnGore.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_GORE, m_headerLoaded );
		m_btnAutopatch.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_AUTO_UPDATE, m_headerLoaded );
		m_btnCustomize.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_CUSTOMIZE, m_headerLoaded );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::~CConfigureDlg (0x404A90)

CConfigureDlg::~CConfigureDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::DoDataExchange (0x404BD0)

void CConfigureDlg::DoDataExchange( CDataExchange* pDX )
{
	//{{AFX_DATA_MAP(CConfigureDlg)
	DDX_Control( pDX, IDC_MULTI_CUSTOMIZE,          m_lblCustomize );
	DDX_Control( pDX, IDC_CONFIGURE_CUSTOMIZE,      m_btnCustomize );
	DDX_Control( pDX, IDC_CONFIGURE_AUTOPATCHHELP,  m_lblAutopatchHelp );
	DDX_Control( pDX, IDC_CONFIGURE_GORE,           m_lblGore );
	DDX_Control( pDX, IDC_BTN_AUTOPATCH,            m_btnAutopatch );
	DDX_Control( pDX, IDC_BTN_GORE,                 m_btnGore );
	DDX_Control( pDX, IDC_CFG_RETURNTOMAIN,         m_lblReturnToMain );
	DDX_Control( pDX, IDC_CFG_VIDHELP,              m_lblVidHelp );
	DDX_Control( pDX, IDC_CFG_CONTROLHELP,          m_lblControlHelp );
	DDX_Control( pDX, IDC_CFG_AUDIOHELP,            m_lblAudioHelp );
	DDX_Control( pDX, IDOK,                         m_btnDone );
	DDX_Control( pDX, IDC_CONFIGURE_VIDEO,          m_btnVideo );
	DDX_Control( pDX, IDC_CONFIGURE_CONTROLS,       m_btnControls );
	DDX_Control( pDX, IDC_CONFIGURE_AUDIO,          m_btnAudio );
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnVideo (0x404CE0)
//
// Each caption button restores the page, flies its sub-page out of the button,
// runs it modally, then restores.

void CConfigureDlg::OnVideo()
{
	CVidSelectDlg	page;

	ShowWindow( SW_RESTORE );
	InitChildDialog( &page, &m_btnVideo );
	page.DoModal();
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnAudio (0x404D70)

void CConfigureDlg::OnAudio()
{
	CAudioDlg	page;

	ShowWindow( SW_RESTORE );
	InitChildDialog( &page, &m_btnAudio );
	page.DoModal();
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnControls (0x404E00)

void CConfigureDlg::OnControls()
{
	CKeyboardDlg	page;

	ShowWindow( SW_RESTORE );
	InitChildDialog( &page, &m_btnControls );
	page.DoModal();
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnInitDialog (0x404E90)

BOOL CConfigureDlg::OnInitDialog()
{
	int		dims[2];
	int		w, h, rowW, ctlX;
	int		extra, y4, y5;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	Launcher_HeaderSize( dims );
	w = dims[0];
	h = dims[1];
	ctlX = w + 60;
	rowW = ( g_nLauncherDefW - 10 ) - ctlX;

	// Row 0.
	m_btnControls.MoveWindow( 50, 140, w, h, TRUE );
	SetWindowTextSafe( &m_btnControls, Launcher_LoadString( IDS_BTN_CONTROLS ) );
	m_lblControlHelp.MoveWindow( ctlX, 146, rowW, h - 6, TRUE );
	m_lblControlHelp.SetTransparent( TRUE );
	m_lblControlHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblControlHelp.SetFontSize( 11, FW_NORMAL );
	m_lblControlHelp.SetWindowText( Launcher_LoadString( IDS_CFG_CONTROLHELP ) );

	// The hidden label + button pair.
	m_btnCustomize.ShowWindow( SW_HIDE );
	m_btnCustomize.EnableWindow( FALSE );
	m_lblCustomize.ShowWindow( SW_HIDE );
	m_lblCustomize.EnableWindow( FALSE );

	// Row 1.
	m_btnAudio.MoveWindow( 50, 172, w, h, TRUE );
	SetWindowTextSafe( &m_btnAudio, Launcher_LoadString( IDS_BTN_AUDIO ) );
	m_lblAudioHelp.MoveWindow( ctlX, 178, rowW, h - 6, TRUE );
	m_lblAudioHelp.SetTransparent( TRUE );
	m_lblAudioHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblAudioHelp.SetFontSize( 11, FW_NORMAL );
	m_lblAudioHelp.SetWindowText( Launcher_LoadString( IDS_CFG_AUDIOHELP ) );

	// Row 2 (value sized by the unwrapped help-string height).
	m_btnVideo.MoveWindow( 50, 204, w, h, TRUE );
	SetWindowTextSafe( &m_btnVideo, Launcher_LoadString( IDS_BTN_VIDEO ) );
	m_lblVidHelp.MoveWindow( ctlX, 210, rowW,
		Launcher_StringHeight( IDS_CONFIGURE_OFFSET, 0 ) + h - 6, TRUE );
	m_lblVidHelp.SetTransparent( TRUE );
	m_lblVidHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblVidHelp.SetFontSize( 11, FW_NORMAL );
	m_lblVidHelp.SetWindowText( Launcher_LoadString( IDS_CFG_VIDHELP ) );

	// Row 3 (value sized by the wrapped help-string height).
	m_btnGore.MoveWindow( 50, 236, w, h, TRUE );
	SetWindowTextSafe( &m_btnGore, Launcher_LoadString( IDS_BTN_GORE ) );
	extra = Launcher_StringHeight( IDS_CONFIGURE_OFFSET, 1 );
	m_lblGore.MoveWindow( ctlX, 242, rowW, extra + h - 6, TRUE );
	m_lblGore.SetTransparent( TRUE );
	m_lblGore.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblGore.SetFontSize( 11, FW_NORMAL );
	m_lblGore.SetWindowText( Launcher_LoadString( IDS_CONFIGURE_GOREHELP ) );

	// Row 4 (advances past the tall row 3).
	y4 = extra + 268;
	m_btnAutopatch.MoveWindow( 50, y4, w, h, TRUE );
	SetWindowTextSafe( &m_btnAutopatch, Launcher_LoadString( IDS_BTN_AUTOPATCH ) );
	m_lblAutopatchHelp.MoveWindow( ctlX, y4 + 6, rowW, h - 6, TRUE );
	m_lblAutopatchHelp.SetTransparent( TRUE );
	m_lblAutopatchHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblAutopatchHelp.SetFontSize( 11, FW_NORMAL );
	m_lblAutopatchHelp.SetWindowText( Launcher_LoadString( IDS_CONFIGURE_AUTOPATCHHELP ) );

	// Row 5 (the OK button row).
	y5 = extra + 300;
	m_btnDone.MoveWindow( 50, y5, w, h, TRUE );
	SetWindowTextSafe( &m_btnDone, Launcher_LoadString( IDS_BTN_DONE ) );
	m_lblReturnToMain.MoveWindow( ctlX, y5 + 6, rowW, h - 6, TRUE );
	m_lblReturnToMain.SetTransparent( TRUE );
	m_lblReturnToMain.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblReturnToMain.SetFontSize( 11, FW_NORMAL );
	m_lblReturnToMain.SetWindowText( Launcher_LoadString( IDS_CFG_RETURNTOMAIN ) );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnAutopatch (0x405310)
//
// Ask before going online; on OK set the restart flag and close, which sends
// the app loop into RunSierraUpdate.

void CConfigureDlg::OnAutopatch()
{
	CPromptDlg	dlg( 2, NULL );

	dlg.SetMessage( Launcher_LoadString( IDS_RUN_PATCH ) );
	if ( dlg.DoModal() == IDOK )
	{
		Launcher_SetRestartFlag( 1 );
		OnOK();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnGore (0x405520)

void CConfigureDlg::OnGore()
{
	CGoreDlg	page;

	ShowWindow( SW_RESTORE );
	InitChildDialog( &page, &m_btnGore );
	page.DoModal();
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnOK (0x4055B0)

void CConfigureDlg::OnOK()
{
	if ( engineapi.ForceReloadProfile )
		engineapi.ForceReloadProfile();

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnActivateApp (0x406FE0)

void CConfigureDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnCustomize (0x40E460)
//
// Mapped but empty.

void CConfigureDlg::OnCustomize()
{
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnPaint (0x412860)

void CConfigureDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnEraseBkgnd (0x412870)

BOOL CConfigureDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CConfigureDlg::OnDisplayChange (0x453D00)

LRESULT CConfigureDlg::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}
