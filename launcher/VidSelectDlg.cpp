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
// Purpose: CVidSelectDlg, the Video hub page (IDD 0xD1 = 209) -- two buttons
//          into CVideoDlg and CVideoModeDlg.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

BEGIN_MESSAGE_MAP( CVidSelectDlg, CDialog )
	//{{AFX_MSG_MAP(CVidSelectDlg)
	ON_COMMAND( IDC_VIDSELECT_VIDEO_MODES, OnVideoModes )
	ON_COMMAND( IDC_VIDSELECT_VIDEO_OPTIONS, OnVideoOptions )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CVidSelectDlg::CVidSelectDlg (0x46D780)

CVidSelectDlg::CVidSelectDlg( CWnd* pParent )
	: CDlgBase( IDD_VIDSELECT, pParent )
{
	int	dims[2];

	m_pSelfWnd = this;

	LoadHeaderBitmap( "head_video", NULL );
	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnOptions.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_OPTIONS, m_headerLoaded );
		m_btnModes.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_MODES, m_headerLoaded );
		m_btnReturn.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DONE, m_headerLoaded );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CVidSelectDlg::DoDataExchange (0x46D8E0)

void CVidSelectDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_VIDSELECT_OPTIONSHELP, m_lblOptionsHelp );
	DDX_Control( pDX, IDC_VIDSELECT_MODESHELP, m_lblModesHelp );
	DDX_Control( pDX, IDC_VIDSELECT_RETURNHELP, m_lblReturnHelp );
	DDX_Control( pDX, IDCANCEL, m_btnReturn );
	DDX_Control( pDX, IDC_VIDSELECT_VIDEO_OPTIONS, m_btnOptions );
	DDX_Control( pDX, IDC_VIDSELECT_VIDEO_MODES, m_btnModes );
}

/////////////////////////////////////////////////////////////////////////////
// CVidSelectDlg::OnVideoModes (0x46D970)

void CVidSelectDlg::OnVideoModes()
{
	CVideoModeDlg	dlg;

	ShowWindow( SW_RESTORE );
	InitChildDialog( &dlg, &m_btnModes );
	dlg.DoModal();
	Dlg_CenterWindow( this );
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CVidSelectDlg::OnVideoOptions (0x46DA10)

void CVidSelectDlg::OnVideoOptions()
{
	CVideoDlg	dlg;

	ShowWindow( SW_RESTORE );
	InitChildDialog( &dlg, &m_btnOptions );
	dlg.DoModal();
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CVidSelectDlg::~CVidSelectDlg (0x46DAA0)

CVidSelectDlg::~CVidSelectDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CVidSelectDlg::OnInitDialog (0x46DB50)

BOOL CVidSelectDlg::OnInitDialog()
{
	int	dims[2];
	int	w, h, helpX, helpW, y, yBottom;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	Launcher_HeaderSize( dims );
	w = dims[0];
	h = dims[1];
	helpX = w + 60;
	helpW = ( g_nLauncherDefW - 10 ) - helpX;

	// "Video options" + its help paragraph.
	m_btnOptions.MoveWindow( 50, 140, w, h, TRUE );
	SetWindowTextSafe( &m_btnOptions, Launcher_LoadString( IDS_BTN_OPTIONS ) );
	m_lblOptionsHelp.MoveWindow( helpX, 146, helpW,
		Launcher_StringHeight( IDS_VIDSELECTDLG_OFFSET, 0 ) + h - 6, TRUE );
	m_lblOptionsHelp.SetTransparent( TRUE );
	m_lblOptionsHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblOptionsHelp.SetFontSize( 11, FW_NORMAL );
	m_lblOptionsHelp.SetWindowText( Launcher_LoadString( IDS_VIDSELECT_OPTIONSHELP ) );

	// "Video modes" + its help paragraph.
	y = Launcher_StringHeight( IDS_VIDSELECTDLG_OFFSET, 1 ) + 172;
	m_btnModes.MoveWindow( 50, y, w, h, TRUE );
	SetWindowTextSafe( &m_btnModes, Launcher_LoadString( IDS_BTN_MODES ) );
	m_lblModesHelp.MoveWindow( helpX, y + 6, helpW, h - 6, TRUE );
	m_lblModesHelp.SetTransparent( TRUE );
	m_lblModesHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblModesHelp.SetFontSize( 11, FW_NORMAL );
	m_lblModesHelp.SetWindowText( Launcher_LoadString( IDS_VIDSELECT_MODESHELP ) );

	// "Done" + its help paragraph.
	y += 32;
	yBottom = y + h;
	m_btnReturn.MoveWindow( 50, y, w, h, TRUE );
	SetWindowTextSafe( &m_btnReturn, Launcher_LoadString( IDS_BTN_DONE ) );
	m_lblReturnHelp.MoveWindow( helpX, y + 6, helpW, yBottom - ( y + 6 ), TRUE );
	m_lblReturnHelp.SetTransparent( TRUE );
	m_lblReturnHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblReturnHelp.SetFontSize( 11, FW_NORMAL );
	m_lblReturnHelp.SetWindowText( Launcher_LoadString( IDS_VIDSELECT_RETURNHELP ) );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CVidSelectDlg::OnActivateApp (0x406FE0)

void CVidSelectDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CVidSelectDlg::OnPaint (0x412860)

void CVidSelectDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CVidSelectDlg::OnEraseBkgnd (0x412870)

BOOL CVidSelectDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}
