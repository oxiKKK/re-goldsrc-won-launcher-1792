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
// Purpose: CODScrollBar, the owner-draw vertical scrollbar.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Owner notifications all go to the owning list's window as WM_VSCROLL; the
// owner reads the scroll code (and, while thumb-tracking, the new position).
#define SBCODE_OWNER_DOWN	0
#define SBCODE_OWNER_UP		1

BEGIN_MESSAGE_MAP( CODScrollBar, CWnd )
	//{{AFX_MSG_MAP(CODScrollBar)
	ON_WM_NCDESTROY()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_SETFOCUS()
#ifdef LAUNCHER_FIXES
	ON_WM_MOUSEWHEEL()
#endif
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::CODScrollBar (0x44DB60)

CODScrollBar::CODScrollBar()
{
	InitMembers();
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::~CODScrollBar (0x44DBD0)

CODScrollBar::~CODScrollBar()
{
	if ( m_hDnArrowD )
		GlobalFree( m_hDnArrowD );
	if ( m_hUpArrowD )
		GlobalFree( m_hUpArrowD );
	if ( m_hDnArrowF )
		GlobalFree( m_hDnArrowF );
	if ( m_hUpArrowF )
		GlobalFree( m_hUpArrowF );
	if ( m_hUpArrowP )
		GlobalFree( m_hUpArrowP );
	if ( m_hDnArrowP )
		GlobalFree( m_hDnArrowP );
	if ( m_hThumb )
		GlobalFree( m_hThumb );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::Create (0x44DC40)

BOOL CODScrollBar::Create( CWnd* pParent, RECT* prc, int nRowHeight )
{
	WNDCLASSA	wc;

	m_nRowHeight = nRowHeight;
	m_nPageRows  = (int)( (double)( prc->bottom - prc->top )
						  / (double)nRowHeight + 0.5 );
	m_pOwner     = pParent;

	memset( &wc, 0, sizeof( wc ) );
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc   = AfxGetAfxWndProc();
	wc.hInstance     = AfxGetInstanceHandle();
	wc.hbrBackground = (HBRUSH)::GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "CODScrollBarCls";
	wc.hCursor       = ::LoadCursorA( NULL, IDC_ARROW );
	if ( !AfxRegisterClass( &wc ) )
	{
		Launcher_ShowMessageById( 0, IDS_ODSCROLL_REGFAIL );
		return FALSE;
	}

	return CreateEx( WS_EX_NOPARENTNOTIFY, "CODScrollBarCls", "",
		WS_CHILD | WS_VISIBLE, prc->left, prc->top,
		prc->right - prc->left, prc->bottom - prc->top,
		pParent ? pParent->GetSafeHwnd() : NULL, NULL ) != 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::InitMembers (0x44DD50)

void CODScrollBar::InitMembers()
{
	char	path[260];

	sprintf( path, "%s%s.bmp", "gfx/shell/", "dnarrowd" );
	m_hDnArrowD = DIB_LoadBitmapFile( path );
	sprintf( path, "%s%s.bmp", "gfx/shell/", "uparrowd" );
	m_hUpArrowD = DIB_LoadBitmapFile( path );
	sprintf( path, "%s%s.bmp", "gfx/shell/", "dnarrowf" );
	m_hDnArrowF = DIB_LoadBitmapFile( path );
	sprintf( path, "%s%s.bmp", "gfx/shell/", "uparrowf" );
	m_hUpArrowF = DIB_LoadBitmapFile( path );
	sprintf( path, "%s%s.bmp", "gfx/shell/", "uparrowp" );
	m_hUpArrowP = DIB_LoadBitmapFile( path );
	sprintf( path, "%s%s.bmp", "gfx/shell/", "dnarrowp" );
	m_hDnArrowP = DIB_LoadBitmapFile( path );
	sprintf( path, "%s%s.bmp", "gfx/shell/", "thumb" );
	m_hThumb = DIB_LoadBitmapFile( path );

	m_nPageRows         = 1;	// recomputed by Create from the client height
	m_bEnabled          = 1;
	m_nPos              = 50;
	m_pOwner            = NULL;
	m_nMin              = 0;
	m_nMax              = 100;
	m_bDragging         = 0;
	m_bActive           = 0;
	m_bArrowRepeat      = 0;
	m_nArrowDir         = 0;
	m_nPageDir          = 0;
	m_bUseParentCapture = 0;
	m_bHoverTimer       = 0;
}

#ifdef LAUNCHER_FIXES
/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::ScrollMax (LAUNCHER_FIXES)

int CODScrollBar::ScrollMax()
{
	int	max = m_nMax;

	if ( m_nPageRows > 0 )
	{
		max -= m_nPageRows;
		if ( max < m_nMin )
			max = m_nMin;
	}
	return max;
}

#endif	// LAUNCHER_FIXES

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::SetRange (0x44DED0)

void CODScrollBar::SetRange( int nMin, int nMax )
{
	int	pos;

	m_nMin = nMin;
	m_nMax = nMax;
	if ( nMax <= nMin )
		m_nMax = nMin + 1;

	pos = GetPos();
	if ( pos < m_nMin )
		SetPos( m_nMin );
	else if ( pos > m_nMax )
		SetPos( m_nMax );

#ifdef LAUNCHER_FIXES
	// A shorter list can leave the thumb parked past the new end.
	if ( GetPos() > ScrollMax() )
		SetPos( ScrollMax() );
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::SetPos (0x44DF20)

void CODScrollBar::SetPos( int nPos )
{
	// (sic) the second clamp reads nPos again, not the value just stored
	m_nPos = ( m_nMin <= nPos ) ? nPos : m_nMin;
	m_nPos = ( m_nMax >= nPos ) ? nPos : m_nMax;

#ifdef LAUNCHER_FIXES
	// Clamp for real: to the last position that scrolls, and -- since the second
	// line above undoes the first for nPos < m_nMin -- back up to the minimum.
	if ( m_nPos > ScrollMax() )
		m_nPos = ScrollMax();
	if ( m_nPos < m_nMin )
		m_nPos = m_nMin;
#endif

	::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( GetSafeHwnd() );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::OnPaint (0x44DF70)

void CODScrollBar::OnPaint()
{
	CPaintDC	dc( this );
	CDC			mem;
	CBitmap		bmp;
	CBitmap*	pOld;
	RECT		rc, rcUp, rcDn, rcThumb;
	POINT		pt;
	int			bDown, active;

	::GetClientRect( GetSafeHwnd(), &rc );

	if ( !mem.CreateCompatibleDC( &dc ) )
		return;

	bmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
		rc.right - rc.left, rc.bottom - rc.top ) );
	pOld = mem.SelectObject( &bmp );

	GetDnArrowRect( &rcDn );
	GetUpArrowRect( &rcUp );

	::GetCursorPos( &pt );
	::ScreenToClient( GetSafeHwnd(), &pt );
	bDown = ::GetAsyncKeyState( VK_LBUTTON ) != 0;

	if ( bDown )
	{
		if ( ::PtInRect( &rcUp, pt ) )
		{
			DrawUpArrow( &mem, 1, 0 );
			DrawDnArrow( &mem, 0, 0 );
		}
		else if ( ::PtInRect( &rcDn, pt ) )
		{
			DrawDnArrow( &mem, 1, 0 );
			DrawUpArrow( &mem, 0, 0 );
		}
		else
		{
			DrawUpArrow( &mem, 0, 0 );
			DrawDnArrow( &mem, 0, 0 );
		}
	}
	else
	{
		if ( ::PtInRect( &rcUp, pt ) )
		{
			DrawUpArrow( &mem, 0, 1 );
			DrawDnArrow( &mem, 0, 0 );
		}
		else if ( ::PtInRect( &rcDn, pt ) )
		{
			DrawDnArrow( &mem, 0, 1 );
			DrawUpArrow( &mem, 0, 0 );
		}
		else
		{
			DrawDnArrow( &mem, 0, 0 );
			DrawUpArrow( &mem, 0, 0 );
		}
	}

	GetThumbRect( &rcThumb );
	if ( bDown && ::PtInRect( &rc, pt ) )
	{
		// a cap is held: dark track, lit thumb
		DrawTrack( &mem, m_bArrowRepeat ? 0 : 1 );
		DrawThumb( &mem, m_nPos, m_nMin, m_nMax, 1, m_bArrowRepeat == 0 );
	}
	else
	{
		active = ( m_bDragging || m_bActive ) ? 1 : 0;
		DrawTrack( &mem, active );
		if ( ::PtInRect( &rcThumb, pt ) )
			DrawThumb( &mem, m_nPos, m_nMin, m_nMax, 1, active );
		else
			DrawThumb( &mem, m_nPos, m_nMin, m_nMax, active, active );
	}

	dc.BitBlt( 0, 0, rc.right - rc.left, rc.bottom - rc.top, &mem, 0, 0, SRCCOPY );
	mem.SelectObject( pOld );
	mem.DeleteDC();
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::DrawUpArrow (0x44E340)

void CODScrollBar::DrawUpArrow( CDC* pDC, int bPressed, int bHover )
{
	HGLOBAL	hDib = m_hUpArrowD;
	RECT	rc, src;

	if ( bPressed )
		hDib = m_hUpArrowP;
	else if ( bHover )
		hDib = m_hUpArrowF;

	GetUpArrowRect( &rc );
	src.left   = 0;
	src.top    = 0;
	src.right  = rc.right - rc.left;
	src.bottom = rc.bottom - rc.top;
	DIB_BlitDib( pDC->GetSafeHdc(), &rc, hDib, &src );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::DrawDnArrow (0x44E3C0)

void CODScrollBar::DrawDnArrow( CDC* pDC, int bPressed, int bHover )
{
	HGLOBAL	hDib = m_hDnArrowD;
	RECT	rc, src;

	if ( bPressed )
		hDib = m_hDnArrowP;
	else if ( bHover )
		hDib = m_hDnArrowF;

	GetDnArrowRect( &rc );
	src.left   = 0;
	src.top    = 0;
	src.right  = rc.right - rc.left;
	src.bottom = rc.bottom - rc.top;
	DIB_BlitDib( pDC->GetSafeHdc(), &rc, hDib, &src );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::GetTrackRect (0x44E440)

void CODScrollBar::GetTrackRect( RECT* prc )
{
	RECT	rc;
	int		width;

	::GetClientRect( GetSafeHwnd(), &rc );
	width = rc.right - rc.left;
	prc->left   = rc.left;
	prc->top    = rc.top + width;
	prc->right  = rc.right;
	prc->bottom = rc.bottom - width;
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::DrawTrack (0x44E490)

void CODScrollBar::DrawTrack( CDC* pDC, int bActive )
{
	CBrush	brush( bActive ? RGB( 0, 0, 0 ) : RGB( 80, 80, 80 ) );
	RECT	rc;

	GetTrackRect( &rc );
	::FillRect( pDC->GetSafeHdc(), &rc, brush );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::DrawThumb (0x44E540)

void CODScrollBar::DrawThumb( CDC* pDC, int /*nPos*/, int /*nMin*/, int /*nMax*/,
							  int bHover, int bActive )
{
	COLORREF	clrOutline = RGB( 80, 80, 80 );
	COLORREF	clrInset   = RGB( 255, 180, 24 );
	RECT		rc, rcThumb, rcInset;
	CPen*		pOldPen;

	if ( bHover )
	{
		if ( bActive )
			clrOutline = RGB( 0, 0, 0 );
	}
	else if ( !bActive )
	{
		clrInset = RGB( 0, 0, 0 );
	}

	::GetClientRect( GetSafeHwnd(), &rc );		// (sic) fetched but unused
	GetThumbRect( &rcThumb );
	rcInset = rcThumb;
	::InflateRect( &rcInset, -4, -4 );

	CPen	pen( PS_SOLID, 1, clrOutline );
	pOldPen = pDC->SelectObject( &pen );
	CBrush	brOutline( clrOutline );
	CBrush	brInset( clrInset );
	::FillRect( pDC->GetSafeHdc(), &rcThumb, brOutline );
	::FillRect( pDC->GetSafeHdc(), &rcInset, brInset );
	pDC->SelectObject( pOldPen );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::OnLButtonDown (0x44E6F0)

void CODScrollBar::OnLButtonDown( UINT /*nFlags*/, CPoint pt )
{
	RECT		rc, rcClient, rcThumb, rcUp, rcDn;
	POINT		p = pt;
	CBitmap*	pOld;

	::GetClientRect( GetSafeHwnd(), &rc );
	if ( !::PtInRect( &rc, p ) )
		return;
	// ignore a fresh press while a drag or a repeat is already running
	if ( m_bDragging || m_bArrowRepeat || m_bActive )
		return;

	if ( HitTestThumb( &p ) )
	{
		m_bDragging = 1;
		GetThumbRect( &rcThumb );
		m_ptDragStartX  = p.x;
		m_ptDragStartY  = p.y;
		m_nDragStartPos = m_nPos;
		SetScrollCapture();
		::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( GetSafeHwnd() );
	}
	else if ( HitTestDnArrow( &p ) )
	{
		LineDown();
		m_bArrowRepeat = 1;
		m_nArrowDir    = -1;
		::SetTimer( GetSafeHwnd(), 1, 50, NULL );
		SetScrollCapture();

		// paint the down cap pressed straight away
		CClientDC	cdc( this );
		CDC			mem;
		if ( mem.CreateCompatibleDC( &cdc ) )
		{
			CBitmap	bmp;

			::GetClientRect( GetSafeHwnd(), &rcClient );
			GetDnArrowRect( &rcDn );
			bmp.Attach( ::CreateCompatibleBitmap( cdc.GetSafeHdc(),
				rcClient.right - rcClient.left, rcClient.bottom - rcClient.top ) );
			pOld = mem.SelectObject( &bmp );
			DrawDnArrow( &mem, 1, 0 );
			cdc.BitBlt( rcDn.left, rcDn.top, rcDn.right - rcDn.left,
				rcDn.bottom - rcDn.top, &mem, rcDn.left, rcDn.top, SRCCOPY );
			mem.SelectObject( pOld );
			mem.DeleteDC();
		}
	}
	else if ( HitTestUpArrow( &p ) )
	{
		LineUp();
		m_bArrowRepeat = 1;
		m_nArrowDir    = 1;
		::SetTimer( GetSafeHwnd(), 1, 50, NULL );
		SetScrollCapture();

		CClientDC	cdc( this );
		CDC			mem;
		if ( mem.CreateCompatibleDC( &cdc ) )
		{
			CBitmap	bmp;

			::GetClientRect( GetSafeHwnd(), &rcClient );
			GetUpArrowRect( &rcUp );
			bmp.Attach( ::CreateCompatibleBitmap( cdc.GetSafeHdc(),
				rcClient.right - rcClient.left, rcClient.bottom - rcClient.top ) );
			pOld = mem.SelectObject( &bmp );
			DrawUpArrow( &mem, 1, 0 );
			cdc.BitBlt( rcUp.left, rcUp.top, rcUp.right - rcUp.left,
				rcUp.bottom - rcUp.top, &mem, rcUp.left, rcUp.top, SRCCOPY );
			mem.SelectObject( pOld );
			mem.DeleteDC();
		}
	}
	else
	{
		if ( HitTestGutter( &p, &m_nPageDir ) )
		{
			::SetTimer( GetSafeHwnd(), 3, 100, NULL );
			Page( m_nPageDir );
		}
		m_bActive = 1;
		SetScrollCapture();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::Page (0x44EBA0)

void CODScrollBar::Page( BOOL bUp )
{
	if ( bUp )
	{
		m_nPos -= m_nPageRows;
		if ( m_nPos < m_nMin )
			m_nPos = m_nMin;
		::SendMessageA( m_pOwner->GetSafeHwnd(), WM_VSCROLL,
			MAKEWPARAM( SBCODE_OWNER_UP, 0 ), 0 );
	}
	else
	{
		m_nPos += m_nPageRows;
#ifdef LAUNCHER_FIXES
		if ( m_nPos > ScrollMax() )
			m_nPos = ScrollMax();
#else
		if ( m_nPos > m_nMax )
			m_nPos = m_nMax;
#endif
		::SendMessageA( m_pOwner->GetSafeHwnd(), WM_VSCROLL,
			MAKEWPARAM( SBCODE_OWNER_DOWN, 0 ), 0 );
	}
	::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( GetSafeHwnd() );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::OnLButtonUp (0x44EC50)

void CODScrollBar::OnLButtonUp( UINT /*nFlags*/, CPoint /*pt*/ )
{
	if ( HasScrollCapture() )
	{
		ReleaseScrollCapture();
		::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( GetSafeHwnd() );
	}

	if ( m_bDragging )
	{
		m_bDragging = 0;
	}
	else if ( m_bArrowRepeat || m_bActive )
	{
		if ( m_bArrowRepeat )
		{
			m_bArrowRepeat = 0;
			::KillTimer( GetSafeHwnd(), 1 );
			m_nArrowDir = 0;
		}
		else if ( m_bActive )
		{
			::KillTimer( GetSafeHwnd(), 3 );
			m_bActive = 0;
		}
	}

	::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( GetSafeHwnd() );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::OnMouseMove (0x44ED10)

void CODScrollBar::OnMouseMove( UINT /*nFlags*/, CPoint pt )
{
	RECT		rc, rcClient, rcUp, rcDn, rcThumb;
	POINT		p = pt;
	CBitmap*	pOld;
	double		range, scale;
	int			track, span, pos, oldPos;

	if ( !m_bDragging )
	{
		::GetClientRect( GetSafeHwnd(), &rc );
		if ( !::PtInRect( &rc, p ) )
			return;

		CClientDC	cdc( this );
		CDC			mem;
		if ( mem.CreateCompatibleDC( &cdc ) )
		{
			CBitmap	bmp;

			::GetClientRect( GetSafeHwnd(), &rcClient );

			if ( HitTestDnArrow( &p ) )
			{
				GetDnArrowRect( &rcDn );
				bmp.Attach( ::CreateCompatibleBitmap( cdc.GetSafeHdc(),
					rcClient.right - rcClient.left, rcClient.bottom - rcClient.top ) );
				pOld = mem.SelectObject( &bmp );
				DrawDnArrow( &mem, 0, 1 );
				cdc.BitBlt( rcDn.left, rcDn.top, rcDn.right - rcDn.left,
					rcDn.bottom - rcDn.top, &mem, rcDn.left, rcDn.top, SRCCOPY );
				mem.SelectObject( pOld );
			}
			else if ( HitTestUpArrow( &p ) )
			{
				GetUpArrowRect( &rcUp );
				bmp.Attach( ::CreateCompatibleBitmap( cdc.GetSafeHdc(),
					rcClient.right - rcClient.left, rcClient.bottom - rcClient.top ) );
				pOld = mem.SelectObject( &bmp );
				DrawUpArrow( &mem, 0, 1 );
				cdc.BitBlt( rcUp.left, rcUp.top, rcUp.right - rcUp.left,
					rcUp.bottom - rcUp.top, &mem, rcUp.left, rcUp.top, SRCCOPY );
				mem.SelectObject( pOld );
			}
			else if ( HitTestThumb( &p ) )
			{
				GetThumbRect( &rcThumb );
				bmp.Attach( ::CreateCompatibleBitmap( cdc.GetSafeHdc(),
					rcClient.right - rcClient.left, rcClient.bottom - rcClient.top ) );
				pOld = mem.SelectObject( &bmp );
				DrawTrack( &mem, ::GetAsyncKeyState( VK_LBUTTON ) != 0 );
				DrawThumb( &mem, m_nPos, m_nMin, m_nMax,
					1, ::GetAsyncKeyState( VK_LBUTTON ) != 0 );
				cdc.BitBlt( rcThumb.left, rcThumb.top, rcThumb.right - rcThumb.left,
					rcThumb.bottom - rcThumb.top, &mem, rcThumb.left, rcThumb.top, SRCCOPY );
				mem.SelectObject( pOld );
			}
			else
			{
				::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
			}

			if ( !m_bHoverTimer )
			{
				m_bHoverTimer = 1;
				::SetTimer( GetSafeHwnd(), 2, 100, NULL );
			}
			mem.DeleteDC();
		}
		return;
	}

	// dragging: map the cursor onto the thumb's travel
	::GetClientRect( GetSafeHwnd(), &rc );
	if ( p.x < rc.left - 20 || p.x > rc.right + 20
	  || p.y < rc.top - 20  || p.y > rc.bottom + 20 )
		return;

	track = ( rc.bottom - rc.top ) + 2 * ( rc.left - rc.right );
	if ( track <= 0 )
		return;

#ifdef LAUNCHER_FIXES
	// Drag over the same travel the thumb is drawn on, or the cursor and the
	// thumb come apart at the bottom of the track.
	range = (double)( ScrollMax() - m_nMin );
#else
	range = (double)( m_nMax - m_nMin );
#endif
	if ( range == 0.0 )
		return;

	GetThumbRect( &rcThumb );
	span  = track + rcThumb.top - rcThumb.bottom;
	scale = range / (double)span;

	pos = m_nDragStartPos + (int)( scale * (double)( p.y - m_ptDragStartY ) + 0.5 );
	if ( pos <= m_nMin )
		pos = m_nMin;
#ifdef LAUNCHER_FIXES
	if ( pos >= ScrollMax() )
		pos = ScrollMax();
#else
	if ( pos >= m_nMax )
		pos = m_nMax;
#endif

	oldPos = m_nPos;
	m_nPos = pos;
	if ( pos != oldPos )
	{
		::RedrawWindow( GetSafeHwnd(), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
		::SendMessageA( m_pOwner->GetSafeHwnd(), WM_VSCROLL,
			MAKEWPARAM( SB_THUMBTRACK, (WORD)m_nPos ), 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::LineUp (0x44F270)

void CODScrollBar::LineUp()
{
	if ( m_nPos > m_nMin )
	{
		m_nPos -= 1;
		::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( GetSafeHwnd() );
	}
	::SendMessageA( m_pOwner->GetSafeHwnd(), WM_VSCROLL,
		MAKEWPARAM( SBCODE_OWNER_UP, 0 ), 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::LineDown (0x44F2C0)

void CODScrollBar::LineDown()
{
#ifdef LAUNCHER_FIXES
	if ( m_nPos < ScrollMax() )
#else
	if ( m_nPos < m_nMax )
#endif
	{
		m_nPos += 1;
		::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( GetSafeHwnd() );
	}
	::SendMessageA( m_pOwner->GetSafeHwnd(), WM_VSCROLL,
		MAKEWPARAM( SBCODE_OWNER_DOWN, 0 ), 0 );
}

#ifdef LAUNCHER_FIXES
/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::OnMouseWheel (LAUNCHER_FIXES)
//
// A notch is worth the system's wheel-lines setting; LineUp/LineDown are what
// tell the owning list to repaint.

BOOL CODScrollBar::OnMouseWheel( UINT /*nFlags*/, short zDelta, CPoint /*pt*/ )
{
	int	steps;

	if ( !m_bEnabled || !m_pOwner || !zDelta )
		return FALSE;

	steps = ( abs( zDelta ) / WHEEL_DELTA ) * Dlg_WheelScrollLines();
	if ( steps < 1 )
		steps = 1;

	while ( steps-- > 0 )
	{
		if ( zDelta > 0 )
			LineUp();
		else
			LineDown();
	}
	return TRUE;
}

#endif	// LAUNCHER_FIXES

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::HitTestThumb (0x44F310)

BOOL CODScrollBar::HitTestThumb( POINT* ppt )
{
	RECT	rcThumb, rc;
	int		width;

	GetThumbRect( &rcThumb );
	::GetClientRect( GetSafeHwnd(), &rc );
	width = rc.right - rc.left;
	if ( rcThumb.bottom - rcThumb.top < width )
		::InflateRect( &rcThumb, 0, width - ( rcThumb.bottom - rcThumb.top ) );
	return ::PtInRect( &rcThumb, *ppt );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::GetThumbRect (0x44F380)

void CODScrollBar::GetThumbRect( RECT* prc )
{
	RECT	rc;
	double	width, track, range, frac, rows, denom, thumb;
	int		y;

	::GetClientRect( GetSafeHwnd(), &rc );

	width = (double)( rc.right - rc.left );
	track = (double)( rc.bottom - rc.top ) - ( width + width );
	range = (double)( m_nMax - m_nMin );
	if ( range == 0.0 )
		range = 1.0;
	frac = (double)( m_nPos - m_nMin ) / range;

	rows  = (double)m_nPageRows;
	denom = ( range <= rows ) ? rows : range;

#ifdef LAUNCHER_FIXES
	// The thumb keeps its proportional size (rows / denom) but travels over the
	// positions that actually scroll, so it lands flush at the bottom.
	if ( ScrollMax() > m_nMin )
		frac = (double)( m_nPos - m_nMin ) / (double)( ScrollMax() - m_nMin );
	else
		frac = 0.0;
	if ( frac > 1.0 )
		frac = 1.0;
#endif
	thumb = rows / denom * track;
	if ( thumb < width )
		thumb = width;

	y = (int)( ( track - thumb ) * frac );
	prc->left   = rc.left;
	prc->right  = rc.right;
	prc->top    = (LONG)width + y;
	prc->bottom = (LONG)width + y + (LONG)thumb;
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::HitTestUpArrow (0x44F480)

BOOL CODScrollBar::HitTestUpArrow( POINT* ppt )
{
	RECT	rc;

	GetUpArrowRect( &rc );
	return ::PtInRect( &rc, *ppt );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::GetUpArrowRect (0x44F4B0)

void CODScrollBar::GetUpArrowRect( RECT* prc )
{
	RECT	rc;

	::GetClientRect( GetSafeHwnd(), &rc );
	prc->right  = rc.right;
	prc->left   = rc.left;
	prc->top    = rc.top;
	prc->bottom = rc.top + ( rc.right - rc.left );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::HitTestDnArrow (0x44F4F0)

BOOL CODScrollBar::HitTestDnArrow( POINT* ppt )
{
	RECT	rc;

	GetDnArrowRect( &rc );
	return ::PtInRect( &rc, *ppt );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::GetDnArrowRect (0x44F520)

void CODScrollBar::GetDnArrowRect( RECT* prc )
{
	RECT	rc;

	::GetClientRect( GetSafeHwnd(), &rc );
	prc->bottom = rc.bottom;
	prc->right  = rc.right;
	prc->left   = rc.left;
	prc->top    = rc.bottom - ( rc.right - rc.left );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::OnTimer (0x44F560)

void CODScrollBar::OnTimer( UINT_PTR nIDEvent )
{
	RECT		rcClient, rcUp, rcDn, rc;
	POINT		pt;
	CClientDC	cdc( this );
	CDC			mem;
	CBitmap		bmp;
	CBitmap*	pOld;

	switch ( nIDEvent )
	{
	case 1:
		if ( m_nArrowDir == 1 )
			LineUp();
		else if ( m_nArrowDir == -1 )
			LineDown();

		// Repaint the held cap pressed.
		if ( mem.CreateCompatibleDC( &cdc ) )
		{
			::GetClientRect( GetSafeHwnd(), &rcClient );
			if ( m_nArrowDir == -1 )
			{
				GetDnArrowRect( &rcDn );
				bmp.Attach( ::CreateCompatibleBitmap( cdc.GetSafeHdc(),
					rcClient.right - rcClient.left, rcClient.bottom - rcClient.top ) );
				pOld = mem.SelectObject( &bmp );
				DrawDnArrow( &mem, 1, 0 );
				cdc.BitBlt( rcDn.left, rcDn.top, rcDn.right - rcDn.left,
					rcDn.bottom - rcDn.top, &mem, rcDn.left, rcDn.top, SRCCOPY );
				mem.SelectObject( pOld );
			}
			else if ( m_nArrowDir == 1 )
			{
				GetUpArrowRect( &rcUp );
				bmp.Attach( ::CreateCompatibleBitmap( cdc.GetSafeHdc(),
					rcClient.right - rcClient.left, rcClient.bottom - rcClient.top ) );
				pOld = mem.SelectObject( &bmp );
				DrawUpArrow( &mem, 1, 0 );
				cdc.BitBlt( rcUp.left, rcUp.top, rcUp.right - rcUp.left,
					rcUp.bottom - rcUp.top, &mem, rcUp.left, rcUp.top, SRCCOPY );
				mem.SelectObject( pOld );
			}
			mem.DeleteDC();
		}
		break;

	case 2:
		::GetCursorPos( &pt );
		::ScreenToClient( GetSafeHwnd(), &pt );
		::GetClientRect( GetSafeHwnd(), &rc );
		if ( !::PtInRect( &rc, pt ) )
		{
			m_bHoverTimer = 0;
			::KillTimer( GetSafeHwnd(), 2 );
			::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
			::UpdateWindow( GetSafeHwnd() );
		}
		break;

	case 3:
		Page( m_nPageDir );
		break;
	}

	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::HitTestGutter (0x44F840)

BOOL CODScrollBar::HitTestGutter( POINT* ppt, int* pbPageUp )
{
	RECT	rc, rcThumb;

	::GetClientRect( GetSafeHwnd(), &rc );		// (sic) fetched but unused
	if ( HitTestThumb( ppt ) )
		return FALSE;
	if ( HitTestDnArrow( ppt ) )
		return FALSE;
	if ( HitTestUpArrow( ppt ) )
		return FALSE;

	GetThumbRect( &rcThumb );
	*pbPageUp = ( ppt->y < rcThumb.top );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::SetRowHeight (0x44F8D0)

void CODScrollBar::SetRowHeight( int nRowHeight )
{
	RECT	rc;

	::GetClientRect( GetSafeHwnd(), &rc );
	m_nRowHeight = nRowHeight;
	m_nPageRows  = (int)( (double)( rc.bottom - rc.top ) / (double)nRowHeight + 0.5 );
	::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( GetSafeHwnd() );
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::SetScrollCapture (0x44F930)

void CODScrollBar::SetScrollCapture()
{
	if ( m_bUseParentCapture )
		GetParent()->SetCapture();
	else
		SetCapture();
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::ReleaseScrollCapture (0x44F970)

BOOL CODScrollBar::ReleaseScrollCapture()
{
	return ::ReleaseCapture();
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::HasScrollCapture (0x44F980)

BOOL CODScrollBar::HasScrollCapture()
{
	CWnd*	pWnd = m_bUseParentCapture ? GetParent() : this;

	return pWnd == GetCapture();
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::OnSetFocus (0x44F9D0)
//
// The bar never keeps the focus; it hands it back to the list it decorates.

void CODScrollBar::OnSetFocus( CWnd* /*pOldWnd*/ )
{
	GetParent()->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::GetPos (0x44A370)

int CODScrollBar::GetPos()
{
	return m_nPos;
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::OnNcDestroy (0x44AD30)
//
// The bar is heap-owned by the list that newed it, and frees itself when its
// window goes away.

void CODScrollBar::OnNcDestroy()
{
	CWnd::OnNcDestroy();
	delete this;
}

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar::OnEraseBkgnd (0x44BDC0)
//
// OnPaint covers the whole bar.

BOOL CODScrollBar::OnEraseBkgnd( CDC* /*pDC*/ )
{
	return TRUE;
}
