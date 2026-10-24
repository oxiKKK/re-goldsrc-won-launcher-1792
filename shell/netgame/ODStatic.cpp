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
// Purpose: CODStatic, the self-painted help label.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

BEGIN_MESSAGE_MAP( CODStatic, CStatic )
	//{{AFX_MSG_MAP(CODStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_DRAWITEM()
	ON_WM_NCPAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODStatic::CODStatic (0x450670)
//
// Transparent by default, text white until SetTextColor overwrites it.

CODStatic::CODStatic()
{
	strText.Empty();
	m_szOffsets    = CSize( 0, 0 );
	m_bCenterText  = FALSE;
	m_clrBgnd      = 0;
	m_bTransparent = TRUE;
	m_clrText      = RGB( 255, 255, 255 );

	m_hStaticFont.CreateFont( -11, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
		VARIABLE_PITCH | FF_DONTCARE, "Arial" );
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::~CODStatic (0x450740)

CODStatic::~CODStatic()
{
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::OnPaint (0x4507C0)

void CODStatic::OnPaint()
{
	CPaintDC	dc( this );
	CDC			mem;
	CBitmap		bmp;
	CBitmap*	pOldBmp;
	CFont*		pOldFont;
	RECT		rcClient, rcDst, rcSrc;
	DRAWTEXTPARAMS	dtp;
	UINT		flags;
	int			w, h;

	::GetClientRect( GetSafeHwnd(), &rcClient );

	if ( !mem.CreateCompatibleDC( &dc ) )
		return;

	w = rcClient.right - rcClient.left;
	h = rcClient.bottom - rcClient.top;
	bmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(), w, h ) );
	pOldBmp = mem.SelectObject( &bmp );

	if ( m_bTransparent )
	{
		// The control's slice of the parent background art.
		::GetWindowRect( GetSafeHwnd(), &rcDst );
		ScreenToClient( &rcDst );
		::GetWindowRect( GetSafeHwnd(), &rcSrc );
		if ( GetParent() )
			GetParent()->ScreenToClient( &rcSrc );
		Launcher_CopyParentBackground( &mem, &rcDst, &rcSrc );
	}
	else
	{
		mem.FillRect( &rcClient, &CBrush( m_clrBgnd ) );
	}

	mem.SetTextColor( m_clrText );
	pOldFont = mem.SelectObject( &m_hStaticFont );
	mem.SetBkMode( TRANSPARENT );

	flags = ( m_bCenterText ? DT_CENTER : DT_LEFT ) | DT_WORDBREAK | DT_VCENTER;
	rcClient.left += m_szOffsets.cx;
	rcClient.top  += m_szOffsets.cy;

	dtp.cbSize       = sizeof( dtp );
	dtp.iTabLength   = 4;
	dtp.iLeftMargin  = 0;
	dtp.iRightMargin = 0;
	::DrawTextExA( mem.GetSafeHdc(), (LPSTR)(LPCSTR)strText, -1, &rcClient, flags, &dtp );

	rcClient.top  -= m_szOffsets.cy;
	rcClient.left -= m_szOffsets.cx;
	dc.BitBlt( rcClient.left, rcClient.top, rcClient.right - rcClient.left,
		rcClient.bottom - rcClient.top, &mem, 0, 0, SRCCOPY );

	mem.SelectObject( pOldFont );
	mem.SelectObject( pOldBmp );
	mem.DeleteDC();
	ValidateRect( &rcClient );
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::OnEraseBkgnd (0x450AC0)
//
// Paints the face immediately and reports handled, so the control never
// flashes an unpainted background.

BOOL CODStatic::OnEraseBkgnd( CDC* /*pDC*/ )
{
	OnPaint();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::SetWindowText (0x450AD0)

void CODStatic::SetWindowText( const char* psz )
{
	CStatic::SetWindowText( "" );
	strText = psz;
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::SetTextColor (0x450B10)

void CODStatic::SetTextColor( COLORREF clr )
{
	m_clrText = clr;
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::SetFontSize (0x450B20)

void CODStatic::SetFontSize( int nSize, int nWeight )
{
	m_hStaticFont.DeleteObject();
	m_hStaticFont.CreateFont( -nSize, 0, 0, 0, nWeight, 0, 0, 0, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
		VARIABLE_PITCH | FF_DONTCARE, "Arial" );
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::SetCentered (0x450B70)

void CODStatic::SetCentered( BOOL /*bCenter*/ )
{
	m_bCenterText = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::SetOffsets (0x450B80)

void CODStatic::SetOffsets( int cx, int cy )
{
	m_szOffsets = CSize( cx, cy );
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::SetWindowText (0x450BA0)

void CODStatic::SetWindowText( const CString& str )
{
	CStatic::SetWindowText( "" );
	strText = str;
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::OnDrawItem (0x40C070)
//
// Mapped but empty.

void CODStatic::OnDrawItem( int /*nIDCtl*/, LPDRAWITEMSTRUCT /*lpDIS*/ )
{
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::SetTransparent (0x441C40)

void CODStatic::SetTransparent( BOOL bOn )
{
	m_bTransparent = bOn;
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::SetBgColor (0x441C50)

void CODStatic::SetBgColor( COLORREF clr )
{
	m_clrBgnd = clr;
}

/////////////////////////////////////////////////////////////////////////////
// CODStatic::OnNcPaint (0x441DF0)
//
// A non-client paint just forces a client repaint.

void CODStatic::OnNcPaint()
{
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}
