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
// Purpose: CODListCtrl, the owner-draw report-list base.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

BEGIN_MESSAGE_MAP( CODListCtrl, CWnd )
	ON_WM_NCDESTROY()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_VSCROLL()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONUP()
	ON_WM_SIZE()
	ON_WM_GETDLGCODE()
	ON_WM_KEYDOWN()
	ON_WM_KILLFOCUS()
	ON_WM_CREATE()
	ON_WM_LBUTTONDBLCLK()
#ifdef LAUNCHER_FIXES
	ON_WM_MOUSEWHEEL()
#endif
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::InitMembers (0x44AB00)

void CODListCtrl::InitMembers()
{
	m_hSortAsc   = DIB_LoadBitmapFile( "gfx/shell/up.bmp" );	// +1212
	m_hSortDesc  = DIB_LoadBitmapFile( "gfx/shell/down.bmp" );	// +1216
	m_bSortEnabled   = 0;			// +1220

	m_nSortKey  = 1;				// +1736
	strcpy( m_sortSpec, "1;" );		// +1480
	strcpy( m_sortKeyName, "" );	// +1224

	m_bWheelScroll   = 1;			// +1864
	m_redrawSuppress = 0;			// +1740
	m_bHdrTransparent = 0;			// +1752
	m_bTransparent   = 1;			// +1760
	m_bDrawFrame     = 0;			// +1748
	m_clrRowText = RGB( 255, 127, 24 );		// +1764  normal row text
	m_clrRowBg   = RGB( 0, 0, 0 );		// +1772
	m_clrSelText = RGB( 255, 200, 24 );		// +1776  selected-row text
	m_clrHighlight   = RGB( 80, 56, 24 );	// +1780  selection bar colour
	m_clrFrame   = RGB( 56, 56, 56 );		// +1756  frame, unfocused
	m_clrFrameFocus  = RGB( 128, 128, 128 );	// +1788  frame, focused
	m_clrBg      = RGB( 0, 0, 0 );		// +1744  black list fill

	m_nCols      = 0;				// +1792
	m_nRows      = 0;				// +1832
	m_rowHeight  = 15;				// +1836
	m_topRow     = 0;				// +1840
	m_bHasScrollbar  = 0;			// +1844
	m_bHeaderVisible = 1;			// +1800
	m_headerHeight   = 16;			// +1796
	m_curSel     = -1;				// +1848
	m_pScrollbar = NULL;			// +1852  Create news the bar

	m_pParent          = NULL;		// +1820  set by Create
	m_bScrollbarAlways = 0;			// +1860  nothing ever turns this on
	m_unk1768     = 0;			// +1768
	m_unk1784     = 255;		// +1784

	m_rows = new odrow_t*[4096];	// +1856
	memset( m_rows, 0, 4096 * sizeof( odrow_t* ) );
	m_nRowsMax = 4096;				// +1868  grown by doubling, not a fixed cap

	InitRowPool();
	// NOTE(ox): +1768 and +1784 have no reader anywhere in the image.

	m_brBg.Attach( ::CreateSolidBrush( m_clrRowBg ) );			// +1804
	m_brHighlight.Attach( ::CreateSolidBrush( m_clrHighlight ) );	// +1812

	HFONT	hf = ::CreateFontA( -11, 0, 0, 0, 400, 0, 0, 0, 0,
		4 /*OUT_TT_PRECIS*/, 0, 2 /*PROOF_QUALITY*/, 2, "Arial" );	// +1824
	if ( hf )
		m_headerFont.Attach( hf );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::CODListCtrl (0x44A5E0)

CODListCtrl::CODListCtrl()
{
	InitMembers();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::~CODListCtrl (0x44A680)

CODListCtrl::~CODListCtrl()
{
	if ( m_rows )
	{
		for ( int i = 0; i < m_nRows; i++ )
		{
			if ( m_rows[i] )
				FreeRow( m_rows[i] );
			m_rows[i] = NULL;
		}
		delete[] m_rows;
		m_rows = NULL;
	}

	if ( m_pRowPool )
	{
		delete[] m_pRowPool;
		m_pRowPool = NULL;
	}

	SaveSortOrder();

	if ( m_hSortAsc )
		GlobalFree( m_hSortAsc );			// +1212
	if ( m_hSortDesc )
		GlobalFree( m_hSortDesc );			// +1216

	m_headerFont.DeleteObject();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::Create (0x44A7C0)

BOOL CODListCtrl::Create( DWORD dwStyle, const RECT& rc, CWnd* pParent, UINT nID )
{
	WNDCLASSA	wc;

	m_pParent = pParent;

	memset( &wc, 0, sizeof( wc ) );
	wc.style         = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_OWNDC;
	wc.lpfnWndProc   = AfxGetAfxWndProc();
	wc.hInstance     = AfxGetInstanceHandle();
	wc.hbrBackground = (HBRUSH)::GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "CODListCtrlCls";
	wc.hCursor       = ::LoadCursorA( NULL, IDC_ARROW );
	if ( !AfxRegisterClass( &wc ) )
	{
		Launcher_ShowMessageById( 0, IDS_ODLIST_REGFAIL );
		return FALSE;
	}

	if ( !CWnd::CreateEx( WS_EX_NOPARENTNOTIFY, "CODListCtrlCls", "",
		dwStyle | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		pParent ? pParent->GetSafeHwnd() : NULL, (HMENU)(UINT_PTR)nID, NULL ) )
		return FALSE;

	// Companion vertical scrollbar: a 16px gutter down the right edge, starting
	// below the header band when the header is shown.
	m_pScrollbar = new CODScrollBar;
	if ( !m_pScrollbar )
		return FALSE;

	RECT	rcBar;
	rcBar.left   = rc.right - rc.left - 16;
	rcBar.top    = m_bHeaderVisible ? m_headerHeight : 0;
	rcBar.right  = rc.right - rc.left;
	rcBar.bottom = rc.bottom - rc.top;

	if ( !m_pScrollbar->Create( this, &rcBar, m_rowHeight ) )
		return FALSE;

	m_pScrollbar->ShowWindow( SW_HIDE );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::SetSortKey (0x44A9C0)
//
// names the profile entry this list persists its column order under, and
// seeds the key stack from what is already saved there.

void  CODListCtrl::SetSortKey( const char* pszReg )
{
	CString	str;

	strcpy( m_sortKeyName, pszReg );

	str = "1;";
	str = Launcher_GetProfileString( "Settings", m_sortKeyName, str );
	strcpy( m_sortSpec, str );

	KeyList_FromString( m_sortSpec );
	m_nSortKey = KeyList_GetKey( 0 );
	if ( !m_nSortKey )
		m_nSortKey = 1;

	m_bSortEnabled = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::SaveSortOrder (0x44AAC0)

void  CODListCtrl::SaveSortOrder()
{
	if ( !m_bSortEnabled )
		return;
	if ( !strlen( m_sortKeyName ) )
		return;
	Launcher_WriteProfileString( "Settings", m_sortKeyName, KeyList_ToString() );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnNcDestroy (0x44AD30)
//
// the binary deletes unconditionally; there is no auto-delete guard on
// this control.

void  CODListCtrl::OnNcDestroy()
{
	CWnd::OnNcDestroy();
	delete this;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::AddColumn (0x44AD50)
//
// append one column to the control's internal column array.

void  CODListCtrl::AddColumn( odcolumn_s* pCol )    { m_cols[m_nCols++] = *pCol; }

static int	g_sortColumns[8];	// 0x4F94B4

/////////////////////////////////////////////////////////////////////////////
// Launcher_GetSortColumn (0x4647A0)

static int  Launcher_GetSortColumn( int a )
{
	return g_sortColumns[a & 7];
}

// Map a control-client rect into parent-client space, for the skin blit.
static void ToParentClient( CWnd* pWnd, RECT* prc )
{
	pWnd->ClientToScreen( prc );
	if ( pWnd->GetParent() )
		pWnd->GetParent()->ScreenToClient( prc );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::DrawHeader (0x44AD80)

void  CODListCtrl::DrawHeader( CDC* pDC )
{
	if ( !m_bHeaderVisible )
		return;

	int		sortCol = m_bSortEnabled ? KeyList_GetKey( 0 ) : 1;

	CRect	rc;
	GetClientRect( &rc );
	rc.bottom = rc.top + m_headerHeight;

	if ( m_bHdrTransparent )
	{
		CRect	src( rc );
		ToParentClient( this, &src );
		Launcher_BlitBackground( pDC, &rc, &src );
	}
	else
	{
		CBrush	bg( m_clrBg );
		pDC->FillRect( &rc, &bg );
	}

	CPen	pen;
	pen.CreatePen( PS_SOLID, 1, RGB( 0, 0, 0 ) );

	int		x = 0;
	for ( int i = 0; i < m_nCols; i++ )
	{
		odcolumn_t*	col = &m_cols[i];
		CRect		cell( x, rc.top, x + col->width, rc.bottom );
		x += col->width;

		if ( !m_bHdrTransparent )
		{
			CBrush	cbg( m_clrBg );
			pDC->FillRect( &cell, &cbg );
		}

		int		arrowW = 0;
		if ( m_bSortEnabled && sortCol && ( sortCol < 0 ? -sortCol : sortCol ) == i + 1 )
		{
			pDC->SetTextColor( RGB( 255, 255, 200 ) );

			HGLOBAL	hArrow;
			int		yOff;
			if ( sortCol <= 0 )	{ hArrow = m_hSortDesc; yOff = 4; }
			else				{ hArrow = m_hSortAsc;  yOff = 0; }

			if ( hArrow )
			{
				RECT	src = { 0, 0, 8, 8 };
				RECT	dst = { cell.left + 1, cell.top + 1, cell.left + 9, cell.top + 9 };
				OffsetRect( &dst, 0, yOff );
				arrowW = 9;
				DIB_BlitDib( pDC->GetSafeHdc(), &dst, hArrow, &src );
			}
		}
		else
		{
			pDC->SetTextColor( RGB( 255, 255, 255 ) );
		}

		pDC->SetBkMode( TRANSPARENT );
		CPen*	pOldPen  = pDC->SelectObject( &pen );
		CFont*	pOldFont = pDC->SelectObject( &m_headerFont );

		char	buf[300];
		wsprintfA( buf, " %s", col->title );

		RECT	tr = { cell.left + arrowW + 2, cell.top, cell.right, cell.bottom };
		const char*	fit = CODList_EllipsizeText( pDC, buf, cell.right - tr.left, 2 );
		pDC->DrawText( fit, -1, &tr, 2052 /*DT_VCENTER|0x800*/ );

		pDC->SelectObject( pOldPen );
		pDC->SelectObject( pOldFont );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnPaint (0x44B2C0)

void  CODListCtrl::OnPaint()
{
	CPaintDC	dc( this );
	CRect		rc;
	GetClientRect( &rc );
	if ( m_bHasScrollbar )
		rc.right -= 16;

	if ( m_bHeaderVisible )
	{
		rc.top += m_headerHeight;
		DrawHeader( &dc );
	}

	// A 3px frame, grey while the list has focus and near-black otherwise.
	if ( m_bDrawFrame )
	{
		for ( int pass = 0; pass < 3; pass++ )
		{
			CBrush	frame( GetFocus() == this ? m_clrFrameFocus : m_clrFrame );
			dc.FrameRect( &rc, &frame );
			rc.DeflateRect( 1, 1 );
		}
	}

	// The companion scrollbar owns the scroll offset: take its position, clamped
	// so the last page is never scrolled past, as the first visible row.
	int	visible = GetVisibleRows();
	if ( m_nRows )
	{
		int	maxTop = m_nRows - visible;
		if ( maxTop < 0 )
			maxTop = 0;

		int	pos = m_pScrollbar->GetPos();
		m_topRow = ( pos <= maxTop ) ? pos : maxTop;
	}

	CDC	mem;
	if ( !mem.CreateCompatibleDC( &dc ) )
		return;

	CBitmap		bmp;
	bmp.CreateCompatibleBitmap( &dc, rc.Width(), rc.Height() );
	CBitmap*	pOldBmp = mem.SelectObject( &bmp );

	CRect	bufRc( 0, 0, rc.Width(), rc.Height() );
	if ( m_bTransparent )
	{
		CRect	dst, src;
		GetClientRect( &dst );
		GetWindowRect( &src );
		if ( GetParent() )
			GetParent()->ScreenToClient( &src );
		if ( m_bHeaderVisible )
		{
			dst.bottom -= m_headerHeight;
			src.top    += m_headerHeight;
		}
		if ( m_bHasScrollbar )
		{
			dst.right -= 16;
			src.right -= 16;
		}
		Launcher_BlitBackground( &mem, &dst, &src );
	}
	else
	{
		mem.FillRect( &bufRc, &m_brBg );
	}

	int	last = m_topRow + visible;
	if ( last > m_nRows )
		last = m_nRows;
	for ( int i = m_topRow; i < last; i++ )
		if ( i >= 0 )
			DrawRow( &mem, i );

	dc.BitBlt( rc.left, rc.top, rc.Width(), rc.Height(), &mem, 0, 0, SRCCOPY );
	mem.SelectObject( pOldBmp );
	::ValidateRect( m_hWnd, &rc );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::DrawRow (0x44B6D0)
//
// paint one report row -- its text in the normal or selected colour, with
// a focus rectangle when the row is the focus item.

void  CODListCtrl::DrawRow( CDC* pDC, int iRow )
{
	CRect	client;
	GetClientRect( &client );

	int		right = client.right - ( m_bHasScrollbar ? 16 : 0 );

	if ( iRow - m_topRow < 0 || !m_rows )
		return;

	odrow_t*	pRow = m_rows[iRow];
	if ( !pRow || !pRow->record )
		return;

	CRect	row;
	row.left   = 0;
	row.top    = ( iRow - m_topRow ) * m_rowHeight;
	row.bottom = row.top + m_rowHeight - 1;
	row.right  = right - client.left;

	pDC->SetTextColor( ( pRow->flags & 1 ) ? m_clrSelText : m_clrRowText );
	pDC->SetBkMode( TRANSPARENT );
	pDC->SetBkColor( m_clrRowBg );
	CFont*	pOld = pDC->SelectObject( &m_headerFont );

	char	buf[300];
	sprintf( buf, " %s", pRow->record );
	row.left += 2;
	pDC->DrawText( buf, -1, &row, 2084 /*DT_SINGLELINE|DT_VCENTER|0x800*/ );
	row.left -= 2;

	if ( pRow->flags & 2 )		// focus item
		pDC->DrawFocusRect( &row );

	pDC->SelectObject( pOld );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::ScrollPageUp (0x44B1A0)

void  CODListCtrl::ScrollPageUp()
{
	int	top = m_topRow - GetVisibleRows();
	if ( top < 0 )
		top = 0;
	m_pScrollbar->SetPos( top );
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::ScrollPageDown (0x44B1E0)
//
// scroll the view down by one page (clamped at the last row).

void  CODListCtrl::ScrollPageDown()
{
	int	top = GetVisibleRows() + m_topRow;
	if ( top >= GetRowCount() )
		top = GetRowCount() - 1;
	m_pScrollbar->SetPos( top );
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::ScrollLineUp (0x44B230)
//
// scroll the view up by a single row.

void  CODListCtrl::ScrollLineUp()
{
	GetVisibleRows();
	int	top = m_topRow - 1;
	if ( top < 0 )
		top = 0;
	m_pScrollbar->SetPos( top );
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::ScrollLineDown (0x44B270)
//
// scroll the view down by a single row.

void  CODListCtrl::ScrollLineDown()
{
	GetVisibleRows();
	int	top = m_topRow + 1;
	if ( top >= GetRowCount() )
		top = GetRowCount() - 1;
	m_pScrollbar->SetPos( top );
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnLButtonDown (0x44B820)

void  CODListCtrl::OnLButtonDown( UINT nFlags, CPoint pt )
{
	CRect	rc;
	int		width, top, row, anchor, lo, hi, i;

	GetClientRect( &rc );
	if ( pt.x < 0 )
		return;

	width = rc.right - rc.left;
	if ( m_bHasScrollbar )
		width -= 16;

	top = m_bHeaderVisible ? m_headerHeight : 0;

	if ( pt.x > width )
		return;
	if ( pt.y < top || pt.y > rc.bottom - rc.top )
		return;

	row = m_topRow + ( pt.y - top ) / m_rowHeight;

	if ( nFlags & MK_CONTROL )
	{
		// Extend: add this row to the selection without clearing the others.
		SelectItem( row, 0 );
	}
	else if ( nFlags & MK_SHIFT )
	{
		// Range: select every row between the focus anchor and the clicked row.
		anchor = m_curSel;
		if ( anchor == -1 || anchor == row )
		{
			SelectItem( row, 0 );
		}
		else
		{
			lo = ( anchor < row ) ? anchor : row;
			hi = ( anchor < row ) ? row : anchor;
			BeginUpdate( 1, 0 );
			for ( i = lo; i <= hi; i++ )
				m_rows[i]->flags |= 1;
			SelectItem( hi, 0 );
			BeginUpdate( 0, 0 );
		}
	}
	else
	{
		// Plain click: single-row select, clearing the rest.
		SelectItem( row, 1 );
	}

	SetFocus();
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnLButtonUp (0x455E00)
//
// folded stub

void  CODListCtrl::OnLButtonUp( UINT nFlags, CPoint pt )
{
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnCreate (0x443FA0)
//
// folded stub

int   CODListCtrl::OnCreate( LPCREATESTRUCT lpcs )
{
	return (int)Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnKillFocus (0x450640)
//
// folded stub; the focus frame changes colour

void  CODListCtrl::OnKillFocus( CWnd* pNewWnd )
{
	Default();
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnGetDlgCode (0x44A4D0)
//
// folded stub: claim the arrow keys

UINT  CODListCtrl::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}


/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::AddRow (0x44B950)
//
// the row record is the caller's pointer; the per-list DrawRow override is
// what turns it into columns.

void  CODListCtrl::AddRow( void* record )
{
	odrow_t*	row = AllocRow();

	row->flags  = 0;
	row->record = record;
	m_rows[m_nRows] = row;

	GrowRows();

	if ( !m_redrawSuppress )
	{
		SelectItem( m_nRows - 1, 1 );
		m_pScrollbar->SetRange( 0, m_nRows );
		m_pScrollbar->SetPos( m_curSel );
		UpdateScrollbar( 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::ResetContent (0x44B9D0)

void  CODListCtrl::ResetContent()
{
	for ( int i = 0; i < m_nRows; i++ )
	{
		if ( m_rows[i] )
			FreeRow( m_rows[i] );
		m_rows[i] = NULL;
	}

	m_nRows = 0;
	memset( m_rows, 0, 4096 * sizeof( odrow_t* ) );	// binary memset 0x4000 over the node array

	SelectItem( -1, 1 );			// vtable slot 50: deselect everything
	m_pScrollbar->SetRange( 0, 100 );
	m_pScrollbar->SetPos( 0 );
	UpdateScrollbar( 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::GetRowCount (0x44BA60)
//
// how many rows the list holds.

int   CODListCtrl::GetRowCount()			{ return m_nRows; }

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::DeleteItem (0x44BA70)

void  CODListCtrl::DeleteItem( int item )
{
	if ( item < 0 || item >= m_nRows )
		return;

	int		prevSel = m_curSel;

	SelectItem( -1, 0 );

	if ( m_rows[item] )
		FreeRow( m_rows[item] );

	for ( int i = item; i < m_nRows - 1; i++ )
		m_rows[i] = m_rows[i + 1];
	m_rows[m_nRows - 1] = NULL;
	m_nRows--;

	if ( m_pScrollbar )
	{
		if ( m_nRows )
		{
			m_pScrollbar->SetRange( 0, m_nRows );
			m_pScrollbar->SetPos( m_curSel );
		}
		else
		{
			m_pScrollbar->SetRange( 0, 100 );
			m_pScrollbar->SetPos( 0 );
		}
	}
	UpdateScrollbar( 0 );

	if ( m_nRows )
	{
		if ( prevSel > item )
			SelectItem( prevSel - 1, 0 );	// shifted down with the rows
		else if ( prevSel < item )
			SelectItem( prevSel, 0 );		// unaffected
		// prevSel == item: the selected row is gone -- leave nothing selected
	}

	UpdateScrollbar( 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::GetItemData (0x44C2C0)
//
// the record stored on row `item` (odrow_t.text), or NULL when the index
// is out of range. Callers cast it back to whatever they handed AddRow.

void* CODListCtrl::GetItemData( int item )
{
	if ( item < 0 || item >= m_nRows )
		return NULL;
	return m_rows[item]->record;
}

static int	g_listAnchor = -1;		// dword_4D0FA4 (multi-select anchor)

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::SelectItem (0x44BB70)

void  CODListCtrl::SelectItem( int item, int bClearOthers )
{
	int		bRepaint = 0;

	if ( bClearOthers )
	{
		for ( int i = 0; i < m_nRows; i++ )
			m_rows[i]->flags = 0;
		bRepaint = 1;
	}

	if ( item == -1 )
	{
		if ( !bClearOthers && g_listAnchor != -1 && g_listAnchor < m_nRows )
		{
			if ( m_bTransparent )
			{
				bRepaint = 1;
			}
			else
			{
				CClientDC	dc( this );
				DrawRow( &dc, g_listAnchor );
			}
		}
		m_curSel     = -1;
		g_listAnchor = -1;
		if ( bRepaint )
		{
			InvalidateRect( NULL, TRUE );
			UpdateWindow();
		}
		return;
	}

	if ( item < 0 || item >= m_nRows )
	{
		if ( bRepaint ) { InvalidateRect( NULL, TRUE ); UpdateWindow(); }
		return;
	}

	if ( ( m_rows[item]->flags & 1 ) && GetAsyncKeyState( VK_CONTROL ) )
	{
		// Ctrl-click on a selected row: move focus there, drop its selected bit.
		g_listAnchor = m_curSel;
		if ( m_curSel != -1 )
			m_rows[m_curSel]->flags &= ~2;
		m_rows[item]->flags |= 2;
		m_rows[item]->flags &= ~1;
	}
	else
	{
		g_listAnchor = m_curSel;
		m_curSel     = item;
		if ( g_listAnchor != -1 )
			m_rows[g_listAnchor]->flags &= ~2;
		m_rows[item]->flags |= 2;	// focus
		m_rows[item]->flags |= 1;	// selected
		if ( g_listAnchor != item )
		{
			CWnd*	pParent = GetParent();
			if ( pParent )
				pParent->SendMessage( WM_COMMAND, MAKEWPARAM( GetDlgCtrlID(), 1 ), (LPARAM)m_hWnd );
		}
	}

	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::GetVisibleRows (0x44BDD0)
//
// how many rows fit below the header (the page size used for scrolling).

int   CODListCtrl::GetVisibleRows()
{
	CRect	rc;
	GetClientRect( &rc );
	int	top = rc.top + ( m_bHeaderVisible ? m_headerHeight : 0 );
	if ( m_rowHeight )
		return (int)( (double)( rc.bottom - top ) / m_rowHeight + 0.5 );
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::UpdateScrollbar (0x44BE40)
//
// bForce shows/hides even when the bar is already in the wanted state, so
// a resize still repaints.

void CODListCtrl::UpdateScrollbar( int bForce )
{
	if ( m_bScrollbarAlways )
	{
		if ( !m_bHasScrollbar || bForce )
		{
			m_bHasScrollbar = 1;
			m_pScrollbar->ShowWindow( SW_RESTORE );
			::InvalidateRect( m_hWnd, NULL, TRUE );
		}
	}
	else if ( m_nRows > GetVisibleRows() && ( !m_bHasScrollbar || bForce ) )
	{
		m_bHasScrollbar = 1;
		m_pScrollbar->ShowWindow( SW_RESTORE );
		::InvalidateRect( m_hWnd, NULL, TRUE );
	}
	else if ( m_nRows <= GetVisibleRows() && ( m_bHasScrollbar || bForce ) )
	{
		m_bHasScrollbar = 0;
		m_pScrollbar->ShowWindow( SW_HIDE );
		::InvalidateRect( m_hWnd, NULL, TRUE );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::InsertItem (0x44BF30)

void  CODListCtrl::InsertItem( int item, void* data )
{
	if ( item < 0 || item > m_nRows )
		return;

	for ( int i = m_nRows; i > item; i-- )
		m_rows[i] = m_rows[i - 1];

	m_rows[item] = AllocRow();
	m_rows[item]->record  = (char*)data;
	m_rows[item]->flags = 0;

	GrowRows();

	if ( !m_redrawSuppress )
		SelectItem( item, 1 );

	m_pScrollbar->SetRange( 0, m_nRows );
	m_pScrollbar->SetPos( m_curSel );
	UpdateScrollbar( 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::RowFromPoint (0x44BFF0)

int   CODListCtrl::RowFromPoint( POINT* pt )
{
	CRect	rc;
	GetClientRect( &rc );

	if ( pt->x < 0 )
		return -1;

	int	width = rc.right - rc.left;
	if ( m_bHasScrollbar )
		width -= 16;

	int	top = m_bHeaderVisible ? m_headerHeight : 0;

	if ( pt->x > width )
		return -1;
	if ( pt->y < top )
		return -1;
	if ( pt->y > rc.bottom - rc.top )
		return -1;

	int	row = m_topRow + ( pt->y - top ) / m_rowHeight;
	if ( row >= m_nRows )
		return -1;
	return row;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnVScroll (0x44A0B0)
//
// the companion scrollbar owns the offset; the list only has to repaint.

void CODListCtrl::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

// Owner-draw style flags (set at list creation, read by the report paint).
void  CODListCtrl::SetHeaderTransparent( int bOn ) { m_bHdrTransparent = bOn; }
void  CODListCtrl::SetDrawFrame( int bOn )         { m_bDrawFrame = bOn; }
void  CODListCtrl::SetTransparent( int bOn )       { m_bTransparent = bOn; }
void  CODListCtrl::SetHighlight( COLORREF clr )     { m_clrHighlight = clr; }

static odrowcmp_t	g_pRowCompare;		// 0x4F4BF0 (latched by SortRows)

/////////////////////////////////////////////////////////////////////////////
// CODList_CompareRows (0x44C080)
//
// qsort thunk: unwrap two row nodes and hand their records to the
// comparison SortRows latched.

static int __cdecl CODList_CompareRows( const void* a, const void* b )
{
	return g_pRowCompare( ( *(odrow_t**)a )->record, ( *(odrow_t**)b )->record, 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::SortRows (0x44C0A0)

void  CODListCtrl::SortRows( odrowcmp_t cmp, int count )
{
	if ( !::IsWindow( GetSafeHwnd() ) )
		return;

	g_pRowCompare = cmp;

	int	n = m_nRows;
	if ( count != -1 && n >= count )
		n = count;

	qsort( m_rows, n, sizeof( odrow_t* ), CODList_CompareRows );

	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::ToggleHeader (0x44C120)
//
// flip the header row on/off, then reposition the companion scrollbar to
// the new top (under the header band when it is now shown) and repaint.

void  CODListCtrl::ToggleHeader()
{
	m_bHeaderVisible = !m_bHeaderVisible;

	CRect	rc;
	GetWindowRect( &rc );
	int		top = m_bHeaderVisible ? m_headerHeight : 0;

	if ( m_pScrollbar )
	{
		m_pScrollbar->MoveWindow( rc.Width() - 16, top, 16, rc.Height() - top, TRUE );
		UpdateScrollbar( 0 );
	}

	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::GetCurSel (0x44C1C0)
//
// the current selected/focus row (-1 = none).

int   CODListCtrl::GetCurSel()				{ return m_curSel; }

/////////////////////////////////////////////////////////////////////////////
// CODList_EllipsizeText (0x44C1D0)
//
// fit lpString into maxWidth pixels (after an indent), truncating with a
// trailing "..." when it overflows. Used by the report header/row
// painters.

const char* CODList_EllipsizeText( CDC* pDC, const char* lpString, int maxWidth, int indent )
{
	static char	buf[2048];		// (binary: byte_4F42EC+2048 scratch)
	HDC			hdc = pDC->GetSafeHdc();
	SIZE		sz;
	int			len = lstrlenA( lpString );

	if ( !len )
		return lpString;

	GetTextExtentPoint32A( hdc, lpString, len, &sz );
	if ( indent + sz.cx <= maxWidth )
		return lpString;

	lstrcpyA( buf, lpString );
	GetTextExtentPoint32A( hdc, "...", 4, &sz );	// binary measures 4 (incl. NUL)
	int	n = len - 1;
	if ( n > 0 )
	{
		int	ellipsisW = sz.cx + indent;
		do
		{
			buf[n] = 0;
			GetTextExtentPoint32A( hdc, buf, n, &sz );
			if ( ellipsisW + sz.cx <= maxWidth )
				break;
			--n;
		} while ( n > 0 );
	}
	lstrcatA( buf, "..." );
	return buf;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::SetRowHeight (0x44C2A0)
//
// the bar repages itself from the new row height; before Create there is
// no bar to tell.

void  CODListCtrl::SetRowHeight( int h )
{
	m_rowHeight = h;
	if ( m_pScrollbar )
		m_pScrollbar->SetRowHeight( h );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::RefitScrollbar (0x44C2F0)

void  CODListCtrl::RefitScrollbar()
{
	m_pScrollbar->SetPos( m_curSel );
	UpdateScrollbar( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::SetBorderColor (0x44C310)
//
// set the unfocused-border colour (the report paint frames the client rect
// with this when the list does not hold the focus).

void  CODListCtrl::SetBorderColor( COLORREF clr )	{ m_clrFrame = clr; }

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnSize (0x44C350)

void  CODListCtrl::OnSize( UINT nType, int cx, int cy )
{
	Default();

	if ( m_pScrollbar && m_pScrollbar->GetSafeHwnd() )
	{
		int	top = m_bHeaderVisible ? m_headerHeight : 0;
		m_pScrollbar->MoveWindow( cx - 16, top, 16, cy - top, TRUE );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::GetItemFlags (0x44C3A0)
//
// the per-row flag word (bit0 selected, bit1 focus), or 0 when the index
// is out of range.

int   CODListCtrl::GetItemFlags( int item )
{
	if ( item < 0 || item >= m_nRows )
		return 0;
	return m_rows[item]->flags;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::PreTranslateMessage (0x44C3D0)

BOOL  CODListCtrl::PreTranslateMessage( MSG* pMsg )
{
	if ( !m_bWheelScroll )
		return CWnd::PreTranslateMessage( pMsg );

	// The binary also matches the registered MSWHEEL_ROLLMSG.
	if ( pMsg->message != WM_MOUSEWHEEL )
		return CWnd::PreTranslateMessage( pMsg );

	if ( (int)pMsg->wParam > 0 )
		ScrollLineUp();
	else
		ScrollLineDown();
	return TRUE;
}

#ifdef LAUNCHER_FIXES
/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::EnsureVisible (LAUNCHER_FIXES)
//
// Scroll the least that brings `item` onto the page.

void  CODListCtrl::EnsureVisible( int item )
{
	int	visible = GetVisibleRows();
	int	top     = m_topRow;

	if ( item < top )
		top = item;
	else if ( item > top + visible - 1 )
		top = item - visible + 1;
	else
		return;

	if ( top < 0 )
		top = 0;

	m_pScrollbar->SetPos( top );
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::NavSelect (LAUNCHER_FIXES)
//
// SelectItem is the same path a plain click takes, so the parent gets its
// selection-changed WM_COMMAND either way.

void  CODListCtrl::NavSelect( int item )
{
	if ( m_nRows <= 0 )
		return;

	if ( item < 0 )
		item = 0;
	if ( item >= m_nRows )
		item = m_nRows - 1;

	SelectItem( item, 1 );
	EnsureVisible( item );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::ActivateSelection (LAUNCHER_FIXES)
//
// ENTER opens the selected row through the row's own double-click handler, so
// each list keeps whatever a double-click already meant to it.

void  CODListCtrl::ActivateSelection()
{
	CPoint	pt;

	if ( m_curSel < 0 || m_curSel >= m_nRows )
		return;

	EnsureVisible( m_curSel );

	pt.x = 4;
	pt.y = ( m_bHeaderVisible ? m_headerHeight : 0 )
		 + ( m_curSel - m_topRow ) * m_rowHeight + m_rowHeight / 2;

	SendMessage( WM_LBUTTONDBLCLK, 0, MAKELPARAM( pt.x, pt.y ) );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnMouseWheel (LAUNCHER_FIXES)
//
// The dialog re-aims the wheel here from wherever the focus happens to be; the
// binary only ever saw it through PreTranslateMessage.

BOOL  CODListCtrl::OnMouseWheel( UINT /*nFlags*/, short zDelta, CPoint /*pt*/ )
{
	int	steps;

	if ( !m_bWheelScroll || !m_pScrollbar || !zDelta )
		return FALSE;

	steps = ( abs( zDelta ) / WHEEL_DELTA ) * Dlg_WheelScrollLines();
	if ( steps < 1 )
		steps = 1;

	while ( steps-- > 0 )
	{
		if ( zDelta > 0 )
			ScrollLineUp();
		else
			ScrollLineDown();
	}
	return TRUE;
}

#endif	// LAUNCHER_FIXES

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnKeyDown (0x44C420)
//
// SHIFT+TAB takes the same forward step as TAB (sic): the binary tests
// VK_SHIFT but passes bPrevious = FALSE either way.

void  CODListCtrl::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
#ifdef LAUNCHER_FIXES
	int	sel = ( m_curSel == -1 ) ? 0 : m_curSel;

	switch ( nChar )
	{
	case VK_PRIOR:	NavSelect( sel - GetVisibleRows() );	return;
	case VK_NEXT:	NavSelect( sel + GetVisibleRows() );	return;
	case VK_UP:		NavSelect( sel - 1 );					return;
	case VK_DOWN:	NavSelect( sel + 1 );					return;
	case VK_HOME:	NavSelect( 0 );							return;
	case VK_END:	NavSelect( m_nRows - 1 );				return;
	case VK_RETURN:	ActivateSelection();					return;
	}
#endif

	switch ( nChar )
	{
	case VK_TAB:		// 9
	{
		HWND	hNext;
		if ( ::GetAsyncKeyState( VK_SHIFT ) )
			hNext = ::GetNextDlgTabItem( GetParent()->GetSafeHwnd(), GetSafeHwnd(), FALSE );
		else
			hNext = ::GetNextDlgTabItem( GetParent()->GetSafeHwnd(), GetSafeHwnd(), FALSE );
		if ( hNext )
			::SetFocus( hNext );
		break;
	}

	case VK_PRIOR:		// 33
		ScrollPageUp();
		break;

	case VK_NEXT:		// 34
		ScrollPageDown();
		break;

	case VK_UP:			// 38
		ScrollLineUp();
		break;

	case VK_DOWN:		// 40
		ScrollLineDown();
		break;

	default:
		Default();
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::HitTestCell (0x44C520)

int   CODListCtrl::HitTestCell( int iRow, int x, RECT* prc, int* pCol )
{
	int	colX = 0;

	prc->right  = 10;
	prc->bottom = 10;
	prc->left   = 0;
	prc->top    = 0;
	*pCol       = 0;

	if ( iRow < 0 || iRow >= m_nRows )
		return 0;

	if ( iRow < m_topRow )
		return 0;

	prc->top    = m_rowHeight * ( iRow - m_topRow );
	prc->bottom = prc->top + m_rowHeight;
	if ( m_bHeaderVisible )
	{
		prc->top    += m_headerHeight;
		prc->bottom += m_headerHeight;
	}

	if ( m_nCols <= 0 )
		return 0;

	for ( int c = 0; c < m_nCols; c++ )
	{
		prc->left  = colX;
		prc->right = colX + m_cols[c].width;
		if ( c == m_nCols - 1 && m_bHasScrollbar )
			prc->right -= 16;

		if ( x >= colX && x <= prc->right )
		{
			*pCol = c;
			return 1;
		}

		colX += m_cols[c].width;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::GetColumnCount (0x44C620)
//
// how many columns the list holds.

int   CODListCtrl::GetColumnCount()		{ return m_nCols; }

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::GetCellRect (0x44C630)

int   CODListCtrl::GetCellRect( int iRow, int iCol, RECT* prc )
{
	prc->left   = 0;
	prc->top    = 0;
	prc->right  = 10;
	prc->bottom = 10;

	if ( iRow < 0 || iRow >= m_nRows )
		return 0;
	if ( iRow < m_topRow || iCol < 0 || iCol >= m_nCols )
		return 0;

	int	top = m_rowHeight * ( iRow - m_topRow );
	prc->top    = top;
	prc->bottom = top + m_rowHeight;
	if ( m_bHeaderVisible )
	{
		prc->top    += m_headerHeight;
		prc->bottom += m_headerHeight;
	}

	int	colX = 0;
	for ( int c = 0; c <= iCol; c++ )
	{
		prc->left  = colX;
		prc->right = colX + m_cols[c].width;
		if ( c == m_nCols - 1 && m_bHasScrollbar )
			prc->right -= 16;
		colX += m_cols[c].width;
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::ColumnFromPoint (0x44C720)
//
// which column header the point falls under, or -1. Only the header band
// is hit-tested; the 5px gutter on each side of a column boundary is dead
// space.

int   CODListCtrl::ColumnFromPoint( POINT* pt )
{
	CRect	rc;
	GetClientRect( &rc );

	if ( !m_bHeaderVisible )
		return -1;

	rc.bottom = rc.top + m_headerHeight;
	if ( !PtInRect( &rc, *pt ) )
		return -1;

	if ( m_nCols <= 0 )
		return -1;

	int	colX = 0;
	for ( int c = 0; c < m_nCols; c++ )
	{
		if ( pt->x >= colX + 5 && pt->x <= m_cols[c].width + colX - 5 )
			return c;
		colX += m_cols[c].width;
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnLButtonDblClk (0x44C7E0)

void  CODListCtrl::OnLButtonDblClk( UINT nFlags, CPoint pt )
{
	if ( GetCurSel() == -1 )
		return;

	UINT	id     = GetDlgCtrlID();
	CWnd*	pOwner = CWnd::FromHandle( ::GetParent( m_hWnd ) );
	if ( pOwner )
		pOwner->SendMessage( WM_COMMAND, id | 0x20000, (LPARAM)m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::BeginUpdate (0x44C840)

void  CODListCtrl::BeginUpdate( int bBegin, int bEnd )
{
	m_redrawSuppress = bBegin;
	if ( !bBegin && bEnd )
	{
		m_pScrollbar->SetRange( 0, m_nRows );
		m_pScrollbar->SetPos( m_curSel );
		UpdateScrollbar( 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::SetHeaderFont (0x44C890)

void  CODListCtrl::SetHeaderFont( int nSize, int cWeight )
{
	m_headerFont.DeleteObject();
	HFONT	hFont = CreateFontA( -nSize, 0, 0, 0, cWeight, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, DEFAULT_PITCH, "Arial" );
	m_headerFont.Attach( hFont );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::GrowRows (0x44C8E0)
//
// count the row that was just stored and, two slots short of capacity,
// double both the pointer array and the node pool. The live nodes are
// copied into the new pool, so every m_rows[] entry is re-pointed.

void  CODListCtrl::GrowRows()
{
	m_nRows++;
	if ( m_nRows < m_nRowsMax - 2 )
		return;

	int	oldMax = m_nRowsMax;
	m_nRowsMax = 2 * oldMax;

	odrow_t**	newRows = new odrow_t*[m_nRowsMax];
	if ( !newRows )
	{
		// (sic) the count lands in AfxMessageBox's nType, so %i never expands
		AfxMessageBox( "Out of memory growing list control to %i items\n", m_nRowsMax, 0 );
		PostQuitMessage( 0 );
		return;
	}

	memset( newRows, 0, m_nRowsMax * sizeof( odrow_t* ) );
	memcpy( newRows, m_rows, oldMax * sizeof( odrow_t* ) );
	delete[] m_rows;
	m_rows = newRows;

	odrow_t*	newPool = new odrow_t[m_nRowsMax];
	memset( newPool, 0, m_nRowsMax * sizeof( odrow_t ) );

	memset( &m_liveRows, 0, sizeof( m_liveRows ) );
	m_liveRows.pNext = m_liveRows.pPrev = &m_liveRows;

	memset( &m_freeRows, 0, sizeof( m_freeRows ) );
	m_freeRows.pNext = m_freeRows.pPrev = &m_freeRows;

	for ( int i = 0; i < m_nRowsMax; i++ )
		LinkRow( &newPool[i], &m_freeRows );

	for ( int r = 0; r < m_nRows; r++ )
	{
		odrow_t*	pOld = m_rows[r];
		if ( !pOld )
			continue;

		odrow_t*	pNew = m_freeRows.pNext;
		pNew->flags = pOld->flags;
		pNew->record  = pOld->record;
		UnlinkRow( pNew, &m_freeRows );
		LinkRow( pNew, &m_liveRows );
		m_rows[r] = pNew;
	}

	delete[] m_pRowPool;
	m_pRowPool = newPool;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::LinkRow (0x44CAA0)
//
// push a node onto the head of a ring.

void  CODListCtrl::LinkRow( odrow_t* pNode, odrow_t* pHead )
{
	pNode->pNext        = pHead->pNext;
	pHead->pNext->pPrev = pNode;
	pHead->pNext        = pNode;
	pNode->pPrev        = pHead;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::UnlinkRow (0x44CAC0)
//
// pHead is unused; the node's own links name the ring it leaves.

void  CODListCtrl::UnlinkRow( odrow_t* pNode, odrow_t* /*pHead*/ )
{
	pNode->pPrev->pNext = pNode->pNext;
	pNode->pNext->pPrev = pNode->pPrev;
	pNode->pPrev = NULL;
	pNode->pNext = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::InitRowPool (0x44CAF0)

void  CODListCtrl::InitRowPool()
{
	memset( &m_freeRows, 0, sizeof( m_freeRows ) );
	m_freeRows.pNext = m_freeRows.pPrev = &m_freeRows;

	memset( &m_liveRows, 0, sizeof( m_liveRows ) );
	m_liveRows.pNext = m_liveRows.pPrev = &m_liveRows;

	m_pRowPool = new odrow_t[4096];
	memset( m_pRowPool, 0, 4096 * sizeof( odrow_t ) );

	for ( int i = 0; i < 4096; i++ )
		LinkRow( &m_pRowPool[i], &m_freeRows );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::AllocRow (0x44CB70)
//
// take the head of the free ring and move it to the live ring; NULL once
// the pool is drained.

odrow_t*  CODListCtrl::AllocRow()
{
	odrow_t*	pRow = m_freeRows.pNext;
	if ( pRow == &m_freeRows )
		return NULL;

	UnlinkRow( pRow, &m_freeRows );
	LinkRow( pRow, &m_liveRows );
	return pRow;
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::FreeRow (0x44CBB0)
//
// the node belongs to the pool; returning it to the free ring is all a
// "free" is.

void  CODListCtrl::FreeRow( odrow_t* pRow )
{
	UnlinkRow( pRow, &m_liveRows );
	LinkRow( pRow, &m_freeRows );
}

/////////////////////////////////////////////////////////////////////////////
// CODListCtrl::OnEraseBkgnd (0x44BDC0)
//
// OnPaint fills the whole client itself

BOOL  CODListCtrl::OnEraseBkgnd( CDC* pDC )
{
	return TRUE;
}

BEGIN_MESSAGE_MAP( CODColorPane, CWnd )
	//{{AFX_MSG_MAP(CODColorPane)
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODColorPane::OnCtlColor (0x44CBE0)

HBRUSH CODColorPane::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH	hbr = CWnd::OnCtlColor( pDC, pWnd, nCtlColor );

	if ( nCtlColor == CTLCOLOR_EDIT )
	{
		pDC->SetBkMode( TRANSPARENT );
		pDC->SetTextColor( m_clrText );
		return (HBRUSH)m_brBg.GetSafeHandle();
	}
	return hbr;
}

/////////////////////////////////////////////////////////////////////////////
// CODColorPane::OnEraseBkgnd (0x44CC30)

BOOL CODColorPane::OnEraseBkgnd( CDC* pDC )
{
	RECT	rc;

	GetClientRect( &rc );
	pDC->FillRect( &rc, &m_brBg );
	return TRUE;
}
