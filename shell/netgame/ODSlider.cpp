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
// Purpose: CODSlider, the owner-draw horizontal slider used by the audio,
//          game-options and video-options pages.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

BEGIN_MESSAGE_MAP( CODSlider, CWnd )
	//{{AFX_MSG_MAP(CODSlider)
	ON_WM_NCDESTROY()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_WM_GETDLGCODE()
	ON_WM_KEYDOWN()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODSlider::CODSlider (0x44F9F0)

CODSlider::CODSlider()
{
	InitMembers();
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::~CODSlider (0x44FA60)

CODSlider::~CODSlider()
{
	if ( m_hThumbDib )
		GlobalFree( m_hThumbDib );
	m_hThumbDib = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::Create (0x44FAA0)

BOOL CODSlider::Create( CWnd* pParent, RECT* prc )
{
	WNDCLASSA	wc;

	m_pOwner = pParent;

	memset( &wc, 0, sizeof( wc ) );
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc   = AfxGetAfxWndProc();
	wc.hInstance     = AfxGetInstanceHandle();
	wc.hbrBackground = (HBRUSH)::GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "CODSliderCls";
	wc.hCursor       = ::LoadCursorA( NULL, IDC_ARROW );
	if ( !AfxRegisterClass( &wc ) )
	{
		Launcher_ShowMessageById( 0, IDS_ODSLIDER_REGFAIL );
		return FALSE;
	}

	return CreateEx( WS_EX_NOPARENTNOTIFY, "CODSliderCls", "",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP, prc->left, prc->top,
		prc->right - prc->left, prc->bottom - prc->top,
		pParent ? pParent->GetSafeHwnd() : NULL, NULL ) != 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::InitMembers (0x44FB80)

void CODSlider::InitMembers()
{
	char				path[260];
	LPBITMAPINFOHEADER	pDib;

	sprintf( path, "%s%s.bmp", "gfx/shell/", "slider" );
	m_hThumbDib = DIB_LoadBitmapFile( path );
	m_cyThumb = 0;
	m_cxThumb = 0;
	if ( m_hThumbDib )
	{
		pDib = (LPBITMAPINFOHEADER)GlobalLock( m_hThumbDib );
		if ( pDib )
		{
			m_cxThumb = DIB_Width( pDib );
			m_cyThumb = DIB_Height( pDib );
			GlobalUnlock( m_hThumbDib );
		}
	}

	m_bHover    = 0;
	m_pOwner    = NULL;
	m_nPos      = 50;
	m_nMin      = 0;
	m_nMax      = 100;
	m_bDragging = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::SetRange (0x44FC10)

void CODSlider::SetRange( int nMin, int nMax )
{
	m_nMin = nMin;
	m_nMax = nMax;
	if ( nMax <= nMin )
		m_nMax = nMin + 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::SetPos (0x44FC30)

void CODSlider::SetPos( int nPos )
{
	if ( nPos >= m_nMin && nPos <= m_nMax )
	{
		m_nPos = nPos;
		::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::OnPaint (0x44FC60)

void CODSlider::OnPaint()
{
	CPaintDC	dc( this );
	CDC			mem;
	CBitmap		bmp;
	CBitmap*	pOld;
	RECT		rc, rcDst, rcSrc;

	::GetClientRect( GetSafeHwnd(), &rc );

	if ( !mem.CreateCompatibleDC( &dc ) )
		return;

	bmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
		rc.right - rc.left, rc.bottom - rc.top ) );
	pOld = mem.SelectObject( &bmp );

	// Show the parent menu artwork behind the track.
	::GetClientRect( GetSafeHwnd(), &rcDst );
	::GetWindowRect( GetSafeHwnd(), &rcSrc );
	if ( CWnd::FromHandle( ::GetParent( GetSafeHwnd() ) ) )
		CWnd::FromHandle( ::GetParent( GetSafeHwnd() ) )->ScreenToClient( &rcSrc );
	Launcher_BlitBackground( &mem, &rcDst, &rcSrc );

	DrawTrack( &mem );
	DrawThumb( &mem, m_nPos, m_nMin, m_nMax );

	dc.BitBlt( 0, 0, rc.right - rc.left, rc.bottom - rc.top, &mem, 0, 0, SRCCOPY );
	mem.SelectObject( pOld );
	mem.DeleteDC();
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::DrawTrack (0x44FE80)

void CODSlider::DrawTrack( CDC* pDC )
{
	RECT	rc, rail, inset;
	int		mid;

	::GetClientRect( GetSafeHwnd(), &rc );

	CBrush	brRail( CWnd::FromHandle( ::GetFocus() ) == this
					? RGB( 128, 128, 128 )
					: RGB( 69, 69, 69 ) );
	CBrush	brInset( RGB( 0, 0, 0 ) );

	mid = ( rc.bottom - rc.top ) / 2;
	rail.left   = rc.left;
	rail.top    = mid - 4;
	rail.right  = rc.right;
	rail.bottom = mid + 4;
	inset = rail;
	::InflateRect( &inset, -3, -3 );

	::FillRect( pDC->GetSafeHdc(), &rail, brRail );
	::FillRect( pDC->GetSafeHdc(), &inset, brInset );
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::DrawThumb (0x44FFE0)

void CODSlider::DrawThumb( CDC* pDC, int /*nPos*/, int /*nMin*/, int /*nMax*/ )
{
	RECT	rc, dst, src;
	double	range;
	int		travel, x;

	::GetClientRect( GetSafeHwnd(), &rc );

	travel = rc.right - m_cxThumb - rc.left;
	if ( rc.right - rc.left < m_cxThumb )
		return;
	range = (double)( m_nMax - m_nMin );
	if ( range == 0.0 )
		return;

	x = (int)( (double)( m_nPos - m_nMin ) / range * (double)travel + 0.5 );

	dst.left   = x;
	dst.top    = ( rc.bottom - m_cyThumb - rc.top ) / 2;
	dst.right  = x + m_cxThumb;
	dst.bottom = ( rc.bottom + m_cyThumb - rc.top ) / 2;

	if ( m_hThumbDib )
	{
		src.left   = 0;
		src.top    = 0;
		src.right  = dst.right - dst.left;
		src.bottom = dst.bottom - dst.top;
		DIB_BlitDib( pDC->GetSafeHdc(), &dst, m_hThumbDib, &src );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::OnLButtonDown (0x4500F0)

void CODSlider::OnLButtonDown( UINT /*nFlags*/, CPoint pt )
{
	RECT	rc;
	POINT	p;
	double	range;
	int		travel, left, oldPos;

	if ( m_bDragging )
		return;

	p = pt;
	if ( HitTestThumb( &p ) )
	{
		m_nDragOffX = pt.x;
		m_bDragging = 1;
		m_nDragOffY = pt.y;
		SetCapture();
		return;
	}

	::GetClientRect( GetSafeHwnd(), &rc );
	if ( pt.x < rc.left - 20 || pt.x > rc.right + 20
	  || pt.y < rc.top - 20  || pt.y > rc.bottom + 20 )
		return;

	if ( rc.right - rc.left < m_cxThumb )
		return;
	range = (double)( m_nMax - m_nMin );
	if ( range == 0.0 )
		return;

	travel = rc.right - m_cxThumb - rc.left;
	left   = pt.x - m_cxThumb / 2;
	if ( left <= 0 )
		left = 0;
	if ( left >= travel )
		left = travel;

	oldPos = m_nPos;
	m_nPos = m_nMin + (int)( (double)left / (double)travel * range + 0.5 );
	if ( m_nPos != oldPos )
	{
		::RedrawWindow( GetSafeHwnd(), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
		::SendMessageA( m_pOwner->GetSafeHwnd(), g_uiScrollMsg,
			MAKEWPARAM( SB_ENDSCROLL, (WORD)m_nPos ), (LPARAM)this );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::OnLButtonUp (0x450260)

void CODSlider::OnLButtonUp( UINT /*nFlags*/, CPoint /*pt*/ )
{
	if ( !m_bDragging )
		return;

	m_bDragging = 0;
	::ReleaseCapture();

	// The drop notification carries the final position; the owner commits it.
	::SendMessageA( m_pOwner->GetSafeHwnd(), g_uiScrollMsg,
		MAKEWPARAM( SB_ENDSCROLL, (WORD)m_nPos ), (LPARAM)this );
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::OnMouseMove (0x4502A0)

void CODSlider::OnMouseMove( UINT /*nFlags*/, CPoint pt )
{
	RECT	rc;
	double	range;
	int		travel, left, oldPos;

	if ( !m_bDragging )
		return;

	::GetClientRect( GetSafeHwnd(), &rc );
	if ( pt.x < rc.left - 20 || pt.x > rc.right + 20
	  || pt.y < rc.top - 20  || pt.y > rc.bottom + 20 )
		return;

	if ( rc.right - rc.left < m_cxThumb )
		return;
	range = (double)( m_nMax - m_nMin );
	if ( range == 0.0 )
		return;

	travel = rc.right - m_cxThumb - rc.left;
	left   = pt.x - m_cxThumb / 2;
	if ( left <= 0 )
		left = 0;
	if ( left >= travel )
		left = travel;

	oldPos = m_nPos;
	m_nPos = m_nMin + (int)( (double)left / (double)travel * range + 0.5 );
	if ( m_nPos != oldPos )
	{
		::RedrawWindow( GetSafeHwnd(), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
		::SendMessageA( m_pOwner->GetSafeHwnd(), g_uiScrollMsg,
			MAKEWPARAM( SB_THUMBTRACK, (WORD)m_nPos ), (LPARAM)this );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::HitTestThumb (0x4503D0)

int CODSlider::HitTestThumb( POINT* pt )
{
	RECT	rc;

	GetThumbRect( &rc );
	return ::PtInRect( &rc, *pt );
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::GetThumbRect (0x450400)

void CODSlider::GetThumbRect( RECT* prc )
{
	RECT	rc;
	double	range;
	int		x;

	::GetClientRect( GetSafeHwnd(), &rc );

	range = (double)( m_nMax - m_nMin );
	if ( range == 0.0 )
		range = 1.0;

	x = (int)( (double)( m_nPos - m_nMin ) / range
			 * (double)( rc.right - m_cxThumb - rc.left ) + 0.5 );

	prc->top    = ( rc.bottom - rc.top - m_cyThumb ) / 2;
	prc->left   = x;
	prc->bottom = ( rc.bottom - rc.top + m_cyThumb ) / 2;
	prc->right  = x + m_cxThumb;
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::GetPos (0x4504C0)

int CODSlider::GetPos() const
{
	return m_nPos;
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::OnKeyDown (0x4504D0)

void CODSlider::OnKeyDown( UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/ )
{
	HWND	hNext;
	int		oldPos = m_nPos;
	int		pos;

	switch ( nChar )
	{
	case VK_TAB:
		// (sic) both arms ask for the next item -- shift-tab does not go back
		if ( ::GetAsyncKeyState( VK_SHIFT ) & 0x8000 )
			hNext = GetParent()
				? ::GetNextDlgTabItem( GetParent()->GetSafeHwnd(), GetSafeHwnd(), FALSE )
				: NULL;
		else
			hNext = GetParent()
				? ::GetNextDlgTabItem( GetParent()->GetSafeHwnd(), GetSafeHwnd(), FALSE )
				: NULL;

		if ( hNext )
			::SetFocus( hNext );
		return;

	case VK_PRIOR:
	case VK_LEFT:
	case VK_DOWN:
		pos = oldPos - 1;
		if ( pos <= m_nMin )
			pos = m_nMin;
		m_nPos = pos;
		break;

	case VK_NEXT:
	case VK_UP:
	case VK_RIGHT:
		pos = oldPos + 1;
		if ( pos >= m_nMax )
			pos = m_nMax;
		m_nPos = pos;
		break;

	default:
		Default();
		return;
	}

	if ( m_nPos != oldPos )
	{
		::RedrawWindow( GetSafeHwnd(), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
		::SendMessageA( m_pOwner->GetSafeHwnd(), g_uiScrollMsg,
			MAKEWPARAM( SB_THUMBTRACK, (WORD)m_nPos ), (LPARAM)this );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::OnKillFocus (0x450640)

void CODSlider::OnKillFocus( CWnd* /*pNewWnd*/ )
{
	Default();
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::OnSetFocus (0x450640)

void CODSlider::OnSetFocus( CWnd* /*pOldWnd*/ )
{
	Default();
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::OnGetDlgCode (0x44A4D0)

UINT CODSlider::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::OnNcDestroy (0x44AD30)

void CODSlider::OnNcDestroy()
{
	CWnd::OnNcDestroy();
	delete this;
}

/////////////////////////////////////////////////////////////////////////////
// CODSlider::OnEraseBkgnd (0x44BDC0)
//
// OnPaint composites the whole client.

BOOL CODSlider::OnEraseBkgnd( CDC* /*pDC*/ )
{
	return TRUE;
}
