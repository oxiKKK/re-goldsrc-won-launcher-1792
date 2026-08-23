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
// Purpose: CODListBox, the owner-draw list box.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Entry order decoded from the AFX_MSGMAP_ENTRY array at 0x4B1930 (map 0x4B1928).
BEGIN_MESSAGE_MAP( CODListBox, CWnd )
	ON_WM_NCDESTROY()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_VSCROLL()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_GETDLGCODE()
	ON_WM_CREATE()
	ON_WM_KEYDOWN()
#ifdef LAUNCHER_FIXES
	ON_WM_MOUSEWHEEL()
#endif
END_MESSAGE_MAP()

// The list's previous-selection anchor (dword_4D0F74): SetCurSel parks the old
// selection here so the deselect repaint can find it.
static int	g_odlbAnchor = -1;		// 0x4D0F74

/////////////////////////////////////////////////////////////////////////////
// CODListBox::CODListBox (0x449400)

CODListBox::CODListBox()
{
	InitMembers();
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::~CODListBox (0x449490)

CODListBox::~CODListBox()
{
	if ( m_pItems )
		delete[] m_pItems;		// the companion scrollbar is not freed here

	m_font.DeleteObject();
	m_brBg.DeleteObject();
	m_brSel.DeleteObject();
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::Create (0x449550)

BOOL CODListBox::Create( DWORD dwStyle, RECT* prc, CWnd* pParent, UINT nID )
{
	RECT	rc;
	CopyRect( &rc, prc );

	m_rowHeight = 15;
	m_pParent   = pParent;

	WNDCLASSA	wc;
	memset( &wc, 0, sizeof( wc ) );
	wc.style		 = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	wc.lpfnWndProc	 = AfxGetAfxWndProc();
	wc.hInstance	 = AfxGetInstanceHandle();
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName	 = NULL;
	wc.lpszClassName = "CODListBoxCls";
	wc.hCursor		 = ::LoadCursorA( NULL, IDC_ARROW );
	if ( !AfxRegisterClass( &wc ) )
	{
		Launcher_ShowMessageById( 0, IDS_ODLISTBOX_REGFAIL );
		return FALSE;
	}

	if ( !CreateEx( 0, "CODListBoxCls", "",
		dwStyle | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP,	// 0x54010000
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		pParent ? pParent->GetSafeHwnd() : NULL, (HMENU)(UINT_PTR)nID, NULL ) )
		return FALSE;

	m_pScrollbar = new CODScrollBar;
	if ( !m_pScrollbar )
		return FALSE;

	RECT	rcBar;
	rcBar.left   = rc.right - rc.left - 19;
	rcBar.top    = 3;
	rcBar.right  = rc.right - rc.left - 3;
	rcBar.bottom = rc.bottom - rc.top - 3;
	if ( !m_pScrollbar->Create( this, &rcBar, m_rowHeight ) )
		return FALSE;

	m_pScrollbar->ShowWindow( SW_HIDE );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::InitMembers (0x449740)

void CODListBox::InitMembers()
{
	m_bTransparent		= 0;
	m_pOwnerCombo		= NULL;
	m_bAutoDelete		= 1;
	m_pParent			= NULL;
	m_nCount			= 0;
	m_rowHeight			= 15;
	m_topRow			= 0;
	m_bHasScrollbar		= 0;
	m_curSel			= -1;
	m_pScrollbar		= NULL;
	m_pOwnerCombo		= NULL;

	m_clrText			= RGB( 255, 127, 24 );
	m_clrSelText		= RGB( 255, 150, 24 );
	m_clrFrameInactive	= RGB( 56, 56, 56 );
	m_clrFocusFrame		= RGB( 128, 128, 128 );

	m_brSel.Attach( ::CreateSolidBrush( RGB( 84, 45, 0 ) ) );
	m_pItems = new odlbitem_t[ODLB_MAX_ITEMS];
	if ( m_pItems )
		memset( m_pItems, 0, ODLB_MAX_ITEMS * sizeof( odlbitem_t ) );
	m_brBg.Attach( ::CreateSolidBrush( RGB( 0, 0, 0 ) ) );

	HFONT	hf = ::CreateFontA( -11, 0, 0, 0, 400, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, 2, "Arial" );
	if ( hf )
		m_font.Attach( hf );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::NavPageUp (0x449810)

void CODListBox::NavPageUp()
{
	int	page = GetVisibleRows();
	int	sel  = GetCurSel() - page;
	if ( sel < 0 )
		sel = 0;

	SetCurSel( sel );

	if ( sel < GetTopIndex() )
		m_pScrollbar->SetPos( sel );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::NavPageDown (0x449850)

void CODListBox::NavPageDown()
{
	int	page = GetVisibleRows();
	int	sel  = GetCurSel() + page;
	if ( sel >= GetCount() )
		sel = GetCount() - 1;

	SetCurSel( sel );

	if ( sel > GetTopIndex() + GetVisibleRows() - 1 )
		m_pScrollbar->SetPos( sel );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::NavLineUp (0x4498B0)

void CODListBox::NavLineUp()
{
	int	sel = GetCurSel() - 1;
	if ( sel < 0 )
		sel = 0;

	SetCurSel( sel );

	if ( sel < GetTopIndex() )
		m_pScrollbar->SetPos( sel );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::NavLineDown (0x4498F0)

void CODListBox::NavLineDown()
{
	int	sel = GetCurSel() + 1;
	if ( sel >= GetCount() )
		sel = GetCount() - 1;

	SetCurSel( sel );

	if ( sel > GetTopIndex() + GetVisibleRows() - 1 )
		m_pScrollbar->SetPos( sel );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnPaint (0x449950)

void CODListBox::OnPaint()
{
	CPaintDC	dc( this );

	// Owned by a combo (0x4034A0 / 0x44A3D0 set these at Create): the owner
	// paints our rows, so we draw nothing and just dirty our slice of it.
	if ( m_bTransparent )
	{
		if ( m_pOwnerCombo )
		{
			RECT	rcMirror;
			::GetWindowRect( GetSafeHwnd(), &rcMirror );
			m_pOwnerCombo->ScreenToClient( &rcMirror );
			::InvalidateRect( m_pOwnerCombo->GetSafeHwnd(), &rcMirror, TRUE );
		}
		return;
	}

	RECT	rc;
	::GetClientRect( GetSafeHwnd(), &rc );
	if ( m_bHasScrollbar )
		rc.right -= 16;
	::ValidateRect( GetSafeHwnd(), &rc );
	InflateRect( &rc, -3, -3 );

	ClampTopIndex();

	CDC	mem;
	if ( !mem.CreateCompatibleDC( &dc ) )
		return;

	CBitmap	bmp;
	bmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
		rc.right - rc.left, rc.bottom - rc.top ) );
	CBitmap*	pOldBmp = mem.SelectObject( &bmp );

	RECT	full = { 0, 0, rc.right - rc.left, rc.bottom - rc.top };
	mem.FillRect( &full, &m_brBg );

	int	last = GetVisibleRows() + m_topRow;
	if ( last > m_nCount )
		last = m_nCount;
	for ( int i = m_topRow; i < last; i++ )
		if ( i >= 0 )
			PaintRow( &mem, i );

	dc.BitBlt( 3, 3, rc.right - rc.left, rc.bottom - rc.top, &mem, 0, 0, SRCCOPY );

	RECT	frame;
	::GetClientRect( GetSafeHwnd(), &frame );
	BOOL	bFocus = ( CWnd::FromHandle( ::GetFocus() ) == this );
	for ( int n = 0; n < 3; n++ )
	{
		CBrush	br( bFocus ? m_clrFocusFrame : m_clrFrameInactive );
		dc.FrameRect( &frame, &br );
		InflateRect( &frame, -1, -1 );
	}

	mem.SelectObject( pOldBmp );

	// Our area is fully covered, so spare the parent from repainting under us.
	CWnd*	pParent = GetParent();
	if ( pParent )
	{
		RECT	rcSelf;
		::GetWindowRect( GetSafeHwnd(), &rcSelf );
		pParent->ScreenToClient( &rcSelf );
		::ValidateRect( pParent->GetSafeHwnd(), &rcSelf );
	}

	// The gutter belongs to the scrollbar child -- hand it the dirty rect.
	if ( m_bHasScrollbar && m_pScrollbar )
	{
		RECT	rcBar;
		::GetClientRect( GetSafeHwnd(), &rcBar );
		::InflateRect( &rcBar, -3, -3 );
		rcBar.left = rcBar.right - 16;
		ClientToScreen( &rcBar );
		m_pScrollbar->ScreenToClient( &rcBar );
		::InvalidateRect( m_pScrollbar->GetSafeHwnd(), &rcBar, TRUE );
	}
}

// CODListBox::PaintRow (0x449D50, or the owning combo's DrawRow)
void CODListBox::PaintRow( CDC* pDC, int iItem )
{
	if ( m_pOwnerCombo )
	{
		m_pOwnerCombo->DrawRow( pDC, iItem );		// child -> owner callback
		return;
	}

	RECT	client;
	::GetClientRect( GetSafeHwnd(), &client );

	int	vis = iItem - m_topRow;
	if ( vis < 0 )
		return;

	int	right = client.right - 6;
	if ( m_bHasScrollbar )
		right -= 16;

	RECT	row;
	row.left   = 0;
	row.right  = right;
	row.top    = vis * m_rowHeight;
	row.bottom = row.top + m_rowHeight;

	const char*	psz = GetText( iItem );
	if ( !psz )
		return;

	CFont*	pOldFont = pDC->SelectObject( &m_font );

	// The row is filled by FillRect; the text then draws over it transparently.
	// Setting the *background* colour to the text colour (as this did) with an opaque
	// bk mode made DrawText paint a solid bar in the shape of each string -- the map
	// list came out as orange blocks instead of names.
	pDC->SetBkMode( TRANSPARENT );
	if ( iItem == m_curSel )
	{
		pDC->FillRect( &row, &m_brSel );
		pDC->SetTextColor( m_clrSelText );
	}
	else
	{
		pDC->FillRect( &row, &m_brBg );
		pDC->SetTextColor( m_clrText );
	}

	row.left += 2;
	pDC->DrawText( psz, -1, &row, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );

	pDC->SelectObject( pOldFont );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnLButtonDown (0x449EB0)

void CODListBox::OnLButtonDown( UINT /*nFlags*/, CPoint pt )
{
	RECT	rc;
	::GetClientRect( GetSafeHwnd(), &rc );
	InflateRect( &rc, -3, -3 );

	SetFocus();

	if ( pt.x < 0 )
		return;

	int	right = rc.right - rc.left;
	if ( m_bHasScrollbar )
		right -= 16;
	if ( pt.x > right )
		return;

	if ( pt.y < 0 || pt.y > rc.bottom - rc.top )
		return;
	if ( !m_rowHeight )
		return;

	int	item = m_topRow + pt.y / m_rowHeight;
	if ( item < m_nCount )
	{
		SetCurSel( item );
		InvalidateRect( NULL, TRUE );
		UpdateWindow();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::AddStringPtr (0x449F60)

void CODListBox::AddStringPtr( void* pData )
{
	if ( !m_pItems || m_nCount >= ODLB_MAX_ITEMS )
		return;

	odlbitem_t*	it = &m_pItems[m_nCount];
	it->pszText = (char*)pData;
	it->bPtr	= 1;

	m_nCount++;
	SetCurSel( m_nCount - 1 );
	if ( m_pScrollbar )
	{
		m_pScrollbar->SetRange( 0, m_nCount );
		m_pScrollbar->SetPos( m_curSel );
	}
	UpdateScrollbar();
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::AddString (0x449FD0)

void CODListBox::AddString( const char* psz )
{
	if ( !m_pItems || m_nCount >= ODLB_MAX_ITEMS )
		return;

	odlbitem_t*	it = &m_pItems[m_nCount];
	strncpy( it->szInline, psz, 0x20 );
	it->szInline[31] = 0;
	it->bPtr		 = 0;

	m_nCount++;
	SetCurSel( m_nCount - 1 );
	if ( m_pScrollbar )
	{
		m_pScrollbar->SetRange( 0, m_nCount );
		m_pScrollbar->SetPos( m_curSel );
	}
	UpdateScrollbar();
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::ResetContent (0x44A060)

void CODListBox::ResetContent()
{
	m_nCount = 0;
	if ( m_pItems )
		memset( m_pItems, 0, ODLB_MAX_ITEMS * sizeof( odlbitem_t ) );
	SetCurSel( -1 );
	m_pScrollbar->SetRange( 0, 100 );
	m_pScrollbar->SetPos( 0 );
	UpdateScrollbar();
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnVScroll (0x44A0B0)
//
// the gutter scrollbar repaints the whole list.

void CODListBox::OnVScroll( UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* /*pScrollBar*/ )
{
	::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( GetSafeHwnd() );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::SetCurSel (0x44A0D0)

void CODListBox::SetCurSel( int i )
{
	if ( i == -1 )
	{
		if ( g_odlbAnchor != -1 && g_odlbAnchor < m_nCount && g_odlbAnchor >= 0 )
			InvalidateRect( NULL, TRUE );
		m_curSel	 = -1;
		g_odlbAnchor = -1;
		return;
	}

	if ( i >= 0 && i < m_nCount )
	{
		g_odlbAnchor = m_curSel;
		m_curSel	 = i;
		InvalidateRect( NULL, TRUE );

		UINT	id = GetDlgCtrlID();
		CWnd*	pGrand = GetParent() ? GetParent()->GetParent() : NULL;
		if ( pGrand )
			::SendMessageA( pGrand->GetSafeHwnd(), WM_COMMAND,
				MAKEWPARAM( id, 1 ), (LPARAM)m_hWnd );
		InvalidateRect( NULL, TRUE );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::GetVisibleRows (0x44A1F0)

int CODListBox::GetVisibleRows()
{
	RECT	rc;
	::GetClientRect( GetSafeHwnd(), &rc );
	InflateRect( &rc, -3, -3 );
	if ( m_rowHeight )
		return (int)( (double)( rc.bottom - rc.top ) / (double)m_rowHeight + 0.5 );
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::UpdateScrollbar (0x44A250)

void CODListBox::UpdateScrollbar()
{
	int	visible = GetVisibleRows();

	if ( m_nCount > visible )
	{
		if ( m_bHasScrollbar )
			goto reshow;			// already up; just make sure it is visible
		m_bHasScrollbar = 1;
		if ( m_pScrollbar )
			m_pScrollbar->ShowWindow( SW_RESTORE );
	}
	else
	{
		if ( !m_bHasScrollbar )
			return;
		m_bHasScrollbar = 0;
		if ( m_pScrollbar )
			m_pScrollbar->ShowWindow( SW_HIDE );
	}
	InvalidateRect( NULL, TRUE );

reshow:
	if ( m_bHasScrollbar && m_pScrollbar )
		m_pScrollbar->ShowWindow( SW_RESTORE );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::SetRowHeight (0x44A2F0)

void CODListBox::SetRowHeight( int h )
{
	m_rowHeight = h;
	if ( m_pScrollbar )
		m_pScrollbar->SetRowHeight( h );	// sub_44F8D0: repage from the client height
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::GetText (0x44A320)

const char* CODListBox::GetText( int i )
{
	if ( i < 0 || i >= m_nCount )
		return NULL;
	odlbitem_t*	it = &m_pItems[i];
	return it->bPtr ? (const char*)it->pszText : it->szInline;
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::FindString (0x44A380)

int CODListBox::FindString( const char* psz )
{
	for ( int i = 0; i < m_nCount; i++ )
	{
		if ( !m_pItems[i].bPtr && !_strcmpi( psz, m_pItems[i].szInline ) )
			return i;
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::ClampTopIndex (0x44A3E0)

int CODListBox::ClampTopIndex()
{
	int	visible = GetVisibleRows();
	if ( m_nCount && m_pScrollbar )
	{
		int	maxTop = m_nCount - visible;
		if ( maxTop < 0 )
			maxTop = 0;
		int	pos = m_pScrollbar->GetPos();
		m_topRow = ( pos <= maxTop ) ? pos : maxTop;
	}
	return visible;
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnLButtonUp (0x44A420)

void CODListBox::OnLButtonUp( UINT /*nFlags*/, CPoint /*pt*/ )
{
	SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnKillFocus (0x44A430)

void CODListBox::OnKillFocus( CWnd* pNewWnd )
{
	CWnd::OnKillFocus( pNewWnd );		// sub_49618F: CWnd::Default

	if ( m_pOwnerCombo )
		m_pOwnerCombo->Collapse();		// sub_445CF0 close-up

	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnSetFocus (0x44A460)

void CODListBox::OnSetFocus( CWnd* pOldWnd )
{
	if ( m_pOwnerCombo )
		m_pOwnerCombo->SetFocus();
	else
		CWnd::OnSetFocus( pOldWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnSize (0x44A490)

void CODListBox::OnSize( UINT /*nType*/, int cx, int cy )
{
	if ( m_pScrollbar && m_pScrollbar->GetSafeHwnd() )
		m_pScrollbar->MoveWindow( cx - 19, 3, 16, cy - 6, TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnGetDlgCode (0x44A4D0)

UINT CODListBox::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnKeyDown (0x44A4E0)

void CODListBox::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	switch ( nChar )
	{
	case VK_PRIOR:	NavPageUp();	break;
	case VK_NEXT:	NavPageDown();	break;
	case VK_UP:		NavLineUp();	break;
	case VK_DOWN:	NavLineDown();	break;
	case VK_TAB:
		{
			BOOL	bShift = ( ::GetAsyncKeyState( VK_SHIFT ) & 0x8000 ) != 0;
			CWnd*	pParent = GetParent();
			if ( pParent )
			{
				CWnd*	pNext = pParent->GetNextDlgTabItem( this, bShift );
				if ( pNext )
					pNext->SetFocus();
			}
		}
		break;
	default:
		break;
	}

	CWnd::OnKeyDown( nChar, nRepCnt, nFlags );	// sub_49618F: CWnd::Default
}

#ifdef LAUNCHER_FIXES
/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnMouseWheel (LAUNCHER_FIXES)
//
// The companion bar owns the scroll offset, so the wheel drives that.

BOOL CODListBox::OnMouseWheel( UINT /*nFlags*/, short zDelta, CPoint /*pt*/ )
{
	int	steps;

	if ( !m_bHasScrollbar || !m_pScrollbar || !zDelta )
		return FALSE;

	steps = ( abs( zDelta ) / WHEEL_DELTA ) * Dlg_WheelScrollLines();
	if ( steps < 1 )
		steps = 1;

	while ( steps-- > 0 )
	{
		if ( zDelta > 0 )
			m_pScrollbar->LineUp();
		else
			m_pScrollbar->LineDown();
	}
	return TRUE;
}

#endif	// LAUNCHER_FIXES

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnCreate (0x443FA0)
//
// ICF-folded with CODComboBox::OnCreate.

int CODListBox::OnCreate( LPCREATESTRUCT lpcs )
{
	return CWnd::OnCreate( lpcs );
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnNcDestroy (0x450F20)
//
// ICF-folded with CODTabCtrl::OnNcDestroy.

void CODListBox::OnNcDestroy()
{
	CWnd::OnNcDestroy();
	if ( m_bAutoDelete )
		delete this;
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnEraseBkgnd (0x4515E0)
//
// ICF-folded with CODTabCtrl::OnEraseBkgnd.

BOOL CODListBox::OnEraseBkgnd( CDC* /*pDC*/ )
{
	CWnd*	pParent = GetParent();
	if ( pParent )
	{
		RECT	rc;
		::GetWindowRect( GetSafeHwnd(), &rc );
		::MapWindowPoints( NULL, pParent->GetSafeHwnd(), (LPPOINT)&rc, 2 );
		::ValidateRect( pParent->GetSafeHwnd(), &rc );
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODListBox::OnMouseMove (0x455E00)

void CODListBox::OnMouseMove( UINT nFlags, CPoint pt )
{
	CWnd::OnMouseMove( nFlags, pt );
}
