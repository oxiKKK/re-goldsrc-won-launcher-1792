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
// Purpose: CFilterDialog, the server-browser filter editor.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Entries at 0x4ADCE0, base map 0x4B4398 = CDialog.
BEGIN_MESSAGE_MAP( CFilterDialog, CDialog )
	//{{AFX_MSG_MAP(CFilterDialog)
	ON_MESSAGE( WM_DISPLAYCHANGE, OnDisplayChange )
	ON_WM_CTLCOLOR()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
	ON_CONTROL( CBN_SELCHANGE, IDC_FILTER_PINGCOMBO, OnSelectPing )
	ON_CONTROL( CBN_SELCHANGE, IDC_FILTER_GAMECOMBO, OnSelectGame )
	ON_COMMAND( IDC_FILTER_ISNOTPROXY, OnIsNotProxy )
	ON_COMMAND( IDC_FILTER_ISPROXY, OnIsProxy )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::CFilterDialog (0x40f560)
//
// The browser hands over its settings section and its queried server list;
// the list is walked in OnInitDialog for the gamedirs to offer.

CFilterDialog::CFilterDialog( const char* pszSection, CServerInfo* pServerList, CWnd* pParent )
	: CDlgBase( IDD_FILTER, pParent )
{
	m_nFilterPingMax    = -1;
	m_bFilterPing       = 0;
	m_bFilterEmpty      = 0;
	m_bFilterFull       = 0;
	m_bFilterResponded  = 0;
	m_bFilterGame       = 0;		// m_bFilterFavorite is left uninitialised (sic)
	m_bFilterMap        = 0;
	m_bFilterDedicated  = 0;
	m_bFilterOS         = 0;
	m_bFilterIsProxy    = 0;
	m_bFilterIsNotProxy = 0;
	m_bFilterAnyProxy   = 0;
	m_pszSection        = pszSection;
	strcpy( m_szGameDir, "valve" );

	m_pServerList = pServerList;
	m_pSelfWnd    = this;

	m_clrText       = RGB( 255, 255, 255 );
	m_clrBk         = RGB( 127, 127, 127 );
	m_clrStaticText = RGB( 255, 192, 127 );
	m_clrStaticBk   = RGB( 63, 63, 63 );
	m_clrFieldText  = RGB( 255, 255, 255 );
	m_clrFieldBk    = RGB( 63, 63, 63 );

	m_font.Attach( CreateFontA( -11, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, FF_SWISS, "Arial" ) );
	m_brushField.Attach( CreateSolidBrush( m_clrFieldBk ) );
	m_brushStatic.Attach( CreateSolidBrush( m_clrStaticBk ) );

	LoadHeaderBitmap( "head_filter", NULL );
	SetupHeaderButtons();

	m_pGameNames = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::~CFilterDialog (0x40f8c0)

CFilterDialog::~CFilterDialog()
{
	while ( m_pGameNames )
	{
		filtergame_t*	pNext = m_pGameNames->pNext;
		delete m_pGameNames;
		m_pGameNames = pNext;
	}
	m_pGameNames = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::AddGameUnique (0x40fad0)
//
// Push a gamedir onto the name list unless it is the engine's own or already
// there.  The combo is filled from the list afterwards, not from here.

void CFilterDialog::AddGameUnique( const char* pszGame )
{
	char	szBase[256];

	if ( !pszGame || !*pszGame )
		return;

	COM_FileBase( pszGame, szBase );
	_strlwr( szBase );
	if ( _strcmpi( szBase, "valve" ) == 0 )
		return;

	for ( filtergame_t* p = m_pGameNames; p; p = p->pNext )
	{
		if ( _strcmpi( szBase, p->szName ) == 0 )
			return;
	}

	filtergame_t*	pNew = new filtergame_t;
	memcpy( pNew, szBase, strlen( szBase ) + 1 );
	pNew->pNext  = m_pGameNames;
	m_pGameNames = pNew;
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::SetupHeaderButtons (0x40fb90)

void CFilterDialog::SetupHeaderButtons()
{
	int	wh[2];

	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( wh );
	m_headerW      = wh[0];
	m_headerH      = wh[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnOK.FreeSkinBitmaps();
		m_btnOK.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_OK_WIDE, m_headerLoaded );
		m_btnCancel.FreeSkinBitmaps();
		m_btnCancel.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_BACK, m_headerLoaded );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::DoDataExchange (0x40fc20)

void CFilterDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_FILTER_BYOS,         m_chkByOS );
	DDX_Control( pDX, IDC_FILTER_BYMAP,        m_chkByMap );
	DDX_Control( pDX, IDC_FILTER_BYDEDICATED,  m_chkDedicated );
	DDX_Control( pDX, IDC_FILTER_BYGAME,       m_chkByGame );
	DDX_Control( pDX, IDC_FILTER_ONFAVORITES,  m_chkFavorites );
	DDX_Control( pDX, IDC_FILTER_RESPONSETIME, m_chkPing );
	DDX_Control( pDX, IDC_FILTER_RESPONDING,   m_chkResponded );
	DDX_Control( pDX, IDC_FILTER_NOTFULL,      m_chkNotFull );
	DDX_Control( pDX, IDC_FILTER_NOTEMPTY,     m_chkNotEmpty );
	DDX_Control( pDX, IDC_FILTER_HEADING,      m_lblHeading );
	DDX_Control( pDX, IDCANCEL,                m_btnCancel );
	DDX_Control( pDX, IDOK,                    m_btnOK );
	DDX_Control( pDX, IDC_FILTER_ISPROXY,      m_chkIsProxy );
	DDX_Control( pDX, IDC_FILTER_ISNOTPROXY,   m_chkIsNotProxy );

	// Cache the criteria off the live checkboxes; OnOK persists these, never
	// the controls.  The lookup is by id, so it does not depend on the DDX
	// pairing above.
	CODBlendCheckBox*	pBox;

	if ( ( pBox = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_RESPONSETIME ) ) != NULL )
		m_bFilterPing = pBox->m_bChecked;
	if ( ( pBox = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_NOTEMPTY ) ) != NULL )
		m_bFilterEmpty = pBox->m_bChecked;
	if ( ( pBox = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_NOTFULL ) ) != NULL )
		m_bFilterFull = pBox->m_bChecked;
	if ( ( pBox = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_RESPONDING ) ) != NULL )
		m_bFilterResponded = pBox->m_bChecked;
	if ( ( pBox = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_ONFAVORITES ) ) != NULL )
		m_bFilterFavorite = pBox->m_bChecked;
	if ( ( pBox = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_BYGAME ) ) != NULL )
		m_bFilterGame = pBox->m_bChecked;
	if ( ( pBox = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_BYMAP ) ) != NULL )
		m_bFilterMap = pBox->m_bChecked;
	if ( ( pBox = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_BYOS ) ) != NULL )
		m_bFilterOS = pBox->m_bChecked;
	if ( ( pBox = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_BYDEDICATED ) ) != NULL )
		m_bFilterDedicated = pBox->m_bChecked;
	if ( ( pBox = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_ISPROXY ) ) != NULL )
		m_bFilterIsProxy = pBox->m_bChecked;
	if ( ( pBox = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_ISNOTPROXY ) ) != NULL )
		m_bFilterIsNotProxy = pBox->m_bChecked;

	m_bFilterAnyProxy = ( m_bFilterIsProxy || m_bFilterIsNotProxy ) ? 1 : 0;

	if ( m_editMapName.m_hWnd )
	{
		if ( m_editMapName.m_pEdit )
			m_editMapName.m_pEdit->GetWindowText( m_strFilterMapName );

		// A map name reaches the query string, so it is clamped and stripped
		// of the characters that would break it.
		m_strFilterMapName = m_strFilterMapName.Left( 32 );
		m_strFilterMapName.Remove( '%' );
		m_strFilterMapName.Remove( '\\' );
		m_strFilterMapName.Remove( '/' );
		m_strFilterMapName.Remove( ':' );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::OnCtlColor (0x40ff30)

HBRUSH CFilterDialog::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH	hbr = CDialog::OnCtlColor( pDC, pWnd, nCtlColor );

	switch ( nCtlColor )
	{
	case CTLCOLOR_EDIT:
		pDC->SetBkMode( TRANSPARENT );
		pDC->SetTextColor( m_clrText );
		pDC->SetBkColor( m_clrBk );
		return (HBRUSH)m_brushField.GetSafeHandle();

	case CTLCOLOR_LISTBOX:
		pDC->SetBkMode( TRANSPARENT );
		pDC->SetBkColor( m_clrBk );
		pDC->SetTextColor( m_clrText );
		return (HBRUSH)m_brushField.GetSafeHandle();

	case CTLCOLOR_STATIC:
		pDC->SetBkMode( TRANSPARENT );
		pDC->SetBkColor( m_clrBk );
		pDC->SetTextColor( m_clrText );
		return (HBRUSH)m_brushStatic.GetSafeHandle();
	}

	return hbr;
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::OnDisplayChange (0x410010)
//
// Re-centre on the new desktop.  Not Dlg_CenterWindow: this one centres the
// window's own size on the screen rather than the launcher's design size.

LRESULT CFilterDialog::OnDisplayChange( WPARAM, LPARAM )
{
	RECT	rc;

	::GetWindowRect( m_hWnd, &rc );

	int	w = rc.right - rc.left;
	int	h = rc.bottom - rc.top;

#ifdef LAUNCHER_FIXES
	Dlg_CenterPopup( this, w, h );
#else
	MoveWindow( ( ::GetSystemMetrics( SM_CXSCREEN ) - w ) / 2,
				( ::GetSystemMetrics( SM_CYSCREEN ) - h ) / 2, w, h, TRUE );
#endif
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::OnInitDialog (0x410080)
//
// Two skinned buttons down the left, eleven criteria rows 25px apart down the
// right, and three value controls beside the rows that need one.  Every width
// is a locale probe away from the English number: the German, Spanish and
// French wraps each widen a different subset of rows, and the Italian build
// pulls the whole button column left.

BOOL CFilterDialog::OnInitDialog()
{
	int		wh[2];
	RECT	rc;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	int	nFrench  = Launcher_StringHeight( IDS_FRENCH, 0 );
	int	nSpanish = Launcher_StringHeight( IDS_SPANISH, 0 );
	int	nGerman  = Launcher_StringHeight( IDS_GERMAN, 0 );
	int	bItalian = ( _strcmpi( Launcher_LoadString( IDS_LANGUAGE ), "Italiano" ) == 0 );

	Launcher_HeaderSize( wh );
	Launcher_HeaderSize( wh );

	int	cw = wh[0] - 20;
	int	ch = wh[1];

	int	nPad = 0;
	if ( nFrench )
		nPad = 40;
	else if ( bItalian )
		nPad = 95;
	else if ( !nGerman && !nSpanish )
		nPad = 40;

	int	xBtn  = 50 - 40 * bItalian;
	int	xBtnR = cw - 50 * nSpanish - 40 * bItalian - nPad + 50;

	m_btnOK.MoveWindow( xBtn, 140, xBtnR - xBtn, ch, TRUE );
	m_btnOK.SetWindowText( Launcher_LoadString( IDS_BTN_FILTER ) );
	m_btnCancel.MoveWindow( xBtn, 165, xBtnR - xBtn, ch, TRUE );
	m_btnCancel.SetWindowText( Launcher_LoadString( IDS_BTN_CANCEL ) );

	int	xCol  = xBtnR + 2;
	int	xWide = g_nLauncherDefW - 220;
	if ( nSpanish )
		xWide += 65;

	m_lblHeading.MoveWindow( xCol, 140, g_nLauncherDefW - 20 - xCol, ch, TRUE );
	m_chkResponded.MoveWindow( xCol, 165, xWide - xCol, ch, TRUE );
	m_chkPing.MoveWindow( xCol, 190, xWide + 35 * nGerman - xCol, ch, TRUE );

	// Max-ping combo, beside the response-time row.  Combos are created at
	// their dropped height and never moved -- MoveWindow afterwards
	// desynchronises ShowDrop's closed-rect compare and it stops opening.
	rc.left   = xCol;
	rc.top    = 190;
	rc.right  = xWide + 35 * nGerman;
	rc.bottom = ch + 190;

	int	nLines = Launcher_StringHeight( IDS_GERMAN, 0 );
	int	xValue = g_nLauncherDefW + 35 * nLines - 120;
	if ( nFrench )
		xValue += 40;
	else if ( nSpanish )
		xValue += 65;
	else if ( bItalian )
		xValue += 40;

	rc.right  = xValue;
	rc.left   = xValue - 80;
	rc.bottom = rc.top + 80;
	m_cbPing.SetDropHeight( 65 );
	m_cbPing.Create( WS_CHILD | WS_VISIBLE, &rc, this, IDC_FILTER_PINGCOMBO );
	m_cbPing.SetAutoDelete( 0 );

	int	xNarrow = xWide - 30;

	m_chkByOS.MoveWindow( xCol, 215, xNarrow + 10 * ( nSpanish + 4 * nGerman ) - xCol, ch, TRUE );
	m_chkDedicated.MoveWindow( xCol, 240, xNarrow + 25 * nGerman + 45 - xCol, ch, TRUE );

	int	xNotEmpty = xNarrow + 15;
	if ( nFrench )
		xNotEmpty = xNarrow + 60;
	else if ( bItalian )
		xNotEmpty = xNarrow + 83;
	else if ( nGerman )
		xNotEmpty = xNarrow + 35;
	m_chkNotEmpty.MoveWindow( xCol, 265, xNotEmpty - xCol, ch, TRUE );

	int	xByMap = xNarrow + 20 * ( bItalian + nGerman ) - 50;
	m_chkByMap.MoveWindow( xCol, 290, xByMap - xCol, ch, TRUE );

	// Map-name field, beside the "are running map" row.
	rc.left   = xCol;
	rc.top    = 290;
	rc.right  = xByMap;
	rc.bottom = ch + 290;

	int	nWideGap = 35 * ( nGerman - 2 );
	int	xField   = g_nLauncherDefW + nWideGap;
	if ( nFrench )
		xField += 40;
	else if ( nSpanish )
		xField += 45;
	else if ( bItalian )
		xField += 40;

	rc.right = xField;
	rc.left  = xField - 150;
	InflateRect( &rc, -1, -1 );
	m_editMapName.Create( WS_CHILD | WS_VISIBLE, &rc, this, (UINT)-1 );
	m_editMapName.SetBorderColor( RGB( 56, 56, 56 ) );

	int	xByGame = 35 * nGerman + xNarrow - 50;
	m_chkByGame.MoveWindow( xCol, 315, xByGame - xCol, ch, TRUE );

	// Game combo, beside the "are running game" row.
	rc.left   = xCol;
	rc.top    = 315;
	rc.right  = xByGame;
	rc.bottom = ch + 315;

	int	xGame = g_nLauncherDefW + nWideGap;
	if ( nFrench )
		xGame += 40;
	else if ( nSpanish )
		xGame += 45;
	else if ( bItalian )
		xGame += 40;

	rc.right  = xGame;
	rc.bottom = 485;
	rc.left   = xGame - 150;
	m_cbGame.SetDropHeight( 115 );
	m_cbGame.Create( WS_CHILD | WS_VISIBLE, &rc, this, IDC_FILTER_GAMECOMBO );
	m_cbGame.SetAutoDelete( 0 );

	m_chkNotFull.MoveWindow( xCol, 340, xNarrow - xCol, ch, TRUE );
	m_chkFavorites.MoveWindow( xCol, 365, xNarrow + 45 * nGerman - xCol, ch, TRUE );
	m_chkIsProxy.MoveWindow( xCol, 390, 200, ch, TRUE );
	m_chkIsNotProxy.MoveWindow( xCol, 415, 230, ch, TRUE );

	// The template carries the boxes as BS_AUTOCHECKBOX; owner-draw is OR-ed on
	// afterwards so CODBlendCheckBox gets to paint the glyph.
	CODBlendCheckBox*	rgStyle[] =
	{
		&m_chkPing, &m_chkResponded, &m_chkNotFull, &m_chkNotEmpty, &m_chkFavorites,
		&m_chkByGame, &m_chkByMap, &m_chkByOS, &m_chkDedicated,
		&m_chkIsProxy, &m_chkIsNotProxy
	};

	int	i;
	for ( i = 0; i < (int)ARRAYSIZE( rgStyle ); i++ )
	{
		::SetWindowLong( rgStyle[i]->GetSafeHwnd(), GWL_STYLE,
			rgStyle[i]->GetStyle() | BS_OWNERDRAW );
	}

	// Font and caption run over the same eleven rows in a second order.
	struct	filterrow_t { CODBlendCheckBox* pBox; UINT nCaption; };
	filterrow_t	rows[] =
	{
		{ &m_chkPing,       IDS_FILTER_RESPONSETIME           },
		{ &m_chkNotFull,    IDS_FILTER_NOTFULL                },
		{ &m_chkNotEmpty,   IDS_FILTER_NOTEMPTY               },
		{ &m_chkResponded,  IDS_FILTER_RESPONDING             },
		{ &m_chkFavorites,  IDS_FILTER_ONFAVORITES            },
		{ &m_chkByGame,     IDS_FILTER_BYGAME                 },
		{ &m_chkByMap,      IDS_FILTER_RUNNINGMAP             },
		{ &m_chkByOS,       IDS_FILTER_LINUXSERVER            },
		{ &m_chkDedicated,  IDS_FILTER_DEDICATED              },
		{ &m_chkIsProxy,    IDS_FILTER_ARE_SPECTATORPROXY     },
		{ &m_chkIsNotProxy, IDS_FILTER_ARE_NOT_SPECTATORPROXY },
	};

	for ( i = 0; i < (int)ARRAYSIZE( rows ); i++ )
		rows[i].pBox->SetFontSize( 12, FW_HEAVY );

	for ( i = 0; i < (int)ARRAYSIZE( rows ); i++ )
		rows[i].pBox->SetWindowText( Launcher_LoadString( rows[i].nCaption ) );

	// Both combos paint a flat 56,56,56 face with amber rows.
	m_cbPing.SetFrameColor( RGB( 56, 56, 56 ) );
	m_cbPing.SetFaceColor( RGB( 56, 56, 56 ) );
	m_cbPing.SetTextColor( RGB( 240, 176, 56 ) );
	m_cbGame.SetFrameColor( RGB( 56, 56, 56 ) );
	m_cbGame.SetFaceColor( RGB( 56, 56, 56 ) );
	m_cbGame.SetTextColor( RGB( 240, 176, 56 ) );

	m_lblHeading.SetTransparent( TRUE );
	m_lblHeading.SetFontSize( 18, FW_HEAVY );
	m_lblHeading.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblHeading.SetWindowText( Launcher_LoadString( IDS_FILTER_HEADING ) );

	// The criteria come back out of the profile into the members; the boxes are
	// only told about them at the end.
	const char*	s = m_pszSection;

	m_bFilterPing      = Launcher_GetProfileInt( s, "Filter Ping",      0 ) != 0;
	m_bFilterEmpty     = Launcher_GetProfileInt( s, "Filter Empty",     0 ) != 0;
	m_bFilterFull      = Launcher_GetProfileInt( s, "Filter Full",      0 ) != 0;
	m_bFilterResponded = Launcher_GetProfileInt( s, "Filter Responded", 0 ) != 0;
	m_bFilterFavorite  = Launcher_GetProfileInt( s, "Filter Favorite",  0 ) != 0;
	m_bFilterDedicated = Launcher_GetProfileInt( s, "Filter Dedicated", 0 ) != 0;
	m_bFilterOS        = Launcher_GetProfileInt( s, "Filter OS",        0 ) != 0;
	m_bFilterMap       = Launcher_GetProfileInt( s, "Filter Map",       0 ) != 0;

	CString	strMapName = Launcher_GetProfileString( s, "Filter Map Name", "" );

	m_bFilterGame = Launcher_GetProfileInt( s, "Filter Game", 0 ) != 0;

	CString	strGameName = Launcher_GetProfileString( s, "Filter Game Name", "Half-Life" );

	m_nFilterPingMax    = Launcher_GetProfileInt( s, "Filter PingMax",    -1 );
	m_bFilterIsProxy    = Launcher_GetProfileInt( s, "Filter IsProxy",    0 ) != 0;
	m_bFilterIsNotProxy = Launcher_GetProfileInt( s, "Filter IsNotProxy", 0 ) != 0;
	m_bFilterAnyProxy   = ( m_bFilterIsProxy || m_bFilterIsNotProxy ) ? 1 : 0;

	// The choices are the upper bound of each of the list control's ping bands.
	int	iPing = 0;
	for ( i = 1; i < g_numPingBands; i++ )
	{
		char	szPing[32];

		sprintf( szPing, "%i ms", g_pingBands[i].hi );
		m_cbPing.AddString( szPing );
		if ( g_pingBands[i].hi == m_nFilterPingMax )
			iPing = i - 1;
	}
	m_cbPing.SetCurSel( iPing );

	// The game list is the engine, the saved choice, every gamedir the browser
	// has actually seen, and every installed mod -- all de-duplicated.
	AddGameUnique( "Half-Life" );
	if ( strGameName.GetLength() )
		AddGameUnique( (LPCTSTR)strGameName );
	for ( CServerInfo* pSI = m_pServerList; pSI; pSI = pSI->m_pNext )
		AddGameUnique( (LPCTSTR)pSI->m_strDir );
	for ( mod_t* m = g_pModList; m; m = m->next )
	{
		const char*	pszDir = m->GetKey( "gamedir" );
		if ( pszDir && *pszDir )
			AddGameUnique( pszDir );
	}
	for ( filtergame_t* p = m_pGameNames; p; p = p->pNext )
		m_cbGame.AddString( p->szName );

	int	iGame = m_cbGame.FindString( (LPCTSTR)strGameName );
	m_cbGame.SetCurSel( iGame == -1 ? 0 : iGame );

	// Finally push the criteria onto the boxes.  The state is poked into
	// m_bChecked directly: BM_SETCHECK would repaint through the template's
	// own class, which is not the one doing the drawing.
	m_chkPing.m_bChecked = m_bFilterPing;
	::InvalidateRect( m_chkPing.m_hWnd, NULL, TRUE );
	m_chkNotFull.m_bChecked = m_bFilterFull;
	::InvalidateRect( m_chkNotFull.m_hWnd, NULL, TRUE );
	m_chkNotEmpty.m_bChecked = m_bFilterEmpty;
	::InvalidateRect( m_chkNotEmpty.m_hWnd, NULL, TRUE );
	m_chkResponded.m_bChecked = m_bFilterResponded;
	::InvalidateRect( m_chkResponded.m_hWnd, NULL, TRUE );
	m_chkFavorites.m_bChecked = m_bFilterFavorite;
	::InvalidateRect( m_chkFavorites.m_hWnd, NULL, TRUE );
	m_chkByGame.m_bChecked = m_bFilterGame;
	::InvalidateRect( m_chkByGame.m_hWnd, NULL, TRUE );
	m_chkByMap.m_bChecked = m_bFilterMap;
	::InvalidateRect( m_chkByMap.m_hWnd, NULL, TRUE );
	m_chkByOS.m_bChecked = m_bFilterOS;
	::InvalidateRect( m_chkByOS.m_hWnd, NULL, TRUE );
	m_chkDedicated.m_bChecked = m_bFilterDedicated;
	::InvalidateRect( m_chkDedicated.m_hWnd, NULL, TRUE );

	m_editMapName.SetText( (LPCTSTR)strMapName );

	m_chkIsProxy.m_bChecked = m_bFilterIsProxy;
	::InvalidateRect( m_chkIsProxy.m_hWnd, NULL, TRUE );
	m_chkIsNotProxy.m_bChecked = m_bFilterIsNotProxy;
	::InvalidateRect( m_chkIsNotProxy.m_hWnd, NULL, TRUE );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::RMLPreIdle (0x410e70)
//
// CDlgBase's per-pass background pump: sync the engine window, then run one
// engine frame.

int CFilterDialog::RMLPreIdle()
{
	Launcher_SyncEngineWindow( this );
	return Eng_Frame( gBackground ) && !gBackground;
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::OnOK (0x410ea0)

void CFilterDialog::OnOK()
{
	UpdateData( TRUE );		// DoDataExchange caches the criteria off the boxes

	const char*	s = m_pszSection;

	Launcher_WriteProfileInt( s, "Filter Ping",      m_bFilterPing      != 0 );
	Launcher_WriteProfileInt( s, "Filter PingMax",   m_nFilterPingMax );
	Launcher_WriteProfileInt( s, "Filter Empty",     m_bFilterEmpty     != 0 );
	Launcher_WriteProfileInt( s, "Filter Full",      m_bFilterFull      != 0 );
	Launcher_WriteProfileInt( s, "Filter Responded", m_bFilterResponded != 0 );
	Launcher_WriteProfileInt( s, "Filter Favorite",  m_bFilterFavorite  != 0 );
	Launcher_WriteProfileInt( s, "Filter Game",      m_bFilterGame      != 0 );
	Launcher_WriteProfileInt( s, "Filter OS",        m_bFilterOS        != 0 );
	Launcher_WriteProfileInt( s, "Filter Dedicated", m_bFilterDedicated != 0 );
	Launcher_WriteProfileInt( s, "Filter Map",       m_bFilterMap       != 0 );

	if ( m_bFilterMap )
		Launcher_WriteProfileString( s, "Filter Map Name", m_strFilterMapName );
	else
		Launcher_WriteProfileString( s, "Filter Map Name", "" );

	int	iSel = m_cbGame.GetCurSel();
	if ( iSel == -1 )
	{
		Launcher_WriteProfileString( s, "Filter Game Name", "Half-Life" );
	}
	else
	{
		char	szItem[260];
		char	szBase[256];

		strcpy( szItem, m_cbGame.GetString( iSel ) );
		COM_FileBase( szItem, szBase );
		_strlwr( szBase );
		Launcher_WriteProfileString( s, "Filter Game Name", szBase );
	}

	Launcher_WriteProfileInt( s, "Filter IsProxy",    m_bFilterIsProxy    != 0 );
	Launcher_WriteProfileInt( s, "Filter IsNotProxy", m_bFilterIsNotProxy != 0 );

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::OnSelectPing (0x411110)

void CFilterDialog::OnSelectPing()
{
	int	iSel = m_cbPing.GetCurSel();

	if ( iSel >= 0 && iSel < g_numPingBands - 1 )
		m_nFilterPingMax = g_pingBands[iSel + 1].hi;
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::OnSelectGame (0x411140)

void CFilterDialog::OnSelectGame()
{
	int	iSel = m_cbGame.GetCurSel();
	if ( iSel == -1 )
		return;

	char	szItem[256];
	char	szBase[256];

	strcpy( szItem, m_cbGame.GetString( iSel ) );
	COM_FileBase( szItem, szBase );
	_strlwr( szBase );
	if ( _strcmpi( szBase, "Half-Life" ) == 0 )
		strcpy( szBase, "valve" );		// the engine's own gamedir

	strcpy( m_szGameDir, szBase );
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::OnIsProxy (0x411220)
//
// The two proxy criteria are exclusive.

void CFilterDialog::OnIsProxy()
{
	CODBlendCheckBox*	pProxy = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_ISPROXY );
	if ( pProxy )
		m_bFilterIsProxy = pProxy->GetCheck();

	CODBlendCheckBox*	pNotProxy = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_ISNOTPROXY );
	if ( pNotProxy && m_bFilterIsProxy )
	{
		pNotProxy->m_bChecked = 0;
		::InvalidateRect( pNotProxy->m_hWnd, NULL, TRUE );
		m_bFilterIsNotProxy = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::OnIsNotProxy (0x411280)

void CFilterDialog::OnIsNotProxy()
{
	CODBlendCheckBox*	pNotProxy = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_ISNOTPROXY );
	if ( pNotProxy )
		m_bFilterIsNotProxy = pNotProxy->GetCheck();

	CODBlendCheckBox*	pProxy = (CODBlendCheckBox*)GetDlgItem( IDC_FILTER_ISPROXY );
	if ( pProxy && m_bFilterIsNotProxy )
	{
		pProxy->m_bChecked = 0;
		::InvalidateRect( pProxy->m_hWnd, NULL, TRUE );
		m_bFilterIsProxy = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::OnActivateApp (0x406fe0)

void CFilterDialog::OnActivateApp( BOOL bActive, DWORD /*dwThreadID*/ )
{
	ActiveApp = bActive;
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::OnPaint (0x412860)

void CFilterDialog::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog::OnEraseBkgnd (0x412870)

BOOL CFilterDialog::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}
