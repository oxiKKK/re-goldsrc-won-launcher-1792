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
// Purpose: CPlayerInfoDlg, the server-browser "details" dialog.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The shared skinned-dialog background paint (0x412860 / 0x412870 thunks), as the
// sibling CDlgBase pages wire it.

// CODBlendBtn::SetDIBData is declared in ODButton.h; the header helper feeds
// it the launcher header strip dimensions (the same {w,h} pair the layout reads).

BEGIN_MESSAGE_MAP( CPlayerInfoDlg, CDialog )
	ON_MESSAGE( WM_DISPLAYCHANGE, &CPlayerInfoDlg::OnDisplayChange )
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_ACTIVATEAPP()
END_MESSAGE_MAP()

// CPlayerInfoDlg::CPlayerInfoDlg (0x452610)
CPlayerInfoDlg::CPlayerInfoDlg( CWnd* pParent, CServerInfo* pServer )
	: CDlgBase( IDD_PLAYERINFO, pParent )		// 0x8F
{
	m_pServer     = pServer;
	m_pPlayerList = NULL;
	m_pSelfWnd    = this;		// gates the slide transition

	LoadHeaderBitmap( "head_multi", NULL );
	LoadHeaderStrip();
}

// CPlayerInfoDlg::~CPlayerInfoDlg (0x452790)
CPlayerInfoDlg::~CPlayerInfoDlg()
{
	// 0x452790 destructs only its embedded CODStatic/CODBlendBtn members.  Both
	// report lists are child windows with m_bAutoDelete set and free themselves
	// from CODListCtrl::OnNcDestroy, so deleting them here is a use-after-free.
}

// CPlayerInfoDlg::LoadHeaderStrip (0x452720)
void CPlayerInfoDlg::LoadHeaderStrip()
{
	m_headerLoaded = Launcher_HeaderLoaded();
	int	dims[2] = { 0, 0 };
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnDone.FreeSkinBitmaps();
		m_btnDone.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DONE, m_headerLoaded );
	}
}

// CPlayerInfoDlg::DoDataExchange (0x452850)
void CPlayerInfoDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_PLAYERINFO_SERVERIP,    m_lblServerIp );		// 1194 +232
	DDX_Control( pDX, IDC_PLAYERINFO_SERVERNAME,  m_lblServerName );		// 1192 +328
	DDX_Control( pDX, IDC_PLAYERINFO_SERVERPING,  m_lblServerPing );		// 1196 +424
	DDX_Control( pDX, IDC_PLAYERINFO_SERVER_PING, m_lblPing );			// 1112 +520
	DDX_Control( pDX, IDC_PLAYERINFO_SERVER_NAME, m_lblName );			// 1110 +616
	DDX_Control( pDX, IDC_PLAYERINFO_SERVER_IP,   m_lblIp );				// 1111 +712
	DDX_Control( pDX, IDOK,                        m_btnDone );			// 1    +808
}

// CPlayerInfoDlg::OnInitDialog (0x4528F0)
BOOL CPlayerInfoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	if ( !m_pServer )
	{
		OnOK();
		return TRUE;
	}

	int	dims[2] = { 0, 0 };
	Launcher_HeaderSize( dims );
	int	hdrW = dims[0];
	int	hdrH = dims[1];

	int	bWide  = Launcher_StringHeight( IDS_SPANISH, 0 );	// localized column-width gate
	int	rowH   = m_headerH;								// label/list row height

	// - player roster list ---------------------------------------------------
	RECT	rc;
	rc.left   = hdrW + 60;
	rc.top    = 140;
	rc.right  = g_nLauncherDefW - 20;
	rc.bottom = g_nLauncherDefH - 170;

	m_pPlayerList = new CODPlayerListCtrl;
	m_pPlayerList->Create( 0, rc, this, IDC_PLAYERINFO_PLAYERLIST );
	m_pPlayerList->MoveWindow( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );

	odcolumn_t	col;

	Launcher_LoadStringInto( col.title, IDS_PLAYERINFO_NUMBER );
	col.width = 25;
	m_pPlayerList->AddColumn( &col );

	Launcher_LoadStringInto( col.title, IDS_PLAYERINFO_NAME );
	col.width = bWide ? 135 : 100;
	m_pPlayerList->AddColumn( &col );

	Launcher_LoadStringInto( col.title, IDS_PLAYERINFO_KILLS );
	col.width = bWide ? 50 : 35;
	if ( Launcher_StringHeight( IDS_GERMAN, 0 ) )
		col.width = 55;
	m_pPlayerList->AddColumn( &col );

	Launcher_LoadStringInto( col.title, IDS_PLAYERINFO_TIME );
	col.width = 75;
	m_pPlayerList->AddColumn( &col );

	m_pPlayerList->SetTransparent( 0 );
	m_pPlayerList->SetHeaderTransparent( 1 );
	m_pPlayerList->SetDrawFrame( 1 );
	m_pPlayerList->SetBorderColor( RGB( 56, 56, 56 ) );
	m_pPlayerList->ShowWindow( SW_RESTORE );

	// - rules list -----------------------------------------------------------
	rc.left   = hdrW + 60;
	rc.top    = g_nLauncherDefH - 160;
	rc.right  = g_nLauncherDefW - 20;
	rc.bottom = g_nLauncherDefH - 20;

	m_pRuleList = new CODRuleListCtrl;
	m_pRuleList->Create( 0, rc, this, IDC_PLAYERINFO_RULELIST );
	m_pRuleList->MoveWindow( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );

	Launcher_LoadStringInto( col.title, IDS_PLAYERINFO_RULENAME );
	col.width = 150;
	m_pRuleList->AddColumn( &col );

	Launcher_LoadStringInto( col.title, IDS_PLAYERINFO_RULEVALUE );
	col.width = ( rc.right - rc.left ) - 150;
	m_pRuleList->AddColumn( &col );

	m_pRuleList->SetTransparent( 0 );
	m_pRuleList->SetHeaderTransparent( 1 );
	m_pRuleList->SetDrawFrame( 1 );
	m_pRuleList->SetBorderColor( RGB( 56, 56, 56 ) );
	m_pRuleList->ShowWindow( SW_RESTORE );

	// - captions (server name + "name", ip:port + "ip", ping + "ping") -------
	int	nameH  = Launcher_StringHeight( IDS_PLAYERINFODLG_OFFSET, 0 );
	int	colRight = hdrW + 57;

	// "Server Name" caption label (1192)
	int	y = 153 + nameH;
	m_lblServerName.MoveWindow( 57, y, colRight - 57, ( 153 + 10 * bWide + hdrH ) - y, TRUE );
	m_lblServerName.SetWindowText( Launcher_LoadString( IDS_PLAYERINFO_SERVERNAME ) );
	m_lblServerName.SetTransparent( TRUE );
	m_lblServerName.SetTextColor( RGB( 240, 180, 56 ) );	// 0x38B4F0
	m_lblServerName.SetFontSize( 14, FW_HEAVY );

	// server name value (1110)
	int	yVal = 185 + 10 * bWide;
	m_lblName.MoveWindow( 57, yVal, colRight - 57, hdrH, TRUE );
	m_lblName.SetWindowText( m_pServer->m_strName );
	m_lblName.SetTransparent( TRUE );
	m_lblName.SetTextColor( RGB( 255, 255, 255 ) );	// 0xFFFFFF
	m_lblName.SetFontSize( 11, FW_NORMAL );

	// "Server IP" caption label (1194)
	int	yIp = yVal + 52;
	if ( Launcher_StringHeight( IDS_FRENCH, 0 ) )
		yIp -= 5;
	m_lblServerIp.MoveWindow( 57, yIp, colRight - 57, ( yVal + 52 + hdrH ) - yIp, TRUE );
	m_lblServerIp.SetWindowText( Launcher_LoadString( IDS_PLAYERINFO_SERVERIP ) );
	m_lblServerIp.SetTransparent( TRUE );
	m_lblServerIp.SetTextColor( RGB( 240, 180, 56 ) );	// 0x38B4F0
	m_lblServerIp.SetFontSize( 14, FW_HEAVY );

	// ip:port value (1111)
	int	yIpVal = ( yVal + 52 ) + 32;
	m_lblIp.MoveWindow( 57, yIpVal, colRight - 57, hdrH, TRUE );
	char	szIp[128];
	sprintf( szIp, "%s:%i", (LPCSTR)m_pServer->m_strAddress, (unsigned short)m_pServer->m_nPort );
	m_lblIp.SetWindowText( szIp );
	m_lblIp.SetTransparent( TRUE );
	m_lblIp.SetTextColor( RGB( 255, 255, 255 ) );	// 0xFFFFFF
	m_lblIp.SetFontSize( 11, FW_NORMAL );

	// "Server Ping" caption label (1196)
	int	yPing = yIpVal + 52;
	m_lblServerPing.MoveWindow( 57, nameH + yPing, colRight - 57,
		( 10 * bWide + yPing + hdrH ) - ( nameH + yPing ), TRUE );
	m_lblServerPing.SetWindowText( Launcher_LoadString( IDS_PLAYERINFO_SERVERPING ) );
	m_lblServerPing.SetTransparent( TRUE );
	m_lblServerPing.SetTextColor( RGB( 240, 180, 56 ) );	// 0x38B4F0
	m_lblServerPing.SetFontSize( 14, FW_HEAVY );

	// ping value (1112)
	int	yPingVal = ( 10 * bWide + yPing ) + 32;
	m_lblPing.MoveWindow( 57, yPingVal, colRight - 57, hdrH, TRUE );
	char	szPing[64];
	sprintf( szPing, "%i ms.", (int)(__int64)( m_pServer->m_dSvPing * 1000.0 ) );
	m_lblPing.SetWindowText( szPing );
	m_lblPing.SetTransparent( TRUE );
	m_lblPing.SetTextColor( RGB( 255, 255, 255 ) );	// 0xFFFFFF
	m_lblPing.SetFontSize( 11, FW_NORMAL );

	// Done button caption.
	m_btnDone.MoveWindow( 50, yPingVal + 52, hdrW, hdrH, TRUE );
	m_btnDone.SetWindowText( Launcher_LoadString( IDS_BTN_DONE ) );

	// - fill the lists -------------------------------------------------------
	if ( m_pServer->m_ppPlayers )
	{
		for ( int i = 0; i < m_pServer->m_nCurrentPlayers; i++ )
		{
			CPlayerInfo*	pPlayer = m_pServer->m_ppPlayers[i];
			if ( pPlayer )
				m_pPlayerList->AddRow( pPlayer );
		}
	}
	m_pPlayerList->SelectItem( 0, 1 );
	m_pPlayerList->RefitScrollbar();

	for ( CServerRule* p = m_pServer->m_rules.m_pNext; p; p = p->m_pNext )
		m_pRuleList->AddRow( p );
	m_pRuleList->SelectItem( 0, 1 );
	m_pRuleList->RefitScrollbar();

	return TRUE;
}

// CPlayerInfoDlg::OnPaint (0x412860)
void CPlayerInfoDlg::OnPaint()
{
	PaintSkinnedDialog();
}

// CPlayerInfoDlg::OnEraseBkgnd (0x412870)
BOOL CPlayerInfoDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

// CPlayerInfoDlg::OnActivateApp (0x406FE0)
void CPlayerInfoDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	Default();
}

// CPlayerInfoDlg::OnDisplayChange (0x453D00)
LRESULT CPlayerInfoDlg::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}
