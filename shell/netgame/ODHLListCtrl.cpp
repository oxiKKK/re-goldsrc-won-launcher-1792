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
// Purpose: CODHLListCtrl, the server-browser report list.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Never assigned -- the 45 is its initialiser.
int	g_nDefaultProtocol = 45;

// The leaf names LoadGlyphs pastes into "gfx/shell/%s.bmp".
static const char*	s_pszGlyphNames[ODGLYPH_COUNT] =
{
	"favorite", "nonfav", "windows", "linux", "dedicate",
	"proxy", "listen", "lock", "unlock"
};

// Ping -> dot count, from the table at 0x4D0DE8.  [lo,hi] is inclusive.  The
// filter dialog offers the same hi values as its max-ping choices.
const pingband_t	g_pingBands[] =
{
	{ 1, 1,    0,    0 },		// ping 0 -> "?"
	{ 0, 8,    1,  100 },
	{ 0, 7,  101,  175 },
	{ 0, 6,  176,  250 },
	{ 0, 5,  251,  350 },
	{ 0, 4,  351,  500 },
	{ 1, 3,  501,  700 },
	{ 1, 2,  701,  900 },
	{ 1, 1,  901, 1100 },
	{ 1, 1, 1101, 1300 },
	{ 1, 1, 1301, 0x7FFF },
};

const int	g_numPingBands = ARRAYSIZE( g_pingBands );

BEGIN_MESSAGE_MAP( CODHLListCtrl, CODListCtrl )
	//{{AFX_MSG_MAP(CODHLListCtrl)
	ON_WM_SIZE()
	ON_WM_RBUTTONUP()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::CODHLListCtrl (0x447940)

CODHLListCtrl::CODHLListCtrl( CWnd* pParent, int bOwnsSort )
{
	m_pParent      = pParent;
	m_bOwnsSort    = bOwnsSort;

	m_bNumericPing = 0;
	if ( CheckParm( "-numericping", NULL ) )
		m_bNumericPing = 1;

	m_clrPingLow   = RGB( 128, 192, 64 );
	m_clrPingHigh  = RGB( 192, 63, 0 );
	m_pSelectedSv  = NULL;
	m_unk1956      = 0;
	m_unk1960      = 0;
	m_unk1976      = 0;
	m_clrProxyBg   = RGB( 63, 255, 63 );
	m_clrProxyText = RGB( 127, 255, 63 );

	SetHeaderTransparent( 1 );
	SetTransparent( 0 );
	SetDrawFrame( 1 );
	LoadGlyphs();
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::~CODHLListCtrl (0x447A40)

CODHLListCtrl::~CODHLListCtrl()
{
	FreeGlyphs();
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::LoadGlyphs (0x447A90)

void CODHLListCtrl::LoadGlyphs()
{
	char	szPath[MAX_PATH];
	int		i;

	for ( i = 0; i < ODGLYPH_COUNT; i++ )
	{
		sprintf( szPath, "%s%s.bmp", "gfx/shell/", s_pszGlyphNames[i] );
		m_hGlyphs[i] = DIB_LoadBitmapFile( szPath );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::FreeGlyphs (0x447C30)

void CODHLListCtrl::FreeGlyphs()
{
	int	i;

	for ( i = 0; i < ODGLYPH_COUNT; i++ )
		if ( m_hGlyphs[i] )
			GlobalFree( m_hGlyphs[i] );
}

/*
==================
ODList_DrawGlyph

NOTE(ox): the binary inlines this blit at each of the nine cells; it is one
helper here only so DrawRow stays readable.
==================
*/
static void ODList_DrawGlyph( CDC* pDC, HGLOBAL hDib, int x, int yTop )
{
	RECT	dst;
	RECT	src = { 0, 0, 16, 16 };

	if ( !hDib )
		return;

	dst.left   = x;
	dst.top    = yTop;
	dst.right  = x + 16;
	dst.bottom = yTop + 16;
	DIB_BlitDib( pDC->GetSafeHdc(), &dst, hDib, &src );
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::DrawRow (0x447CC0)
//
// Nine cells: three glyph columns, the server name (dimmed on a foreign
// protocol), the ping graph, map, mod and the player counts.  The first cell
// starts one column width in -- m_cols[0] is the left margin.

void CODHLListCtrl::DrawRow( CDC* pDC, int iRow )
{
	odrow_t*		pRow;
	CServerInfo*	pSI;
	RECT			rcRow;
	RECT			rcCell;
	CFont*			pOldRowFont;
	COLORREF		clrText;
	COLORREF		clrTint;
	COLORREF		clrSave;
	BOOL			bSel, bTint;
	char			s[256];
	char			szPing[32];
	int				x, col, w;

	pRow = m_rows ? m_rows[iRow] : NULL;
	if ( !pRow )
		return;
	pSI = (CServerInfo*)pRow->record;
	if ( !pSI )
		return;

	bSel = ( pRow->flags & 1 );

	GetClientRect( &rcRow );
	if ( m_bHasScrollbar )
		rcRow.right -= 16;
	rcRow.top    = ( iRow - m_topRow ) * m_rowHeight;
	rcRow.bottom = m_rowHeight + rcRow.top - 1;

	pDC->SetBkColor( m_clrRowBg );

	if ( m_bTransparent )
	{
		// Transparent lists let the page art through; only the selection bar
		// is painted, and it is OR-ed over the background.
		if ( bSel )
		{
			CDC			memDC;
			CBitmap		bmp;
			CBitmap*	pOldBmp;
			CBrush		brSel( RGB( 80, 56, 24 ) );
			CRect		rcBuf( 0, 0, rcRow.right - rcRow.left, rcRow.bottom - rcRow.top );

			memDC.CreateCompatibleDC( pDC );
			bmp.CreateCompatibleBitmap( pDC, rcBuf.Width(), rcBuf.Height() );
			pOldBmp = memDC.SelectObject( &bmp );
			memDC.FillRect( &rcBuf, &brSel );
			pDC->BitBlt( rcRow.left, rcRow.top, rcBuf.Width(), rcBuf.Height(),
				&memDC, 0, 0, SRCPAINT );
			memDC.SelectObject( pOldBmp );
		}
	}
	else
	{
		pDC->FillRect( &rcRow, bSel ? &m_brHighlight : &m_brBg );
	}

	pOldRowFont = pDC->SelectObject( &m_headerFont );

	// text colour: proxy rows, then selection
	clrText = bSel ? m_clrSelText : m_clrRowText;
	if ( pSI->m_bProxy )
		clrText = bSel ? m_clrProxyText : m_clrProxyBg;
	pDC->SetBkMode( TRANSPARENT );
	pDC->SetBkColor( m_clrRowBg );
	pDC->SetTextColor( clrText );

	// A known protocol other than ours dims the server-name cell only.
	bTint   = ( pSI->m_nProtocol && pSI->m_nProtocol != g_nDefaultProtocol );
	clrTint = bSel ? RGB( 200, 200, 200 ) : RGB( 56, 56, 56 );

	x = m_cols[0].width;
	for ( col = 0; col < m_nCols; col++ )
	{
		w = m_cols[col].width;
		rcCell.left   = x;
		rcCell.top    = rcRow.top;
		rcCell.right  = x + w;
		rcCell.bottom = rcRow.bottom;

		switch ( col )
		{
		case 0:
			ODList_DrawGlyph( pDC, m_hGlyphs[pSI->m_bFavorite ? ODGLYPH_FAVORITE : ODGLYPH_NONFAV],
				x, rcRow.top );
			break;

		case 1:
			ODList_DrawGlyph( pDC, m_hGlyphs[pSI->m_bPassword ? ODGLYPH_LOCK : ODGLYPH_UNLOCK],
				x, rcRow.top );
			break;

		case 2:
			if ( pSI->m_bProxy )
				ODList_DrawGlyph( pDC, m_hGlyphs[ODGLYPH_PROXY], x, rcRow.top );
			else if ( pSI->m_cSvType == 'd' )
				ODList_DrawGlyph( pDC, m_hGlyphs[ODGLYPH_DEDICATED], x, rcRow.top );
			else if ( pSI->m_cSvType == 'l' )
				ODList_DrawGlyph( pDC, m_hGlyphs[ODGLYPH_LISTEN], x, rcRow.top );
			break;

		case 3:
			if ( pSI->m_cSvOs == 'w' )
				ODList_DrawGlyph( pDC, m_hGlyphs[ODGLYPH_WINDOWS], x, rcRow.top );
			else if ( pSI->m_cSvOs == 'l' )
				ODList_DrawGlyph( pDC, m_hGlyphs[ODGLYPH_LINUX], x, rcRow.top );
			break;

		case 4:
			sprintf( s, " %s", (LPCSTR)pSI->m_strName );
			if ( bTint )
				pDC->SetTextColor( clrTint );
			pDC->DrawText( CODList_EllipsizeText( pDC, s, w, 0 ), -1, &rcCell,
				DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
			if ( bTint )
				pDC->SetTextColor( clrText );
			break;

		case 5:
			sprintf( szPing, "%i", (int)( pSI->m_dSvPing * 1000.0 ) );
			DrawPingBars( pDC, szPing, &rcCell, &rcCell );
			if ( m_bNumericPing )
			{
				CFont	fntPing;
				CFont*	pOldPing;

				fntPing.Attach( ::CreateFontA( -9, 0, 0, 0, 400, 0, 0, 0, 0,
					OUT_TT_PRECIS, 0, PROOF_QUALITY, 2, "Arial" ) );
				pOldPing = pDC->SelectObject( &fntPing );
				clrSave  = pDC->SetTextColor( RGB( 200, 200, 200 ) );

				if ( pSI->m_dSvPing == 0.0 )
					strcpy( s, "9999" );
				else
					sprintf( s, "%i ", (int)( pSI->m_dSvPing * 1000.0 ) );

				rcCell.left += 20;
				pDC->DrawText( CODList_EllipsizeText( pDC, s, w - 20, 0 ), -1, &rcCell,
					DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );

				pDC->SetTextColor( clrSave );
				pDC->SelectObject( pOldPing );
			}
			break;

		case 6:
			sprintf( s, " %s", (LPCSTR)pSI->m_strMap );
			pDC->DrawText( CODList_EllipsizeText( pDC, s, w, 0 ), -1, &rcCell,
				DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
			break;

		case 7:
			sprintf( s, " %s", (LPCSTR)pSI->m_strGame );
			pDC->DrawText( CODList_EllipsizeText( pDC, s, w, 0 ), -1, &rcCell,
				DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
			break;

		case 8:
			if ( pSI->m_bProxy && pSI->m_nProxyMaxPlayers )
				sprintf( s, " %i/%i", pSI->m_nProxyCurPlayers, pSI->m_nProxyMaxPlayers );
			else
				sprintf( s, " %i/%i", pSI->m_nCurrentPlayers, pSI->m_nMaxPlayers );
			pDC->DrawText( s, -1, &rcCell, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
			break;

		default:
			break;
		}
		x += w;
	}

	pDC->SelectObject( pOldRowFont );
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::DrawPingBars (0x448730)
//
// The ping graph: up to eight dots in two rows, or "?" when the server never
// answered.

void CODHLListCtrl::DrawPingBars( CDC* pDC, const char* pszPing, RECT* prcText, RECT* prcBars )
{
	const pingband_t*	pBand;
	CPen*				pOldPen;
	CBrush*				pOldBrush;
	int					ping;
	int					idx;
	int					yMid;
	int					bUp;
	int					i, x, y;
	COLORREF			clr;

	ping = atoi( pszPing );

	idx = 0;
	while ( ping < g_pingBands[idx].lo || ping > g_pingBands[idx].hi )
	{
		if ( ++idx >= g_numPingBands )
		{
			pDC->DrawText( "?", 1, prcText,
				DT_NOPREFIX | DT_NOCLIP | DT_SINGLELINE | DT_VCENTER );
			return;
		}
	}

	pBand = &g_pingBands[idx];
	if ( idx == 0 )						// ping 0 sentinel
	{
		pDC->DrawText( "?", 1, prcText,
			DT_NOPREFIX | DT_NOCLIP | DT_SINGLELINE | DT_VCENTER );
		return;
	}

	if ( prcBars->right - prcBars->left < 20 )	// too narrow for dots
	{
		pDC->DrawText( "...", 3, prcText,
			DT_NOPREFIX | DT_NOCLIP | DT_SINGLELINE | DT_VCENTER );
		return;
	}

	yMid = prcBars->top + (int)( (double)( prcBars->bottom - prcBars->top ) * 0.5 ) - 1;
	clr  = pBand->flag ? m_clrPingHigh : m_clrPingLow;

	CPen	pen( PS_SOLID, 1, clr );
	CBrush	brush( clr );

	pOldPen   = pDC->SelectObject( &pen );
	pOldBrush = pDC->SelectObject( &brush );

	bUp = 1;
	for ( i = 0; i < pBand->count; i++ )
	{
		x   = prcBars->left + 4 * ( i / 2 ) + 2;
		y   = bUp ? ( yMid - 5 ) : ( yMid + 1 );
		bUp = !bUp;
		pDC->Ellipse( x, y, x + 3, y + 5 );
	}

	pDC->SelectObject( pOldBrush );
	pDC->SelectObject( pOldPen );
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::OnSize (0x448950)

void CODHLListCtrl::OnSize( UINT nType, int cx, int cy )
{
	m_unk1956 = cx;
	CODListCtrl::OnSize( nType, cx, cy );
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::NotifyFocusChanged (0x40E460)
//
// Folded onto the image's shared empty stub.

void CODHLListCtrl::NotifyFocusChanged()
{
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::OnSetFocus (0x448970)

void CODHLListCtrl::OnSetFocus( CWnd* pOldWnd )
{
	Default();

	if ( !pOldWnd || CWnd::FromHandle( ::GetParent( pOldWnd->GetSafeHwnd() ) ) != this )
		NotifyFocusChanged();
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::OnKillFocus (0x4489A0)

void CODHLListCtrl::OnKillFocus( CWnd* pNewWnd )
{
	CODListCtrl::OnKillFocus( pNewWnd );

	if ( !pNewWnd || CWnd::FromHandle( ::GetParent( pNewWnd->GetSafeHwnd() ) ) != this )
		NotifyFocusChanged();
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::OnRButtonUp (0x4489E0)
//
// The server context menu; the LAN page owns its own sort and takes none.

void CODHLListCtrl::OnRButtonUp( UINT nFlags, CPoint point )
{
	char	szAddFav[64], szDropFav[64], szRemove[64], szQuick[64], szRefresh[64];
	int		iRow;

	UNUSED_ALWAYS( nFlags );

	if ( m_bOwnsSort )
	{
		Default();
		return;
	}

	m_pSelectedSv = NULL;

	iRow = RowFromPoint( &point );
	if ( iRow == -1 )
		return;

	m_pSelectedSv = (CServerInfo*)m_rows[iRow]->record;
	if ( !m_pSelectedSv )
		return;

	Launcher_LoadStringInto( szAddFav,  IDS_SERVER_MENU_ADDTOFAVORITE );
	Launcher_LoadStringInto( szDropFav, IDS_SERVER_MENU_REMOVEFROMFAV );
	Launcher_LoadStringInto( szRemove,  IDS_SERVER_MENU_REMOVE );
	Launcher_LoadStringInto( szQuick,   IDS_SERVER_MENU_QUICK );
	Launcher_LoadStringInto( szRefresh, IDS_SERVER_REFRESH );

	::ClientToScreen( m_hWnd, &point );

	CODMenu	menu( "Arial", 10, 400 );
	menu.Attach( ::CreatePopupMenu() );
	::AppendMenu( menu.m_hMenu, MF_OWNERDRAW, IDC_NET_FAVORITE_ON,      szAddFav );
	::AppendMenu( menu.m_hMenu, MF_OWNERDRAW, IDC_NET_FAVORITE_OFF,     szDropFav );
	::AppendMenu( menu.m_hMenu, MF_OWNERDRAW, IDC_NET_DELETE_SELECTED,  szRemove );
	::AppendMenu( menu.m_hMenu, MF_OWNERDRAW, IDC_NET_SORT_LIST,        szQuick );
	::AppendMenu( menu.m_hMenu, MF_OWNERDRAW, IDC_NET_REFRESH_SELECTED, szRefresh );

	menu.TrackPopupMenu( TPM_RIGHTBUTTON, point.x, point.y, m_pParent, NULL );
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::OnLButtonDblClk (0x448C60)
//
// Double-clicking a row joins that server; off a row the base class handles it.

void CODHLListCtrl::OnLButtonDblClk( UINT nFlags, CPoint point )
{
	int	iRow;

	m_pSelectedSv = NULL;

	iRow = RowFromPoint( &point );
	if ( iRow == -1 )
	{
		CODListCtrl::OnLButtonDblClk( nFlags, point );
		return;
	}

	m_pSelectedSv = (CServerInfo*)m_rows[iRow]->record;

	if ( m_bOwnsSort )
		( (CLan*)m_pParent )->OnConnect();
	else
		( (CServerBrowserDlg*)m_pParent )->ConnectSelected();
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::GetSelectedServer (0x448CD0)

CServerInfo* CODHLListCtrl::GetSelectedServer()
{
	int	sel = GetCurSel();

	if ( sel == -1 )
		return NULL;

	m_pSelectedSv = (CServerInfo*)GetItemData( sel );
	return m_pSelectedSv;
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::InsertRecord (0x448D00)

void CODHLListCtrl::InsertRecord( void* pRecord, int iAt )
{
	if ( pRecord && iAt >= 0 )
	{
		InsertItem( iAt, pRecord );
		UpdateScrollbar( 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::ResortByRefreshOrder (0x448D30)
//
// Stamps every row with its current position, sorts the queried servers to
// the top, then re-selects them back-to-front.

void CODHLListCtrl::ResortByRefreshOrder()
{
	CServerInfo*	pSI;
	int				nQueried = 0;
	int				iOrder = 1;
	int				rows;
	int				i;

	BeginUpdate( 1, 0 );
	SelectItem( -1, 1 );

	rows = GetRowCount();
	for ( i = 0; i < rows; i++ )
	{
		pSI = (CServerInfo*)m_rows[i]->record;
		if ( pSI )
		{
			pSI->m_iOrder = iOrder++;
			if ( pSI->m_pOwnedQuery )
				nQueried++;
		}
	}

	SortRows( (odrowcmp_t)ODList_CompareRefreshOrder, -1 );
	if ( nQueried )
		SortRows( (odrowcmp_t)ServerBrowser_CompareInfo, nQueried );

	for ( i = GetRowCount() - 1; i >= 0; i-- )
	{
		pSI = (CServerInfo*)m_rows[i]->record;
		if ( pSI && pSI->m_pOwnedQuery )
			SelectItem( i, 0 );
	}

	RefitScrollbar();
	BeginUpdate( 0, 0 );
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl::OnLButtonDown (0x448E10)
//
// A click in the header re-keys the sort: the same column again flips the
// direction, a different one becomes the new primary key.

void CODHLListCtrl::OnLButtonDown( UINT nFlags, CPoint point )
{
	int	iRow;
	int	iCol;
	int	key;

	m_pSelectedSv = NULL;

	iRow = RowFromPoint( &point );
	if ( iRow != -1 )
		m_pSelectedSv = (CServerInfo*)m_rows[iRow]->record;

	if ( m_bHeaderVisible )
	{
		iCol = ColumnFromPoint( &point );
		if ( iCol != -1 )
		{
			iCol++;						// keys are 1-based column numbers
			key = KeyList_GetKey( 0 );
			if ( !key )
				key = 1;

			if ( abs( key ) == iCol )
				key = -key;				// same column -> flip the direction
			else
				key = iCol;

			KeyList_Set( key );
			SortRows( (odrowcmp_t)ServerBrowser_CompareInfo, -1 );
			return;
		}
	}

	CODListCtrl::OnLButtonDown( nFlags, point );
}
