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
// Purpose: the WON "Internet Games" screen: CNetGameDlg, CNetGameDoc,
//          CServerBrowserDlg and the server-query backend.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"


// 0x4E1D90 -- the launcher's cached local IP; read only by
// ServerBrowser_CollectJoinable and never assigned.
static unsigned char	s_localIp[4];

// The proxy-dedup scratch array ApplyFilter grows on demand (4EA978 / 4D0864).
static CServerInfo**	s_rgProxyGroup;
static int			s_nProxyGroupMax = 128;

// Set while the internet-games page owns the sheet; cleared once the persisted
// document has been loaded for the browse half (0x4D0860).
// Two separate file statics, despite doing similar jobs.  0x4EA984 is set by
// OnRefreshCriteria and consumed by ApplyFilter to treat a still-unpinged row as
// pending; 0x4EA98C is RMLPreIdle's own "the engine drew, re-flow on return"
// latch.  MultiSelectDlg's g_bHubNeedsRefresh is a third address again.
static int	s_bPingsPending;	// (4EA984)
static int	s_bRelayoutOnReturn;	// (4EA98C)

static int	s_bNetGamePageLive = 1;

// The live internet-games page (4EA988); the frame pump and the dtor use it.
static CServerBrowserDlg*	s_pNetGamePage;

// Chat flood limiter (4EB690 / 4F36D8).
static double	s_flLastChatSend;
static double	s_flChatCooldown;

// "Internet Games sheet opened" latch; also set from LanDlg.cpp.
int		g_bNetGameSheetOpened;

// Three shared scratch buffers, sized by the gap to the next global.
static char	s_szFmt[2048];		// (4EA990) SetStatusText / ChatPrintf vsprintf target
static char	s_szCleanName[1024];	// (4EB190) NET_CleanServerName
static char	s_szAdrToString[256];	// (4EB590) NET_AdrToString

static int __cdecl ServerBrowser_CompareServers( const void* a, const void* b );

// The sort keys KeyList holds are 1-based indices into the browser's nine
// report columns: four unlabelled glyph columns, then the text ones.  A negative
// key means the column is sorted descending.
enum
{
	SORTKEY_FAVORITE	= 1,
	SORTKEY_PASSWORD	= 2,
	SORTKEY_SVTYPE		= 3,
	SORTKEY_SVOS		= 4,
	SORTKEY_NAME		= 5,
	SORTKEY_PING		= 6,	// the "Net Spd" column
	SORTKEY_MAP			= 7,
	SORTKEY_GAME		= 8,
	SORTKEY_PLAYERS		= 9,
	SORTKEY_MAXPLAYERS	= 10,
	SORTKEY_PROXY		= 11
};

enum
{
	CHAT_TEXT_BROADCAST	= 0xFFFE,
	CHAT_SERVER_ERROR	= 0xFFFF
};

// Chat-server launch (LaunchChatServer).
#define MAX_FACTORY_SERVERS		100	// candidates kept from the directory
#define FACTORY_SHUFFLE_PASSES	300	// swaps; the result is never read (sic)

// Room occupancy probe (FetchRoomList).
#define ROOM_PROBE_QUERY_SIZE	0x100
#define ROOM_PROBE_PORT			6100	// Used when the room address carries no ":port".
#define ROOM_PROBE_INTERVAL		200		// Milliseconds between probes of one room.
#define ROOM_PROBE_BUDGET		20000	// Milliseconds for the whole sweep.
#define ROOM_PROBE_IO_TIMEOUT	10		// per send/recv

static const unsigned char g_acKeyTable[16] =
{
	0x05, 0x61, 0x7A, 0xED, 0x1B, 0xCA, 0x0D, 0x9B,
	0x4A, 0xF1, 0x64, 0xC7, 0xB5, 0x8E, 0xDF, 0xA0
};

/*
==================
AntiCheat_EncodeString (0x431d00)
==================
*/
void AntiCheat_EncodeString( void* pData, int len, int key )
{
	unsigned int*	p = (unsigned int*)pData;
	int				blocks = len / 4;
	int				b, j;

	for ( b = 0; b < blocks; ++b )
	{
		unsigned int	x = _byteswap_ulong( p[b] ^ ~(unsigned int)key );
		unsigned char*	px = (unsigned char*)&x;

		for ( j = 0; j < 4; ++j )
			px[j] ^= (unsigned char)( j | ( j << j ) | g_acKeyTable[( b + j ) & 0xF] | 0xA5 );

		p[b] = (unsigned int)key ^ x;
	}
}

/*
==================
AntiCheat_DecodeString (0x431da0)
==================
*/
void AntiCheat_DecodeString( void* pData, int len, int key )
{
	unsigned int*	p = (unsigned int*)pData;
	int				blocks = len / 4;
	int				b, j;

	for ( b = 0; b < blocks; ++b )
	{
		unsigned int	x = (unsigned int)key ^ p[b];
		unsigned char*	px = (unsigned char*)&x;

		for ( j = 0; j < 4; ++j )
			px[j] ^= (unsigned char)( j | ( j << j ) | g_acKeyTable[( b + j ) & 0xF] | 0xA5 );

		x = _byteswap_ulong( x );
		p[b] = ~(unsigned int)key ^ x;
	}
}

/*
==================
Rate_ApplyFromConfig (0x431e30)
==================
*/
void Rate_ApplyFromConfig( void )
{
	char	szCmd[256];

	sprintf( szCmd, "rate %f\n", g_pServerBrowser->m_playerConfig.rate );
	engineapi.Cbuf_AddText( szCmd );
}

/*
==================
ServerBrowser_CompareKey (0x431e70)
==================
*/
static int ServerBrowser_CompareKey( const CServerInfo* a, const CServerInfo* b, int key, int dir )
{
	// Descending is done by swapping the operands, not by negating the result --
	// which is why every arm below is a plain ascending three-way compare.
	const CServerInfo*	p1 = ( dir > 0 ) ? a : b;
	const CServerInfo*	p2 = ( dir > 0 ) ? b : a;

	switch ( key )
	{
	case SORTKEY_FAVORITE:
		if ( p1->m_bFavorite != p2->m_bFavorite )
			return p1->m_bFavorite ? 1 : -1;
		return 0;

	case SORTKEY_PASSWORD:
		if ( p1->m_bPassword != p2->m_bPassword )
			return p1->m_bPassword ? 1 : -1;
		return 0;

	case SORTKEY_SVTYPE:
	{
		unsigned char	c1 = (unsigned char)p1->m_cSvType;
		unsigned char	c2 = (unsigned char)p2->m_cSvType;

		if ( c1 < c2 )
			return -1;
		return c1 > c2;
	}

	case SORTKEY_SVOS:
	{
		unsigned char	c1 = (unsigned char)p1->m_cSvOs;
		unsigned char	c2 = (unsigned char)p2->m_cSvOs;

		if ( c1 < c2 )
			return -1;
		return c1 > c2;
	}

	case SORTKEY_NAME:
		return _mbsicmp( MBSTR( p1->m_strName ), MBSTR( p2->m_strName ) );

	case SORTKEY_PING:
		// A zero ping is a server that never answered, and sorts last whichever
		// way the column is pointing.
		if ( p1->m_dSvPing == 0.0 && p2->m_dSvPing != 0.0 )
			return 1;
		if ( p1->m_dSvPing != 0.0 && p2->m_dSvPing == 0.0 )
			return -1;
		if ( p1->m_dSvPing < p2->m_dSvPing )
			return -1;
		return p1->m_dSvPing > p2->m_dSvPing;

	case SORTKEY_MAP:
		return _mbsicmp( MBSTR( p1->m_strMap ), MBSTR( p2->m_strMap ) );

	case SORTKEY_GAME:
		return _mbsicmp( MBSTR( p1->m_strGame ), MBSTR( p2->m_strGame ) );

	case SORTKEY_PLAYERS:
	case SORTKEY_MAXPLAYERS:		// both sort on the current count
		if ( p1->m_nCurrentPlayers < p2->m_nCurrentPlayers )
			return -1;
		return p1->m_nCurrentPlayers > p2->m_nCurrentPlayers;

	case SORTKEY_PROXY:
		// (sic) inverted against every other flag key: ascending puts proxies
		// first, where ascending puts favorites last.
		if ( p1->m_bProxy != p2->m_bProxy )
			return p1->m_bProxy ? -1 : 1;
		return 0;

	default:						// an unknown column falls back to the name
		return _mbsicmp( MBSTR( p1->m_strName ), MBSTR( p2->m_strName ) );
	}
}

/*
==================
ServerBrowser_CompareInfo (0x432280)
==================
*/
int __stdcall ServerBrowser_CompareInfo( const CServerInfo* a, const CServerInfo* b, int /*unused*/ )
{
	int	result;
	int	i;
	int	signedKey;
	int	dir;

	if ( !a || !b )
		return 0;

	result = 0;
	for ( i = 0; ; i++ )
	{
		signedKey = KeyList_GetKey( i );
		if ( !signedKey )
			break;

		dir = ( signedKey <= 0 ) ? -1 : 1;
		result = ServerBrowser_CompareKey( a, b, signedKey * dir, dir );
		if ( result )
			return result;
	}

	// Tie-break on the unique browse-order id.
	if ( a->m_nServerId < b->m_nServerId )	return -1;
	if ( a->m_nServerId > b->m_nServerId )	return 1;
	return result;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnOK (0x432320)

void CServerBrowserDlg::OnOK()
{
	SaveConnectionSpeed();
	::ShowWindow( ::GetParent( m_hWnd ), SW_SHOW );
	::ShowWindow( gLauncherWnd, SW_SHOW );
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnCancel (0x432360)

void CServerBrowserDlg::OnCancel()
{
	::ShowWindow( ::GetParent( m_hWnd ), SW_SHOW );
	::ShowWindow( gLauncherWnd, SW_SHOW );
	CDialog::OnCancel();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::InitButtonStrips (0x432390)

void CServerBrowserDlg::InitButtonStrips()
{
	int	wh[2];

	LoadHeaderBitmap( GetHeaderBitmap(), 0 );
	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( wh );
	m_headerW      = wh[0];
	m_headerH      = wh[1];
	m_headerStride = Launcher_HeaderStride();

	if ( !m_headerLoaded )
		return;

	// Resume is the odd one out: the binary slices it and *then* frees, where every
	// other button frees first. (sic)
	m_btnResume.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_RESUME_GAME, m_headerLoaded );
	m_btnResume.FreeSkinBitmaps();

	m_btnDisconnect.FreeSkinBitmaps();
	m_btnDisconnect.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DISCONNECT, m_headerLoaded );

	m_btnJoin.FreeSkinBitmaps();
	m_btnJoin.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_CONNECT, m_headerLoaded );

	m_btnCreateServer.FreeSkinBitmaps();
	m_btnCreateServer.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_CREATE, m_headerLoaded );

	m_btnSwitchPage.FreeSkinBitmaps();
	m_btnSwitchPage.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_CHAT_ROOMS, m_headerLoaded );

	m_btnDone.FreeSkinBitmaps();
	m_btnDone.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DONE, m_headerLoaded );

	m_btnFilter.FreeSkinBitmaps();
	m_btnFilter.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_FILTER, m_headerLoaded );

	m_btnRefresh.FreeSkinBitmaps();
	m_btnRefresh.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_REFRESH, m_headerLoaded );

	m_btnCreateRoom.FreeSkinBitmaps();
	m_btnCreateRoom.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_CREATEROOM, m_headerLoaded );

	m_btnFind.FreeSkinBitmaps();
	m_btnFind.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_FIND, m_headerLoaded );

	m_btnAddServer.FreeSkinBitmaps();
	m_btnAddServer.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_ADDSERVER, m_headerLoaded );

	m_btnUpdate.FreeSkinBitmaps();
	m_btnUpdate.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_UPDATE, m_headerLoaded );

	m_btnServerInfo.FreeSkinBitmaps();
	m_btnServerInfo.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_SERVERINFO, m_headerLoaded );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::CServerBrowserDlg (0x432590)
//
// Every geometry constant LayoutPage reads is set here; without them
// MoveWindow gets heap fill and the whole page lands off-screen.

CServerBrowserDlg::CServerBrowserDlg( int nMode, CWnd* pParent )
	: CDlgConnectableBase( IDD, pParent )
{
	m_pSelfWnd = this;
	m_bReady       = 0;
	m_bSelSpectate = 0;

	InitButtonStrips();

	m_nMode              = nMode;
	m_bFavoritesOnlyView = 0;
	m_pBrowserEngine     = NULL;
	m_unk4576         = 0;
	m_nRowHeight         = 13;

	// Two locale probes widen the button column for the longer translations.
	int	bWideButtons = Launcher_StringHeight( IDS_SPANISH, 0 );
	int	bMidButtons  = Launcher_StringHeight( IDS_FRENCH, 0 );

	m_yBanner     = 30;
	m_xButtons    = 30;
	m_yServerList = 140;
	m_unk4596  = 170;
	m_wChatPrompt = 100;
	if ( bWideButtons )
		m_xButtons = 10;

	m_unk4608 = 140;
	m_wButtons   = 126;
	if ( bWideButtons )
		m_wButtons = 206;
	if ( bMidButtons )
		m_wButtons = 166;

	m_unk4616  = 32;
	m_xServerList = m_xButtons + m_wButtons + 10;
	m_xRight      = g_nLauncherDefW - 10;
	m_unk4632  = 18;
	m_yBottom     = g_nLauncherDefH - 45;
	m_unk4636  = 50;
	m_unk4640  = 20;
	m_unk4644  = 5;
	m_hChatInput  = 22;
	m_unk4656  = 64;
	m_unk4660  = 20;
	m_nSidebarW   = 128;

	m_pFontArial9 = new CFont;
	if ( m_pFontArial9 )
		m_pFontArial9->CreatePointFont( 90, "Arial" );

	m_iSelRow     = -1;
	m_unk4552  = 0;
	m_unk4556  = 0;
	m_pServerList = NULL;
	m_pChatText   = NULL;
	m_pChatInput  = NULL;
	m_pUserList   = NULL;
	m_pLblSpeed   = NULL;
	m_pComboSpeed = NULL;

	m_fontChatInput.CreatePointFont( 90, "Arial" );
	m_fontChatText.Attach( CreateFontA( -11, 0, 0, 0, 400, 0, 0, 0, 0,
						OUT_TT_PRECIS, 0, PROOF_QUALITY, VARIABLE_PITCH, "Arial" ) );
	m_unk4532 = 0;
	m_bkBrush.Attach( CreateSolidBrush( RGB( 63, 63, 63 ) ) );

	s_bPingsPending    = 0;
	s_bNetGamePageLive = 1;

	// NOTE(ox): the ctor also writes a {0,0,100,100} RECT at +224..236, which is
	// past CDlgBase (m_rcHeader is +184, m_hHeaderDib +200) -- an unmodelled
	// CDlgConnectableBase member.  Nothing reconstructed reads it yet.

	SetFramePump( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::~CServerBrowserDlg (0x4329b0)

CServerBrowserDlg::~CServerBrowserDlg()
{
	s_pNetGamePage = NULL;

	if ( m_pBrowserEngine )
	{
		// The sheet is told to stop pumping before it goes; the pointer itself is
		// left dangling, as in the binary.
		m_pBrowserEngine->m_unk80 = 1;
		delete m_pBrowserEngine;
	}

	delete m_pFontArial9;
	delete m_pChatInput;
	delete m_pLblSpeed;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::DoDataExchange (0x432be0)

void CServerBrowserDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_PLAYERINFO_SERVERNAME, m_lblVersion );
	DDX_Control( pDX, IDC_DLG156_TOTAL,          m_lblStatus );
	DDX_Control( pDX, IDC_BTN_DISCONNECT,        m_btnDisconnect );
	DDX_Control( pDX, IDC_BTN_RESUME,            m_btnResume );
	DDX_Control( pDX, IDC_MAIN_MINIMIZE,         m_btnMinimize );
	DDX_Control( pDX, IDC_BTN_INFO,              m_btnServerInfo );
	DDX_Control( pDX, IDC_BTN_UPDATE,            m_btnUpdate );
	DDX_Control( pDX, IDC_BTN_ADDSERVER,         m_btnAddServer );
	DDX_Control( pDX, IDC_DLG156_ROOM,           m_lblConnecting );
	DDX_Control( pDX, IDC_BTN_LISTMODE,          m_btnSwitchPage );
	DDX_Control( pDX, IDC_BTN_CONNECT,           m_btnJoin );
	DDX_Control( pDX, IDC_FILTER_HEADING,        m_lblChatPrompt );
	DDX_Control( pDX, IDC_BTN_REFRESH,           m_btnRefresh );
	DDX_Control( pDX, IDC_BTN_FIND,              m_btnFind );
	DDX_Control( pDX, IDC_BTN_LISTROOMS,         m_btnCreateRoom );
	DDX_Control( pDX, IDC_BTN_FILTER,            m_btnFilter );
	DDX_Control( pDX, IDC_BTN_CREATESV,          m_btnCreateServer );
	DDX_Control( pDX, IDOK,                      m_btnDone );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnInitDialog (0x432d30)

BOOL CServerBrowserDlg::OnInitDialog()
{
	odcolumn_t	col;
	char		szTitle[64];

	CDialog::OnInitDialog();
	s_pNetGamePage = this;
	Dlg_CenterWindow( this );

	m_btnMinimize.ShowWindow( SW_HIDE );
	m_btnMinimize.SetSkin( "min_n", "min_d", "min_f" );
	m_btnMinimize.MoveWindow( g_nLauncherDefW - 48, 10, 19, 19, TRUE );

	if ( !HasControls() )
	{
		LOG( "BAILING: list=%p chat=%p users=%p input=%p -- page will be blank",
			 m_pServerList, m_pChatText, m_pUserList, m_pChatInput );
		return TRUE;
	}
	LOG( "controls ok; mode=%d", m_nMode );

	LoadFilter( &m_filter );

	// Nine report columns: four unlabelled glyph columns, then the text ones.
	static const int	s_colWidths[9] = { 16, 16, 16, 16, 110, 50, 75, 0, 70 };
	static const UINT	s_colTitles[9] =
	{
		0, 0, 0, 0,
		// The row paint draws name, ping, map, game, players in that order -- "Game Type"
		// comes before "Players/Max", which is also what the stock header shows.
		IDS_SERVER_GAMESERVER, IDS_SERVER_SPEED, IDS_SERVER_MAP,
		IDS_SERVER_GAME, IDS_SERVER_PLAYERS
	};

	int	widths[9];
	int	i;
	for ( i = 0; i < 9; i++ )
		widths[i] = s_colWidths[i];
	widths[7] = Launcher_StringHeight( IDS_NETGAMEDLG_OFFSET, 0 ) + 80;

	if ( Launcher_StringHeight( IDS_SPANISH, 0 ) )
	{
		widths[4] = 90;
		widths[5] = 44;
		widths[7] = 80;
	}
	if ( Launcher_StringHeight( IDS_GERMAN, 0 ) )
	{
		widths[4] = 100;
		widths[5] = 65;
	}

	for ( i = 0; i < 9; i++ )
	{
		szTitle[0] = 0;
		if ( s_colTitles[i] )
			Launcher_LoadStringInto( szTitle, s_colTitles[i] );
		strcpy( col.title, szTitle );
		col.width = widths[i];
		m_pServerList->AddColumn( &col );
	}

	m_pChatInput->ClearEditSelection();
	m_pUserList->ResetContent();
	m_pUserList->UpdateScrollbar( 0 );
	m_pChatText->SetWindowText( "" );
	m_pChatInput->SetText( "" );

	m_lblConnecting.SetText( "Connecting..." );
	m_lblConnecting.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblConnecting.SetTransparent( 1 );
	m_lblConnecting.SetFontSize( 22, FW_HEAVY );
	m_lblConnecting.SetOffsets( 5, 3 );

	m_lblStatus.SetWindowText( "" );
	m_lblStatus.SetTextColor( RGB( 200, 200, 200 ) );
	m_lblStatus.SetTransparent( 1 );
	m_lblStatus.SetFontSize( 11, FW_NORMAL );

	m_lblVersion.SetWindowText( "" );
	m_lblVersion.SetTextColor( RGB( 150, 90, 0 ) );
	m_lblVersion.SetTransparent( 1 );
	m_lblVersion.SetFontSize( 9, FW_NORMAL );

	ChatWnd_Printf( m_pChatText, 0, "Half-Life Chat\r\n" );

	if ( m_pBrowserEngine->m_pSelfIdentity )
	{
		const char*	pszNick = m_pBrowserEngine->m_pSelfIdentity->GetPlayerName();
		m_pChatText->SetSelfNick( NET_CleanServerName( pszNick ) );
	}
	if ( m_pBrowserEngine->m_pServerListHead )
		ApplyFilter( &m_filter );

	SetWindowTextSafe( &m_btnResume,       Launcher_LoadString( IDS_BTN_RESUME ) );
	SetWindowTextSafe( &m_btnDisconnect,   Launcher_LoadString( IDS_BTN_DISCONNECT ) );
	SetWindowTextSafe( &m_btnCreateServer, Launcher_LoadString( IDS_BTN_CREATESV ) );
	SetWindowTextSafe( &m_btnDone,         Launcher_LoadString( IDS_BTN_DONE ) );
	SetWindowTextSafe( &m_btnFilter,       Launcher_LoadString( IDS_BTN_FILTER ) );
	SetWindowTextSafe( &m_btnCreateRoom,   Launcher_LoadString( IDS_BTN_ROOM ) );
	SetWindowTextSafe( &m_btnFind,         Launcher_LoadString( IDS_BTN_FIND ) );
	SetWindowTextSafe( &m_btnRefresh,      Launcher_LoadString( IDS_BTN_REFRESH ) );
	SetWindowTextSafe( &m_btnJoin,         Launcher_LoadString( GetJoinCaptionId() ) );
	SetWindowTextSafe( &m_btnAddServer,    Launcher_LoadString( IDS_BTN_ADDSERVER ) );
	SetWindowTextSafe( &m_btnUpdate,       Launcher_LoadString( IDS_BTN_UPDATE ) );
	SetWindowTextSafe( &m_btnServerInfo,   Launcher_LoadString( IDS_BTN_INFO ) );

	m_lblChatPrompt.SetTransparent( 1 );
	m_lblChatPrompt.SetFontSize( 12, FW_NORMAL );
	m_lblChatPrompt.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblChatPrompt.SetWindowText( Launcher_LoadString( IDS_CHAT_PROMPT ) );

	LayoutPage( m_nMode );
	m_pChatInput->SetFocus();
	::InvalidateRect( m_hWnd, NULL, TRUE );
	ShowWindow( SW_SHOW );
	::UpdateWindow( m_hWnd );
	UpdateRoomBanner();

	if ( m_nMode )
	{
		if ( CheckParm( "-nopersist", 0 ) )
			OnUpdateFromMaster();
		else
			RebuildVisibleList();
	}

	m_bReady = 1;
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::GetJoinCaptionId (0x4333a0)

UINT CServerBrowserDlg::GetJoinCaptionId()
{
	return IDS_BTN_JOIN;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::LoadFilter (0x4333b0)

void CServerBrowserDlg::LoadFilter( netfilter_t* pFilter )
{
	const char*	pszSection = GetSettingsSection();

	memset( pFilter, 0, sizeof( *pFilter ) );

	pFilter->m_bHideNoResponse = Launcher_GetProfileInt( pszSection, "Filter Responded", 0 ) != 0;
	pFilter->m_bHideEmpty      = Launcher_GetProfileInt( pszSection, "Filter Empty",     0 ) != 0;
	pFilter->m_bHideFull       = Launcher_GetProfileInt( pszSection, "Filter Full",      0 ) != 0;
	pFilter->m_bFavoritesOnly  = Launcher_GetProfileInt( pszSection, "Filter Favorite",  0 ) != 0;
	pFilter->m_bLimitPing      = Launcher_GetProfileInt( pszSection, "Filter Ping",      0 ) != 0;
	pFilter->m_bLinuxOnly      = Launcher_GetProfileInt( pszSection, "Filter OS",        0 ) != 0;

	int	bProxiesOnly = Launcher_GetProfileInt( pszSection, "Filter IsProxy",    0 ) != 0;
	int	bHideProxies = Launcher_GetProfileInt( pszSection, "Filter IsNotProxy", 0 ) != 0;
	pFilter->m_bProxiesOnly   = bProxiesOnly;
	pFilter->m_bHideProxies   = bHideProxies;
	pFilter->m_bFilterProxies = ( bProxiesOnly || bHideProxies ) ? 1 : 0;

	pFilter->m_bDedicatedOnly = Launcher_GetProfileInt( pszSection, "Filter Dedicated", 0 ) != 0;
	pFilter->m_bByMap         = Launcher_GetProfileInt( pszSection, "Filter Map",       0 ) != 0;

	const char*	pszMap = Launcher_GetProfileString( pszSection, "Filter Map Name", "" );
	strcpy( pFilter->m_szMap, pszMap ? pszMap : "" );

	pFilter->m_nPingMax = Launcher_GetProfileInt( pszSection, "Filter PingMax", -1 );
	pFilter->m_bByGame  = Launcher_GetProfileInt( pszSection, "Filter Game",     0 ) != 0;

	// "Half-Life" (or empty) means the running game directory.
	const char*	pszGame = Launcher_GetProfileString( pszSection, "Filter Game Name", "Half-Life" );
	if ( !pszGame || !*pszGame || !strcmp( pszGame, "Half-Life" ) )
		pszGame = com_gamedir;
	strcpy( pFilter->m_szGame, pszGame );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnUserListValidate (0x433640)

void CServerBrowserDlg::OnUserListValidate()
{
	::ValidateRect( m_hWnd, NULL );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnRefreshCriteria (0x433650)

void CServerBrowserDlg::OnRefreshCriteria()
{
	CWaitCursor	wait;

	RefreshCriteria_t	criteria;

	memset( &criteria, 0, sizeof( criteria ) );
	criteria.m_nMaxOutstanding = m_pBrowserEngine->m_nMaxSockets;
	criteria.m_nMaxRetries     = m_pBrowserEngine->m_nRetries;
	criteria.m_dStateTimeout   = m_pBrowserEngine->m_dTimeout;
	criteria.m_nPhaseMask      = 2;

	CRefreshDlg	dlg( &criteria, m_pBrowserEngine->m_pServerListHead, NULL );
	dlg.DoModal();

	s_bPingsPending = 1;
	ApplyFilter( &m_filter );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnRefresh (0x4337b0)

void CServerBrowserDlg::OnRefresh()
{
	m_pServerList->ResetContent();

	// Reset every non-LAN, unfiltered record back to a fresh query state.
	for ( CServerInfo* p = m_pBrowserEngine->m_pServerListHead; p; p = p->m_pNext )
	{
		if ( p->m_bLan || p->GetFiltered() )
			continue;

		if ( p->m_pSocket )
			p->CloseSocket();
		p->ClearPlayers();
		p->ClearRules();
		p->m_nStatus         = SVQ_QUEUED;
		p->m_nRetry          = 0;
		p->m_dSendTime       = engineapi.Sys_FloatTime();
		p->m_dSvPing         = 0.0;
		p->m_bNoResponse     = 0;
		p->m_strMap          = "";
		p->m_nMaxPlayers     = 0;
		p->m_nCurrentPlayers = 0;
		p->m_nProtocol       = 0;
	}

	OnRefreshCriteria();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::RebuildVisibleList (0x433850)

void CServerBrowserDlg::RebuildVisibleList()
{
	if ( !HasControls() )
		return;

	m_pServerList->ResetContent();
	m_pServerList->BeginUpdate( 1, 0 );

	for ( CServerInfo* p = m_pBrowserEngine->m_pServerListHead; p; p = p->m_pNext )
	{
		if ( !p->GetFiltered() && ( !m_bFavoritesOnlyView || p->m_bFavorite ) )
			m_pServerList->AddRow( p );	// row record = the CServerInfo*
	}

	m_pServerList->BeginUpdate( 0, 1 );
	::InvalidateRect( m_pServerList->m_hWnd, NULL, TRUE );
	m_pServerList->SortRows( (odrowcmp_t)ServerBrowser_CompareInfo, -1 );
	m_pServerList->SelectItem( 0, 1 );
	m_pServerList->RefitScrollbar();
	m_pServerList->UpdateScrollbar( 1 );

	int	nVisible = m_pBrowserEngine->CountVisible();
	if ( nVisible > 0 )
	{
		int		nPlayers = m_pBrowserEngine->CountPlayers();
		char	szCount[256];
		sprintf( szCount, Launcher_LoadString( IDS_INTERNET_CURRENTTOTALS ), nVisible, nPlayers );
		m_lblStatus.SetWindowText( szCount );
	}

	BuildFilterHelp();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnCreate (0x433990)
//
// Every child control the page owns; OnInitDialog refuses to lay out until
// all four of the main ones exist.

int CServerBrowserDlg::OnCreate( LPCREATESTRUCT lpcs )
{
	RECT	rc;

	if ( Default() == -1 )
		return -1;

	::SetRect( &rc, 0, 0, 100, 100 );

	m_pLblSpeed = new CODStatic();
	m_pLblSpeed->Create( "", WS_CHILD | WS_VISIBLE, rc, this, (UINT)-1 );
	m_pLblSpeed->SetTransparent( 1 );
	m_pLblSpeed->SetTextColor( RGB( 255, 255, 255 ) );
	m_pLblSpeed->SetFontSize( 12, FW_NORMAL );
	m_pLblSpeed->SetWindowText( Launcher_LoadString( IDS_MULTI_TYPE ) );

	// The connection-speed combo, then select the preset matching the config rate.
	m_pComboSpeed = new CODComboBox();
	m_pComboSpeed->SetDropHeight( 120 );
	m_pComboSpeed->Create( WS_CHILD | WS_VISIBLE, &rc, this, IDC_NET_SPEED_COMBO );
	m_pComboSpeed->AddString( Launcher_LoadString( IDS_MODEM_CUSTOM ) );
	m_pComboSpeed->AddString( Launcher_LoadString( IDS_MODEM14K ) );
	m_pComboSpeed->AddString( Launcher_LoadString( IDS_MODEM28K ) );
	m_pComboSpeed->AddString( Launcher_LoadString( IDS_MODEM33K ) );
	m_pComboSpeed->AddString( Launcher_LoadString( IDS_MODEM56K ) );
	m_pComboSpeed->AddString( Launcher_LoadString( IDS_MULTI_ISDN ) );
	m_pComboSpeed->AddString( Launcher_LoadString( IDS_MULTI_DSL ) );
	m_pComboSpeed->AddString( Launcher_LoadString( IDS_MULTI_LAN ) );

	int	nRate = (int)g_pServerBrowser->m_playerConfig.rate;
	if ( abs( nRate - 9999 ) < 5 )		m_pComboSpeed->SetCurSel( 7 );
	else if ( abs( nRate - 7500 ) < 5 )	m_pComboSpeed->SetCurSel( 6 );
	else if ( abs( nRate - 5000 ) < 5 )	m_pComboSpeed->SetCurSel( 5 );
	else if ( abs( nRate - 3500 ) < 5 )	m_pComboSpeed->SetCurSel( 4 );
	else if ( abs( nRate - 3000 ) < 5 )	m_pComboSpeed->SetCurSel( 3 );
	else if ( abs( nRate - 2500 ) < 5 )	m_pComboSpeed->SetCurSel( 2 );
	else								m_pComboSpeed->SetCurSel( abs( nRate - 1500 ) < 5 );

	m_pChatText = new CODChatEdit();
	if ( !m_pChatText )
	{
		Launcher_ShowMessageById( 0, IDS_MULTI_NOCHATWINDOW );
		return -1;
	}
	::SetRect( &rc, 100, 20, g_nLauncherDefW - m_nSidebarW, 200 );
	m_pChatText->Create( WS_CHILD | WS_VISIBLE | WS_TABSTOP, &rc, this, IDC_NET_CHAT_TEXT );

	m_pChatInput = new CHLChatLineCtrl( this );
	if ( !m_pChatInput )
	{
		Launcher_ShowMessageById( 0, IDS_MULTI_NOCHATINPUT );
		return -1;
	}
	m_pChatInput->SetAutoHScroll();		// before Create -- it picks the edit style
	::SetRect( &rc, 100, 200, g_nLauncherDefW, 220 );
	m_pChatInput->Create( WS_CHILD | WS_VISIBLE | WS_TABSTOP, &rc, this, IDC_NET_CHAT_INPUT );
	m_pChatInput->SetActive( 1 );

	m_pUserList = new CODIRCUserListCtrl( this );
	if ( !m_pUserList )
	{
		Launcher_ShowMessageById( 0, IDS_MULTI_NOUSERLIST );
		return -1;
	}
	::SetRect( &rc, g_nLauncherDefW - m_nSidebarW, 20, g_nLauncherDefW, 200 );
	m_pUserList->Create( 0, rc, this, IDC_NET_USER_LIST );
	m_pUserList->SetTransparent( 0 );
	m_pUserList->SetDrawFrame( 1 );
	m_pUserList->ToggleHeader();

	m_pChatText->SendMessage( WM_SETFONT, (WPARAM)m_fontChatText.m_hObject, 1 );
	m_pChatInput->SendMessage( WM_SETFONT, (WPARAM)m_fontChatInput.m_hObject, 1 );
	m_pChatInput->SetBorderColor( RGB( 56, 56, 56 ) );
	m_pChatInput->SetEditTextColor( RGB( 255, 255, 255 ) );

	m_pServerList = new CODHLListCtrl( this, 0 );
	if ( !m_pServerList )
	{
		Launcher_ShowMessageById( 0, IDS_MULTI_NOSERVERLIST );
		return -1;
	}
	::SetRect( &rc, 100, 240, g_nLauncherDefW, g_nLauncherDefH );
	// Created without WS_BORDER; the 3px frame is the
	// control's own paint, and a system border would double it above the header.
	m_pServerList->Create( WS_CHILD | WS_VISIBLE | WS_TABSTOP, rc, this, IDC_NET_SERVER_LIST );
	m_pServerList->SetSortKey( "Net Sort Order" );
	m_pServerList->SetRowHeight( 16 );
	m_pServerList->SetHeaderFont( 10, 400 );

	m_pBrowserEngine->SetPage( this );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::HasControls (0x433f50)
//
// True once every child control the page draws through exists.

BOOL CServerBrowserDlg::HasControls()
{
	return m_pServerList && m_pChatText && m_pUserList && m_pChatInput;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnCtlColor (0x433f90)
//
// The chat panes use the page's brush.

HBRUSH CServerBrowserDlg::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH	hbr = CDlgConnectableBase::OnCtlColor( pDC, pWnd, nCtlColor );

	if ( nCtlColor <= CTLCOLOR_EDIT && m_pChatText && m_pChatInput
	  && ( pWnd->m_hWnd == m_pChatText->m_hWnd || pWnd->m_hWnd == m_pChatInput->m_hWnd ) )
	{
		pDC->SetTextColor( RGB( 255, 255, 255 ) );
		pDC->SetBkColor( RGB( 63, 63, 63 ) );
		pDC->SetBkMode( TRANSPARENT );
		return (HBRUSH)m_bkBrush.m_hObject;
	}
	return hbr;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnChatSend (0x434000)

void CServerBrowserDlg::OnChatSend()
{
	if ( m_pChatInput )
	{
		m_pChatInput->Submit( NULL );
		m_pChatInput->SetFocus();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnInsertUserNick (0x434030)

void CServerBrowserDlg::OnInsertUserNick()
{
	if ( !m_pUserList || !m_pChatInput )
		return;

	int	iRow = m_pUserList->GetCurSel();
	if ( iRow == -1 )
		return;

	CChatUser*	pUser = (CChatUser*)m_pUserList->GetItemData( iRow );
	if ( !pUser )
		return;

	m_pChatInput->Submit( NET_CleanServerName( pUser->m_szNick ) );
	m_pChatInput->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnFind (0x434090)

void CServerBrowserDlg::OnFind()
{
	CInputDlg	dlg( 0 );

	dlg.SetPrompt( Launcher_LoadString( IDS_FINDPLAYER_PLAYER ) );
	if ( dlg.DoModal() == IDOK && !dlg.m_strInput.IsEmpty() )
	{
		if ( m_nMode )
			SelectMatchingRows( dlg.m_strInput );
		else
			FindUserOnServers( dlg.m_strInput );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnCreateRoom (0x434140)

void CServerBrowserDlg::OnCreateRoom()
{
	CWaitCursor	wait;

	chatroom_t*	pWas = m_pBrowserEngine->GetCurrentRoom();
	char		szWas[128];

	if ( pWas )
		strcpy( szWas, pWas->m_szName );
	else
		szWas[0] = 0;

	// The dialog takes the sheet itself.
	CRoomDialog	dlg( m_pBrowserEngine, this );
	InitChildDialog( &dlg, &m_btnCreateRoom );
	int	nResult = dlg.DoModal();
	RestoreAfterModal();

	if ( nResult == IDOK )
	{
		const char*	pszPicked = dlg.GetPickedRoomName();
		if ( pszPicked && m_pBrowserEngine->m_pSelfIdentity )
		{
			chatroom_t*	pRoom = m_pBrowserEngine->m_pRoomList->FindByName( pszPicked );
			if ( pRoom && _strcmpi( pRoom->m_szName, szWas ) )
				EnterRoom( pRoom );
		}
	}
	else if ( !m_pBrowserEngine->m_pRoomList->FindByName( szWas ) )
	{
		// The room we were in vanished while the dialog was up.
		CRoomList*	pHead = m_pBrowserEngine->m_pRoomList;
		if ( pHead->m_pNext == pHead )
			m_lblConnecting.SetText( Launcher_LoadString( IDS_CHAT_NOROOM ) );
		else
			EnterRoom( pHead->m_pNext );
	}

	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnEditFilter (0x434320)

void CServerBrowserDlg::OnEditFilter()
{
	ShowWindow( SW_RESTORE );

	CFilterDialog	dlg( GetSettingsSection(), m_pBrowserEngine->m_pServerListHead, this );
	InitChildDialog( &dlg, &m_btnFilter );
	int	nResult = dlg.DoModal();
	RestoreAfterModal();

	if ( nResult == IDOK )
	{
		// Field order is the binary's write order.
		memset( &m_filter, 0, sizeof( m_filter ) );

		m_filter.m_bHideNoResponse = dlg.m_bFilterResponded;
		m_filter.m_bHideEmpty      = dlg.m_bFilterEmpty;
		m_filter.m_bHideFull       = dlg.m_bFilterFull;
		m_filter.m_bFavoritesOnly  = dlg.m_bFilterFavorite;
		m_filter.m_bLimitPing      = dlg.m_bFilterPing;
		m_filter.m_nPingMax        = dlg.m_nFilterPingMax;
		m_filter.m_bByGame         = dlg.m_bFilterGame;
		strcpy( m_filter.m_szGame, dlg.m_szGameDir );
		m_filter.m_bByMap          = dlg.m_bFilterMap;
		strcpy( m_filter.m_szMap, dlg.m_strFilterMapName );
		m_filter.m_bDedicatedOnly  = dlg.m_bFilterDedicated;
		m_filter.m_bLinuxOnly      = dlg.m_bFilterOS;
		m_filter.m_bProxiesOnly    = dlg.m_bFilterIsProxy;
		m_filter.m_bFilterProxies  = dlg.m_bFilterAnyProxy;
		m_filter.m_bHideProxies    = dlg.m_bFilterIsNotProxy;

		ApplyFilter( &m_filter );
	}

	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::ApplyFilter (0x434510)
//
// Mark every row the filter rejects, then collapse duplicate proxies down
// to the best-scoring entry before the list is rebuilt.

void CServerBrowserDlg::ApplyFilter( netfilter_t* pFilter )
{
	if ( !s_rgProxyGroup )
		s_rgProxyGroup = (CServerInfo**)malloc( sizeof( CServerInfo* ) * s_nProxyGroupMax );

	int	nGrouped = 0;
	int	cbGroup  = 0;

	for ( CServerInfo* p = m_pBrowserEngine->m_pServerListHead; p; p = p->m_pNext )
	{
		p->SetFiltered( 0 );

		char	szBase[260];
		int		bReject = 0;

		if ( pFilter->m_bHideNoResponse && p->m_bNoResponse )
			bReject = 1;
		else if ( pFilter->m_bHideEmpty && p->m_nMaxPlayers > 0 && p->m_nCurrentPlayers < 1 )
			bReject = 1;
		else if ( pFilter->m_bHideFull && p->m_nMaxPlayers > 0
			   && p->m_nCurrentPlayers == p->m_nMaxPlayers )
			bReject = 1;
		else if ( pFilter->m_bFavoritesOnly && !p->m_bFavorite )
			bReject = 1;
		else if ( pFilter->m_bLimitPing && pFilter->m_nPingMax != -1
			   && ( (int)( p->m_dSvPing * 1000.0 ) > pFilter->m_nPingMax
				 || ( s_bPingsPending && !(int)( p->m_dSvPing * 1000.0 ) ) ) )
			bReject = 1;
		else if ( pFilter->m_bByGame
			   && ( COM_FileBase( p->m_strDir, szBase ), _strcmpi( pFilter->m_szGame, szBase ) ) )
			bReject = 1;
		else if ( pFilter->m_bByMap && _strcmpi( pFilter->m_szMap, p->m_strMap ) )
			bReject = 1;
		else if ( pFilter->m_bLinuxOnly && p->m_cSvOs != 'l' )
			bReject = 1;
		else if ( pFilter->m_bDedicatedOnly && p->m_cSvType != 'd' )
			bReject = 1;
		else if ( pFilter->m_bFilterProxies && pFilter->m_bProxiesOnly && !p->m_bProxy )
			bReject = 1;

		if ( bReject )
		{
			p->SetFiltered( 1 );
			continue;
		}

		// "Hide proxies" filters the row rather than skipping it.
		if ( pFilter->m_bFilterProxies && pFilter->m_bHideProxies )
		{
			if ( p->m_bProxy )
				p->SetFiltered( 1 );
			continue;
		}

		// Proxies only: collapse entries sharing a target address, keeping the one
		// with the better ping-squared-per-slot score and summing the player counts.
		if ( !p->m_bProxy )
			continue;

		int	iSame = 0;
		for ( ; iSame < nGrouped; iSame++ )
		{
			if ( s_rgProxyGroup[iSame]->m_dwProxyIp == p->m_dwProxyIp
			  && s_rgProxyGroup[iSame]->m_iProxyPort == p->m_iProxyPort )
				break;
		}

		if ( iSame >= nGrouped )
		{
			if ( nGrouped >= s_nProxyGroupMax )
			{
				s_nProxyGroupMax *= 2;
				CServerInfo**	pGrown = (CServerInfo**)malloc( sizeof( CServerInfo* ) * s_nProxyGroupMax );
				memcpy( pGrown, s_rgProxyGroup, cbGroup );
				free( s_rgProxyGroup );
				s_rgProxyGroup = pGrown;
			}
			p->m_nProxyMaxPlayers = p->m_nMaxPlayers;
			p->m_nProxyCurPlayers = p->m_nCurrentPlayers;
			s_rgProxyGroup[nGrouped++] = p;
			cbGroup += sizeof( CServerInfo* );
			continue;
		}

		CServerInfo*	pKept = s_rgProxyGroup[iSame];
		double			dLo   = ( p->m_dSvPing >= pKept->m_dSvPing ) ? pKept->m_dSvPing : p->m_dSvPing;
		double			dHi   = ( p->m_dSvPing >= pKept->m_dSvPing ) ? p->m_dSvPing : pKept->m_dSvPing;

		int	nHi = (int)( dHi * 1000.0 );
		int	nLo = (int)( dLo * 1000.0 );
		if ( nHi * nHi * p->m_nMaxPlayers / ( p->m_nCurrentPlayers + 1 )
		  <= nLo * nLo * pKept->m_nMaxPlayers / ( pKept->m_nCurrentPlayers + 1 ) )
		{
			p->SetFiltered( 1 );
			pKept->m_nProxyMaxPlayers += p->m_nMaxPlayers;
			pKept->m_nProxyCurPlayers += p->m_nCurrentPlayers;
		}
		else
		{
			pKept->SetFiltered( 1 );
			p->m_nProxyMaxPlayers = p->m_nMaxPlayers + pKept->m_nProxyMaxPlayers;
			p->m_nProxyCurPlayers = p->m_nCurrentPlayers + pKept->m_nProxyCurPlayers;
			s_rgProxyGroup[iSame] = p;
		}
	}

	RebuildVisibleList();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::Relayout (0x434900)

void CServerBrowserDlg::Relayout()
{
	::InvalidateRect( m_hWnd, NULL, TRUE );
	InitButtonStrips();
	LayoutPage( m_nMode );
	::SetActiveWindow( m_hWnd );
	SetFocus();
	ShowWindow( SW_SHOWNORMAL );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::ConnectSelected (0x434960)

void CServerBrowserDlg::ConnectSelected()
{
	if ( !m_pServerList )
		return;

	int	iRow = m_pServerList->GetCurSel();
	if ( iRow == -1 )
		return;

	CServerInfo*	pInfo = (CServerInfo*)m_pServerList->GetItemData( iRow );
	if ( pInfo )
		ConnectToSelectedServer( m_pBrowserEngine, pInfo );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnServerInfo (0x4349a0)

void CServerBrowserDlg::OnServerInfo()
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
	criteria.m_nMaxOutstanding = m_pBrowserEngine->m_nMaxSockets;
	criteria.m_nMaxRetries     = m_pBrowserEngine->m_nRetries;
	criteria.m_dStateTimeout   = m_pBrowserEngine->m_dTimeout;
	criteria.m_nPhaseMask      = 14;			// info + players + rules

	CServerInfo*	pSavedNext = pInfo->m_pBatchNext;
	pInfo->m_pBatchNext = NULL;

	CRefreshDlg	refresh( &criteria, pInfo, NULL );
	refresh.DoModal();

	pInfo->m_pBatchNext = pSavedNext;

	CPlayerInfoDlg	info( this, pInfo );
	InitChildDialog( &info, &m_btnServerInfo );
	info.DoModal();
	RestoreAfterModal();

	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnMarkFavorite (0x434b80)

void CServerBrowserDlg::OnMarkFavorite()
{
	SetFavoriteOnSelection( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnClearFavorite (0x434b90)

void CServerBrowserDlg::OnClearFavorite()
{
	SetFavoriteOnSelection( 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::SetFavoriteOnSelection (0x434ba0)

void CServerBrowserDlg::SetFavoriteOnSelection( int bFavorite )
{
	if ( !m_pServerList )
		return;

	for ( int i = m_pServerList->GetRowCount() - 1; i >= 0; --i )
	{
		if ( !( m_pServerList->GetItemFlags( i ) & 1 ) )
			continue;

		CServerInfo*	pInfo = (CServerInfo*)m_pServerList->GetItemData( i );
		if ( pInfo && !pInfo->GetFiltered() )
			pInfo->m_bFavorite = bFavorite;
	}

	::InvalidateRect( m_pServerList->m_hWnd, NULL, TRUE );
	::UpdateWindow( m_pServerList->m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::SelectMatchingRows (0x434c20)

void CServerBrowserDlg::SelectMatchingRows( const char* pszNeedle )
{
	CString	strNeedle;
	CString	strName;
	int		nRows;
	int		i;
	CServerInfo*	pInfo;

	if ( !m_pServerList || m_pServerList->GetCurSel() == -1 )
		return;

	strNeedle = pszNeedle;
	strNeedle.MakeLower();
	nRows = m_pServerList->GetRowCount();
	for ( i = 0; i < nRows; i++ )
	{
		pInfo = (CServerInfo*)m_pServerList->GetItemData( i );
		if ( !pInfo )
			continue;

		strName = pInfo->m_strName;
		strName.MakeLower();
		if ( strName.Find( strNeedle ) != -1 )
			m_pServerList->SelectItem( i, 0 );
	}

	::InvalidateRect( m_pServerList->m_hWnd, NULL, TRUE );
	::UpdateWindow( m_pServerList->m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::GetChatText (0x434cd0)

CODChatEdit* CServerBrowserDlg::GetChatText()
{
	return m_pChatText;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::EnterRoom (0x434ce0)

void CServerBrowserDlg::EnterRoom( chatroom_t* pRoom )
{
	m_pBrowserEngine->SetCurrentRoom( pRoom );
	if ( pRoom )
		m_lblConnecting.SetText( pRoom->m_szName );

	if ( m_pBrowserEngine->m_pSelfIdentity )
	{
		CServerAddr	addr;
		int			nPort = 0;
		COM_ParseHostPort( pRoom->m_szAddress, addr.host_name, &nPort, 6100 );
		addr.port      = (unsigned short)nPort;
		addr.reserved1 = 0;
		gFavorites->entries[0] = addr;

		// NOTE(ox): deliberate deviation.  0x434DB4 passes the record's +32 (the topic)
		// here, which with our directory data is always the literal "No topic"; the
		// room name at +288 is what actually reads correctly.  The banner just above
		// (0x434D26) does use +288.
		ChatWnd_Printf( m_pChatText, 0, "Entering room %s\r\n", pRoom->m_szName );
		m_pBrowserEngine->JoinRoom( m_pBrowserEngine->m_pSelfIdentity );
	}

	UpdateRoomBanner();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::SaveConnectionSpeed (0x434e10)

void CServerBrowserDlg::SaveConnectionSpeed()
{
	int	iSel = m_pComboSpeed->GetCurSel();
	if ( iSel == -1 )
		return;

	if ( iSel )
	{
		int	nRate = 2500;

		switch ( iSel )
		{
		case 1:	nRate = 1500;	break;
		case 2:	nRate = 2500;	break;
		case 3:	nRate = 3000;	break;
		case 4:	nRate = 3500;	break;
		case 5:	nRate = 5000;	break;
		case 6:	nRate = 7500;	break;
		case 7:	nRate = 9999;	break;
		}

		g_pServerBrowser->m_playerConfig.rate = nRate;
		if ( m_pBrowserEngine )
			m_pBrowserEngine->ComputeMaxSockets();
	}
	Launcher_SavePlayerInfoTo( "Player", &g_pServerBrowser->m_playerConfig );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnDoneClicked (0x434ee0)

void CServerBrowserDlg::OnDoneClicked()
{
	SaveConnectionSpeed();
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnUpdateFromMaster (0x434f00)

void CServerBrowserDlg::OnUpdateFromMaster()
{
	CWaitCursor	wait;

	if ( m_pServerList )
		m_pServerList->ResetContent();

	m_pBrowserEngine->SetListDirty( TRUE );
	m_pBrowserEngine->QueryMaster( &m_filter );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnCreateServer (0x434f80)

void CServerBrowserDlg::OnCreateServer()
{
	if ( !m_pBrowserEngine )
		return;

	resumeOnSwitch = 0;

	if ( engineapi.Cbuf_AddText )
		engineapi.Cbuf_AddText( "disconnect\n" );

	CCreateServerDlg	dlg( m_pBrowserEngine, this );
	InitChildDialog( &dlg, &m_btnCreateServer );
	int	nResult = dlg.DoModal();
	RestoreAfterModal();

	if ( nResult == IDOK )
	{
		if ( dlg.m_bDedicated )
		{
			// Dedicated: relaunch as hlds.exe and quit this process.
			char	szCmd[1024];
			sprintf( szCmd, "hlds.exe +maxplayers %i +sv_password \"%s\" +hostname \"%s\" +map %s",
					 dlg.m_nMaxPlayers, dlg.m_szPassword, dlg.m_szHostName, dlg.m_szMap );

			// Only when the active mod is not the base game.
			if ( g_pCurrentMod && g_pCurrentMod != g_pValveMod )
			{
				char*	pszGameDir = g_pCurrentMod->GetKeyString( "gamedir" );

				strcat( szCmd, " -game " );
				strcat( szCmd, pszGameDir );
			}

			Eng_Shutdown();
			WinExec( szCmd, SW_RESTORE );
			PostQuitMessage( 0 );
			return;
		}

		char	szCmds[1024];
		sprintf( szCmds, "disconnect\nsv_lan 0\nsetmaster enable\nmaxplayers %i\n"
						 "sv_password \"%s\"\nhostname \"%s\"\nmap %s\n",
				 dlg.m_nMaxPlayers, dlg.m_szPassword, dlg.m_szHostName, dlg.m_szMap );

		if ( engineapi.Cbuf_AddText )
			engineapi.Cbuf_AddText( szCmds );
		Eng_Frame( 1 );

		if ( !Launcher_StartEngine( 0 ) )
		{
			gDLLState = 0;
			Eng_Load( 0, 0 );
			VID_HideEngineWindow();
			return;
		}
		if ( engineapi.Cbuf_AddText )
			engineapi.Cbuf_AddText( szCmds );
	}

	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnJoin (0x4351d0)

void CServerBrowserDlg::OnJoin()
{
	SaveConnectionSpeed();
	ConnectSelected();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::FindUserOnServers (0x4351f0)

void CServerBrowserDlg::FindUserOnServers( const char* pszNick )
{
	if ( m_pBrowserEngine )
		m_pBrowserEngine->FindPlayer( pszNick );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnDeleteSelected (0x435210)

void CServerBrowserDlg::OnDeleteSelected()
{
	if ( !m_pServerList )
		return;

	int	nChecked = CountCheckedRows();
	if ( !nChecked )
	{
		Launcher_ShowMessageById( 0, IDS_MULTI_NOSELECTION );
		return;
	}

	char		szPrompt[1024];
	CPromptDlg	prompt( 2, NULL );
	Launcher_LoadStringInto( szPrompt, IDS_SERVER_REMOVE, nChecked );
	prompt.SetMessage( szPrompt );
	if ( prompt.DoModal() != IDOK )
		return;

	for ( int i = m_pServerList->GetRowCount() - 1; i >= 0; --i )
	{
		if ( !( m_pServerList->GetItemFlags( i ) & 1 ) )
			continue;

		CServerInfo*	pInfo = (CServerInfo*)m_pServerList->GetItemData( i );
		if ( pInfo && !pInfo->GetFiltered() )
		{
			m_pServerList->DeleteItem( i );
			m_pBrowserEngine->RemoveServer( pInfo );
		}
	}

	::InvalidateRect( m_pServerList->m_hWnd, NULL, TRUE );
	::UpdateWindow( m_pServerList->m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnRefreshSelected (0x4354c0)
//
// Splice the checked rows out into their own chain, refresh just those,
// then thread them back onto the front of the list.

void CServerBrowserDlg::OnRefreshSelected()
{
	if ( !m_pServerList )
		return;

	if ( !CountCheckedRows() )
	{
		Launcher_ShowMessageById( 0, IDS_MULTI_NOSELECTION );
		return;
	}

	for ( CServerInfo* p = m_pBrowserEngine->m_pServerListHead; p; p = p->m_pNext )
		p->m_pOwnedQuery = NULL;

	CServerInfo*	pBatch = NULL;
	for ( int i = m_pServerList->GetRowCount() - 1; i >= 0; --i )
	{
		if ( !( m_pServerList->GetItemFlags( i ) & 1 ) )
			continue;

		CServerInfo*	pInfo = (CServerInfo*)m_pServerList->GetItemData( i );
		if ( !pInfo || pInfo->GetFiltered() )
			continue;

		m_pServerList->DeleteItem( i );
		CServerInfo*	pTaken = ServerBrowser_UnlinkServer( &m_pBrowserEngine->m_pServerListHead, pInfo );
		if ( pTaken )
		{
			pTaken->m_pBatchNext = pBatch;
			pTaken->m_dSendTime = 0.0;
			pTaken->m_unk492 = 1;
			pBatch = pTaken;
		}
	}

	if ( !pBatch )
	{
		::InvalidateRect( m_pServerList->m_hWnd, NULL, TRUE );
		::UpdateWindow( m_pServerList->m_hWnd );
		return;
	}

	CWaitCursor	wait;

	RefreshCriteria_t	criteria;
	memset( &criteria, 0, sizeof( criteria ) );
	criteria.m_nMaxOutstanding = m_pBrowserEngine->m_nMaxSockets;
	criteria.m_nMaxRetries     = m_pBrowserEngine->m_nRetries;
	criteria.m_dStateTimeout   = m_pBrowserEngine->m_dTimeout;
	criteria.m_nPhaseMask      = 2;

	CRefreshDlg	dlg( &criteria, pBatch, NULL );
	dlg.DoModal();

	for ( CServerInfo* p = pBatch; p; )
	{
		CServerInfo*	pNext = p->m_pBatchNext;
		p->m_pBatchNext = m_pBrowserEngine->m_pServerListHead;
		m_pBrowserEngine->m_pServerListHead = p;
		p = pNext;
	}

	ApplyFilter( &m_filter );
	m_pServerList->ResortByRefreshOrder();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnResortList (0x435760)

void CServerBrowserDlg::OnResortList()
{
	int	nServers = 0;
	for ( CServerInfo* p = m_pBrowserEngine->m_pServerListHead; p; p = p->m_pNext )
		++nServers;
	if ( !nServers )
		return;

	// Sort a flat array, then re-thread the singly-linked list in the new order.
	CServerInfo**	rgpSorted = new CServerInfo*[nServers];
	memset( rgpSorted, 0, sizeof( CServerInfo* ) * nServers );

	int	n = 0;
	for ( CServerInfo* p = m_pBrowserEngine->m_pServerListHead; p; p = p->m_pNext )
		rgpSorted[n++] = p;

	qsort( rgpSorted, nServers, sizeof( CServerInfo* ), ServerBrowser_CompareServers );

	for ( int i = 0; i < nServers - 1; i++ )
		rgpSorted[i]->m_pNext = rgpSorted[i + 1];
	rgpSorted[nServers - 1]->m_pNext = NULL;
	m_pBrowserEngine->m_pServerListHead = rgpSorted[0];
	delete[] rgpSorted;

	m_pServerList->ResetContent();
	m_pServerList->BeginUpdate( 1, 0 );
	for ( CServerInfo* p = m_pBrowserEngine->m_pServerListHead; p; p = p->m_pNext )
		m_pServerList->AddRow( p );
	m_pServerList->BeginUpdate( 0, 1 );

	::InvalidateRect( m_pServerList->m_hWnd, NULL, TRUE );
	m_pServerList->SelectItem( 0, 1 );
	m_pServerList->RefitScrollbar();
	m_pServerList->UpdateScrollbar( 1 );

	int	nVisible = m_pBrowserEngine->CountVisible();
	if ( nVisible > 0 )
	{
		char	szCount[256];
		sprintf( szCount, Launcher_LoadString( IDS_INTERNET_CURRENTTOTALS ),
				 nVisible, m_pBrowserEngine->CountPlayers() );
		m_lblStatus.SetWindowText( szCount );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::CountCheckedRows (0x435920)

int CServerBrowserDlg::CountCheckedRows()
{
	int	nChecked = 0;
	for ( int i = 0; i < m_pServerList->GetRowCount(); i++ )
	{
		if ( m_pServerList->GetItemFlags( i ) & 1 )
			++nChecked;
	}
	return nChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::Run (0x435960)

int CServerBrowserDlg::Run()
{
	delete m_pBrowserEngine;
	m_pBrowserEngine = NULL;

	m_pBrowserEngine = new CNetGameDlg( NULL, 0 );

	if ( !g_bWonLoginRequired )
		return 1;

	CLoginDlg	login( m_pBrowserEngine, NULL );
	if ( login.DoModal() == IDOK )
		return 1;

	delete m_pBrowserEngine;
	m_pBrowserEngine = NULL;	// cancelled: release the engine and abort
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::MaybeLoadRoomList (0x435b90)

void CServerBrowserDlg::MaybeLoadRoomList()
{
	if ( m_pBrowserEngine && !m_pBrowserEngine->m_pSelfIdentity && !m_nMode )
		m_pBrowserEngine->RequestRoomList();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::LayoutPage (0x435bc0)
//
// One layout with two halves: nonzero bMode is the server browser
// ("head_inetgames"), zero is the chat rooms view ("head_room").

void CServerBrowserDlg::LayoutPage( int bMode )
{
	RECT	rcClient;

	MaybeLoadRoomList();
	::GetClientRect( m_hWnd, &rcClient );
	::LockWindowUpdate( m_hWnd );

	if ( bMode )
	{
		// Server browser half.
		m_nHdrPad = 0;		// draw the header art (the list starts below it)

		int	xList = m_xServerList - 10 * Launcher_StringHeight( IDS_FRENCH, 0 );
		int	yList = m_yServerList - 15;

		m_pServerList->MoveWindow( xList, yList, m_xRight - xList, m_yBottom - yList, TRUE );
		m_pServerList->UpdateScrollbar( 1 );
		m_pServerList->ShowWindow( SW_RESTORE );

		if ( m_lblStatus.m_hWnd )
		{
			m_lblStatus.EnableWindow( TRUE );
			m_lblStatus.ShowWindow( SW_RESTORE );
			m_lblStatus.MoveWindow( m_xServerList, m_yBottom + 5,
									m_xServerList + 200, m_yBottom + 20, TRUE );
		}
		if ( m_lblVersion.m_hWnd )
		{
			m_lblVersion.EnableWindow( TRUE );
			m_lblVersion.ShowWindow( SW_RESTORE );
			m_lblVersion.MoveWindow( 15, m_yBottom + 20, m_xRight, m_yBottom + 35, TRUE );
		}

		// Connection-speed caption + combo, bottom right.
		int	xSpeed = m_xRight - 10 * ( Launcher_StringHeight( IDS_GERMAN, 0 ) + 15 );
		if ( m_pLblSpeed && m_pLblSpeed->m_hWnd )
		{
			m_pLblSpeed->MoveWindow( xSpeed, m_yServerList - 70, m_xRight - xSpeed, 15, TRUE );
			m_pLblSpeed->EnableWindow( TRUE );
			m_pLblSpeed->ShowWindow( SW_RESTORE );
		}
		if ( m_pComboSpeed && m_pComboSpeed->m_hWnd )
		{
			RECT	rcCombo;
			rcCombo.left   = xSpeed;
			rcCombo.top    = m_yServerList - 50;
			rcCombo.right  = m_xRight;
			rcCombo.bottom = m_yServerList - 35;
			m_pComboSpeed->MoveTo( &rcCombo, 1 );
			m_pComboSpeed->EnableWindow( TRUE );
			m_pComboSpeed->ShowWindow( SW_RESTORE );
		}

		// Resume/Disconnect replace Join while a game is live.
		GameInfo_t	gi;
		int			bInGame = 0;
		if ( engineapi.GetGameInfo( &gi, 0 ) && gi.state == ca_active )
			bInGame = ( gi.signon != 0 );

		int	y = m_yServerList;
		m_xButtons += -25 * Launcher_StringHeight( IDS_GERMAN, 0 );
		m_wButtons += 25 * Launcher_StringHeight( IDS_GERMAN, 0 );

		if ( bInGame )
		{
			m_btnResume.ShowWindow( SW_SHOW );
			m_btnResume.EnableWindow( TRUE );
			m_btnDisconnect.ShowWindow( SW_SHOW );
			m_btnDisconnect.EnableWindow( TRUE );
			m_btnJoin.ShowWindow( SW_HIDE );
			m_btnJoin.EnableWindow( FALSE );
			m_btnResume.MoveWindow( m_xButtons, y, m_wButtons, 26, TRUE );
			y += 30;
			m_btnDisconnect.MoveWindow( m_xButtons, y, m_wButtons, 26, TRUE );
		}
		else
		{
			m_btnResume.ShowWindow( SW_HIDE );
			m_btnResume.EnableWindow( FALSE );
			m_btnDisconnect.ShowWindow( SW_HIDE );
			m_btnDisconnect.EnableWindow( FALSE );
			m_btnJoin.ShowWindow( SW_SHOW );
			m_btnJoin.EnableWindow( TRUE );
			m_btnJoin.MoveWindow( m_xButtons, y, m_wButtons, 26, TRUE );
		}
		y += 30;

		// The remaining command column, in binary order.
		if ( HasCreateGameButton() && m_btnCreateServer.m_hWnd )
		{
			m_btnCreateServer.MoveWindow( m_xButtons, y, m_wButtons, 26, TRUE );
			m_btnCreateServer.EnableWindow( TRUE );
			m_btnCreateServer.ShowWindow( SW_RESTORE );
			y += 30;
		}
		else if ( m_btnCreateServer.m_hWnd )
		{
			m_btnCreateServer.ShowWindow( SW_HIDE );
		}

		if ( m_btnServerInfo.m_hWnd )
		{
			m_btnServerInfo.MoveWindow( m_xButtons, y, m_wButtons + 5, 26, TRUE );
			m_btnServerInfo.EnableWindow( TRUE );
			m_btnServerInfo.ShowWindow( SW_RESTORE );
		}
		y += 30;
		if ( m_btnRefresh.m_hWnd )
		{
			m_btnRefresh.MoveWindow( m_xButtons, y, m_wButtons, 26, TRUE );
			m_btnRefresh.EnableWindow( TRUE );
			m_btnRefresh.ShowWindow( SW_RESTORE );
		}
		y += 30;
		if ( m_btnUpdate.m_hWnd )
		{
			m_btnUpdate.MoveWindow( m_xButtons, y, m_wButtons, 26, TRUE );
			m_btnUpdate.EnableWindow( TRUE );
			m_btnUpdate.ShowWindow( SW_RESTORE );
		}
		y += 30;
		if ( m_btnFilter.m_hWnd )
		{
			m_btnFilter.MoveWindow( m_xButtons, y, m_wButtons, 26, TRUE );
			m_btnFilter.EnableWindow( TRUE );
			m_btnFilter.ShowWindow( SW_RESTORE );
			SetWindowTextSafe( &m_btnFilter, Launcher_LoadString( IDS_BTN_FILTER ) );
		}
		y += 30;
		if ( m_btnAddServer.m_hWnd )
		{
			m_btnAddServer.MoveWindow( m_xButtons, y, m_wButtons, 26, TRUE );
			m_btnAddServer.EnableWindow( TRUE );
			m_btnAddServer.ShowWindow( SW_RESTORE );
		}
		y += 30;
		if ( m_btnSwitchPage.m_hWnd )
		{
			m_btnSwitchPage.MoveWindow( m_xButtons, y, m_wButtons, 26, TRUE );
			m_btnSwitchPage.ShowWindow( SW_RESTORE );
			SetWindowTextSafe( &m_btnSwitchPage, Launcher_LoadString( IDS_BTN_CHAT ) );
		}
		y += 30;
		if ( m_btnDone.m_hWnd )
			m_btnDone.MoveWindow( m_xButtons, y, m_wButtons, 26, TRUE );

		// Chat-only furniture stays hidden here.
		if ( m_btnCreateRoom.m_hWnd )	m_btnCreateRoom.ShowWindow( SW_HIDE );
		if ( m_btnFind.m_hWnd )
		{
			m_btnFind.ShowWindow( SW_HIDE );
			SetWindowTextSafe( &m_btnFind, "" );
		}
		if ( m_lblChatPrompt.m_hWnd )	m_lblChatPrompt.ShowWindow( SW_HIDE );
		m_lblConnecting.ShowWindow( SW_HIDE );
		m_pChatText->ShowWindow( SW_HIDE );
		m_pUserList->ShowWindow( SW_HIDE );
		m_pChatInput->ShowWindow( SW_HIDE );

		m_xButtons += 25 * Launcher_StringHeight( IDS_GERMAN, 0 );
		m_wButtons += -25 * Launcher_StringHeight( IDS_GERMAN, 0 );
	}
	else
	{
		// Chat-rooms half.
		// LayoutPage sets this too: the chat half skips DrawDialogContent, so the
		// head_room DIB loaded below is never composited.  The page's "Room:" caption
		// comes from CRoomStatic::OnPaint, not from the header art.
		m_nHdrPad = 1;

		int	xLeft = Launcher_StringHeight( IDS_FRENCH, 0 ) ? 10 : 30;

		m_lblConnecting.MoveWindow( xLeft - 6, 30, 523 - ( xLeft - 6 ), m_yBanner, TRUE );
		m_lblConnecting.ShowWindow( SW_RESTORE );

		int	yRow    = m_yBanner + 40;
		int	xSaved  = m_xButtons;

		// Create-room, find and done sit in one row under the banner.
		int	xRoom = m_xButtons - 12;
		int	wRoom = xRoom + m_wButtons + Launcher_StringHeight( IDS_NETGAMEDLG_OFFSET, 1 );
		m_xButtons = wRoom;
		if ( m_btnCreateRoom.m_hWnd )
		{
			m_btnCreateRoom.MoveWindow( xRoom, yRow, wRoom - xRoom, 26, TRUE );
			m_btnCreateRoom.EnableWindow( TRUE );
			m_btnCreateRoom.ShowWindow( SW_RESTORE );
		}

		int	nOffset2 = Launcher_StringHeight( IDS_NETGAMEDLG_OFFSET, 2 );
		int	nNarrow  = Launcher_StringHeight( IDS_SPANISH, 0 );
		int	xFind    = m_xButtons;
		int	wFind    = nOffset2 + xFind + m_wButtons - 10 * ( 3 * nNarrow + 3 );
		m_xButtons = xFind + m_wButtons + nOffset2 - 30;
		if ( m_btnFind.m_hWnd )
		{
			m_btnFind.MoveWindow( xFind, yRow, wFind - xFind, 26, TRUE );
			m_btnFind.EnableWindow( TRUE );
			m_btnFind.ShowWindow( SW_RESTORE );
			SetWindowTextSafe( &m_btnFind, Launcher_LoadString( IDS_BTN_FIND ) );
		}

		int	xSwitch = m_xButtons - 30 * nNarrow;
		int	wSwitch = m_xButtons + m_wButtons - 20 * ( 3 * nNarrow );
		m_xButtons = m_xButtons + m_wButtons - 20 * ( 3 * nNarrow ) + 30;
		if ( m_btnSwitchPage.m_hWnd )
		{
			m_btnSwitchPage.MoveWindow( xSwitch, yRow, wSwitch - xSwitch, 26, TRUE );
			m_btnSwitchPage.ShowWindow( SW_RESTORE );
			SetWindowTextSafe( &m_btnSwitchPage, Launcher_LoadString( IDS_BTN_BROWSE ) );
		}

		if ( m_btnDone.m_hWnd )
			m_btnDone.MoveWindow( m_xButtons, yRow, m_wButtons, 26, TRUE );
		m_xButtons = xSaved;

		// Transcript left, member list right, input line across the bottom.
		int	yTop    = yRow + 31;
		int	yBottom = rcClient.bottom - rcClient.top - m_hChatInput - 20;
		int	xSplit  = m_xRight - m_nSidebarW;

		m_pChatText->MoveWindow( xLeft, yTop, xSplit - xLeft, yBottom - yTop, TRUE );
		m_pChatText->ShowWindow( SW_RESTORE );
		m_pChatText->UpdateScrollbarVisibility();

		m_pUserList->MoveWindow( xSplit, yTop, m_xRight - xSplit, yBottom - yTop, TRUE );
		m_pUserList->ShowWindow( SW_RESTORE );
		m_pUserList->UpdateScrollbar( 0 );

		int	yInput = yBottom + 2;
		int	xInput = xLeft + m_wChatPrompt + 5;
		m_pChatInput->MoveWindow( xInput, yInput, m_xRight - xInput, m_hChatInput, TRUE );
		m_pChatInput->ShowWindow( SW_RESTORE );

		if ( m_lblChatPrompt.m_hWnd )
		{
			m_lblChatPrompt.MoveWindow( xLeft, yInput + 3, m_wChatPrompt,
										m_hChatInput - 3, TRUE );
			m_lblChatPrompt.ShowWindow( SW_RESTORE );
		}

		// Browser-only furniture stays hidden here.
		m_pServerList->ShowWindow( SW_HIDE );
		if ( m_btnJoin.m_hWnd )
		{
			m_btnJoin.EnableWindow( FALSE );
			m_btnJoin.ShowWindow( SW_HIDE );
		}
		if ( m_btnRefresh.m_hWnd )
		{
			m_btnRefresh.EnableWindow( FALSE );
			m_btnRefresh.ShowWindow( SW_HIDE );
		}
		if ( m_btnFilter.m_hWnd )
		{
			m_btnFilter.EnableWindow( FALSE );
			m_btnFilter.ShowWindow( SW_HIDE );
			SetWindowTextSafe( &m_btnFilter, "" );
		}
		if ( m_btnServerInfo.m_hWnd )
		{
			m_btnServerInfo.EnableWindow( FALSE );
			m_btnServerInfo.ShowWindow( SW_HIDE );
		}
		if ( m_btnUpdate.m_hWnd )
		{
			m_btnUpdate.EnableWindow( FALSE );
			m_btnUpdate.ShowWindow( SW_HIDE );
		}
		if ( m_btnAddServer.m_hWnd )
		{
			m_btnAddServer.EnableWindow( FALSE );
			m_btnAddServer.ShowWindow( SW_HIDE );
		}
		if ( m_btnCreateServer.m_hWnd )
		{
			m_btnCreateServer.EnableWindow( FALSE );
			m_btnCreateServer.ShowWindow( SW_HIDE );
		}
		if ( m_pLblSpeed && m_pLblSpeed->m_hWnd )
		{
			m_pLblSpeed->EnableWindow( FALSE );
			m_pLblSpeed->ShowWindow( SW_HIDE );
		}
		if ( m_pComboSpeed && m_pComboSpeed->m_hWnd )
		{
			m_pComboSpeed->EnableWindow( FALSE );
			m_pComboSpeed->ShowWindow( SW_HIDE );
		}
		// Hidden first and disabled second, unlike every other row. (sic)
		if ( m_btnResume.m_hWnd )
		{
			m_btnResume.ShowWindow( SW_HIDE );
			m_btnResume.EnableWindow( FALSE );
		}
		if ( m_btnDisconnect.m_hWnd )
		{
			m_btnDisconnect.ShowWindow( SW_HIDE );
			m_btnDisconnect.EnableWindow( FALSE );
		}
		if ( m_lblStatus.m_hWnd )
		{
			m_lblStatus.EnableWindow( FALSE );
			m_lblStatus.ShowWindow( SW_HIDE );
		}
		if ( m_lblVersion.m_hWnd )		m_lblVersion.ShowWindow( SW_HIDE );
	}

	// The page-switch button carries the other half's strip cell and header art.
	if ( m_btnSwitchPage.m_hWnd )
	{
		m_btnSwitchPage.FreeSkinBitmaps();
		if ( bMode )
		{
			m_btnSwitchPage.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_CHAT_ROOMS, m_headerLoaded );
			LoadHeaderBitmap( GetHeaderBitmap(), 0 );
			m_btnJoin.SetFocus();
		}
		else
		{
			m_btnSwitchPage.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_INTERNET_GAMES, m_headerLoaded );
			LoadHeaderBitmap( "head_room", 0 );
			m_pChatInput->SetFocus();
		}
		::InvalidateRect( m_hWnd, NULL, TRUE );
	}

	::LockWindowUpdate( NULL );
	::InvalidateRect( m_hWnd, NULL, TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::GetHeaderBitmap (0x436a80)

const char* CServerBrowserDlg::GetHeaderBitmap()
{
	return "head_inetgames";
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnSwitchPage (0x436a90)

void CServerBrowserDlg::OnSwitchPage()
{
	CWaitCursor	wait;

	int	bWasBrowse = m_nMode;
	m_nMode = ( bWasBrowse == 0 );

	// First entry into the browse half loads the persisted document.
	if ( s_bNetGamePageLive && !bWasBrowse )
	{
		if ( CheckParm( "-nopersist", 0 ) )
		{
			if ( m_pServerList )
				m_pServerList->ResetContent();
		}
		else
		{
			RebuildVisibleList();
		}
		m_pBrowserEngine->LoadDoc();
		s_bNetGamePageLive = 0;
	}

	LayoutPage( m_nMode );

	if ( !m_nMode && !m_pBrowserEngine->m_bConnecting )
	{
		if ( m_pBrowserEngine->GetCurrentRoom() )
			m_lblConnecting.SetText( m_pBrowserEngine->GetCurrentRoom()->m_szName );
		else
			OnCreateRoom();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::RMLPreIdle (0x436ba0)

int CServerBrowserDlg::RMLPreIdle()
{
	// Swap the join button's strip cell when the selection is a spectator proxy.
	CServerInfo*	pSel = m_pServerList->GetSelectedServer();
	if ( pSel && pSel->m_bProxy != m_bSelSpectate )
	{
		m_bSelSpectate = pSel->m_bProxy;
		m_btnJoin.SetDIBData( CSize( m_headerW, m_headerH ), m_bSelSpectate ? BTNSTRIP_SPECTATE_JOIN : BTNSTRIP_CONNECT, m_headerLoaded );
		::InvalidateRect( m_btnJoin.m_hWnd, NULL, TRUE );
	}

	Launcher_SyncEngineWindow( this );

	if ( Eng_Frame( gBackground ) && !gBackground && ActiveApp )
	{
		s_bRelayoutOnReturn = 1;
		return TRUE;
	}

	if ( m_pBrowserEngine )
		m_pBrowserEngine->Pump();

	if ( ActiveApp )
	{
		if ( s_bRelayoutOnReturn )
		{
			Relayout();
			s_bRelayoutOnReturn = 0;
			if ( gDLLState == DLL_ACTIVE || gDLLState == DLL_PAUSED )
				gBackground = 1;
		}
		if ( Launcher_GetRestartFlag() )
			OnOK();
	}
	return FALSE;
}

/*
==================
ChatUserList_Reseed (0x436cb0)

Clear the listbox and re-add a row for every member on the roster, in list
order (used after a full member-list reset).
==================
*/
void ChatUserList_Reseed( CServerBrowserDlg* pPage )
{
	CODIRCUserListCtrl*	pList = pPage->m_pUserList;
	if ( !pList )
		return;

	pList->ResetContent();
	for ( CChatUser* p = pPage->m_pBrowserEngine->m_pUserList; p; p = p->m_pNext )
		pList->AddRow( p );
}

/*
==================
ChatUserList_AddRow (0x436cf0)

Sorted insert of one member (slot 49).
==================
*/
void ChatUserList_AddRow( CServerBrowserDlg* pPage, CChatUser* pUser )
{
	CODIRCUserListCtrl*	pList = pPage->m_pUserList;
	if ( pList )
		pList->AddRow( pUser );
}

/*
==================
ChatUserList_RemoveRow (0x436d10)
==================
*/
void ChatUserList_RemoveRow( CServerBrowserDlg* pPage, CChatUser* pUser )
{
	CODIRCUserListCtrl*	pList = pPage->m_pUserList;
	if ( !pList )
		return;

	for ( int i = pList->m_nRows - 1; i >= 0; --i )
	{
		if ( (CChatUser*)pList->m_rows[i]->record != pUser )
			continue;

		// CODListCtrl::DeleteRow, inlined.
		int	nOldSel = pList->m_curSel;

		pList->SelectItem( -1, 0 );				// drop the focus row first
		pList->FreeRow( pList->m_rows[i] );		// return the row node

		for ( int j = i; j < pList->m_nRows - 1; ++j )
			pList->m_rows[j] = pList->m_rows[j + 1];
		pList->m_rows[pList->m_nRows - 1] = 0;
		--pList->m_nRows;

		if ( pList->m_nRows )
		{
			if ( nOldSel > i )			pList->SelectItem( nOldSel - 1, 0 );
			else if ( nOldSel < i )		pList->SelectItem( nOldSel, 0 );
		}
		pList->RefitScrollbar();
		pList->UpdateScrollbar( 0 );
	}

	::InvalidateRect( pList->m_hWnd, NULL, TRUE );
	::UpdateWindow( pList->m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::UpdateRoomBanner (0x436d80)

void CServerBrowserDlg::UpdateRoomBanner()
{
	chatroom_t*	pRoom = m_pBrowserEngine->GetCurrentRoom();
	if ( pRoom )
		m_lblConnecting.SetText( pRoom->m_szName );		// the name, not the topic
	else
		m_lblConnecting.SetText( Launcher_LoadString( IDS_CHAT_NOROOM ) );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnAddByAddress (0x436dd0)

void CServerBrowserDlg::OnAddByAddress()
{
	CInputDlg	dlg( 0 );

	dlg.SetPrompt( Launcher_LoadString( IDS_MULTI_ADDSERVERPROMPT ) );
	if ( dlg.DoModal() != IDOK || dlg.m_strInput.IsEmpty() )
		return;

	char			szAddr[256];
	sockaddr_in		adr;
	strcpy( szAddr, dlg.m_strInput );

	if ( !NET_StringToAdr( szAddr, &adr ) )
	{
		Launcher_ShowMessageById( 0, IDS_MULTI_ADDIPUNRESOLVABLE );
		return;
	}

	sprintf( szAddr, "%s", inet_ntoa( adr.sin_addr ) );
	int	nPort = ntohs( adr.sin_port );
	if ( !nPort )
		nPort = atoi( "27015" );

	CServerInfo*	pInfo = m_pBrowserEngine->AddServer( szAddr, nPort, 0 );
	if ( pInfo )
	{
		pInfo->m_nFullMax = 1;

		CWaitCursor	wait;

		RefreshCriteria_t	criteria;
		memset( &criteria, 0, sizeof( criteria ) );
		criteria.m_nMaxOutstanding = 1;
		criteria.m_nMaxRetries     = ( m_pBrowserEngine->m_nRetries <= 3 )
									 ? m_pBrowserEngine->m_nRetries : 3;
		criteria.m_dStateTimeout   = m_pBrowserEngine->m_dTimeout;
		criteria.m_nPhaseMask      = 2;
		criteria.m_bReportErrors   = 1;
		criteria.m_flOverallTimeout = 3.0;

		CServerInfo*	pSavedNext = pInfo->m_pBatchNext;
		pInfo->m_pBatchNext = NULL;

		CRefreshDlg	refresh( &criteria, pInfo, NULL );
		refresh.DoModal();

		pInfo->m_pBatchNext = pSavedNext;
	}

	RebuildVisibleList();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnSpeedChanged (0x437070)

void CServerBrowserDlg::OnSpeedChanged()
{
	if ( m_bReady )
		SaveConnectionSpeed();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnMinimize (0x437080)

void CServerBrowserDlg::OnMinimize()
{
	if ( !g_bEngineWindowUp )
		::ShowWindow( gLauncherWnd, SW_MINIMIZE );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnDisconnect (0x4370a0)

void CServerBrowserDlg::OnDisconnect()
{
	GameInfo_t	gi;
	int			bConnected = 0;

	if ( engineapi.GetGameInfo( &gi, 0 ) && gi.state == ca_active )
		bConnected = ( gi.active != 0 );

	gBackground = 1;
	if ( bConnected )
	{
		resumeOnSwitch = 0;
		engineapi.Cbuf_AddText( "disconnect\n" );
		Eng_Frame( 1 );
	}
	gBackground = 0;

	LayoutPage( m_nMode );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnResume (0x437130)

void CServerBrowserDlg::OnResume()
{
	SaveConnectionSpeed();

	GameInfo_t	gi;
	if ( engineapi.GetGameInfo( &gi, 0 ) && gi.state == ca_active && gi.active )
	{
		Launcher_StartEngineFg();
		::ShowWindow( ::GetParent( m_hWnd ), SW_HIDE );
		::ShowWindow( gLauncherWnd, SW_HIDE );
		gBackground = 0;
		Rate_ApplyFromConfig();
	}
	else
	{
		LayoutPage( m_nMode );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::BuildFilterHelp (0x4371c0)

void CServerBrowserDlg::BuildFilterHelp()
{
	char	szOut[2048];
	char	szTmp[256];
	int		nFilters = 0;

	szOut[0] = 0;
	Launcher_LoadStringInto( szOut, IDS_FILTER_ACTIVE );
	strcat( szOut, "  " );

	if ( m_filter.m_bByGame )
	{
		Launcher_LoadStringInto( szTmp, IDS_FILTER_GAME, m_filter.m_szGame );
		strcat( szOut, szTmp );
		nFilters = 1;
	}
	if ( m_filter.m_bByMap )
	{
		Launcher_LoadStringInto( szTmp, IDS_FILTER_MAP, m_filter.m_szMap );
		if ( nFilters )	strcat( szOut, ", " );
		strcat( szOut, szTmp );
		++nFilters;
	}

	const struct { int bOn; UINT ids; } kFlags[] =
	{
		{ m_filter.m_bHideEmpty,     IDS_FILTER_NOTEMPTY_SHORT },
		{ m_filter.m_bHideFull,      IDS_FILTER_NOTFULL_SHORT },
		{ m_filter.m_bDedicatedOnly, IDS_FILTER_DEDICATEDSHORT },
		{ m_filter.m_bLinuxOnly,     IDS_FILTER_LINUXSHORT },
		{ m_filter.m_bFavoritesOnly, IDS_FILTER_FAVORITESONLY },
	};
	for ( int i = 0; i < (int)ARRAYSIZE( kFlags ); i++ )
	{
		if ( !kFlags[i].bOn )
			continue;
		Launcher_LoadStringInto( szTmp, kFlags[i].ids );
		if ( nFilters )	strcat( szOut, ", " );
		strcat( szOut, szTmp );
		++nFilters;
	}

	if ( m_filter.m_bLimitPing )
	{
		Launcher_LoadStringInto( szTmp, IDS_FILTER_PING, m_filter.m_nPingMax );
		if ( nFilters )	strcat( szOut, ", " );
		strcat( szOut, szTmp );
		++nFilters;
	}

	if ( m_filter.m_bFilterProxies )
	{
		Launcher_LoadStringInto( szTmp, m_filter.m_bProxiesOnly ? IDS_FILTER_SPECTATORPROXY : IDS_FILTER_NOTSPECTATORPROXY );
		if ( nFilters )	strcat( szOut, ", " );
		strcat( szOut, szTmp );
		++nFilters;
	}

	if ( m_lblVersion.m_hWnd )
		m_lblVersion.SetWindowText( nFilters ? szOut : Launcher_LoadString( IDS_FILTER_NONE ) );
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::HasCreateGameButton (0x437700)

BOOL CServerBrowserDlg::HasCreateGameButton()
{
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::GetSettingsSection (0x437710)

const char* CServerBrowserDlg::GetSettingsSection()
{
	return "Settings";
}

/*
==================
NetGame_CreateSheet (0x4379c0)
==================
*/
CNetGameDlg* NetGame_CreateSheet()
{
	return new CNetGameDlg( NULL, 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::CNetGameDlg (0x437a40)

CNetGameDlg::CNetGameDlg( CServerBrowser* pDoc, int nReserved )
	: CPropertySheet()
{
	m_pMsgRing = NULL;
	m_pRoomList = NULL;
	m_pDoc            = pDoc;
	m_unk172          = nReserved;
	m_nRetries        = 3;
	// 5.0f is the query timeout.  Left at 0 the refresh pump's
	// "flSince >= m_flStateTimeout" is true on the first pass, so every server
	// burns its retries and is marked no-response before an info request goes out.
	// NOTE(ox): the 5.0 is from that store; the +48 offset for this field is our own
	// mapping and is not confirmed against it.
	m_dTimeout        = 5.0;
	m_nMaxSockets     = 0;
	m_pTitanSocket = NULL;
	m_pLanSocket0     = 0;
	m_pLanSocket1     = 0;

	// The live-session span the binary's ctor clears in one sweep, including the
	// 32 KB Titan receive buffer.  OnInitDialog tests these before anything has
	// assigned them, so leaving them as heap fill is a crash.
	m_pPage = NULL;
	m_pMaster = NULL;
	m_pServerListHead = NULL;
	m_pIncoming = NULL;
	m_pCurrentRoom = NULL;
	m_pUserList = NULL;
	m_pSelfIdentity = NULL;
	m_pPendingQuery = NULL;
	m_bDirty          = 0;
	m_bListDirty      = 0;
	m_bConnecting     = 0;
	m_bJoinAnnounced  = 0;
	m_bTitanGotData   = 0;
	m_bNeedReconnect  = 0;
	m_nReconnectTries = 3;
	m_nChatSessionId  = 0;
	m_nQueryGeneration = 0;
	m_szStatus[0]     = 0;
	memset( m_msgBuffer, 0, sizeof( m_msgBuffer ) );

	SetListDirty( FALSE );

	SetTitle( Launcher_LoadString( IDS_CHAT_STATUSUNCONNECTED ) );

	// WON room + server message-list objects: each ring closes onto itself.
	m_pMsgRing  = new CRoomList;
	m_pRoomList = new CRoomList;

	// -timeout <seconds> connect timeout override.
	char*	pszTimeout = 0;
	if ( CheckParm( "-timeout", &pszTimeout ) && pszTimeout )
		m_dTimeout = atof( pszTimeout );

	// -retries <n> connect retry count override (default 3).
	m_nRetries = 3;
	char*	pszRetries = 0;
	if ( CheckParm( "-retries", &pszRetries ) && pszRetries )
		m_nRetries = atoi( pszRetries );

	ComputeMaxSockets();

	// WON address resolution on the favorites object
	if ( g_bNetGameSheetOpened )
		gFavorites->ResolveMasterLists();
	g_bNetGameSheetOpened = 1;

	// Load the persisted server-browser document.
	GetDoc();
}

// Chat-transport "WON login required" latch.
int	g_bWonLoginRequired;

BEGIN_MESSAGE_MAP( CServerBrowserDlg, CDialog )
	//{{AFX_MSG_MAP(CServerBrowserDlg)
	ON_WM_ERASEBKGND()
	ON_COMMAND( IDC_NET_REFRESH_ALL, OnRefresh )
	ON_WM_CREATE()
	ON_WM_CTLCOLOR()
	ON_COMMAND( IDOK, OnDoneClicked )
	ON_COMMAND( IDC_BTN_CREATESV, OnCreateServer )
	ON_WM_PAINT()
	ON_COMMAND( IDC_BTN_CONNECT, OnJoin )
	ON_WM_ACTIVATEAPP()
	ON_COMMAND( IDC_BTN_LISTMODE, OnSwitchPage )
	ON_COMMAND( IDC_MAIN_MINIMIZE, OnMinimize )
	ON_COMMAND( IDC_BTN_DISCONNECT, OnDisconnect )
	ON_COMMAND( IDC_BTN_RESUME, OnResume )
	ON_WM_LBUTTONUP()
	ON_CONTROL( LBN_SELCHANGE, IDC_NET_USER_LIST, OnUserListValidate )
	ON_CONTROL( CBN_SELCHANGE, IDC_NET_SPEED_COMBO, OnSpeedChanged )
	ON_COMMAND( IDC_NET_REBUILD_LIST, RebuildVisibleList )
	ON_COMMAND( IDC_BTN_FIND, OnFind )
	ON_COMMAND( IDC_BTN_LISTROOMS, OnCreateRoom )
	ON_COMMAND( IDC_NET_CHAT_SEND, OnChatSend )
	ON_COMMAND( IDC_NET_USER_INSERT, OnInsertUserNick )
	ON_COMMAND( IDC_BTN_INFO, OnServerInfo )
	ON_COMMAND( IDC_BTN_REFRESH, OnRefresh )
	ON_COMMAND( IDC_BTN_UPDATE, OnUpdateFromMaster )
	ON_COMMAND( IDC_BTN_FILTER, OnEditFilter )
	ON_COMMAND( IDC_BTN_ADDSERVER, OnAddByAddress )
	ON_COMMAND( IDC_NET_FAVORITE_ON, OnMarkFavorite )
	ON_COMMAND( IDC_NET_FAVORITE_OFF, OnClearFavorite )
	ON_COMMAND( IDC_NET_DELETE_SELECTED, OnDeleteSelected )
	ON_COMMAND( IDC_NET_REFRESH_SELECTED, OnRefreshSelected )
	ON_COMMAND( IDC_NET_SORT_LIST, OnResortList )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
==================
TitanSocket_SendBuffer (0x437ce0)
==================
*/
static void TitanSocket_SendBuffer( EasyTitanSocket* pSocket, WriteBuffer& wb )
{
	if ( pSocket )
		pSocket->sendBuffer( wb.getBuffer(), wb.getSize(), NULL, 200 );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::ReceiveTitanMsg (0x437d20)
//
// Pull one WON message off the Titan socket. The transport prefixes a
// 12-byte header the CWONMsg reader skips. A hard error after data has
// flowed arms the reconnect path.

int CNetGameDlg::ReceiveTitanMsg( EasyTitanSocket* pSocket, void* pBuffer, CWONMsg* pMsg,
	unsigned int* pnService, unsigned int* pnMsgType, unsigned int nMax )
{
	unsigned int	nGot = nMax;
	int			nErr;

	if ( !pSocket )
		return 0;

	nErr = pSocket->recvTMessage( pBuffer, &nGot, pnService, pnMsgType, nMax );
	if ( nErr == ES_TIMED_OUT )		// nothing pending yet
		return 0;

	if ( nErr )
	{
		if ( m_bTitanGotData )
		{
			m_bNeedReconnect   = 1;
			m_nReconnectTries  = 3;
		}
		return 0;
	}

	m_bTitanGotData   = 1;
	m_bJoinAnnounced  = 1;
	pMsg->SetBuffer( (const BYTE*)pBuffer + 12, nGot - 12 );
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::OnMemberList (0x437db0)

void CNetGameDlg::OnMemberList( CWONMsg* pMsg, BOOL bResetList )
{
	WORD		nCount = 0;
	int		i;
	long		lSelfStatus;
	DWORD		dwStatus;
	wchar_t		wszNick[256];
	char		szNick[256];
	CChatUser*	pUser;

	pMsg->ReadShort( &nCount );
	if ( bResetList )
	{
		RemoveAllUsers();

		lSelfStatus = m_pSelfIdentity->GetSelfStatusRaw();
		AddUser( m_pSelfIdentity->GetPlayerName(), lSelfStatus );
	}

	for ( i = 0; i < nCount; i++ )
	{
		dwStatus = 0;
		if ( !pMsg->ReadLong( &dwStatus ) )
			break;
		if ( !pMsg->ReadWString( wszNick, 256 ) )
			break;

		WideCharToMultiByte( CP_ACP, 0, wszNick, -1, szNick, sizeof( szNick ), NULL, NULL );

		pUser = AddUser( szNick, dwStatus );

		if ( m_pPage && !bResetList )
			ChatUserList_AddRow( m_pPage, pUser );
	}

	if ( m_pPage && bResetList )
		ChatUserList_Reseed( m_pPage );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::OnUsersLeft (0x438000)
//
// WON opcode 5.

BOOL CNetGameDlg::OnUsersLeft( CWONMsg* pMsg )
{
	WORD		nCount = 0;
	int		i;
	DWORD		dwUserId;
	CChatUser*	pUser;

	if ( !pMsg->ReadShort( &nCount ) )
	{
		printf( "Invalid message received.\n" );
		exit( 1 );
	}

	for ( i = 0; i < nCount; i++ )
	{
		dwUserId = 0;
		if ( !pMsg->ReadLong( &dwUserId ) )
		{
			printf( "Invalid message received.\n" );
			exit( 1 );
		}

		if ( FindUser( dwUserId ) )
		{
			pUser = FindAndUnlinkUser( dwUserId );

			if ( m_pPage )
				ChatUserList_RemoveRow( m_pPage, pUser );

			if ( pUser )
				delete pUser;
		}
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::OnChatText (0x438120)
//
// WON opcode 7.

BOOL CNetGameDlg::OnChatText( CWONMsg* pMsg )
{
	DWORD		dwUserId = 0;
	WORD		wFlag = 0;
	int		cbText;
	const BYTE*	pText;
	char		szText[512];
	int		cb;
	CChatUser*	pUser;
	char		szLine[600];

	if ( !pMsg->ReadLong( &dwUserId ) || !pMsg->ReadShort( &wFlag ) )
	{
		printf( "Invalid message received.\n" );
		exit( 1 );
	}

	cbText = 0;
	pText = pMsg->ReadRemaining( &cbText );
	cb = ( cbText >= (int)sizeof( szText ) - 1 ) ? (int)sizeof( szText ) - 1 : cbText;
	if ( cb < 0 ) cb = 0;
	memcpy( szText, pText, cb );
	szText[cb] = 0;

	if ( wFlag == CHAT_TEXT_BROADCAST )
	{
		pUser = FindUser( dwUserId );
		if ( pUser && m_pPage && m_pPage->GetChatText() )
		{
			if ( !_strnicmp( szText, "/me", 3 ) && strlen( szText ) >= 5 )
			{
				// "/me" emote -> action line: "<nick> <body>" with no <> framing
				_snprintf( szLine, sizeof( szLine ) - 1, "%s %s", pUser->m_szNick, szText + 4 );
				szLine[sizeof( szLine ) - 1] = 0;
				m_pPage->GetChatText()->AddChatLine( 0, NULL, szLine );
			}
			else
			{
				// sink frames "<nick> text"
				m_pPage->GetChatText()->AddChatLine( 0, pUser->m_szNick, szText );
			}
		}
	}
	else if ( wFlag == CHAT_SERVER_ERROR )
	{
		printf( " *** Server Error : %s\n", szText );
	}
	// any other flag value is silently dropped
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::SendChatText (0x438430)
//
// Flood-limited: at least 0.1 s between lines, plus a length-proportional
// cooldown of (len / 20) * 0.25 s before the next one is accepted.

void CNetGameDlg::SendChatText( const void* pText, int cbText )
{
	double	flNow = engineapi.Sys_FloatTime();

	if ( flNow - s_flLastChatSend < 0.1
	  || ( s_flChatCooldown != 0.0 && flNow - s_flLastChatSend < s_flChatCooldown ) )
	{
		ChatPrintf( Launcher_LoadString( IDS_CHAT_FLOOD ) );
		return;
	}

	s_flChatCooldown = ( cbText / 20 ) * 0.25;
	s_flLastChatSend = flNow;

	WriteBuffer	wb( 0x100 );
	wb.appendLong( 0 );				// length, patched below
	wb.appendLong( WONMsg::ChatServer );
	wb.appendLong( WONMsg::Chat_SimpleChatDataMessage );
	wb.appendLong( m_nChatSessionId );
	wb.appendShort( CHAT_TEXT_BROADCAST );
	wb.append( pText, cbText );
	wb.setLong( 0, wb.getSize() );

	TitanSocket_SendBuffer( m_pTitanSocket, wb );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::ServiceChat (0x4385a0)

void CNetGameDlg::ServiceChat()
{
	CWONMsg		msg;
	unsigned int	nService = 0;
	unsigned int	nMsgType = 0;

	if ( !m_pTitanSocket )
		return;

	// Reconnect path: the socket dropped after data had flowed, so walk the retry
	// budget down and re-enter the room.
	if ( m_bNeedReconnect && m_pSelfIdentity )
	{
		m_bNeedReconnect  = 0;
		m_bTitanGotData   = 0;
		Sleep( 100 );

		if ( m_nReconnectTries >= 0 )
		{
			m_bJoinAnnounced = 1;
			--m_nReconnectTries;
			ChatPrintf( Launcher_LoadString( IDS_CHAT_SOCKETERROR ) );
			if ( JoinRoom( m_pSelfIdentity ) )
				ChatPrintf( Launcher_LoadString( IDS_CHAT_RECONNECTSUCCESS ) );
		}
		else if ( m_bJoinAnnounced )
		{
			m_bJoinAnnounced = 0;
			ChatPrintf( Launcher_LoadString( IDS_CHAT_RECONNECTFAIL ) );
		}
		return;
	}

	if ( !ReceiveTitanMsg( m_pTitanSocket, m_msgBuffer, &msg, &nService, &nMsgType, 10 ) )
		return;

	LOG( "svc %u msg %u", nService, nMsgType );

	switch ( nMsgType )
	{
	case WONMsg::Chat_UserJoined:				OnMemberList( &msg, FALSE );	break;
	case WONMsg::Chat_UserLeft:					OnUsersLeft( &msg );			break;
	case WONMsg::Chat_SimpleChatDataMessage:	OnChatText( &msg );				break;
	case WONMsg::Chat_UsersHere:				OnMemberList( &msg, TRUE );		break;
	default:									break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::CloseTitanSocket (0x4387c0)

void CNetGameDlg::CloseTitanSocket()
{
	delete m_pTitanSocket;
	m_pTitanSocket = NULL;
}

/*
==================
NET_AdrToString (0x4387e0)
==================
*/
char* NET_AdrToString( struct in_addr* pAddr )
{
	return strcpy( s_szAdrToString, inet_ntoa( *pAddr ) );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::FindPlayer (0x438820)
//
// Ask the WON directory for the chat servers, then UDP-probe each one for
// the nickname. Three passes over the list, each reply polled for up to a
// second.

void CNetGameDlg::FindPlayer( const char* pszNick )
{
	CWONMsg	reply;

	ChatPrintf( Launcher_LoadString( IDS_CHAT_SEARCH ), pszNick );

	TitanRequest	dir( m_pSelfIdentity->GetChatHost(), m_pSelfIdentity->GetChatPort() );

	int	nEntries = WONComm_GetDirectory( &dir, WON_DIR_PUBLIC, &reply );
	if ( !nEntries )
	{
		ChatPrintf( Launcher_LoadString( IDS_CHAT_NOINFO ) );
		return;
	}

	std::string*	rgHosts    = new std::string[nEntries];
	std::string*	rgNames    = new std::string[nEntries];
	int*			rgPorts    = new int[nEntries];
	int*			rgAnswered = new int[nEntries];
	int				nServers   = 0;

	for ( int i = 0; i < nEntries; i++ )
	{
		direntry_t	entry;
		if ( !WON_ParseDirReply( &reply, &entry ) )
		{
			ChatPrintf( Launcher_LoadString( IDS_CHAT_NOFIND ), pszNick );
			delete[] rgHosts;
			delete[] rgNames;
			delete[] rgPorts;
			delete[] rgAnswered;
			return;
		}

		if ( entry.m_type != ET_SERVICE )
			continue;

		char	szName[256];
		WideCharToMultiByte( CP_ACP, 0, entry.m_wsName, -1, szName, sizeof( szName ), NULL, NULL );
		rgNames[nServers] = szName;

		struct in_addr	addr;
		addr.S_un.S_addr = entry.m_addr;
		rgHosts[nServers]    = NET_AdrToString( &addr );
		rgPorts[nServers]    = ntohs( entry.m_port );
		rgAnswered[nServers] = 0;
		++nServers;
	}

	if ( !nServers )
	{
		ChatPrintf( Launcher_LoadString( IDS_CHAT_NOFIND ), pszNick );
		delete[] rgHosts;
		delete[] rgNames;
		delete[] rgPorts;
		delete[] rgAnswered;
		return;
	}

	// The probe packet: 03 01 03 <server index:16> 00 <wide nickname>
	WriteBuffer	wb( 0x100 );
	wb.appendByte( ROOMQ_HEADER0 );
	wb.appendByte( ROOMQ_HEADER1 );
	wb.appendByte( ROOMQ_FINDPLAYER_REQUEST );
	wb.appendShort( 0 );			// index, patched per pass
	wb.appendByte( 0 );

	wchar_t	wszNick[256];
	MultiByteToWideChar( CP_ACP, 0, pszNick, -1, wszNick, 256 );
	wb.appendWString( wszNick );

	EasySocket	udp( EasySocket::UDP );
	int			bFound  = 0;
	int			nPasses = 3 * nServers;
	DWORD		dwStart = GetTickCount();

	for ( int iPass = 0; iPass < nPasses; iPass++ )
	{
		int	iServer = iPass % nServers;
		wb.setShort( 3, (short)iServer );

		udp.sendBufferTo( wb.getBuffer(), wb.getSize(),
						  rgHosts[iServer], rgPorts[iServer], 10 );
		dwStart = GetTickCount();

		// Poll for a 03 01 04 <index> reply until the second runs out.
		do
		{
			BYTE	rgReply[6];
			if ( udp.recvBuffer( rgReply, sizeof( rgReply ), NULL, 10 ) != ES_NO_ERROR )
				continue;

			if ( rgReply[0] != ROOMQ_HEADER0 || rgReply[1] != ROOMQ_HEADER1
			  || rgReply[2] != ROOMQ_FINDPLAYER_REPLY )
				continue;

			unsigned short	iFrom = *(unsigned short*)&rgReply[4];
			if ( iFrom >= nServers || rgAnswered[iFrom] )
				continue;

			rgAnswered[iFrom] = 1;
			bFound            = 1;
			ChatPrintf( Launcher_LoadString( IDS_CHAT_FIND ), pszNick, rgNames[iFrom].c_str() );
			ChatPrintf( Launcher_LoadString( IDS_CHAT_JOINHINT ) );
		}
		while ( GetTickCount() - dwStart <= 1000 );
	}

	delete[] rgHosts;
	delete[] rgNames;
	delete[] rgPorts;
	delete[] rgAnswered;

	if ( !bFound )
		ChatPrintf( Launcher_LoadString( IDS_CHAT_NOFIND ), pszNick );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::FetchRoomList (0x439370)

int CNetGameDlg::FetchRoomList( int /*nFlag*/ )
{
	CWaitCursor		wait;
	CWONMsg			reply;
	CRoomList		local;					// built here, spliced into m_pRoomList at the end
	CChatClient*	pClient = m_pSelfIdentity;
	int				bPrinted = 0;
	int				nCount = 0;

	if ( !crypt.IsAuthenticated() && !Authenticate( 0 ) && !CheckParm( "-noauth", NULL ) )
	{
		if ( !Launcher_GetRestartFlag() )
			ChatPrintf( Launcher_LoadString( IDS_CHAT_NOAUTH ) );
		return 0;
	}

	gFavorites->PartitionList();

	// Three passes over the configured WON servers (the Titan block of
	// woncomm.lst), advancing the chat cursor until one answers with a directory.
	for ( int nTry = 2; nTry >= 0 && nCount <= 0; nTry-- )
	{
		pClient->ResetChatServer();
		while ( pClient->NextChatServer() )
		{
			TitanRequest	dir( pClient->GetChatHost(), pClient->GetChatPort() );

			nCount = WONComm_GetDirectory( &dir, WON_DIR_PUBLIC, &reply );
			if ( nCount > 0 )
				break;

			// Only the first server that fails says so.
			if ( !bPrinted )
			{
				ChatPrintf( Launcher_LoadString( IDS_CHAT_NODIR ) );
				bPrinted = 1;
			}
			pClient->SetChatServerFlag( 1 );	// mark this one as not answering
		}
	}
	if ( nCount <= 0 )
		return 0;

	pClient->SetChatServerFlag( 0 );

	local.Clear();

	int	nIndex = 0;
	for ( int i = 0; i < nCount; i++ )
	{
		direntry_t	entry;

		// A record that will not parse is skipped, not fatal.
		if ( WON_ParseDirReply( &reply, &entry ) && entry.m_type == ET_SERVICE )
		{
			char	szName[256];

			WideCharToMultiByte( CP_ACP, 0, entry.m_wsName, -1, szName, sizeof( szName ), NULL, NULL );

			chatroom_t*	pRoom = local.AddRoom( szName, "No topic", 0, 0, 0, 0, 0 );
			if ( pRoom )
			{
				struct in_addr	addr;

				pRoom->m_nIndex = nIndex++;
				addr.S_un.S_addr = entry.m_addr;
				sprintf( pRoom->m_szAddress, "%s:%i",
						 NET_AdrToString( &addr ), ntohs( entry.m_port ) );
			}
		}
	}

	// Ask every room for its live occupancy over UDP: the query is
	// 03 01 01 <index>, the answer 03 01 02 <index> <players>.  All rooms are in
	// flight at once -- one pass sends to whoever is due, then a single short recv
	// credits whichever room answers first, keyed by the index echoed back.
	WriteBuffer	query( ROOM_PROBE_QUERY_SIZE );
	query.appendByte( ROOMQ_HEADER0 );
	query.appendByte( ROOMQ_HEADER1 );
	query.appendByte( ROOMQ_PLAYERCOUNT_REQUEST );
	query.appendShort( 0 );			// patched per room by setShort( 3, index )

	EasySocket		probe( EasySocket::UDP );
	unsigned long	dwStart = GetTickCount();

	for ( ; ; )
	{
		chatroom_t*	p;
		int			bPending = 0;

		for ( p = local.m_pNext; p != &local; p = p->m_pNext )
		{
			if ( p->m_nProbesLeft <= 0 || p->m_bAnswered )
				continue;

			// Still owed a probe, so the loop keeps running even while throttled.
			bPending = 1;
			if ( GetTickCount() - p->m_dwLastProbe < ROOM_PROBE_INTERVAL )
				continue;

			char	szHost[256];
			int		nPort;

			COM_ParseHostPort( p->m_szAddress, szHost, &nPort, ROOM_PROBE_PORT );

			query.setShort( 3, (short)p->m_nIndex );
			probe.sendBufferTo( query.getBuffer(), query.getSize(),
								std::string( szHost ), nPort, ROOM_PROBE_IO_TIMEOUT );

			p->m_nProbesLeft--;
			p->m_dwLastProbe = GetTickCount();
		}

		if ( !bPending )
			break;
		if ( GetTickCount() - dwStart > ROOM_PROBE_BUDGET )
			break;

		unsigned char	rgProbeReply[9];

		// The cast picks the UDP overload out of four; a bare NULL is ambiguous.
		if ( probe.recvBufferFrom( rgProbeReply, sizeof( rgProbeReply ), (long*)NULL, NULL, NULL,
								   ROOM_PROBE_IO_TIMEOUT ) != ES_NO_ERROR )
			continue;

		if ( rgProbeReply[0] != ROOMQ_HEADER0 || rgProbeReply[1] != ROOMQ_HEADER1
		  || rgProbeReply[2] != ROOMQ_PLAYERCOUNT_REPLY )
			continue;

		unsigned short	nKey = (unsigned short)( rgProbeReply[3] | ( rgProbeReply[4] << 8 ) );

		for ( p = local.m_pNext; p != &local; p = p->m_pNext )
		{
			if ( (unsigned short)p->m_nIndex == nKey )
			{
				p->m_bAnswered = 1;
				p->m_nPlayers  = (unsigned short)( rgProbeReply[5] | ( rgProbeReply[6] << 8 ) );
				break;
			}
		}
	}

	// Move the probed rooms into the sheet's list.
	m_pRoomList->Clear();
	m_pCurrentRoom = NULL;

	for ( chatroom_t* p = local.m_pNext; p != &local; )
	{
		chatroom_t*	pNext = p->m_pNext;

		local.Unlink( p );
		m_pRoomList->Link( p );
		p = pNext;
	}

	if ( strlen( pClient->GetServerAddr()->host_name ) )
		m_pCurrentRoom = m_pRoomList->FindByAddress( pClient->GetServerAddr()->host_name,
													 pClient->GetServerPort() );

	if ( m_pPage )
		m_pPage->UpdateRoomBanner();

	return 1;
}

/*
==================
Rooms_StripHiddenPrefix (0x439d70)

Drop a leading "Lobby " from a room name, then blank the name entirely
when it matches one of the hidden rooms listed in the rooms file.
==================
*/
char* Rooms_StripHiddenPrefix( char** ppszName )
{
	char*	psz = *ppszName;

	if ( !_strnicmp( psz, "Lobby", strlen( "Lobby" ) ) )
	{
		psz += strlen( "Lobby" );
		if ( *psz == ' ' )
			++psz;
	}

	int		nCount  = 0;
	char*	pHidden = NULL;
	Rooms_Load( &pHidden, &nCount );

	if ( pHidden )
	{
		for ( int i = 0; i < nCount; i++ )		// 16-byte name records
		{
			if ( !_strcmpi( psz, pHidden + i * 16 ) )
			{
				*psz = 0;
				break;
			}
		}
		delete[] pHidden;
	}

	*ppszName = psz;
	return psz;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::LaunchChatServer (0x439e20)

int CNetGameDlg::LaunchChatServer( char* pszArgs )
{
	CWaitCursor		wait;
	CWONMsg			reply;
	std::string		sArgs;
	char*			psz = pszArgs;
	const char*		pszPass = NULL;
	const char*		pHit;
	char			szDirAddr[256];					// the directory, "host:port"
	char			szAddr[256];					// the started room, "ip:port"
	u_short			rgPorts[MAX_FACTORY_SERVERS];
	struct in_addr	rgAddrs[MAX_FACTORY_SERVERS];
	int				rgOrder[MAX_FACTORY_SERVERS];
	struct in_addr	addr;
	int				nFound = 0;
	int				nStarted = 0;
	int				bStarted = 0;
	int				i;

	if ( !psz || !*psz )
		return 0;

	Rooms_StripHiddenPrefix( &psz );
	if ( !psz || !*psz )
		return 0;

	// "<room args> -pass <password>": everything before the switch is public, and
	// the password stays NULL when there is no switch -- which selects between the
	// two WONComm_StartProcess shapes below.
	pHit = strstr( psz, " -pass" );
	if ( pHit )
	{
		for ( const char* p = psz; p != pHit; p++ )
			sArgs += *p;
		pszPass = pHit + 6;
	}
	else
	{
		sArgs = psz;
	}

	if ( !crypt.IsAuthenticated() && !Authenticate( 0 ) && !CheckParm( "-noauth", NULL ) )
	{
		if ( !Launcher_GetRestartFlag() )
			ChatPrintf( Launcher_LoadString( IDS_CHAT_NOAUTH ) );
		return 0;
	}

	gFavorites->PartitionList();

	for ( int nTry = 2; nTry >= 0 && !bStarted; nTry-- )
	{
		m_pSelfIdentity->ResetChatServer();
		while ( m_pSelfIdentity->NextChatServer() )
		{
			TitanRequest	dir( m_pSelfIdentity->GetChatHost(), m_pSelfIdentity->GetChatPort() );

			int	nEntries = WONComm_GetDirectory( &dir, WON_DIR_HALFLIFE, &reply );
			if ( !nEntries )
			{
				m_pSelfIdentity->SetChatServerFlag( 1 );
				continue;
			}
			m_pSelfIdentity->SetChatServerFlag( 0 );

			// Collect the factory servers the directory advertises.
			nFound = 0;
			for ( i = 0; i < nEntries && nFound < MAX_FACTORY_SERVERS; i++ )
			{
				direntry_t	entry;

				if ( !WON_ParseDirReply( &reply, &entry ) )
					continue;
				if ( entry.m_type != ET_SERVICE )
					continue;
				if ( wcscmp( entry.m_wsField14, L"TitanFactoryServer" ) )
					continue;

				rgAddrs[nFound].S_un.S_addr = entry.m_addr;
				rgPorts[nFound] = ntohs( entry.m_port );
				nFound++;
			}

			// A directory with no factory in it burns the whole pass rather than
			// moving on to the next chat server.
			if ( !nFound )
				break;

			sprintf( szDirAddr, "%s:%i", m_pSelfIdentity->GetChatHost(),
					 m_pSelfIdentity->GetChatPort() );

			// (sic) the shuffled order is never read, so the candidates below are
			// tried in directory order and the "random" factory server is not.
			for ( i = 0; i < nFound; i++ )
				rgOrder[i] = i;

			for ( i = FACTORY_SHUFFLE_PASSES; i; i-- )
			{
				int	nA = rand() % nFound;
				int	nB = rand() % nFound;
				int	nSwap = rgOrder[nA];

				rgOrder[nA] = rgOrder[nB];
				rgOrder[nB] = nSwap;
			}

			for ( i = 0; i < nFound; i++ )
			{
				addr = rgAddrs[i];

				FactoryRequest	factory( NET_AdrToString( &addr ), rgPorts[i] );

				if ( !pszPass )
					nStarted = WONComm_StartProcess( &factory, "HLChatServ", szDirAddr,
										WON_DIR_PUBLIC, WON_ToWideString( sArgs ), "" );
				else
					nStarted = WONComm_StartProcess( &factory, "HLChatServ", szDirAddr,
										WON_DIR_PUBLIC, WON_ToWideString( sArgs ),
										"-password " + std::string( pszPass ) );

				if ( nStarted )
					break;
			}

			if ( i != nFound )
			{
				bStarted = 1;
				break;
			}
		}
	}

	if ( !bStarted )
		return 0;

	// The room just started becomes the chat server the launcher talks to.
	if ( m_pSelfIdentity )
	{
		CServerAddr	favorite;

		strcpy( favorite.host_name, NET_AdrToString( &addr ) );
		favorite.port      = nStarted;
		favorite.reserved1 = 0;
		favorite.reserved2 = 1;
		gFavorites->entries[SERVERLIST_IRC] = favorite;
	}

	sprintf( szAddr, "%s:%i", NET_AdrToString( &addr ), nStarted );
	m_pRoomList->AddRoom( sArgs.c_str(), sArgs.c_str(), 0, 0, szAddr, 1, pszPass );

	return nStarted;
}

/*
==================
Chat_BuildJoinRequest (0x43acb0)
==================
*/
static void Chat_BuildJoinRequest( const char* pszNick, const char* pszPassword,
	WriteBuffer& wb )
{
	wb.rewind();
	wb.appendLong( 0 );			// length, patched below
	wb.appendLong( WONMsg::ChatServer );
	wb.appendLong( WONMsg::Chat_UserJoin );

	wchar_t	wszNick[256];
	MultiByteToWideChar( CP_ACP, 0, pszNick, -1, wszNick, 256 );
	wb.appendWString( wszNick );

	wb.appendByte( 1 );
	if ( pszPassword && *pszPassword )
		wb.appendString( pszPassword );

	wb.setLong( 0, wb.getSize() );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::JoinRoom (0x43ae60)
//
// Authenticate, open the Titan socket, WON-handshake against the room host,
// then send the join request. A -1103 reply means the room is passworded:
// prompt once and retry with the password attached.

BOOL CNetGameDlg::JoinRoom( CChatClient* pSelf )
{
	if ( !pSelf )
		return FALSE;

	if ( !crypt.IsAuthenticated() && !Authenticate( 0 ) && !CheckParm( "-noauth", 0 ) )
	{
		if ( !Launcher_GetRestartFlag() )
			ChatPrintf( Launcher_LoadString( IDS_CHAT_NOAUTH ) );
		return FALSE;
	}

	CWaitCursor	wait;

	AuthRequest*	pAuth = (AuthRequest*)crypt.GetAuthObject();
	SetQueryGeneration( 0 );

	WriteBuffer	wb( 0x100 );
	Chat_BuildJoinRequest( pSelf->GetPlayerName(), NULL, wb );

	CWONMsg	reply;
	BOOL	bJoined   = FALSE;
	int		nAttempts = 2;
	DWORD	lStatus   = 0;

	while ( !bJoined )
	{
		int	bRetryWithPass = 0;

		for ( ;; )
		{
			if ( bRetryWithPass )
			{
				Chat_BuildJoinRequest( pSelf->GetPlayerName(), pSelf->GetAuthToken(), wb );
			}
			else
			{
				Chat_BuildJoinRequest( pSelf->GetPlayerName(), NULL, wb );

				if ( !m_pTitanSocket )
				{
					m_pTitanSocket = new EasyTitanSocket( EasySocket::TCP, 0x8000 );
					if ( !m_pTitanSocket )
					{
						nAttempts = 0;
						goto retry;
					}
				}

				if ( m_pTitanSocket->connect( pSelf->GetChatHost(),
						pSelf->GetServerPort(), 5000, 1 ) != ES_NO_ERROR )
					goto retry;

				// Skipped entirely under -noauth: there is no certificate to present.
				if ( pAuth )
				{
					WON_AuthCertificate1*	pPeerCert = NULL;

					// Sequence on, encryption off, session id on; the peer key and
					// session id are not collected.
					if ( !pAuth->peerLogin( m_pTitanSocket, pSelf->GetChatHost(),
							pSelf->GetServerPort(), 1, 0, 1,
							&pPeerCert, NULL, NULL, 1 ) )
					{
						ChatPrintf( Launcher_LoadString( IDS_CHAT_NOAUTH ) );
						goto retry;
					}
				}
			}

			TitanSocket_SendBuffer( m_pTitanSocket, wb );

			unsigned int	nService = 0;
			unsigned int	nMsgType = 0;
			if ( !ReceiveTitanMsg( m_pTitanSocket, m_msgBuffer, &reply,
					&nService, &nMsgType, 2500 ) )
				goto retry;

			WORD	wResult = 0;
			BOOL	bRead = reply.ReadShort( &wResult );
			if ( !( reply.ReadLong( &lStatus ) & bRead ) || nService != 50 || nMsgType != 1 )
				goto retry;

			if ( !wResult )
				break;					// joined
			if ( (short)wResult != -1103 )
				goto retry;

			// Passworded room: ask once, stash the answer as the auth token.
			bRetryWithPass = 1;

			CInputDlg	dlg( 0 );
			dlg.SetPrompt( Launcher_LoadString( IDS_ROOM_NEEDPASS ) );
			if ( dlg.DoModal() != IDOK || dlg.m_strInput.IsEmpty() )
			{
				nAttempts = 0;
				goto retry;
			}
			pSelf->SetAuthToken( dlg.m_strInput );
		}
		bJoined = TRUE;

retry:
		if ( --nAttempts < 0 )
			break;
	}

	if ( !bJoined )
	{
		// Give up: name the room we failed to enter, then clear the session.
		chatroom_t*	pRoom = m_pRoomList->FindByAddress( pSelf->GetServerAddr()->host_name,
													   pSelf->GetServerPort() );
		if ( pRoom )
			ChatPrintf( Launcher_LoadString( IDS_CHAT_JOINFAILED ), pRoom->m_szName );

		m_pCurrentRoom = NULL;
		RemoveAllUsers();
		if ( m_pPage )
		{
			ChatUserList_Reseed( m_pPage );
			m_pPage->UpdateRoomBanner();
		}
		return FALSE;
	}

	m_nChatSessionId = lStatus;
	pSelf->SetSelfStatus( lStatus );

	if ( m_pPage && m_pPage->GetChatText() )
		m_pPage->GetChatText()->SetSelfNick( pSelf->GetPlayerName() );

	RemoveAllUsers();
	AddUser( pSelf->GetPlayerName(), lStatus );
	SetQueryGeneration( 1 );

	m_pCurrentRoom = m_pRoomList->FindByAddress( pSelf->GetServerAddr()->host_name,
												 pSelf->GetServerPort() );
	if ( m_pPage )
	{
		ChatUserList_Reseed( m_pPage );
		m_pPage->UpdateRoomBanner();
	}

	m_nReconnectTries = 3;
	m_bTitanGotData   = 1;
	m_bNeedReconnect  = 0;
	m_bJoinAnnounced  = 1;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::GetDoc (0x43b4b0)

CServerBrowser* CNetGameDlg::GetDoc()
{
	if ( !m_pDoc )
		m_pDoc = LoadDoc();
	return m_pDoc;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::CloseSockets (0x43b4c0)

void CNetGameDlg::CloseSockets()
{
	delete m_pMaster;
	delete m_pLanSocket0;
	delete m_pLanSocket1;
	delete m_pSelfIdentity;
	CloseTitanSocket();
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::RequestRoomList (0x43b510)

void CNetGameDlg::RequestRoomList()
{
	if ( m_bConnecting )
		return;

	if ( !m_pSelfIdentity )
		m_pSelfIdentity = new CChatClient( this, gFavorites );

	if ( !crypt.IsAuthenticated() && !Authenticate( 0 ) && !CheckParm( "-noauth", 0 ) )
	{
		if ( !Launcher_GetRestartFlag() )
			Launcher_ShowMessageById( 0, IDS_WON_AUTHFAILURE );
		return;
	}

	if ( !FetchRoomList( 0 ) )
	{
		LOG( "FetchRoomList failed" );
		return;
	}

	// Prefer the room named "Lobby"; otherwise take the first real entry.
	chatroom_t*	pRoom = m_pRoomList->m_pNext;
	while ( pRoom != m_pRoomList )
	{
		if ( !_strnicmp( pRoom->m_szName, "Lobby", strlen( "Lobby" ) ) )
			break;
		pRoom = pRoom->m_pNext;
	}
	if ( pRoom == m_pRoomList )
	{
		pRoom = pRoom->m_pNext;
		if ( pRoom == m_pRoomList )
			return;
	}

	// Point the favourites' first slot at the chosen room and enter it.
	CServerAddr	addr;
	int			nPort = 0;
	COM_ParseHostPort( pRoom->m_szAddress, addr.host_name, &nPort, 6100 );
	addr.port      = (unsigned short)nPort;
	addr.reserved1 = 0;
	addr.reserved2 = 0;
	gFavorites->entries[0] = addr;

	JoinRoom( m_pSelfIdentity );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::LoadDoc (0x43b700)

CServerBrowser* CNetGameDlg::LoadDoc()
{
	if ( CheckParm( "-nopersist", 0 ) || g_bEnforceServerCap )
		this->ClearServers( 1 );	// keep favorites

	if ( !g_pServerBrowser )
		return g_pServerBrowser;

	this->m_pServerListHead = NULL;

	int	bCorrupt = 0;

	if ( !g_bEnforceServerCap )
	{
		const char*	pszFile = g_pServerBrowser->m_szPersistFile;
		FILE*		fp       = fopen( pszFile, "rb" );
		if ( fp )
		{
			fseek( fp, 0, SEEK_END );
			long	nLen = ftell( fp );
			fseek( fp, 0, SEEK_SET );

			char*	pBuf = (char*)malloc( nLen + 1 );
			fread( pBuf, nLen, 1, fp );
			pBuf[nLen] = 0;
			fclose( fp );

			CToken	tok( pBuf );
			tok.SetQuoteMode( 1 );
			tok.SetCommentMode( 1 );

			tok.ParseNextToken();
			if ( strlen( tok.token ) && !strcmp( tok.token, "{" ) )
			{
				do
				{
					tok.ParseNextToken();
					if ( !strlen( tok.token ) || !_strcmpi( tok.token, "}" ) )
						break;

					if ( !_strcmpi( tok.token, "server" ) )
					{
						CServerInfo*	pInfo = new CServerInfo( "?", 27015 );

						char*	pCursor = tok.GetData();
						bCorrupt = !pInfo->LoadFromBuffer( &pCursor );
						tok.SetData( pCursor );			// resync the outer cursor

						if ( bCorrupt )
						{
							delete pInfo;
						}
						else
						{
							pInfo->m_pNext         = this->m_pServerListHead;	// prepend
							this->m_pServerListHead = pInfo;
						}
					}
				}
				while ( !bCorrupt );
			}

			free( pBuf );

			if ( bCorrupt )
			{
				// Corrupt persisted list: offer to discard it.
				CPromptDlg	dlg( 2, 0 );
				dlg.SetMessage( Launcher_LoadString( IDS_FAVSVRS_CORRUPT ) );
				if ( dlg.DoModal() == IDOK )
					_unlink( pszFile );
			}
		}
	}

	if ( CheckParm( "-nopersist", 0 ) || g_bEnforceServerCap )
	{
		netfilter_t	filter;
		this->BuildFilter( &filter );
		this->QueryMaster( &filter );
	}

	return g_pServerBrowser;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::AddUser (0x43bc90)

CChatUser* CNetGameDlg::AddUser( const char* pszNick, long lStatus )
{
	if ( !pszNick )
		return NULL;

	CChatUser*	pUser = new CChatUser( pszNick, lStatus );
	if ( !pUser )
	{
		Launcher_ShowMessageById( 0, IDS_CHAT_USER_NOMEM );		// out of memory
		return NULL;
	}

	if ( m_pUserList )
		pUser->m_pNext = m_pUserList;
	m_pUserList = pUser;
	return pUser;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::AddServer (0x43bd50)

CServerInfo* CNetGameDlg::AddServer( const char* pszAddr, int nPort, int bForceNew )
{
	if ( !pszAddr )
		return NULL;

	if ( nPort == 0 )
		nPort = 26000;

	if ( !bForceNew )
	{
		for ( CServerInfo* p = this->m_pServerListHead; p; p = p->m_pNext )
		{
			if ( !_stricmp( (LPCSTR)p->m_strAddress, pszAddr ) && p->m_nPort == nPort )
				return p;	// existing match
		}
	}

	CServerInfo*	pInfo = new CServerInfo( pszAddr, nPort );	// port doubles as the row userdata
	if ( !pInfo )
	{
		Launcher_ShowMessageById( 0, IDS_MULTI_SERVER_NOMEM );	// out of memory
		return NULL;
	}

	pInfo->m_strGame    = "HALFLIFE";
	pInfo->m_strAddress = pszAddr;
	pInfo->m_nPort      = nPort;
	pInfo->m_strName    = pszAddr;		// placeholder until the info reply fills it

	pInfo->m_pNext          = this->m_pServerListHead;	// prepend
	this->m_pServerListHead  = pInfo;
	return pInfo;
}

/*
==================
ServerBrowser_PruneServers (0x43be60)
==================
*/
void ServerBrowser_PruneServers( CServerInfo** ppHead, int bKeepFavorites )
{
	if ( !ppHead )
		return;

	CServerInfo*	pKept = NULL;
	for ( CServerInfo* p = *ppHead; p; )
	{
		CServerInfo*	pNext = p->m_pNext;
		if ( p->m_bFavorite && bKeepFavorites )
		{
			p->m_pNext = pKept;		// keep -- prepend onto the survivor list
			pKept      = p;
		}
		else
		{
			delete p;				// slot-0 scalar deleting dtor
		}
		p = pNext;
	}
	*ppHead = pKept;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::ClearServers (0x43beb0)

void CNetGameDlg::ClearServers( int bKeepFavorites )
{
	ServerBrowser_PruneServers( &m_pServerListHead, bKeepFavorites );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::RemoveAllUsers (0x43bed0)

void CNetGameDlg::RemoveAllUsers()
{
	CChatUser*	pUser = m_pUserList;
	while ( pUser )
	{
		CChatUser*	pNext = pUser->m_pNext;
		delete pUser;
		pUser = pNext;
	}
	m_pUserList = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::FindAndUnlinkUser (0x43bf00)

CChatUser* CNetGameDlg::FindAndUnlinkUser( long lUserId )
{
	CChatUser*	pPrev = NULL;
	for ( CChatUser* p = m_pUserList; p; pPrev = p, p = p->m_pNext )
	{
		if ( p->m_lStatus == lUserId )
		{
			if ( pPrev )	pPrev->m_pNext = p->m_pNext;
			else			m_pUserList    = p->m_pNext;
			return p;
		}
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::FindUser (0x43bf60)

CChatUser* CNetGameDlg::FindUser( long lUserId )
{
	for ( CChatUser* pUser = m_pUserList; pUser; pUser = pUser->m_pNext )
		if ( pUser->m_lStatus == lUserId )
			return pUser;
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::CountVisible (0x43bf90)

int CNetGameDlg::CountVisible()
{
	int	n = 0;
	for ( CServerInfo* p = m_pServerListHead; p; p = p->m_pNext )
		if ( !p->m_bFiltered )
			++n;
	return n;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::CountPlayers (0x43bfc0)

int CNetGameDlg::CountPlayers()
{
	int	n = 0;
	for ( CServerInfo* p = m_pServerListHead; p; p = p->m_pNext )
		if ( !p->GetFiltered() )
			n += p->m_nCurrentPlayers;
	return n;
}

/*
==================
ServerBrowser_SplitFavorites (0x43bff0)
==================
*/
CServerInfo* ServerBrowser_SplitFavorites( CNetGameDlg* pSheet, int bKeepFavorites )
{
	CServerInfo*	pRejects = NULL;
	CServerInfo*	pKept    = NULL;

	for ( CServerInfo* p = pSheet->m_pServerListHead; p; )
	{
		CServerInfo*	pNext = p->m_pNext;
		if ( p->m_bFavorite && bKeepFavorites )
		{
			p->m_pNext = pKept;
			pKept      = p;
		}
		else
		{
			p->m_pNext = pRejects;
			pRejects   = p;
		}
		p = pNext;
	}

	pSheet->m_pServerListHead = pKept;
	return pRejects;
}

/*
==================
ServerBrowser_CompareAddr (0x43c050)
==================
*/
int ServerBrowser_CompareAddr( const void* a, const void* b )
{
	const CServerInfo*	pa = *(const CServerInfo* const*)a;
	const CServerInfo*	pb = *(const CServerInfo* const*)b;

	if ( !pa || !pb )
		return 0;

	int	cmp = _mbscmp( MBSTR( pa->m_strAddress ),
					   MBSTR( pb->m_strAddress ) );
	if ( cmp < 0 )	return -1;
	if ( cmp > 0 )	return 1;

	if ( pa->m_nPort < pb->m_nPort )	return -1;
	return pa->m_nPort > pb->m_nPort;
}

/*
==================
ServerBrowser_MergeIncoming (0x43c0e0)

Copy the freshly queried details from the incoming list onto the matching
records in the live list, then drop the incoming list.
==================
*/
void ServerBrowser_MergeIncoming( CNetGameDlg* pSheet )
{
	if ( !pSheet->m_pIncoming )
		return;

	if ( !pSheet->m_pServerListHead )
	{
		ServerBrowser_PruneServers( &pSheet->m_pIncoming, 0 );
		return;
	}

	int	nIncoming = 0;
	for ( CServerInfo* p = pSheet->m_pIncoming; p; p = p->m_pNext )
		++nIncoming;

	CServerInfo**	rgpIncoming = new CServerInfo*[nIncoming];
	int				n = 0;
	for ( CServerInfo* p = pSheet->m_pIncoming; p; p = p->m_pNext )
		rgpIncoming[n++] = p;

	qsort( rgpIncoming, nIncoming, sizeof( CServerInfo* ), ServerBrowser_CompareAddr );

	for ( CServerInfo* pLive = pSheet->m_pServerListHead; pLive; pLive = pLive->m_pNext )
	{
		if ( pLive->m_bFavorite )
			continue;

		CServerInfo**	ppFound = (CServerInfo**)bsearch( &pLive, rgpIncoming, nIncoming,
									sizeof( CServerInfo* ), ServerBrowser_CompareAddr );
		if ( !ppFound || !*ppFound )
			continue;

		CServerInfo*	pNew = *ppFound;
		pLive->m_strName        = pNew->m_strName;
		pLive->m_strMap         = pNew->m_strMap;
		pLive->m_strGame        = pNew->m_strGame;
		pLive->m_strDir         = pNew->m_strDir;
		pLive->m_nMaxPlayers    = pNew->m_nMaxPlayers;
		pLive->m_nProtocol      = pNew->m_nProtocol;
		pLive->m_strUrl         = pNew->m_strUrl;
		pLive->m_bMod           = pNew->m_bMod;
		pLive->m_nVersion       = pNew->m_nVersion;
		pLive->m_nSize          = pNew->m_nSize;
		pLive->m_bSvSide        = pNew->m_bSvSide;
		pLive->m_bClDll         = pNew->m_bClDll;
		pLive->m_cSvType        = pNew->m_cSvType;
		pLive->m_cSvOs          = pNew->m_cSvOs;
		pLive->m_bPassword      = pNew->m_bPassword;
		strcpy( pLive->m_szHLVersion, pNew->m_szHLVersion );
		pLive->m_dSvPing        = pNew->m_dSvPing;
		pLive->m_flPacketLoss   = pNew->m_flPacketLoss;
		pLive->m_bProxy         = pNew->m_bProxy;
		pLive->m_bProxyTarget   = pNew->m_bProxyTarget;
		pLive->m_strProxyAddress = pNew->m_strProxyAddress;
		pLive->m_dwProxyIp      = pNew->m_dwProxyIp;
		pLive->m_iProxyPort     = pNew->m_iProxyPort;
	}

	delete[] rgpIncoming;
	ServerBrowser_PruneServers( &pSheet->m_pIncoming, 0 );
}

/*
==================
ServerBrowser_BuildFilterInfo (0x43c390)
==================
*/
int ServerBrowser_BuildFilterInfo( const netfilter_t* f, char* pszInfo, int nMaxSize )
{
	pszInfo[0] = 0;

	if ( f->m_bDedicatedOnly )						Info_SetValueForKey( pszInfo, "type",    "d",          nMaxSize );
	if ( f->m_bByGame && f->m_szGame[0] )			Info_SetValueForKey( pszInfo, "gamedir", f->m_szGame,  nMaxSize );
	if ( f->m_bByMap && f->m_szMap[0] )				Info_SetValueForKey( pszInfo, "map",     f->m_szMap,   nMaxSize );
	if ( f->m_bLinuxOnly )							Info_SetValueForKey( pszInfo, "linux",   "1",          nMaxSize );
	if ( f->m_bHideEmpty )							Info_SetValueForKey( pszInfo, "empty",   "1",          nMaxSize );
	if ( f->m_bHideFull )							Info_SetValueForKey( pszInfo, "full",    "1",          nMaxSize );
	if ( f->m_bFilterProxies && f->m_bProxiesOnly )	Info_SetValueForKey( pszInfo, "type",    "p",          nMaxSize );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::BuildFilter (0x43c490)
//
// Each browser page keeps its filter under its own profile section.

void CNetGameDlg::BuildFilter( netfilter_t* f )
{
	const char*	pszSection = m_pPage ? m_pPage->GetSettingsSection() : "Settings";

	memset( f, 0, sizeof( *f ) );

	f->m_bHideNoResponse = Launcher_GetProfileInt( pszSection, "Filter Responded", 0 ) != 0;
	f->m_bHideEmpty      = Launcher_GetProfileInt( pszSection, "Filter Empty",     0 ) != 0;
	f->m_bHideFull       = Launcher_GetProfileInt( pszSection, "Filter Full",      0 ) != 0;
	f->m_bFavoritesOnly  = Launcher_GetProfileInt( pszSection, "Filter Favorite",  0 ) != 0;
	f->m_bLimitPing      = Launcher_GetProfileInt( pszSection, "Filter Ping",      0 ) != 0;
	f->m_bLinuxOnly      = Launcher_GetProfileInt( pszSection, "Filter OS",        0 ) != 0;

	int	bIsProxy    = Launcher_GetProfileInt( pszSection, "Filter IsProxy",    0 ) != 0;
	int	bIsNotProxy = Launcher_GetProfileInt( pszSection, "Filter IsNotProxy", 0 ) != 0;
	f->m_bProxiesOnly    = bIsProxy;
	f->m_bHideProxies    = bIsNotProxy;
	f->m_bFilterProxies  = ( bIsProxy || bIsNotProxy ) ? 1 : 0;

	f->m_bDedicatedOnly = Launcher_GetProfileInt( pszSection, "Filter Dedicated", 0 ) != 0;
	f->m_bByMap         = Launcher_GetProfileInt( pszSection, "Filter Map",       0 ) != 0;

	const char*	pszMap = Launcher_GetProfileString( pszSection, "Filter Map Name", "" );
	strncpy( f->m_szMap, pszMap ? pszMap : "", sizeof( f->m_szMap ) - 1 );
	f->m_szMap[sizeof( f->m_szMap ) - 1] = 0;

	f->m_nPingMax = Launcher_GetProfileInt( pszSection, "Filter PingMax", -1 );
	f->m_bByGame  = Launcher_GetProfileInt( pszSection, "Filter Game",     0 ) != 0;

	const char*	pszGame = Launcher_GetProfileString( pszSection, "Filter Game Name", "Half-Life" );
	if ( !pszGame || !*pszGame || !strcmp( pszGame, "Half-Life" ) )
		pszGame = "valve";		// binary: default game-dir when the name is HL/empty
	strncpy( f->m_szGame, pszGame, sizeof( f->m_szGame ) - 1 );
	f->m_szGame[sizeof( f->m_szGame ) - 1] = 0;
}

/*
==================
QueryMaster_Done

NOTE(ox): ours, not the binary's.  The masterfetch callback table holds
CNetGameDlg::OnMasterQueryDone and ::SetStatusText directly, called with the
owner as their first argument; MSVC cannot spell that, so these forward.
==================
*/
static void QueryMaster_Done( void* pOwner, int nResult )
{
	( (CNetGameDlg*)pOwner )->OnMasterQueryDone( nResult );
}

/*
==================
QueryMaster_Status

NOTE(ox): ours, not the binary's -- see QueryMaster_Done.
==================
*/
static void QueryMaster_Status( void* pOwner, const char* pszFormat, ... )
{
	char	szText[1024];
	va_list	va;

	va_start( va, pszFormat );
	// Bounded because this buffer is the shim's own, not the binary's, and the
	// status strings carry a host name that came off the wire.
	_vsnprintf( szText, sizeof( szText ) - 1, pszFormat, va );
	szText[sizeof( szText ) - 1] = 0;
	va_end( va );

	( (CNetGameDlg*)pOwner )->SetStatusText( "%s", szText );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::QueryMaster (0x43c700)

void CNetGameDlg::QueryMaster( const netfilter_t* pFilter )
{
	// Drop the previous pass's incoming list and re-split the favourites into it;
	// the master reply is merged against what is left.
	ServerBrowser_PruneServers( &m_pIncoming, 0 );
	m_pIncoming = ServerBrowser_SplitFavorites( this, 1 );

	if ( m_pPendingQuery )
	{
		MasterFetch_CloseSocket( m_pPendingQuery );
		delete m_pPendingQuery;
		m_pPendingQuery = NULL;
	}

	m_pPendingQuery = new masterfetch_t;
	if ( m_pPendingQuery )
		MasterFetch_Init( m_pPendingQuery, this,
			QueryMaster_Done, QueryMaster_Status );

	char	szQuery[2048];
	ServerBrowser_BuildFilterInfo( pFilter, szQuery, sizeof( szQuery ) );
	MasterFetch_Start( m_pPendingQuery, szQuery );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::SetPage (0x43c7f0)

void CNetGameDlg::SetPage( CServerBrowserDlg* pPage )
{
	if ( !pPage )
	{
		Launcher_ShowMessageById( 0, IDS_MULTI_NODIALOG );
		exit( 1 );
	}
	m_pPage = pPage;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::RemoveServer (0x43c810)

void CNetGameDlg::RemoveServer( CServerInfo* pInfo )
{
	CServerInfo*	pKept = NULL;

	for ( CServerInfo* p = m_pServerListHead; p; )
	{
		CServerInfo*	pNext = p->m_pNext;
		if ( p == pInfo )
		{
			delete p;
		}
		else
		{
			p->m_pNext = pKept;
			pKept      = p;
		}
		p = pNext;
	}
	m_pServerListHead = pKept;
}

/*
==================
ServerBrowser_RequestPings (0x43c870)
==================
*/
void ServerBrowser_RequestPings( CServerInfo* pHead )
{
	for ( CServerInfo* p = pHead; p; p = p->m_pNext )
	{
		if ( !p->m_bLan && !p->GetFiltered() )
		{
			if ( p->m_pSocket )
				p->CloseSocket();
			p->ClearPlayers();
			p->m_nStatus     = SVQ_QUEUED;
			p->m_nRetry      = 0;
			p->m_dSendTime   = engineapi.Sys_FloatTime();
			p->m_dSvPing     = 0.0;
			p->m_bNoResponse = 0;
		}
	}
}

/*
==================
ServerBrowser_RefreshActiveTimes (0x43c8d0)
==================
*/
int ServerBrowser_RefreshActiveTimes( CServerInfo* pHead )
{
	int	n = 0;
	for ( CServerInfo* p = pHead; p; p = p->m_pNext )
	{
		if ( !p->m_bLan && !p->m_bFiltered )
		{
			p->m_dSendTime = engineapi.Sys_FloatTime();
			++n;
		}
	}
	return n;
}

/*
==================
ServerBrowser_RateBucket (0x4559B0)

Map the effective network rate to a discrete connection-quality bucket.
==================
*/
static int ServerBrowser_RateBucket( float flRate )
{
	if ( flRate > 9500.0f )	return 32;
	if ( flRate > 4900.0f )	return 24;
	if ( flRate > 3400.0f )	return 16;
	if ( flRate > 2900.0f )	return 10;
	if ( flRate > 2400.0f )	return 6;
	return 4;
}

/*
==================
ServerBrowser_SlotScore (0x455A30)

Slot score: 100 when the rate bucket covers the server's max slots, falling
off (into negatives) up to twice the bucket, else 0.
==================
*/
static int ServerBrowser_SlotScore( int nBucket, int nMaxPlayers )
{
	if ( nBucket >= nMaxPlayers )
		return 100;
	if ( nMaxPlayers <= 2 * nBucket )
		return 100 - (int)( (double)nMaxPlayers * 100.0 / (double)nBucket );
	return 0;
}

/*
==================
ServerBrowser_PopScore (0x455AD0)

Population score: peaks when the server is half-full, floored at 20, clamped
to 100; empty / full / max-less servers score 0.
==================
*/
static int ServerBrowser_PopScore( int nCur, int nMaxPlayers )
{
	int	half;
	int	d;
	int	nScore;

	if ( !nMaxPlayers )
		return 0;
	if ( nCur == nMaxPlayers )
		return 0;
	if ( !nCur )
		return 0;

	half = nMaxPlayers / 2;
	if ( !half )
		half = 1;
	d = half - abs( half - nCur );
	nScore = 20 - (int)( (double)d * -80.0 / (double)half );
	if ( nScore < 20 )	return 20;
	if ( nScore > 100 )	return 100;
	return nScore;
}

/*
==================
ServerBrowser_PingScore (0x455A70)

Ping score (seconds): <50ms -> 100, scaling to 0 by ~800ms, else 0.
==================
*/
static int ServerBrowser_PingScore( double dPing )
{
	double	ms;

	if ( dPing == 0.0 )
		return 0;
	ms = dPing * 1000.0;
	if ( ms < 50.0 )
		return 100;
	if ( ms <= 800.0 )
		return (int)( ( 800.0 - ms ) * (4.0/3.0) );
	return 0;
}

/*
==================
ServerBrowser_CollectJoinable (0x43c910)
==================
*/
void ServerBrowser_CollectJoinable( CNetGameDlg* pSheet, CServerInfo** ppChain,
	CServerInfo** ppBest, int* pnCount, int bSkipLocal )
{
	char	szLocal[64];
	char	szBase[260];
	double	dBestPing = 100000.0;

	*ppBest  = NULL;
	*ppChain = NULL;
	*pnCount = 0;

	// Nothing ever writes s_localIp, so the self-skip below never matches (sic).
	if ( bSkipLocal )
		sprintf( szLocal, "%i.%i.%i.%i", s_localIp[0], s_localIp[1], s_localIp[2], s_localIp[3] );

	// Restrict to the running mod's game directory when one is active.
	const char*	pszWantDir = com_gamedir;
	if ( g_pCurrentMod )
	{
		pszWantDir = g_pCurrentMod->GetKeyString( "gamedir" );
		if ( !strlen( pszWantDir ) )
			pszWantDir = com_gamedir;
	}

	for ( CServerInfo* p = pSheet->m_pServerListHead; p; p = p->m_pNext )
	{
		if ( p->m_nStatus != SVQ_DEAD || p->m_bNoResponse || p->m_dSvPing == 0.0 )
			continue;
		if ( p->m_nMaxPlayers <= 1 || p->m_nCurrentPlayers >= p->m_nMaxPlayers )
			continue;

		if ( g_pCurrentMod )
		{
			if ( ( (LPCSTR)p->m_strDir )[0] == 0 )
				continue;
			COM_FileBase( p->m_strDir, szBase );
			if ( _strcmpi( szBase, pszWantDir ) )
				continue;
		}

		if ( bSkipLocal && !_strcmpi( p->m_strAddress, szLocal ) )
			continue;
		if ( p->m_nProtocol && p->m_nProtocol != g_nDefaultProtocol )
			continue;

		++*pnCount;
		p->m_pJoinNext = *ppChain;
		*ppChain = p;

		if ( p->m_dSvPing < dBestPing )
		{
			dBestPing = p->m_dSvPing;
			*ppBest   = p;
		}
	}
}

/*
==================
ServerBrowser_SortList (0x43cac0)
==================
*/
void ServerBrowser_SortList( int nCount, CServerInfo* pHead )
{
	CServerInfo**	rgp = (CServerInfo**)malloc( 4 * (size_t)nCount );
	memset( rgp, 0, 4 * (size_t)nCount );

	// m_pJoinNext is the sort/join chain, distinct from the m_pNext browse list.
	int	i = 0;
	for ( CServerInfo* p = pHead; p; p = p->m_pJoinNext )
		rgp[i++] = p;

	qsort( rgp, (size_t)nCount, sizeof( CServerInfo* ), ServerBrowser_CompareServers );

	for ( i = 0; i < nCount - 1; ++i )
		rgp[i]->m_pJoinNext = rgp[i + 1];
	rgp[nCount - 1]->m_pJoinNext = NULL;

	free( rgp );
}

/*
==================
NET_CleanServerName (0x43cb50)
==================
*/
char* NET_CleanServerName( const char* pszName )
{
	if ( !pszName || !pszName[0] )
		return strcpy( s_szCleanName, "?" );

	strcpy( s_szCleanName, pszName );
	return s_szCleanName;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::SetQueryGeneration (0x43cbc0)

void CNetGameDlg::SetQueryGeneration( int nGen )
{
	m_nQueryGeneration = nGen;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::IsDirty (0x43cbd0)

int CNetGameDlg::IsDirty()
{
	return m_bDirty;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::SetDirty (0x43cbe0)

void CNetGameDlg::SetDirty( int bDirty )
{
	m_bDirty = bDirty;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::ConnectMaster (0x43cbf0)

void CNetGameDlg::ConnectMaster( const char* pszHost, unsigned int nPort )
{
	m_bDirty = 0;

	if ( m_pMaster )
	{
		delete m_pMaster;
		m_pMaster = NULL;
	}

	ServerBrowser_CreateMasterSocket( this, &m_pMaster );
	if ( !m_pMaster )
		return;

	// Two retries at 50 ms before giving the socket up.
	for ( int nTries = 2; !m_pMaster->Connect( pszHost, nPort ); --nTries )
	{
		Sleep( 50 );
		if ( nTries - 1 < 0 )
		{
			delete m_pMaster;
			m_pMaster = NULL;
			return;
		}
	}

	CMessageBuffer*	pMsg = m_pMaster->m_pBuffer;
	pMsg->SZ_Clear();
	pMsg->MSG_WriteChar( A2M_GETMASTERSERVERS );
	pMsg->MSG_WriteString( g_szPatchVersion );
	pMsg->MSG_WriteString( gpszCmdLine );
	m_pMaster->AsyncSelect( FD_READ );
	m_pMaster->Send( pMsg->GetData(), pMsg->GetCurSize(), 0 );
	pMsg->SZ_Clear();
}

/*
==================
ServerBrowser_CreateMasterSocket (0x43ccf0)
==================
*/
void ServerBrowser_CreateMasterSocket( CNetGameDlg* pSheet, CHLMasterAsyncSocket** ppOut )
{
	*ppOut = new CHLMasterAsyncSocket( pSheet );

	if ( *ppOut && !( *ppOut )->Create( 0, SOCK_DGRAM,
			FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE, NULL ) )
	{
		delete *ppOut;
		*ppOut = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::SetCurrentRoom (0x43cd80)

void CNetGameDlg::SetCurrentRoom( chatroom_t* pRoom )
{
	m_pCurrentRoom = pRoom;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::Authenticate (0x43cda0)
//
// Refresh the WON certificate and report whether we ended up authenticated.
// A -1501 failure is actionable: state 0 means the CD key was rejected
// (clear it and re-prompt), state 1 means the client is out of date (offer
// a restart to patch).

BOOL CNetGameDlg::Authenticate( int /*bForce*/ )
{

	crypt.GetNewCertificate();

	AuthRequest*	pAuth = (AuthRequest*)crypt.GetAuthObject();
	if ( !pAuth )
		return FALSE;

	if ( crypt.IsAuthenticated() )
		return TRUE;

	Launcher_ErrorMessageBox( 0, "%s", pAuth->GetLastError() );

	if ( pAuth->mErrorStatus != -1 && pAuth->mErrorCode == -1501 )
	{
		if ( !pAuth->mErrorStatus )
		{
			Launcher_WriteProfileString( "Settings", "Key", "" );
			if ( !CheckCDKey() )
				PostQuitMessage( 0 );
		}
		else if ( pAuth->mErrorStatus == 1 )
		{
			CPromptDlg	prompt( 2, NULL );
			prompt.SetMessage( Launcher_LoadString( IDS_RUN_PATCH ) );
			if ( prompt.DoModal() == IDOK )
				Launcher_SetRestartFlag( 1 );
		}
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::RefreshPageIfDirty (0x43cff0)

void CNetGameDlg::RefreshPageIfDirty()
{
	if ( IsListDirty() && m_pPage )
		m_pPage->OnRefresh();
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::StartLanQuery (0x43d010)

void CNetGameDlg::StartLanQuery( BOOL bEnable )
{
	SetListDirty( bEnable );

	if ( !m_pLanSocket0 )
		m_pLanSocket0 = new CHLLanAsyncSocket( this );

	CHLLanAsyncSocket*	pSock0 = m_pLanSocket0;
	if ( !pSock0 )
		return;

	if ( !pSock0->Open() )
	{
		delete pSock0;
		m_pLanSocket0 = 0;
		return;
	}

	pSock0->BroadcastQuery();

	if ( !m_pLanSocket1 )
		m_pLanSocket1 = new CHLLanAsyncSocket( this );

	CHLLanAsyncSocket*	pSock1 = m_pLanSocket1;
	if ( !pSock1 )
		return;

	pSock1->m_bIpx = TRUE;			// the IPX broadcast variant
	if ( pSock1->Open() )
	{
		pSock1->BroadcastQuery();
		SetListDirty( TRUE );
	}
	else
	{
		delete pSock1;
		m_pLanSocket1 = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::StopLanQuery (0x43d170)

int CNetGameDlg::StopLanQuery()
{
	if ( m_pLanSocket0 )
		delete m_pLanSocket0;
	if ( m_pLanSocket1 )
		delete m_pLanSocket1;
	m_pLanSocket0 = 0;
	m_pLanSocket1 = 0;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::ChatPrintf (0x43d1b0)

void CNetGameDlg::ChatPrintf( const char* pszFormat, ... )
{
	va_list	va;
	va_start( va, pszFormat );
	vsprintf( s_szFmt, pszFormat, va );
	va_end( va );

	if ( m_pPage && m_pPage->GetChatText() )
		ChatWnd_Printf( m_pPage->GetChatText(), 0, s_szFmt );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::ListRooms (0x43d200)
//
// The chat window's /rooms command.

void CNetGameDlg::ListRooms()
{
	ChatPrintf( "Rooms" );

	for ( chatroom_t* p = m_pRoomList->m_pNext; p != m_pRoomList; p = p->m_pNext )
	{
		ChatPrintf( "%s:  %s, %i %i", p->m_szName, p->m_szAddress,
					p->m_nPlayers, p->m_nGroup );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::JoinRoomByName (0x43d260)
//
// The chat window's /join command.

void CNetGameDlg::JoinRoomByName( const char* pszName )
{
	if ( !pszName || !*pszName )
		return;

	int	cchName = (int)strlen( pszName );

	chatroom_t*	pRoom = m_pRoomList->m_pNext;
	while ( pRoom != m_pRoomList )
	{
		if ( !_strnicmp( pRoom->m_szName, pszName, cchName ) )
			break;
		pRoom = pRoom->m_pNext;
	}

	if ( pRoom == m_pRoomList )
	{
		ChatPrintf( Launcher_LoadString( IDS_CHAT_NOSUCHROOM ), pszName );
		return;
	}

	if ( pRoom->m_nPlayers >= 90 )		// occupancy cap
	{
		ChatPrintf( Launcher_LoadString( IDS_CHAT_ROOMFULL ), pszName, pRoom->m_nPlayers );
		return;
	}

	CServerAddr	addr;
	int			nPort = 0;
	COM_ParseHostPort( pRoom->m_szAddress, addr.host_name, &nPort, 6100 );
	addr.port      = (unsigned short)nPort;
	addr.reserved1 = 0;
	addr.reserved2 = 0;
	gFavorites->entries[0] = addr;

	JoinRoom( m_pSelfIdentity );
}

/*
==================
ServerBrowser_UnlinkServer (0x43d3c0)
==================
*/
CServerInfo* ServerBrowser_UnlinkServer( CServerInfo** ppHead, CServerInfo* pInfo )
{
	if ( !pInfo )
		return NULL;
	if ( !ppHead )
		return pInfo;

	CServerInfo*	pPrev = *ppHead;
	if ( pPrev == pInfo )
	{
		*ppHead = pPrev->m_pNext;
		pInfo->m_pNext = NULL;
		return pInfo;
	}

	for ( CServerInfo* p = pPrev->m_pNext; p; p = p->m_pNext )
	{
		if ( p == pInfo )
		{
			pPrev->m_pNext = pInfo->m_pNext;
			pInfo->m_pNext = NULL;
			return pInfo;
		}
		pPrev = p;
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::ComputeMaxSockets (0x43d430)

int CNetGameDlg::ComputeMaxSockets()
{
	int	rate;
	int	budget;
	int	cap;

	rate = (int)g_pServerBrowser->m_playerConfig.rate;
	if ( rate <= 5100 )
	{
		if ( rate <= 3600 )
			budget = ( rate <= 2600 ) ? 15 : 20;
		else
			budget = 40;
	}
	else
	{
		budget = 100;
	}

	cap = WSA_GetNumSockets();
	m_nMaxSockets = ( budget < cap ) ? budget : cap;

	char*	pszVal = 0;
	if ( CheckParm( "-maxsockets", &pszVal ) && pszVal )
		m_nMaxSockets = atoi( pszVal );

	if ( m_nMaxSockets <= 4 )
		m_nMaxSockets = 4;
	return m_nMaxSockets;
}

/*
==================
ServerBrowser_DedupeByAddr (0x43d4d0)

Address-sort the live list, then delete adjacent duplicates.
==================
*/
void ServerBrowser_DedupeByAddr( CNetGameDlg* pSheet )
{
	int	nServers = 0;
	for ( CServerInfo* p = pSheet->m_pServerListHead; p; p = p->m_pNext )
		++nServers;
	if ( nServers <= 1 )
		return;

	CServerInfo**	rgpSorted = new CServerInfo*[nServers];
	int				n = 0;
	for ( CServerInfo* p = pSheet->m_pServerListHead; p; p = p->m_pNext )
		rgpSorted[n++] = p;

	qsort( rgpSorted, nServers, sizeof( CServerInfo* ), ServerBrowser_CompareAddr );

	pSheet->m_pServerListHead = rgpSorted[0];
	for ( int i = 0; i < nServers - 1; i++ )
		rgpSorted[i]->m_pNext = rgpSorted[i + 1];
	rgpSorted[nServers - 1]->m_pNext = NULL;
	delete[] rgpSorted;

	CServerInfo*	pPrev = NULL;
	CServerInfo*	pKept = NULL;
	for ( CServerInfo* p = pSheet->m_pServerListHead; p; )
	{
		CServerInfo*	pNext = p->m_pNext;
		if ( pPrev
		  && !_mbscmp( MBSTR( pPrev->m_strAddress ),
					   MBSTR( p->m_strAddress ) )
		  && pPrev->m_nPort == p->m_nPort )
		{
			delete p;
		}
		else
		{
			p->m_pNext = pKept;
			pKept      = p;
			pPrev      = p;
		}
		p = pNext;
	}
	pSheet->m_pServerListHead = pKept;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::OnMasterQueryDone (0x43d5e0)

void CNetGameDlg::OnMasterQueryDone( int bFailed )
{
	if ( !m_pPendingQuery )
		return;

	if ( bFailed )
	{
		SetStatusText( "Couldn't get list of servers." );
	}
	else
	{
		ServerBrowser_DedupeByAddr( this );
		ServerBrowser_MergeIncoming( this );
		if ( m_pPage )
			m_pPage->RebuildVisibleList();
		RefreshPageIfDirty();
	}

	MasterFetch_CloseSocket( m_pPendingQuery );
	delete m_pPendingQuery;
	m_pPendingQuery = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::SetStatusText (0x43d650)

void CNetGameDlg::SetStatusText( const char* pszFormat, ... )
{
	va_list	va;
	va_start( va, pszFormat );
	vsprintf( s_szFmt, pszFormat, va );
	va_end( va );

	strcpy( m_szStatus, s_szFmt );
	if ( m_pPage )
		m_pPage->m_lblStatus.SetWindowText( s_szFmt );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::ServicePendingQuery (0x43d6c0)

void CNetGameDlg::ServicePendingQuery()
{
	if ( m_pPendingQuery )
		MasterFetch_Service( m_pPendingQuery );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::HasPendingQuery (0x43d6d0)

BOOL CNetGameDlg::HasPendingQuery()
{
	return m_pPendingQuery != NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg::Pump (0x43d6e0)

void CNetGameDlg::Pump()
{
	ServiceChat();
	ServicePendingQuery();
}

/////////////////////////////////////////////////////////////////////////////
// Bodies emitted outside this band -- ICF folds and the document
// functions the linker placed with the player-info dialog.  Ordered by
// address among themselves.

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnPaint (0x412860)

void CServerBrowserDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnEraseBkgnd (0x412870)

BOOL CServerBrowserDlg::OnEraseBkgnd( CDC* pDC )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnActivateApp (0x4311f0)

void CServerBrowserDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	Default();

	ActiveApp = bActive;
	if ( bActive && gDLLState == DLL_ACTIVE && gEngineVidType == VT_Direct3D )
		AppActivate( bActive, 0 );
}

/*
==================
CServerBrowser::CServerBrowser (0x453100)
==================
*/
CServerBrowser::CServerBrowser()
{
	CString	str;

	str = Launcher_GetProfileString( "Settings", "Logo", "None" );
	strcpy( m_szLogoName, str );
	str = Launcher_GetProfileString( "Settings", "Logo Color", "Orange" );
	strcpy( m_szLogoColor, str );

	memset( &m_playerConfig, 0, sizeof( m_playerConfig ) );
}

/*
==================
CServerBrowser::~CServerBrowser (0x4531f0)
==================
*/
CServerBrowser::~CServerBrowser()
{
	CFG_FreeBindings( m_playerConfig.m_binds );
}

/*
==================
CServerBrowser::SaveFavoriteServers (0x453200)
==================
*/
void CServerBrowser::SaveFavoriteServers( void )
{
	sprintf( m_szPersistFile, "%s/favsvrs.dat", gFavorites->GetBaseDir() );
	Launcher_LoadPlayerInfo( "Player", &m_playerConfig );
}

/*
==================
Launcher_SaveFavoriteServers

NOTE(ox): ours, not the binary's.  vid_win.cpp is compiled without the MFC
headers and cannot name CServerBrowser.
==================
*/
void Launcher_SaveFavoriteServers( void* pBrowser )
{
	( (CServerBrowser*)pBrowser )->SaveFavoriteServers();
}

/*
==================
CServerBrowser::GetPlayerName (0x453230)
==================
*/
char* CServerBrowser::GetPlayerName( void )
{
	return m_playerConfig.name;
}

/*
==================
ServerBrowser_CopyConfig (0x453240)

m_pad132 is deliberately not copied.
==================
*/
void ServerBrowser_CopyConfig( CServerBrowser* pDest, const CServerBrowser* pSrc )
{
	strcpy( pDest->m_szPersistFile, pSrc->m_szPersistFile );
	strcpy( pDest->m_szLogoName, pSrc->m_szLogoName );
	strcpy( pDest->m_szLogoColor, pSrc->m_szLogoColor );
	CFG_CopyConfig( pDest->m_playerConfig.m_binds,
		pSrc->m_playerConfig.m_binds );
}

/*
==================
ServerBrowser_CompareServers (0x455b50)
==================
*/
static int __cdecl ServerBrowser_CompareServers( const void* a, const void* b )
{
	CServerInfo*	pa = *(CServerInfo* const*)a;
	CServerInfo*	pb = *(CServerInfo* const*)b;

	if ( !pa || !pb )
		return 0;

	if ( pa->m_bProxy )
		return pb->m_bProxy ? 0 : -1;
	if ( pb->m_bProxy )
		return 1;

	if ( pa->m_dSvPing == 0.0 && pb->m_dSvPing == 0.0 )
		return 0;
	if ( pa->m_dSvPing != 0.0 && pb->m_dSvPing == 0.0 )
		return -1;
	if ( pa->m_dSvPing == 0.0 && pb->m_dSvPing != 0.0 )
		return 1;

	int	aUnnamed = !_mbscmp( MBSTR( pa->m_strMap ), (const unsigned char*)"?" );
	int	bUnnamed = !_mbscmp( MBSTR( pb->m_strMap ), (const unsigned char*)"?" );
	if ( aUnnamed && bUnnamed )
		return 0;
	if ( aUnnamed && !bUnnamed )
		return 1;
	if ( !aUnnamed && bUnnamed )
		return -1;

	if ( pa->m_cSvType == 'd' )
	{
		if ( pb->m_cSvType != 'd' )
			return -1;
	}
	else if ( pb->m_cSvType == 'd' )
	{
		return 1;
	}

	int	aPop = ServerBrowser_PopScore( pa->m_nCurrentPlayers, pa->m_nMaxPlayers );
	int	bPop = ServerBrowser_PopScore( pb->m_nCurrentPlayers, pb->m_nMaxPlayers );
	if ( aPop )
	{
		if ( !bPop )
			return -1;
	}
	else if ( bPop )
	{
		return 1;
	}

	int	nBucket = ServerBrowser_RateBucket(
		g_pServerBrowser->m_playerConfig.rate );
	int	aSlots  = ServerBrowser_SlotScore( nBucket, pa->m_nMaxPlayers );
	int	bSlots  = ServerBrowser_SlotScore( nBucket, pb->m_nMaxPlayers );

	int	aScore = 3 * aPop + 5 * aSlots + 4 * ServerBrowser_PingScore( pa->m_dSvPing );
	int	bScore = 3 * bPop + 5 * bSlots + 4 * ServerBrowser_PingScore( pb->m_dSvPing );
	if ( aScore < bScore )	return 1;
	if ( aScore > bScore )	return -1;

	if ( pa->GetServerId() < pb->GetServerId() )	return -1;
	return pb->GetServerId() < pa->GetServerId() ? 1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg::OnLButtonUp (0x455e00)
//
// A bare thunk to CWnd::Default.

void CServerBrowserDlg::OnLButtonUp( UINT /*nFlags*/, CPoint /*point*/ )
{
	Default();
}
