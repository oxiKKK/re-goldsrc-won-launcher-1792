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
// Purpose: CCreateServerDlg, the Create Server page.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Entries at 0x4AD2F8, base map 0x4B4398 = CDialog.  Notification code 1 on the
// two custom controls has no named xxN_* constant in this build.
BEGIN_MESSAGE_MAP( CCreateServerDlg, CDialog )
	//{{AFX_MSG_MAP(CCreateServerDlg)
	ON_MESSAGE( WM_DISPLAYCHANGE, OnDisplayChange )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_ACTIVATEAPP()
	ON_BN_CLICKED( IDC_CREATESERVER_ADVANCED_MULTIPLAYER, OnAdvanced )
	ON_CONTROL( 1, IDC_CREATESERVER_MAPNOTIFY, OnMapListNotify )
	ON_CONTROL( 1, IDC_CREATESERVER_MAPLIST, OnMapListValidate )
	ON_BN_CLICKED( IDC_CREATESERVER_DEDICATED, OnDedicated )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CCreateServerDlg::CCreateServerDlg (0x407070)
CCreateServerDlg::CCreateServerDlg( CNetGameDlg* pBrowser, CWnd* pParent )
	: CDlgBase( IDD_CREATESERVER, pParent )
{
	m_pBrowser     = pBrowser;
	m_pSelfWnd     = this;		// gates the slide transition
	m_pMapList     = NULL;
	m_pDescription = NULL;

	LoadHeaderBitmap( "head_creategame", NULL );
	m_bkBrush.CreateSolidBrush( RGB( 0, 0, 0 ) );
	LoadButtonStrips();
}

// CCreateServerDlg::~CCreateServerDlg (0x407230)
CCreateServerDlg::~CCreateServerDlg()
{
	// Compiler-generated teardown only; the description outlives the page.
}

// CCreateServerDlg::DoDataExchange (0x407440)
void CCreateServerDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_CREATESERVER_DEDICATED,             m_btnDedicated );	// 1043
	DDX_Control( pDX, IDC_CREATESERVER_ADVANCED_MULTIPLAYER,  m_btnAdvanced );	// 29
	// These four ids are the template's STATIC labels, not the entry fields.
	DDX_Control( pDX, IDC_CREATESERVER_NAME,                  m_lblName );		// 1110
	DDX_Control( pDX, IDC_CREATESERVER_MAXPLAYERS,            m_lblMaxPlayers );// 1111
	DDX_Control( pDX, IDC_CREATESERVER_MAP,                   m_lblMap );		// 1112
	DDX_Control( pDX, IDC_NEWPROFILE_PASSWORD,                m_lblPassword );	// 1115
	DDX_Control( pDX, IDOK,                                   m_btnOK );
	DDX_Control( pDX, IDCANCEL,                               m_btnCancel );

	// The read-back is gated on the list existing, not on the exchange
	// direction, and it goes through each edit's inner control.
	if ( !m_pMapList )
		return;

	CString	str;

	if ( m_editMaxPlayers.m_pEdit )
		m_editMaxPlayers.m_pEdit->GetWindowText( str );
	m_nMaxPlayers = atoi( str );
	if ( m_nMaxPlayers < 2 )
		m_nMaxPlayers = 2;
	if ( m_nMaxPlayers > 32 )
		m_nMaxPlayers = 32;

	if ( m_editName.m_pEdit )
		m_editName.m_pEdit->GetWindowText( m_strScratch );
	strncpy( m_szHostName, m_strScratch, sizeof( m_szHostName ) - 1 );
	m_szHostName[sizeof( m_szHostName ) - 1] = 0;

	if ( m_editPassword.m_pEdit )
		m_editPassword.m_pEdit->GetWindowText( m_strScratch );
	strncpy( m_szPassword, m_strScratch, sizeof( m_szPassword ) - 1 );
	m_szPassword[sizeof( m_szPassword ) - 1] = 0;

	int	nSel = m_pMapList->GetCurSel();
	if ( nSel != -1 )
	{
		strncpy( m_szMap, m_pMapList->GetText( nSel ), sizeof( m_szMap ) );
		m_szMap[sizeof( m_szMap ) - 1] = 0;

		char*	pDot = strstr( m_szMap, "." );		// drop any extension
		if ( pDot )
			*pDot = 0;
	}
	else
	{
		sprintf( m_szMap, "unknown" );
	}
}

// CCreateServerDlg::OnInitDialog (0x407660)
BOOL CCreateServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	int	wh[2] = { 0, 0 };

	Launcher_HeaderSize( wh );

	int	cw = wh[0];
	int	ch = wh[1];

	// The binary has no fallback here.  It does not need one: by the time it
	// reaches this page the create-game strip is always the loaded header, so
	// the cell height is never zero.  Ours can be, and every control below is
	// sized off it -- a zero leaves the page blank.
	if ( ch <= 0 )
		ch = 20;

	int	xField = cw + 60;					// 0x407979 -- middle (label/entry) column
	int	wField = cw;
	int	xRight = 2 * cw + 70;				// 0x407B06 -- the map column
	int	xList  = g_nLauncherDefW - 50;
	int	yList  = g_nLauncherDefH - 20;

	// The three command buttons are captioned first, then the labels.
	m_btnAdvanced.SetWindowText( Launcher_LoadString( IDS_BTN_ADVANCEDSVR ) );
	m_btnCancel.SetWindowText( Launcher_LoadString( IDS_BTN_CANCEL ) );
	m_btnOK.SetWindowText( Launcher_LoadString( IDS_BTN_OK ) );

	// The labels are CODStatic: their text goes in via SetWindowText, which is
	// what CODStatic::OnPaint draws.
	struct	lbl_t { CODStatic* p; int y; UINT id; };
	lbl_t	labels[] =
	{
		{ &m_lblName,       140, IDS_CREATESERVER_NAME       },	// 0x9A
		{ &m_lblMaxPlayers, 199, IDS_CREATESERVER_MAXPLAYERS },	// 0x9E
		{ &m_lblPassword,   253, IDS_NEWPROFILE_PASSWORD     },	// 0xCE
	};
	for ( int i = 0; i < (int)ARRAYSIZE( labels ); i++ )
	{
		labels[i].p->MoveWindow( xField, labels[i].y, wField, ch, TRUE );
		labels[i].p->SetTransparent( 1 );
		labels[i].p->SetFontSize( 14, FW_NORMAL );
		labels[i].p->SetTextColor( RGB( 255, 255, 255 ) );
		labels[i].p->SetWindowText( Launcher_LoadString( labels[i].id ) );
	}
	m_lblMap.MoveWindow( xRight, 140, xList - xRight, ch, TRUE );
	m_lblMap.SetTransparent( 1 );
	m_lblMap.SetFontSize( 14, FW_NORMAL );
	m_lblMap.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblMap.SetWindowText( Launcher_LoadString( IDS_CREATESERVER_MAP ) );	// 0x9B

	// Entry fields, created here (the template only carries the labels).
	CRect	rcTmp( 0, 0, 100, 100 );
	if ( !m_editName.GetSafeHwnd() )
		m_editName.Create( 0, &rcTmp, this, 1040 );
	m_editName.SetBorderColor( RGB( 56, 56, 56 ) );			// 0x383838
	m_editName.SetText( g_pServerBrowser->GetPlayerName() );
	m_editName.SetActive( 1 );

	if ( !m_editMaxPlayers.GetSafeHwnd() )
		m_editMaxPlayers.Create( 0, &rcTmp, this, 1042 );
	m_editMaxPlayers.SetBorderColor( RGB( 56, 56, 56 ) );
	m_editMaxPlayers.SetText( "8" );
	m_editMaxPlayers.SetActive( 1 );

	if ( !m_editPassword.GetSafeHwnd() )
		m_editPassword.Create( 0, &rcTmp, this, 126 );
	m_editPassword.SetBorderColor( RGB( 56, 56, 56 ) );
	m_editPassword.SetText( "" );
	m_editPassword.SetActive( 1 );

	m_editName.MoveWindow( xField, 167, wField, ch, TRUE );
	m_editMaxPlayers.MoveWindow( xField, 226, wField, ch, TRUE );
	m_editPassword.MoveWindow( xField, 280, wField, ch, TRUE );

	// The map list: its own CODListBox with child id 1006 (0x407B41), filling the
	// right column under the "Map:" label.
	if ( !m_pMapList )
		m_pMapList = new CODListBox();
	if ( m_pMapList )
	{
		if ( !m_pMapList->GetSafeHwnd() )
		{
			RECT	rcList = { xRight, 172, xList, yList };
			m_pMapList->Create( WS_CHILD | WS_VISIBLE, &rcList, this, 1006 );
		}
		m_pMapList->MoveWindow( xRight, 172, xList - xRight, yList - 172, TRUE );
	}

	// Dedicated checkbox along the bottom.  The caption is a full sentence, so
	// it wraps -- DT_WORDBREAK, not the ctor's DT_SINGLELINE.
	m_bDedicated = 0;
	::SetWindowLong( m_btnDedicated.GetSafeHwnd(), GWL_STYLE,
		m_btnDedicated.GetStyle() | BS_OWNERDRAW );
	m_btnDedicated.SetWindowText( Launcher_LoadString( IDS_DEDICATED ) );
	m_btnDedicated.m_textFlags = DT_WORDBREAK;
	m_btnDedicated.m_bChecked  = m_bDedicated;
	::InvalidateRect( m_btnDedicated.m_hWnd, NULL, TRUE );
	m_btnDedicated.MoveWindow( 50, yList - 2 * ch, 2 * cw, 2 * ch, TRUE );

	if ( g_pCurrentMod )
		PopulateMapList( g_pCurrentMod->GetKey( "gamedir" ) );
	else
		PopulateMapList( "valve" );

	LayoutHeaderButtons();
	::InvalidateRect( m_hWnd, NULL, TRUE );
	ShowWindow( SW_RESTORE );
	::UpdateWindow( m_hWnd );
	return TRUE;
}

// CCreateServerDlg::LoadButtonStrips (0x407390)
void CCreateServerDlg::LoadButtonStrips()
{
	m_bHeaderLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( &m_headerW );		// {w,h}
	m_headerStride  = Launcher_HeaderStride();

	if ( m_bHeaderLoaded )
	{
		m_btnAdvanced.FreeSkinBitmaps();
		m_btnAdvanced.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_ADVANCED_WIDE, m_bHeaderLoaded );
		m_btnOK.FreeSkinBitmaps();
		m_btnOK.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_OK, m_bHeaderLoaded );
		m_btnCancel.FreeSkinBitmaps();
		m_btnCancel.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_BACK, m_bHeaderLoaded );
	}
}

// CCreateServerDlg::RefreshAndShow (0x407C10)
void CCreateServerDlg::RefreshAndShow()
{
	::InvalidateRect( m_hWnd, NULL, TRUE );
	LoadButtonStrips();
	LayoutHeaderButtons();
	SetActiveWindow();
	SetFocus();
	ShowWindow( SW_SHOWNORMAL );
}

// CCreateServerDlg::OnCtlColor (0x407C60)
HBRUSH CCreateServerDlg::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH	hbr = CDialog::OnCtlColor( pDC, pWnd, nCtlColor );

	if ( nCtlColor <= CTLCOLOR_EDIT )
	{
		pDC->SetTextColor( RGB( 255, 127, 24 ) );	// 0x187FFF
		pDC->SetBkMode( TRANSPARENT );
		pDC->SetBkColor( RGB( 0, 0, 0 ) );
		return (HBRUSH)m_bkBrush.GetSafeHandle();
	}
	return hbr;
}

// CCreateServerDlg::PopulateMapList (0x407CC0)
void CCreateServerDlg::PopulateMapList( const char* pszGameDir )
{
	CWaitCursor	wait;

	m_pMapList->ResetContent();

	mapinfo_t*	pList = NULL;
	if ( !COM_GetMapList( &pList, GETMAPS_LOOSE | GETMAPS_PAKS, 1 ) )
		return;

	// Count the maps belonging to this gamedir.  With none, the list falls back
	// to showing every map that is *not* this gamedir's.
	int			nMatch = 0;
	int			nTotal = 0;
	mapinfo_t*	p;

	for ( p = pList; p; p = p->next )
	{
		if ( !_strcmpi( p->gamedir, pszGameDir ) )
			nMatch++;
		nTotal++;
	}

	int	bInvert;
	int	nSlots;

	if ( !nMatch && nTotal )
	{
		bInvert = 1;
		nSlots  = nTotal;
	}
	else if ( nMatch )
	{
		bInvert = 0;
		nSlots  = nMatch;
	}
	else
	{
		COM_FreeMapList( &pList );
		m_editMaxPlayers.SetText( "8" );
		return;
	}

	char**	ppNames = new char*[nSlots];
	int		cnt = 0;

	memset( ppNames, 0, sizeof( char* ) * nSlots );

	for ( p = pList; p; p = p->next )
	{
		int	bTake = ( _strcmpi( p->gamedir, pszGameDir ) == 0 );

		if ( bInvert )
			bTake = !bTake;

		// p->name is "maps/<name>.bsp"; the extension is kept.
		if ( !_strnicmp( p->name, "maps", 4 ) && strlen( p->name ) > 5 && bTake )
		{
			ppNames[cnt] = new char[260];
			memset( ppNames[cnt], 0, 260 );
			sprintf( ppNames[cnt], "%s", p->name + 5 );
			_strlwr( ppNames[cnt] );
			cnt++;
		}
	}

	// Bubble sort, swapping the strings themselves rather than the pointers.
	for ( int i = 0; i < cnt - 1; i++ )
	{
		for ( int j = i + 1; j < cnt; j++ )
		{
			if ( _strcmpi( ppNames[i], ppNames[j] ) > 0 )
			{
				char	szSwap[260];

				strcpy( szSwap, ppNames[i] );
				strcpy( ppNames[i], ppNames[j] );
				strcpy( ppNames[j], szSwap );
			}
		}
	}

	for ( int k = 0; k < cnt; k++ )
		m_pMapList->AddString( ppNames[k] );

	for ( int n = 0; n < cnt; n++ )
	{
		delete ppNames[n];
		ppNames[n] = NULL;
	}
	delete[] ppNames;

	m_pMapList->SetCurSel( 0 );
	m_pMapList->GetScrollbar()->SetPos( 0 );
	::InvalidateRect( m_pMapList->m_hWnd, NULL, TRUE );
	::UpdateWindow( m_pMapList->m_hWnd );

	COM_FreeMapList( &pList );
	m_editMaxPlayers.SetText( "8" );
}

// CCreateServerDlg::RMLPreIdle (0x408050) -- CDlgBase frame slot 56 ("OnIdle")
static int	g_bWasInGame = 0;	// (4E19A0) latched while the engine rendered

int CCreateServerDlg::RMLPreIdle()
{
	Launcher_SyncEngineWindow( this );

	if ( Eng_Frame( gBackground ) && !gBackground )
	{
		g_bWasInGame = 1;
		return 1;
	}

	if ( g_bWasInGame && Launcher_AppOwnsForeground() )
	{
		g_bWasInGame = 0;
		RefreshAndShow();
	}

	if ( Launcher_GetRestartFlag() )
		OnCancel();

	return 0;
}

// CCreateServerDlg::OnAdvanced (0x4080C0)
void CCreateServerDlg::OnAdvanced()
{
	m_pDescription = new CServerDescription;

	// Never open the dialog without a description: with an empty object list its OK
	// handler rewrites settings.scr as a bare header.
	if ( !m_pDescription->InitFromFile( "settings.scr" ) )
	{
		delete m_pDescription;
		m_pDescription = NULL;
		Launcher_ShowMessageById( 0, IDS_CREATESV_NOADVANCED );		// 0x1C9
		return;
	}

	// (sic) CAdvancedMPDlg's second parameter is declared int, but what the
	// binary forwards here is this page's browser pointer.
	CAdvancedMPDlg	dlg( m_pDescription, (int)m_pBrowser, NULL );

	InitChildDialog( &dlg, &m_btnAdvanced );
	dlg.DoModal();
	RestoreAfterModal();

	delete m_pDescription;
	m_pDescription = NULL;
}

// CCreateServerDlg::LayoutHeaderButtons (0x408210)
void CCreateServerDlg::LayoutHeaderButtons()
{
	int		wh[2] = { 0, 0 };
	Launcher_HeaderSize( wh );		// {w,h}
	int		w = wh[0];
	int		h = wh[1];

	::LockWindowUpdate( m_hWnd );

	m_btnAdvanced.MoveWindow( 50, 140, w, h, TRUE );

	m_btnOK.ShowWindow( SW_SHOW );
	m_btnOK.MoveWindow( 50, 172, w, h, TRUE );

	int		extra = Launcher_StringHeight( IDS_CREATEROOM_OFFSET, 0 );
	m_btnCancel.MoveWindow( 50, 204, w + extra, h, TRUE );

	::LockWindowUpdate( NULL );
	::InvalidateRect( m_hWnd, NULL, TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// CCreateServerDlg::OnPaint (0x412860)

void CCreateServerDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CCreateServerDlg::OnEraseBkgnd (0x412870)

BOOL CCreateServerDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CCreateServerDlg::OnDedicated (0x4082D0)

void CCreateServerDlg::OnDedicated()
{
	m_bDedicated = m_btnDedicated.GetCheck();
}

/////////////////////////////////////////////////////////////////////////////
// CCreateServerDlg::OnActivateApp (0x406FE0)

void CCreateServerDlg::OnActivateApp( BOOL bActive, DWORD /*dwThreadID*/ )
{
	ActiveApp = bActive;
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CCreateServerDlg::OnMapListNotify (0x40E460)
//
// The map list reports a selection the page does not act on.

void CCreateServerDlg::OnMapListNotify()
{
}

/////////////////////////////////////////////////////////////////////////////
// CCreateServerDlg::OnMapListValidate (0x433640)
//
// Folded with CServerBrowserDlg's own list handler, which is why the shared
// body returns ValidateRect's result; nothing reads it.

void CCreateServerDlg::OnMapListValidate()
{
	ValidateRect( NULL );
}

/////////////////////////////////////////////////////////////////////////////
// CCreateServerDlg::OnDisplayChange (0x453D00)

LRESULT CCreateServerDlg::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}

