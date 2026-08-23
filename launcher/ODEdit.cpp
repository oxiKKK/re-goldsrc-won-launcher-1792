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
// Purpose: CODEdit, the owner-draw rich-text panel.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

BEGIN_MESSAGE_MAP( CODEdit, CWnd )
	//{{AFX_MSG_MAP(CODEdit)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODEdit::CODEdit (0x446E10)

CODEdit::CODEdit()
{
	HFONT	hf;

	m_clrBg          = RGB( 56, 56, 56 );
	m_clrText        = RGB( 128, 128, 128 );
	m_bScrollVisible = 0;
	m_pScrollbar     = NULL;
	m_pRichEdit      = NULL;

	hf = ::CreateFontA( -11, 0, 0, 0, 400, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, 2, "Arial" );
	if ( hf )
		m_font.Attach( hf );
}

/////////////////////////////////////////////////////////////////////////////
// CODEdit::~CODEdit (0x446EC0)

CODEdit::~CODEdit()
{
	delete m_pRichEdit;
}

/////////////////////////////////////////////////////////////////////////////
// CODEdit::OnCreate (0x446F50)
//
// The panel builds its rich-edit child and scrollbar end cap from the
// CREATESTRUCT rect, so a plain CWnd::Create is all an owner has to do.

int CODEdit::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
	CRect		rc;
	CHARFORMAT	cf;
	RECT		rcBar;

	if ( CWnd::OnCreate( lpCreateStruct ) == -1 )
		return -1;

	m_pRichEdit = new CRichEditCtrl;

	rc.SetRect( lpCreateStruct->x, lpCreateStruct->y,
				lpCreateStruct->x + lpCreateStruct->cx,
				lpCreateStruct->y + lpCreateStruct->cy );
	rc.InflateRect( -3, -3 );

	if ( !m_pRichEdit->Create( WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL, rc, this, 0 ) )
		return -1;

	m_pRichEdit->SetFont( &m_font );

	memset( &cf, 0, sizeof( cf ) );
	cf.cbSize      = sizeof( cf );
	cf.dwMask      = CFM_COLOR | CFM_BOLD | CFM_ITALIC;
	cf.dwEffects   = 0;
	cf.crTextColor = RGB( 240, 127, 24 );
	m_pRichEdit->SetDefaultCharFormat( cf );

	m_pScrollbar = new CODScrollBar;
	if ( !m_pScrollbar )
		return -1;

	m_nLineHeight = 15;							// provisional pitch, refined by OnPaint

	// Scrollbar end cap, 16px wide, inset 3px on the right.
	rcBar.left   = lpCreateStruct->cx - 19;
	rcBar.top    = 3;
	rcBar.right  = lpCreateStruct->cx - 3;
	rcBar.bottom = lpCreateStruct->cy - 3;
	if ( !m_pScrollbar->Create( this, &rcBar, 15 ) )
		return -1;

	m_pScrollbar->ShowWindow( SW_HIDE );
	m_bScrollVisible = 0;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODEdit::SetText (0x447140)

void CODEdit::SetText( const char* psz )
{
	int	lines;

	SetWindowText( psz );
	if ( m_pRichEdit )
	{
		m_pRichEdit->SetWindowText( psz );
		lines = m_pRichEdit->GetLineCount();
		if ( m_pScrollbar )
		{
			m_pScrollbar->SetRange( 0, lines );
			m_pScrollbar->SetPos( 0 );
		}
	}
	Finalize();
}

/////////////////////////////////////////////////////////////////////////////
// CODEdit::OnPaint (0x4471A0)
//
// The rich edit is never shown; its text is rendered into an off-screen
// buffer with EM_FORMATRANGE and blitted inside the panel's frame.

void CODEdit::OnPaint()
{
	CPaintDC	dc( this );
	TEXTMETRIC	tm;
	CRect		rc;
	CDC			memDC;
	CBitmap		bmp;
	CBitmap*	pOldBmp;
	CBrush		brBg( RGB( 0, 0, 0 ) );
	FORMATRANGE	fr;
	COLORREF	clr;
	HDC			hicScreen;
	int			logX, logY;
	int			scrollPos, lineCount;
	int			i;

	memset( &tm, 0, sizeof( tm ) );
	GetClientRect( &rc );

	// Three concentric frames: focus colour if this panel has the focus.
	for ( i = 0; i < 3; i++ )
	{
		clr = ( CWnd::FromHandle( ::GetFocus() ) == this ) ? m_clrText : m_clrBg;
		CBrush	br( clr );
		dc.FrameRect( &rc, &br );
		rc.InflateRect( -1, -1 );
	}

	if ( m_bScrollVisible )						// reserve the scrollbar column
		rc.right -= 16;

	if ( rc.Width() < 5 || rc.Height() < 5 )
		return;

	// Off-screen buffer the size of the text area.
	memDC.CreateCompatibleDC( &dc );
	bmp.CreateCompatibleBitmap( &dc, rc.Width(), rc.Height() );
	pOldBmp = memDC.SelectObject( &bmp );

	CRect	rcBuf( 0, 0, rc.Width(), rc.Height() );
	memDC.FillRect( &rcBuf, &brBg );

	// Twips-per-pixel from a screen IC.
	hicScreen = ::CreateICA( "DISPLAY", NULL, NULL, NULL );
	logX = ::GetDeviceCaps( hicScreen, LOGPIXELSX );
	logY = ::GetDeviceCaps( hicScreen, LOGPIXELSY );
	::DeleteDC( hicScreen );

	dc.GetTextMetrics( &tm );
	m_nLineHeight = tm.tmHeight - 3;

	if ( m_pScrollbar )
		m_pScrollbar->SetRowHeight( tm.tmHeight );

	scrollPos = m_pScrollbar ? m_pScrollbar->GetPos() : 0;
	lineCount = m_pRichEdit->GetLineCount();

	// rcPage is the buffer; rc is the page window, shifted up by the scroll.
	memset( &fr, 0, sizeof( fr ) );
	fr.hdc           = memDC.GetSafeHdc();
	fr.hdcTarget     = memDC.GetSafeHdc();
	fr.rcPage.left   = rcBuf.left   * ( 1440 / logX );
	fr.rcPage.top    = rcBuf.top    * ( 1440 / logY );
	fr.rcPage.right  = rcBuf.right  * ( 1440 / logX );
	fr.rcPage.bottom = ( rcBuf.top + m_nLineHeight * lineCount ) * ( 1440 / logY );
	fr.rc.left       = 2 * ( 1440 / logX );
	fr.rc.top        = -( scrollPos * m_nLineHeight ) * ( 1440 / logY );
	fr.rc.right      = ( rc.Width() - 4 ) * ( 1440 / logX );
	fr.rc.bottom     = ( lineCount * m_nLineHeight ) * ( 1440 / logY );
	fr.chrg.cpMin    = 0;
	fr.chrg.cpMax    = -1;

	memDC.SetTextColor( RGB( 255, 255, 255 ) );
	memDC.SetBkColor( RGB( 0, 0, 0 ) );

	m_pRichEdit->FormatRange( NULL, FALSE );
	m_pRichEdit->FormatRange( &fr, TRUE );
	m_pRichEdit->FormatRange( NULL, FALSE );

	dc.BitBlt( 3, 3, rcBuf.Width(), rcBuf.Height(), &memDC, 0, 0, SRCCOPY );

	memDC.SelectObject( pOldBmp );
}

/////////////////////////////////////////////////////////////////////////////
// CODEdit::OnVScroll (0x4476D0)

void CODEdit::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	UNUSED_ALWAYS( nSBCode );
	UNUSED_ALWAYS( pScrollBar );

	m_pRichEdit->SetScrollPos( SB_VERT, nPos, TRUE );
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODEdit::Finalize (0x447700)

void CODEdit::Finalize()
{
	CRect	rc;
	int		lines;

	m_pRichEdit->GetRect( &rc );
	lines = m_pRichEdit->GetLineCount();

	if ( lines > VisibleLines() && !m_bScrollVisible )
	{
		m_bScrollVisible = 1;
		if ( m_pScrollbar )
		{
			m_pScrollbar->ShowWindow( SW_SHOWNA );
			m_pScrollbar->SetPos( 0 );			// park the thumb at the top
		}
		InvalidateRect( NULL, TRUE );
		return;
	}

	if ( lines <= VisibleLines() && m_bScrollVisible )
	{
		m_bScrollVisible = 0;
		if ( m_pScrollbar )
			m_pScrollbar->ShowWindow( SW_HIDE );
		InvalidateRect( NULL, TRUE );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODEdit::VisibleLines (0x4477C0)

int CODEdit::VisibleLines()
{
	CRect	rc;

	GetClientRect( &rc );
	rc.InflateRect( -3, -3 );

	if ( m_nLineHeight )
		return (int)( (double)rc.Height() / (double)m_nLineHeight + 0.5 );
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODEdit::OnSize (0x447820)

void CODEdit::OnSize( UINT nType, int cx, int cy )
{
	CRect	rcClient;
	CRect	rc( 0, 0, cx, cy );

	CWnd::OnSize( nType, cx, cy );

	GetClientRect( &rcClient );

	rc.InflateRect( -3, -3 );
	rc.right -= 16;
	if ( m_pRichEdit )
		m_pRichEdit->MoveWindow( rc.left, rc.top, rc.Width(), rc.Height(), TRUE );

	if ( m_pScrollbar )
		m_pScrollbar->MoveWindow( rcClient.Width() - 19, 3, 16, rcClient.Height() - 6, TRUE );

	Finalize();
}

/*
==================
ODList_CompareRefreshOrder (0x4478D0)

Queried servers first, then the caller's stamped refresh order.
==================
*/
int __stdcall ODList_CompareRefreshOrder( const CServerInfo* a, const CServerInfo* b, int )
{
	if ( !a || !b )
		return 0;

	if ( a->m_pOwnedQuery )
	{
		if ( !b->m_pOwnedQuery )
			return -1;
	}
	else if ( b->m_pOwnedQuery )
	{
		return 1;
	}

	if ( a->m_iOrder < b->m_iOrder )
		return -1;
	return a->m_iOrder > b->m_iOrder;
}
