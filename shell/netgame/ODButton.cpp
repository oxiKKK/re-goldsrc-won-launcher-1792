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
// Purpose: the owner-draw skin buttons: CODBitmapButton, CODBlendBtn,
//          CODBlendCheckBox.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The owner-draw caption buffer CODBlendBtn::GetCaption fills (4F36DC).
static char	s_szButtonCaption[260];

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::CODBlendBtn (0x43EF50)

CODBlendBtn::CODBlendBtn()
{
	m_unk60        = 0;
	m_hFaceDib     = NULL;
	m_bTwoBitmap   = 0;
	m_bHasArrow    = 1;
	m_bTransparent = 1;
	m_textYOffset  = 0;
	m_textFlags    = DT_CENTER | DT_SINGLELINE;
	m_cellW        = 0;
	m_cellH        = 0;
	m_states       = 0;
	m_bStripMode   = 0;
	m_hStripDib    = NULL;
	m_bHighlight   = 0;
	m_bSkinDirty   = 1;				// load the skin on the first DrawItem
	m_clrArrowSel  = RGB( 255, 255, 255 );
	m_clrArrowNorm = RGB( 127, 127, 127 );
	m_clrText      = RGB( 255, 255, 127 );
	m_bFade        = 0;
	m_fadeEnd      = 0.5f;
	m_fadeStart    = 0.1f;
	m_timeCur      = 0.0;
	m_timeStart    = 0.0;
	m_clrDown      = RGB( 192, 192, 192 );
	m_clrBg        = RGB( 0, 0, 0 );
	m_clrHover     = RGB( 255, 180, 24 );
	m_blendBufBase    = NULL;
	m_blendBufOverlay = NULL;
	m_blendCapBase    = 0;
	m_blendCapOverlay = 0;
	m_b3State      = 0;

	HFONT	hMain = ::CreateFontA( -11, 0, 0, 0, 400, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, 2, "Arial" );
	if ( hMain )
		m_mainFont.Attach( hMain );
	HFONT	hShadow = ::CreateFontA( -11, 0, 0, 0, 900, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, 2, "Arial" );
	if ( hShadow )
		m_shadowFont.Attach( hShadow );
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::IsHighlighted (0x441D70)
//
// forced highlight, set for the single-player buttons while a
// multiplayer-only mod is active.

int  CODBlendBtn::IsHighlighted()                 { return m_bHighlight; }

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::SetHighlight (0x441D40)

void CODBlendBtn::SetHighlight( int bOn )
{
	m_bHighlight = bOn;
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::DrawDefault (0x441DF0)
//
// force a repaint of the owner-draw face.

void CODBlendBtn::DrawDefault()                   { InvalidateRect( NULL, TRUE ); UpdateWindow(); }

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::SetTransparent (0x441C40)
//
// ICF-folded with CODStatic::SetTransparent.

void CODBlendBtn::SetTransparent( BOOL bOn )       { m_bTransparent = bOn; }

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::SetFontSize (0x441C80)
//
// vtbl+188.

void CODBlendBtn::SetFontSize( int nSize, int nWeight )
{
	m_mainFont.DeleteObject();
	m_mainFont.Attach( ::CreateFontA( -nSize, 0, 0, 0, nWeight, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, 2, "Arial" ) );

	int	shadowWeight = nWeight + 200;
	if ( shadowWeight >= 900 )
		shadowWeight = 900;
	m_shadowFont.DeleteObject();
	m_shadowFont.Attach( ::CreateFontA( -nSize, 0, 0, 0, shadowWeight, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, 2, "Arial" ) );
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::PrimeBlendBuffers (0x441390)
//
// read the rendered face back as a 24-bit DIB into both blend scratch
// buffers (growing them to fit) and put it down again, so
// BlendSlice/BlendStates start from a known bit depth.

int CODBlendBtn::PrimeBlendBuffers( CDC* pDC, CBitmap* pBmp, RECT* /*prc*/ )
{
	BITMAP		bm;
	BITMAPINFO	bmi;
	int			size;

	pBmp->GetObject( sizeof( bm ), &bm );

	memset( &bmi, 0, sizeof( bmi ) );
	size = ( ( 3 * bm.bmWidth + 3 ) & ~3 ) * bm.bmHeight + sizeof( BITMAPINFOHEADER );

	bmi.bmiHeader.biSize        = sizeof( BITMAPINFOHEADER );
	bmi.bmiHeader.biWidth       = bm.bmWidth;
	bmi.bmiHeader.biHeight      = bm.bmHeight;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 24;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage   = 0;

	if ( !m_blendBufBase )
	{
		m_blendBufBase = new BYTE[size];
		m_blendCapBase = size;
	}
	else if ( size > m_blendCapBase )
	{
		delete[] m_blendBufBase;
		m_blendBufBase = new BYTE[size];
		m_blendCapBase = size;
	}

	if ( !m_blendBufOverlay )
	{
		m_blendBufOverlay = new BYTE[size];
		m_blendCapOverlay = size;
	}
	else if ( size > m_blendCapOverlay )
	{
		delete[] m_blendBufOverlay;
		m_blendBufOverlay = new BYTE[size];
		m_blendCapOverlay = size;
	}

	::GetDIBits( pDC->GetSafeHdc(), (HBITMAP)pBmp->GetSafeHandle(), 0, bm.bmHeight,
		m_blendBufBase, &bmi, DIB_RGB_COLORS );
	memcpy( m_blendBufOverlay, m_blendBufBase, size );

	::SetStretchBltMode( pDC->GetSafeHdc(), COLORONCOLOR );
	return ::SetDIBitsToDevice( pDC->GetSafeHdc(), 0, 0, bm.bmWidth, bm.bmHeight,
		0, 0, 0, bm.bmHeight, m_blendBufOverlay, &bmi, DIB_RGB_COLORS );
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::EnsureSkinLoaded (0x4417A0)
//
// render the button face once into m_bmpOverlay, which the blend passes
// later use as the glow overlay.

int CODBlendBtn::EnsureSkinLoaded()
{
	CClientDC	dc( this );
	RECT		rcClient;

	GetClientRect( &rcClient );
	m_faceW = rcClient.right - rcClient.left;
	m_faceH = rcClient.bottom - rcClient.top;

	CDC		memDC;
	if ( !memDC.CreateCompatibleDC( &dc ) )
	{
		DrawDefault();
		return 0;
	}

	m_bmpOverlay.Attach( ::CreateCompatibleBitmap( dc.m_hDC, m_faceW, m_faceH ) );

	CBitmap*	pOld = memDC.SelectObject( &m_bmpOverlay );
	RECT		rc;

	rc.left   = 0;
	rc.top    = 0;
	rc.right  = m_faceW;
	rc.bottom = m_faceH;

	{
		CBrush	bg( m_clrBg );
		memDC.FillRect( &rc, &bg );
	}

	DrawButtonFace( &memDC, &m_bmpOverlay, &rc, 1, 1, 1 );
	PrimeBlendBuffers( &memDC, &m_bmpOverlay, &rc );

	memDC.SelectObject( pOld );
	memDC.DeleteDC();
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::EnsureStripSkin (0x441980)
//
// the strip variant: the normal face into m_bmpOverlay, then the alternate
// face into m_bmpOverlayAlt with m_bTwoBitmap forced on for the duration
// of the second pass.

int CODBlendBtn::EnsureStripSkin()
{
	if ( !Launcher_MainButtonsLoaded() )
		return 1;

	CClientDC	dc( this );
	RECT		rcClient;

	GetClientRect( &rcClient );
	m_faceW = rcClient.right - rcClient.left;
	m_faceH = rcClient.bottom - rcClient.top;

	CDC		memDC;
	if ( !memDC.CreateCompatibleDC( &dc ) )
	{
		DrawDefault();
		return 0;
	}

	RECT	rc;

	rc.left   = 0;
	rc.top    = 0;
	rc.right  = m_faceW;
	rc.bottom = m_faceH;

	m_bmpOverlay.Attach( ::CreateCompatibleBitmap( dc.m_hDC, m_faceW, m_faceH ) );

	CBitmap*	pOld = memDC.SelectObject( &m_bmpOverlay );

	{
		CBrush	bg( m_clrBg );
		memDC.FillRect( &rc, &bg );
	}
	DrawStripFace( &memDC, &m_bmpOverlay, &rc, 1, 1, 1 );

	m_bmpOverlayAlt.Attach( ::CreateCompatibleBitmap( dc.m_hDC, m_faceW, m_faceH ) );
	memDC.SelectObject( &m_bmpOverlayAlt );

	{
		CBrush	bg( m_clrBg );
		memDC.FillRect( &rc, &bg );
	}
	m_bTwoBitmap = 1;
	DrawStripFace( &memDC, &m_bmpOverlayAlt, &rc, 1, 1, 1 );
	m_bTwoBitmap = 0;

	memDC.SelectObject( pOld );
	memDC.DeleteDC();
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::SetDIBData (0x43F110)

void CODBlendBtn::SetDIBData( const CSize& size, int nIndex, HGLOBAL hDib )
{
	if ( hDib )
	{
		m_cellW      = size.cx;
		m_cellH      = size.cy;
		m_states     = nIndex;
		m_hStripDib  = hDib;
		m_bStripMode = 1;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::PreSubclassWindow (0x43F260)

void CODBlendBtn::PreSubclassWindow()
{
	ModifyStyle( 0, WS_CHILD | BS_ICON | BS_OWNERDRAW );
	CButton::PreSubclassWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::Create (0x441C00)
//
// ORs the owner-draw button style (WS_CHILD | BS_ICON | BS_OWNERDRAW) into the caller's dwStyle
// before forwarding to CButton::Create.

BOOL CODBlendBtn::Create( LPCTSTR lpszCaption, DWORD dwStyle, const RECT& rect,
	CWnd* pParentWnd, UINT nID )
{
	return CButton::Create( lpszCaption, dwStyle | WS_CHILD | BS_ICON | BS_OWNERDRAW, rect, pParentWnd, nID );
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn_DrawArrow (0x440240)
//
// the focus brackets: a "[ ]" frame hugging the left and right edges of
// the button rect, thicker when pressed.

void CODBlendBtn_DrawArrow( CDC* pDC, RECT* prc, COLORREF color, int pressed )
{
	CPen	pen;
	pen.CreatePen( PS_SOLID, pressed ? 5 : 3, color );
	CPen*	pOld = pDC->SelectObject( &pen );

	int	left   = prc->left;
	int	top    = prc->top;
	int	right  = prc->right - 1;
	int	bottom = prc->bottom - 1;

	pDC->MoveTo( left + 5, top );		// left bracket  [
	pDC->LineTo( left, top );
	pDC->LineTo( left, bottom );
	pDC->LineTo( left + 5, bottom );

	pDC->MoveTo( right - 5, top );		// right bracket ]
	pDC->LineTo( right, top );
	pDC->LineTo( right, bottom );
	pDC->LineTo( right - 5, bottom );

	pDC->SelectObject( pOld );
}

/////////////////////////////////////////////////////////////////////////////
// CODBitmapButton::CODBitmapButton (0x43E920)
//
// m_stateIndex is left at 1; the skin arrives later through SetSkin.

CODBitmapButton::CODBitmapButton()
{
	m_bCapturing  = 0;
	m_bHovering   = 0;
	m_stateIndex  = 1;
	m_bSkinLoaded = 0;
	m_dibNormal   = NULL;
	m_dibDown     = NULL;
	m_dibFocus    = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CODBitmapButton::~CODBitmapButton (0x43E970)

CODBitmapButton::~CODBitmapButton()
{
	if ( m_dibNormal )
		GlobalFree( m_dibNormal );
	m_dibNormal = NULL;
	if ( m_dibDown )
		GlobalFree( m_dibDown );
	m_dibDown = NULL;
	if ( m_dibFocus )
		GlobalFree( m_dibFocus );
	m_dibFocus = NULL;
}

// 0x457170 -- the paging arrows are BUTTON windows with BS_OWNERDRAW.
BOOL CODBitmapButton::CreateGlyph( DWORD dwStyle, const RECT& rc, CWnd* pParent, UINT nID )
{
	return CreateEx( 0, "BUTTON", "", dwStyle,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		pParent ? pParent->GetSafeHwnd() : NULL, (HMENU)(UINT_PTR)nID, NULL );
}

/////////////////////////////////////////////////////////////////////////////
// CODBitmapButton::SetSkin (0x43E9C0)

void CODBitmapButton::SetSkin( const char* pszNormal, const char* pszDown, const char* pszFocus )
{
	char	szPath[260];

	if ( m_bSkinLoaded )
	{
		// Free the previous set.
		if ( m_dibNormal ) GlobalFree( m_dibNormal );
		m_dibNormal = NULL;
		if ( m_dibDown ) GlobalFree( m_dibDown );
		m_dibDown = NULL;
		if ( m_dibFocus ) GlobalFree( m_dibFocus );
		m_dibFocus = NULL;
	}

	sprintf( szPath, "%s%s.bmp", "gfx/shell/", pszNormal );
	m_dibNormal = DIB_LoadBitmapFile( szPath );
	sprintf( szPath, "%s%s.bmp", "gfx/shell/", pszDown );
	m_dibDown = DIB_LoadBitmapFile( szPath );
	sprintf( szPath, "%s%s.bmp", "gfx/shell/", pszFocus );
	m_dibFocus = DIB_LoadBitmapFile( szPath );

	m_stateIndex = 0;
	m_bSkinLoaded = 1;
}

BEGIN_MESSAGE_MAP( CODBitmapButton, CButton )
	//{{AFX_MSG_MAP(CODBitmapButton)
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODBitmapButton::OnTimer (0x43EAB0)
//
// the hover tick drops the highlight once the cursor leaves the glyph.

void CODBitmapButton::OnTimer( UINT_PTR nIDEvent )
{
	POINT	pt;
	RECT	rc;

	if ( nIDEvent == 0 )
	{
		GetCursorPos( &pt );
		::ScreenToClient( m_hWnd, &pt );
		GetClientRect( &rc );
		if ( !PtInRect( &rc, pt ) )
		{
			m_bHovering = 0;
			KillTimer( 0 );
			InvalidateRect( NULL, TRUE );
			UpdateWindow();
		}
	}
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODBitmapButton::OnMouseMove (0x43EDE0)
//
// arm the 100 ms hover tick.

void CODBitmapButton::OnMouseMove( UINT, CPoint )
{
	if ( m_stateIndex || m_bHovering )
		return;

	if ( SetTimer( 0, 100, NULL ) )
	{
		m_bHovering = 1;
		InvalidateRect( NULL, TRUE );
		UpdateWindow();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODBitmapButton::OnLButtonDown (0x43EE30)

void CODBitmapButton::OnLButtonDown( UINT, CPoint )
{
	if ( m_stateIndex || m_bCapturing )
		return;

	m_bCapturing = 1;
	SetFocus();
	SetCapture();
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODBitmapButton::OnLButtonUp (0x43EE80)

void CODBitmapButton::OnLButtonUp( UINT, CPoint pt )
{
	RECT	rc;
	CWnd*	pParent;

	if ( m_stateIndex )
	{
		Snd_PlayMenuSound( UISND_SELECT2 );
		Default();
		return;
	}

	if ( !m_bCapturing )
		return;

	m_bCapturing = 0;
	ReleaseCapture();
	GetClientRect( &rc );
	if ( !PtInRect( &rc, pt ) )
		return;

	Snd_PlayMenuSound( UISND_SELECT2 );
	pParent = GetParent();
	if ( pParent )
		pParent->SendMessage( WM_COMMAND, GetDlgCtrlID(), (LPARAM)GetSafeHwnd() );
}

// CODBitmapButton::DrawItem (0x43EB40, the close/minimise glyph painter)
void CODBitmapButton::DrawItem( LPDRAWITEMSTRUCT lpDIS )
{
	if ( !m_bSkinLoaded )
		return;

	RECT	rcDst;
	CopyRect( &rcDst, &lpDIS->rcItem );
	CDC*	pDC = CDC::FromHandle( lpDIS->hDC );

	// Live cursor test: hover lights the focus glyph, a held button the down glyph.
	POINT	pt;
	GetCursorPos( &pt );
	::ScreenToClient( m_hWnd, &pt );
	BOOL	bHover   = PtInRect( &rcDst, pt );
	BOOL	bPressed = ( bHover && GetAsyncKeyState( VK_LBUTTON ) ) ? TRUE : FALSE;

	CDC		mem;
	if ( !mem.CreateCompatibleDC( pDC ) )
		return;

	int		w = rcDst.right - rcDst.left;
	int		h = rcDst.bottom - rcDst.top;
	CBitmap	bmp;
	bmp.Attach( ::CreateCompatibleBitmap( pDC->GetSafeHdc(), w, h ) );
	CBitmap*	pOld = mem.SelectObject( &bmp );

	// Background slice under the control (its position in the menu-background art).
	RECT	rcSrc;
	::GetWindowRect( m_hWnd, &rcSrc );
	if ( GetParent() )
		GetParent()->ScreenToClient( &rcSrc );
	Launcher_BlitBackground( &mem, &rcDst, &rcSrc );

	HGLOBAL	hDib;
	if ( m_stateIndex )
		hDib = m_dibNormal;
	else if ( bPressed )
		hDib = m_dibDown;
	else if ( !bHover )
		hDib = m_dibNormal;
	else
		hDib = m_dibFocus;

	if ( hDib )
	{
		void*	pBits = GlobalLock( hDib );
		if ( pBits )
		{
			RECT	rcGlyph = { 0, 0, (LONG)DIB_Width( (LPBITMAPINFOHEADER)pBits ),
								  (LONG)DIB_Height( (LPBITMAPINFOHEADER)pBits ) };
			GlobalUnlock( hDib );
			DIB_BlitDib( mem.GetSafeHdc(), &rcDst, hDib, &rcGlyph );
		}
	}

	pDC->BitBlt( 0, 0, w, h, &mem, 0, 0, SRCCOPY );
	mem.SelectObject( pOld );
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::BlendStates (0x43F290)

void CODBlendBtn::BlendStates( RECT* /*prc*/, CDC* pSrcDC, CDC* pDstDC,
	CBitmap* pOverlay, CBitmap* pBase, int mode )
{
	CDC*				dc[2];
	CBitmap*			bmp[2];
	BITMAPINFOHEADER	bmih[2];
	BYTE*				bits[2];
	int					size[2];
	int					stride[2];
	BITMAP				bm;
	BYTE*				out;
	int					i, x, y, w, h, rowbytes;

	dc[0]  = pDstDC;  dc[1]  = pSrcDC;		// hdc[0]=dst, hdc[1]=src
	bmp[0] = pBase;   bmp[1] = pOverlay;	// hbm[0]=base, hbm[1]=overlay

	if ( !pBase || !pBase->m_hObject || !pOverlay || !pOverlay->m_hObject )
		return;

	// Pull both bitmaps into 24-bit DIBs.
	for ( i = 0; i < 2; i++ )
	{
		GetObjectA( bmp[i]->m_hObject, sizeof( bm ), &bm );
		w = bm.bmWidth;
		h = bm.bmHeight;

		memset( &bmih[i], 0, sizeof( BITMAPINFOHEADER ) );
		bmih[i].biSize = sizeof( BITMAPINFOHEADER );
		bmih[i].biWidth = w;
		bmih[i].biHeight = h;
		bmih[i].biPlanes = 1;
		bmih[i].biBitCount = 24;
		bmih[i].biCompression = BI_RGB;

		stride[i] = ( ( w + 1 ) * 3 ) & ~3;
		size[i] = h * stride[i] + sizeof( BITMAPINFOHEADER );	// over-alloc, as in the binary
		bits[i] = (BYTE*)malloc( size[i] );
		memset( bits[i], 0, size[i] );

		GetDIBits( dc[i] ? dc[i]->GetSafeHdc() : NULL, (HBITMAP)bmp[i]->m_hObject,
			0, h, bits[i], (BITMAPINFO*)&bmih[i], DIB_RGB_COLORS );
	}

	if ( size[0] != size[1] )
	{
		free( bits[0] );
		free( bits[1] );
		return;
	}

	out = (BYTE*)malloc( size[0] );
	memset( out, 0, size[0] );

	w = bmih[0].biWidth;
	h = bmih[0].biHeight;
	rowbytes = stride[0];

	for ( y = 0; y < h; y++ )
	{
		BYTE*	pBaseRow	= bits[0] + y * rowbytes;
		BYTE*	pOverlayRow	= bits[1] + y * rowbytes;
		BYTE*	pOut		= out      + y * rowbytes;

		// Three colour bytes (B,G,R) per pixel.
		for ( x = 0; x < w * 3; x++ )
		{
			BYTE	b = pBaseRow[x];
			BYTE	o = pOverlayRow[x];

			switch ( mode )
			{
			case 1:	pOut[x] = ( b <= o ) ? (BYTE)( o >> 1 ) : b; break;
			case 2:	pOut[x] = ( b <= o ) ? o : b;                break;
			case 4:	pOut[x] = ( b <= o ) ? (BYTE)( o >> 2 ) : b; break;
			default: pOut[x] = b;                                break;
			}
		}
	}

	SetStretchBltMode( pDstDC ? pDstDC->GetSafeHdc() : NULL, COLORONCOLOR );
	SetDIBitsToDevice( pDstDC ? pDstDC->GetSafeHdc() : NULL,
		0, 0, w, h, 0, 0, 0, h, out, (BITMAPINFO*)&bmih[0], DIB_RGB_COLORS );

	free( bits[0] );
	free( bits[1] );
	free( out );
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::DrawItem (0x43F6F0)

void CODBlendBtn::DrawItem( LPDRAWITEMSTRUCT pdis )
{
	int			stripMode	= m_bStripMode;
	int			skinDirty	= m_bSkinDirty;
	int			transparent	= m_bTransparent;
	COLORREF	bgColor		= m_clrBg;
	HWND		hCtl		= m_hWnd;
	CDC*		pItemDC;
	RECT		rcDst;
	int			w, h, bSelected, bFocus;

	// A "strip" button (a slice of btns_main.bmp) takes the other path.
	if ( stripMode )
	{
		DrawStripButton( pdis );
		return;
	}

	// Lazily (re)load the per-button skin bitmaps.
	if ( skinDirty )
	{
		if ( !hCtl )
			return;
		if ( !EnsureSkinLoaded() )
			return;
		m_bSkinDirty = 0;
	}

	CopyRect( &rcDst, &pdis->rcItem );
	pItemDC   = CDC::FromHandle( pdis->hDC );
	bSelected = pdis->itemState & ODS_SELECTED;	// &1
	bFocus    = pdis->itemState & ODS_FOCUS;		// &0x10
	w = rcDst.right - rcDst.left;
	h = rcDst.bottom - rcDst.top;

	CDC		memDC;
	if ( !memDC.CreateCompatibleDC( pItemDC ) )
	{
		DrawDefault();
		return;
	}

	CBitmap		memBmp;
	memBmp.CreateCompatibleBitmap( pItemDC, w, h );
	CBitmap*	pOldMem = memDC.SelectObject( &memBmp );

	// Background fill.
	{
		CBrush	bgBrush( bgColor );
		memDC.FillRect( &rcDst, &bgBrush );
	}

	// Transparent buttons show the parent's background underneath: the destination
	// is the button's own client box, the source the same box in parent client
	// coordinates.  Both corners must be mapped -- an equal-sized src and dst is
	// what makes DIB_BlitDib take the 1:1 SetDIBitsToDevice path instead of
	// rescaling the artwork.
	if ( transparent )
	{
		RECT	rcCtl, rcParent;

		GetWindowRect( &rcCtl );
		ScreenToClient( &rcCtl );

		GetWindowRect( &rcParent );
		if ( GetParent() )
			GetParent()->ScreenToClient( &rcParent );

		Launcher_CopyParentBackground( &memDC, &rcCtl, &rcParent );
	}

	// Paint the button face into a second buffer via the virtual glyph drawer.
	CDC		glyphDC;
	if ( glyphDC.CreateCompatibleDC( pItemDC ) )
	{
		CBitmap		glyphBmp;
		glyphBmp.CreateCompatibleBitmap( pItemDC, w, h );
		CBitmap*	pOldGlyph = glyphDC.SelectObject( &glyphBmp );

		{
			CBrush	gBrush( bgColor );
			glyphDC.FillRect( &rcDst, &gBrush );
		}

		// The face glyph is the virtual DrawButtonFace (the binary's vtbl+192);
		// a strip button overrides it via the m_bStripMode path.
		DrawButtonFace( &glyphDC, &glyphBmp, &rcDst, bFocus, bSelected, 0 );

		if ( IsHighlighted() )
		{
			RECT	rc = { 0, 0, w, h };
			BlendStates( &rc, &glyphDC, &memDC, &glyphBmp, &memBmp, 4 );
		}
		else
		{
			memDC.BitBlt( 0, 0, w, h, &glyphDC, 0, 0, SRCPAINT );	// 0xEE0086
		}

		glyphDC.SelectObject( pOldGlyph );
	}
	else
	{
		DrawDefault();
	}

	// Composite to screen.
	pItemDC->BitBlt( 0, 0, w, h, &memDC, 0, 0, SRCCOPY );	// 0xCC0020
	memDC.SelectObject( pOldMem );
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::DrawStripButton (0x43FB30)

void CODBlendBtn::DrawStripButton( LPDRAWITEMSTRUCT pdis )
{
	HWND	hCtl = m_hWnd;
	CDC*	pItemDC;
	RECT	rcDst;
	int		w, h, bSel, bFocus;

	// Lazily (re)load the strip skin.
	if ( m_bSkinDirty )
	{
		if ( !hCtl )
			return;
		if ( !EnsureStripSkin() )
			return;
		m_bSkinDirty = 0;
	}

	CopyRect( &rcDst, &pdis->rcItem );
	pItemDC = CDC::FromHandle( pdis->hDC );
	bSel   = pdis->itemState & ODS_SELECTED;
	bFocus = pdis->itemState & ODS_FOCUS;
	w = rcDst.right - rcDst.left;
	h = rcDst.bottom - rcDst.top;

	CDC		memDC;
	if ( !memDC.CreateCompatibleDC( pItemDC ) )
	{
		DrawDefault();
		return;
	}

	CBitmap		memBmp;
	memBmp.CreateCompatibleBitmap( pItemDC, w, h );
	CBitmap*	pOldMem = memDC.SelectObject( &memBmp );

	{
		CBrush	bg( m_clrBg );
		memDC.FillRect( &rcDst, &bg );
	}

	if ( m_bTransparent )		// transparent: copy parent background
	{
		RECT	rcCtl, rcParent;
		GetWindowRect( &rcCtl );
		ScreenToClient( &rcCtl );
		GetWindowRect( &rcParent );
		if ( GetParent() )
			GetParent()->ScreenToClient( &rcParent );
		Launcher_CopyParentBackground( &memDC, &rcCtl, &rcParent );
	}

	CDC		glyphDC;
	if ( glyphDC.CreateCompatibleDC( pItemDC ) )
	{
		CBitmap		glyphBmp;
		glyphBmp.CreateCompatibleBitmap( pItemDC, w, h );
		CBitmap*	pOldGlyph = glyphDC.SelectObject( &glyphBmp );

		{
			CBrush	gBrush( m_clrBg );
			glyphDC.FillRect( &rcDst, &gBrush );
		}

		// Draw the strip face (this=button; args: glyphDC, glyphBmp, rect,
		// focus, selected, 0 -- the binary's order).
		DrawStripFace( &glyphDC, &glyphBmp, &rcDst, bFocus, bSel, 0 );

		{
			RECT		rc = { 0, 0, w, h };
			CBitmap*	pScreen = CBitmap::FromHandle( (HBITMAP)GetCurrentObject( memDC.GetSafeHdc(), OBJ_BITMAP ) );
			int			mode = IsHighlighted() ? 4 : 2;
			BlendStates( &rc, &glyphDC, &memDC, &glyphBmp, pScreen, mode );
		}

		glyphDC.SelectObject( pOldGlyph );
	}
	else
	{
		DrawDefault();
	}

	pItemDC->BitBlt( 0, 0, w, h, &memDC, 0, 0, SRCCOPY );
	memDC.SelectObject( pOldMem );
}

BEGIN_MESSAGE_MAP( CODBlendBtn, CButton )
	//{{AFX_MSG_MAP(CODBlendBtn)
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_SETFOCUS()
	ON_WM_DESTROY()
	ON_WM_GETDLGCODE()
	ON_WM_DRAWITEM()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::~CODBlendBtn (0x43F140)

CODBlendBtn::~CODBlendBtn()
{
	delete[] m_blendBufBase;
	delete[] m_blendBufOverlay;

	if ( m_hFaceDib )
	{
		GlobalFree( m_hFaceDib );
		m_hFaceDib = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::OnDestroy (0x43F250)

void CODBlendBtn::OnDestroy()
{
	CButton::OnDestroy();
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::OnGetDlgCode (0x441C30)

UINT CODBlendBtn::OnGetDlgCode()
{
	return (UINT)Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::SetHasArrow (0x441C50)

void CODBlendBtn::SetHasArrow( int bOn )
{
	m_bHasArrow = bOn;
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::SetTextColor (0x441C60)

void CODBlendBtn::SetTextColor( COLORREF clr )
{
	m_clrDown = clr;
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::SetBkColor (0x441C70)

void CODBlendBtn::SetBkColor( COLORREF clr )
{
	m_clrBg = clr;
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::SetLeftAlign (0x441D20)

void CODBlendBtn::SetLeftAlign()
{
	m_textFlags &= ~DT_CENTER;
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::SetRightAlign (0x441D30)

void CODBlendBtn::SetRightAlign()
{
	m_textFlags = ( m_textFlags & ~DT_CENTER ) | DT_RIGHT;
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::OnLButtonDown (0x441D80)
//
// a dimmed button swallows the click entirely, not even reaching the
// default proc.

void CODBlendBtn::OnLButtonDown( UINT, CPoint )
{
	if ( IsHighlighted() )
		return;

	Snd_PlayMenuSound( UISND_SELECT2 );
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::OnMouseMove (0x43FF60)
//
// arm the fade timer and (re)stamp the fade-start time.

void CODBlendBtn::OnMouseMove( UINT /*nFlags*/, CPoint /*pt*/ )
{
	if ( !m_bFade )
	{
		if ( SetTimer( 1, 50, NULL ) )
			m_bFade = 1;
	}
	m_timeStart = engineapi.Sys_FloatTime();
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::OnTimer (0x43FFB0)
//
// advance the fade clock.

void CODBlendBtn::OnTimer( UINT_PTR nIDEvent )
{
	if ( nIDEvent == 1 )
	{
		RECT	rc;
		POINT	pt;

		m_timeCur = engineapi.Sys_FloatTime();
		::GetWindowRect( m_hWnd, &rc );
		GetCursorPos( &pt );
		if ( PtInRect( &rc, pt ) )
			m_timeStart = m_timeCur;

		if ( m_timeCur - m_timeStart >= (double)m_fadeEnd )
		{
			KillTimer( 1 );
			m_bFade = 0;
		}

		RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
	}
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::OnEraseBkgnd (0x440070)
//
// lays the menu background under the button through its own paint DC; the
// caller's pDC is ignored.

BOOL CODBlendBtn::OnEraseBkgnd( CDC* /*pDC*/ )
{
	CPaintDC	dc( this );
	CDC			memDC;
	CBitmap		memBmp;
	CBitmap*	pOld;
	RECT		rc;
	int			w, h;

	GetClientRect( &rc );

	if ( !memDC.CreateCompatibleDC( &dc ) )
	{
		DrawDefault();
		return FALSE;
	}

	w = rc.right - rc.left;
	h = rc.bottom - rc.top;

	memBmp.CreateCompatibleBitmap( &dc, w, h );
	pOld = memDC.SelectObject( &memBmp );

	Launcher_BlitBackground( &memDC, &rc, &rc );
	dc.BitBlt( rc.left, rc.top, w, h, &memDC, 0, 0, SRCCOPY );

	memDC.SelectObject( pOld );
	memDC.DeleteDC();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::OnSetFocus (0x440370)
//
// identical to a mouse-over (arm + stamp).

void CODBlendBtn::OnSetFocus( CWnd* pOldWnd )
{
	CButton::OnSetFocus( pOldWnd );
	if ( !m_bFade )
	{
		if ( SetTimer( 1, 50, NULL ) )
			m_bFade = 1;
	}
	m_timeStart = engineapi.Sys_FloatTime();
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::BlitStripSlice (0x4403B0)

void* CODBlendBtn::BlitStripSlice( CDC* pDstDC, int slice, DWORD rop )
{
	HGLOBAL		hStrip = m_hStripDib;
	void*		pDib;
	int			cellH, states, cellW, y0;
	RECT		rc;
	RECT		src;

	pDib = GlobalLock( hStrip );
	if ( !pDib )
		return pDib;

	DIB_Height( (LPBITMAPINFOHEADER)pDib );
	states = m_states;
	cellH  = m_cellH;
	cellW  = m_cellW;
	y0 = cellH * ( slice + 3 * states );

	CDC		memDC;
	if ( !memDC.CreateCompatibleDC( pDstDC ) )
	{
		DrawDefault();
		return NULL;
	}

	CBitmap		memBmp;
	memBmp.CreateCompatibleBitmap( pDstDC, cellW, cellH );
	CBitmap*	pOld = memDC.SelectObject( &memBmp );

	rc.left = 0;
	rc.top = 0;
	rc.right = cellW;
	rc.bottom = cellH;

	{
		CBrush	bg( m_clrBg );
		memDC.FillRect( &rc, &bg );
	}

	// Blit the slice from the strip DIB (source cell = {0, y0, cellW, y0+cellH}).
	src.left   = 0;
	src.top    = y0;
	src.right  = cellW;
	src.bottom = y0 + cellH;
	DIB_BlitDib( memDC.GetSafeHdc(), &rc, hStrip, &src );
	GlobalUnlock( hStrip );

	pDstDC->BitBlt( 0, 0, cellW, cellH, &memDC, 0, 0, rop );

	memDC.SelectObject( pOld );
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::DrawStripFace (0x4405F0)

void CODBlendBtn::DrawStripFace( CDC* pDC, CBitmap* /*pBmp*/, RECT* prc,
	int bSel, int bFocus, int bDisabled )
{
	int		twoBitmap = m_bTwoBitmap;
	int		fadeOn    = m_bFade;
	int		w = prc->right - prc->left;
	int		h = prc->bottom - prc->top;
	int		level = 0;

	if ( !Launcher_MainButtonsLoaded() )
		return;

	// Highlight pre-pass: blend the overlay bitmap over the current screen
	// content where the button sits.
	if ( bFocus && !bDisabled )
	{
		CDC		preDC;
		if ( !preDC.CreateCompatibleDC( pDC ) )
		{
			DrawDefault();
			return;
		}

		// As in the fade path, the cached face bitmap is both selected into the
		// scratch DC and passed as the BlendStates overlay -- one CBitmap object.
		CBitmap*	pOverlay = twoBitmap ? &m_bmpOverlayAlt : &m_bmpOverlay;
		CBitmap*	pScreen  = CBitmap::FromHandle( (HBITMAP)GetCurrentObject( pDC->GetSafeHdc(), OBJ_BITMAP ) );
		RECT		rc = { 0, 0, w, h };
		CBitmap*	pOldPre = preDC.SelectObject( pOverlay );
		BlendStates( &rc, &preDC, pDC, pOverlay, pScreen, 2 );
		preDC.SelectObject( pOldPre );
	}

	// Hover fade level (0..31) from the animation timers.
	if ( fadeOn && !IsHighlighted() )
	{
		double	elapsed   = m_timeCur - m_timeStart;
		float	fadeStart = m_fadeStart;
		float	fadeEnd   = m_fadeEnd;

		if ( elapsed >= fadeStart )
		{
			if ( elapsed >= (double)fadeEnd )
			{
				level = 31;
			}
			else
			{
				float	span = fadeEnd - fadeStart;
				double	t = ( span == 0.0f ) ? 100.0 : ( elapsed - fadeStart ) / span;
				level = (int)( t * 32.0 + 0.5 );
				if ( level < 0 )  level = 0;
				if ( level > 31 ) level = 31;
			}
		}
	}

	if ( bDisabled )
	{
		// Dimmed slice straight to screen.
		BlitStripSlice( pDC, 2, SRCCOPY );
	}
	else
	{
		CDC		memDC;
		if ( !memDC.CreateCompatibleDC( pDC ) )
		{
			DrawDefault();
			return;
		}

		CBitmap		memBmp;
		memBmp.CreateCompatibleBitmap( pDC, w, h );
		CBitmap*	pOldMem = memDC.SelectObject( &memBmp );

		{
			CBrush	bg( m_clrBg );
			memDC.FillRect( prc, &bg );
		}

		if ( !bFocus )
			BlitStripSlice( &memDC, 0, SRCCOPY );	// normal slice

		if ( fadeOn && !bFocus )
		{
			// Cross-fade the normal slice toward the down slice by `level`.
			CDC			downDC;
			CBitmap*	pScratch = twoBitmap ? &m_bmpOverlayAlt : &m_bmpOverlay;
			if ( downDC.CreateCompatibleDC( pDC ) )
			{
				CBitmap*	pOldDown = downDC.SelectObject( pScratch );
				BlitStripSlice( &downDC, 1, SRCCOPY );	// down slice -> scratch
				BlendSlice( &memDC, &memBmp, 0, &downDC, pScratch, level );
				downDC.SelectObject( pOldDown );
			}
		}
		else if ( bFocus )
		{
			// Focused: blit the down/over slice, then merge with the screen.
			CDC		ovDC;
			if ( ovDC.CreateCompatibleDC( pDC ) )
			{
				ovDC.SelectObject( twoBitmap ? &m_bmpOverlayAlt : &m_bmpOverlay );
				BlitStripSlice( &memDC, twoBitmap ? 1 : 2, SRCCOPY );
			}
		}

		// Blend the composed face with the screen content and present.
		{
			CBitmap*	pScreen = CBitmap::FromHandle( (HBITMAP)GetCurrentObject( pDC->GetSafeHdc(), OBJ_BITMAP ) );
			RECT		rc = { 0, 0, w, h };
			BlendStates( &rc, &memDC, pDC, &memBmp, pScreen, 2 );
		}

		memDC.SelectObject( pOldMem );
	}

	// Three-state buttons paint all three slices into the off-screen buffer.
	if ( m_b3State )
	{
		CDC		triDC;
		if ( triDC.CreateCompatibleDC( pDC ) )
		{
			CBitmap		triBmp;
			triBmp.CreateCompatibleBitmap( pDC, w, h );
			CBitmap*	pOldTri = triDC.SelectObject( &triBmp );
			int			slice;

			for ( slice = 0; slice < 3; slice++ )
			{
				CBrush	bg( m_clrBg );
				triDC.FillRect( prc, &bg );
				BlitStripSlice( &triDC, slice, SRCPAINT );
			}

			triDC.SelectObject( pOldTri );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::DrawButtonFace (0x440EC0)
//
// vtbl+192.

void CODBlendBtn::DrawButtonFace( CDC* pDC, CBitmap* /*pBmp*/, RECT* prc,
	int bFocus, int bSelected, int bDisabled )
{
	int			textYOffset	= m_textYOffset;
	UINT		textFlags	= m_textFlags;
	int			hasArrow	= m_bHasArrow;
	COLORREF	textColor	= m_clrText;
	int			fadeEnabled	= m_bFade;
	COLORREF	hoverColor	= m_clrHover;
	COLORREF	downColor	= m_clrDown;
	COLORREF	bgColor		= m_clrBg;
	CFont*		pMainFont	= &m_mainFont;
	CFont*		pShadowFont	= &m_shadowFont;
	CFont*		pOldFont;
	COLORREF	oldColor;
	COLORREF	drawColor;

	if ( !Launcher_MainButtonsLoaded() )
		return;

	// A "strip" button draws from the shared btns_main strip instead.
	if ( m_bStripMode )
	{
		DrawStripFace( pDC, NULL, prc, bSelected, bFocus, bDisabled );
		return;
	}

	GetWindowText( s_szButtonCaption, sizeof( s_szButtonCaption ) );

	pOldFont  = pDC->SelectObject( pMainFont );
	drawColor = bFocus ? hoverColor : downColor;
	oldColor  = pDC->SetTextColor( drawColor );
	pDC->SetBkMode( TRANSPARENT );

	// Drop shadow for the pressed-but-enabled state: the caption drawn in the
	// background colour, one font-offset down.
	if ( bSelected && !bDisabled )
	{
		CDC		shadowDC;
		if ( !shadowDC.CreateCompatibleDC( pDC ) )
		{
			DrawDefault();
			return;
		}

		CGdiObject*	pOldShadow = shadowDC.SelectObject( (CGdiObject*)&m_bmpOverlay );	// shadow mask
		pDC->BitBlt( 0, 0, prc->right - prc->left, prc->bottom - prc->top, &shadowDC, 0, 0, SRCPAINT );
		shadowDC.SelectObject( pOldShadow );

		pDC->SelectObject( pShadowFont );
		pDC->SetTextColor( bgColor );
		prc->top += textYOffset;
		InflateRect( prc, -2, 0 );
		pDC->DrawText( s_szButtonCaption, -1, prc, textFlags | DT_SINGLELINE );
		InflateRect( prc, 2, 0 );
		prc->top -= textYOffset;

		pDC->SelectObject( pMainFont );
		pDC->SetTextColor( drawColor );
	}

	// Mouse-over colour fade: lerp the text colour toward the hover colour over
	// the [+176,+172) time window.
	if ( fadeEnabled && !IsHighlighted() )
	{
		double	elapsed   = m_timeCur - m_timeStart;
		float	fadeStart = m_fadeStart;
		float	fadeEnd   = m_fadeEnd;

		if ( elapsed >= fadeStart )
		{
			if ( elapsed < (double)fadeEnd )
			{
				const BYTE*	from = (const BYTE*)&textColor;	// +164 base
				const BYTE*	to   = (const BYTE*)&drawColor;	// target (hover/down)
				float		span = fadeEnd - fadeStart;
				double		t = ( span == 0.0f ) ? 100.0 : ( elapsed - fadeStart ) / span;
				BYTE		lerp[3];
				int			i;

				for ( i = 0; i < 3; i++ )
					lerp[i] = (BYTE)( (double)from[i] - (double)( from[i] - to[i] ) * t );

				pDC->SetTextColor( RGB( lerp[0], lerp[1], lerp[2] ) );
			}
		}
		else
		{
			pDC->SetTextColor( textColor );
		}
	}

	// State colour / arrow.
	if ( bDisabled )
	{
		pDC->SetTextColor( downColor );
	}
	else if ( hasArrow )
	{
		COLORREF	clrArrow = bSelected ? m_clrArrowSel : m_clrArrowNorm;
		CODBlendBtn_DrawArrow( pDC, prc, clrArrow, bSelected );
	}

	// Main caption.
	prc->top += textYOffset;
	InflateRect( prc, -2, 0 );
	pDC->DrawText( s_szButtonCaption, -1, prc, textFlags | DT_SINGLELINE );
	InflateRect( prc, 2, 0 );
	prc->top -= textYOffset;

	pDC->SelectObject( pOldFont );
	pDC->SetTextColor( oldColor );
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::BlendSlice (0x441520)

int CODBlendBtn::BlendSlice( CDC* pDstDC, CBitmap* pBaseBmp,
	int /*a4*/, CDC* pOverlayDC, CBitmap* pOverlayBmp, int level )
{
	BYTE**	pBaseBuf	= &m_blendBufBase;
	BYTE**	pOverlayBuf	= &m_blendBufOverlay;
	int*	pBaseCap	= &m_blendCapBase;
	int*	pOverlayCap	= &m_blendCapOverlay;
	BITMAPINFO	bmi;
	BITMAP	bm;
	int		w, h, stride, size, x, y;

	GetObjectA( pBaseBmp->m_hObject, sizeof( bm ), &bm );
	w = bm.bmWidth;
	h = bm.bmHeight;

	memset( &bmi, 0, sizeof( bmi ) );
	stride = ( 3 * ( w + 1 ) ) & ~3;
	size = h * stride + sizeof( BITMAPINFOHEADER );
	bmi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;

	// Lazily (re)grow the two cached scratch buffers.
	if ( !*pBaseBuf || size > *pBaseCap )
	{
		if ( *pBaseBuf ) free( *pBaseBuf );
		*pBaseBuf = (BYTE*)malloc( size );
		*pBaseCap = size;
	}
	if ( !*pOverlayBuf || size > *pOverlayCap )
	{
		if ( *pOverlayBuf ) free( *pOverlayBuf );
		*pOverlayBuf = (BYTE*)malloc( size );
		*pOverlayCap = size;
	}

	GetDIBits( pDstDC ? pDstDC->GetSafeHdc() : NULL, (HBITMAP)pBaseBmp->m_hObject,
		0, h, *pOverlayBuf, &bmi, DIB_RGB_COLORS );
	GetDIBits( pOverlayDC ? pOverlayDC->GetSafeHdc() : NULL, (HBITMAP)pOverlayBmp->m_hObject,
		0, h, *pBaseBuf, &bmi, DIB_RGB_COLORS );

	for ( y = 0; y < h; y++ )
	{
		BYTE*	pBase    = *pOverlayBuf + y * stride;
		BYTE*	pOverlay = *pBaseBuf    + y * stride;

		for ( x = 0; x < w * 3; x++ )
		{
			int	v = ( 32 * pBase[x] + ( 31 - level ) * pOverlay[x] ) >> 5;
			if ( v > 255 )
				v = 255;
			pBase[x] = (BYTE)v;
		}
	}

	SetStretchBltMode( pDstDC ? pDstDC->GetSafeHdc() : NULL, COLORONCOLOR );
	return SetDIBitsToDevice( pDstDC ? pDstDC->GetSafeHdc() : NULL,
		0, 0, w, h, 0, 0, 0, h, *pOverlayBuf, &bmi, DIB_RGB_COLORS );
}

#define	CB_TEXT_DIMMED		RGB( 56, 56, 56 )

BEGIN_MESSAGE_MAP( CODBlendCheckBox, CODBlendBtn )
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODBlendCheckBox::EnsureSkin (0x441E70)

void CODBlendCheckBox::EnsureSkin()
{
	char	path[260];

	if ( m_bSkinLoaded )
		return;

	sprintf( path, "%s%s.bmp", "gfx/shell/", "cb_disabled" );
	m_dibDisabled = DIB_LoadBitmapFile( path );	// +276
	sprintf( path, "%s%s.bmp", "gfx/shell/", "cb_checked" );
	m_dibChecked = DIB_LoadBitmapFile( path );		// +260
	sprintf( path, "%s%s.bmp", "gfx/shell/", "cb_down" );
	m_dibDown = DIB_LoadBitmapFile( path );		// +264
	sprintf( path, "%s%s.bmp", "gfx/shell/", "cb_empty" );
	m_dibEmpty = DIB_LoadBitmapFile( path );		// +268
	sprintf( path, "%s%s.bmp", "gfx/shell/", "cb_over" );
	m_dibOver = DIB_LoadBitmapFile( path );		// +272

	m_bSkinLoaded = 1;									// +240
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendCheckBox::FreeSkin (0x441F80)

void CODBlendCheckBox::FreeSkin()
{
	if ( !m_bSkinLoaded )
		return;

	if ( m_dibDisabled )	::GlobalFree( m_dibDisabled );
	m_dibDisabled = NULL;
	if ( m_dibChecked )		::GlobalFree( m_dibChecked );
	m_dibChecked = NULL;
	if ( m_dibDown )		::GlobalFree( m_dibDown );
	m_dibDown = NULL;
	if ( m_dibEmpty )		::GlobalFree( m_dibEmpty );
	m_dibEmpty = NULL;
	if ( m_dibOver )		::GlobalFree( m_dibOver );
	m_dibOver = NULL;

	m_bSkinLoaded = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendCheckBox::CODBlendCheckBox (0x442010)

CODBlendCheckBox::CODBlendCheckBox()
{
	m_bTransparent = 1;		// +244  default: composite over the parent
	m_bCapturing   = 0;		// +248
	m_bChecked     = 0;		// +252
	m_textFlags    = DT_SINGLELINE;	// +256  (binary literal 32)
	m_bSkinLoaded  = 0;		// +240

	m_dibChecked   = NULL;	// +260
	m_dibDown      = NULL;	// +264
	m_dibEmpty     = NULL;	// +268
	m_dibOver      = NULL;	// +272
	m_dibDisabled  = NULL;	// +276
	m_clrBg        = 0;		// +280

	m_font.Attach( ::CreateFontA( -12, 0, 0, 0, 400, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, FF_ROMAN, "Arial" ) );
	m_smallFont.Attach( ::CreateFontA( -11, 0, 0, 0, 400, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, FF_ROMAN, "Arial" ) );

	EnsureSkin();			// prime the glyph cache (sub_441E70)
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendCheckBox::~CODBlendCheckBox (0x442140)

CODBlendCheckBox::~CODBlendCheckBox()
{
	FreeSkin();				// sub_441F80
	m_font.DeleteObject();
	m_smallFont.DeleteObject();
}

// CODBlendCheckBox::DrawItem (0x4421D0, vtbl tail 0x4B0E64)
void CODBlendCheckBox::DrawItem( LPDRAWITEMSTRUCT lpDIS )
{
	if ( !m_bSkinLoaded )		// glyphs never loaded -> nothing to draw (+240)
		return;

	RECT	rc;
	CopyRect( &rc, &lpDIS->rcItem );

	CDC*	pItemDC = CDC::FromHandle( lpDIS->hDC );

	POINT	pt;
	GetCursorPos( &pt );
	ScreenToClient( &pt );
	int		bChecked = m_bChecked;					// +252
	BOOL	bHover   = PtInRect( &rc, pt );
	BOOL	bPressed = ( bHover && ( GetAsyncKeyState( VK_LBUTTON ) != 0 ) );

	int		w = rc.right - rc.left;
	int		h = rc.bottom - rc.top;

	CDC		memDC;
	memDC.CreateCompatibleDC( pItemDC );
	if ( !memDC.GetSafeHdc() )
	{
		DrawDefault();			// CODBlendBtn::DrawDefault (0x441DF0)
		return;
	}

	CBitmap	bmMem;
	bmMem.CreateCompatibleBitmap( pItemDC, w, h );
	CBitmap*	pOldBmp = memDC.SelectObject( &bmMem );

	// - background ---
	if ( m_bTransparent )		// +244
	{
		RECT	rcWnd, rcParent;
		GetWindowRect( &rcWnd );	ScreenToClient( &rcWnd );
		GetWindowRect( &rcParent );
		if ( GetParent() )
			GetParent()->ScreenToClient( &rcParent );
		Launcher_CopyParentBackground( &memDC, &rcWnd, &rcParent );
	}
	else
	{
		CBrush	br( m_clrBg );		// +280
		memDC.FillRect( &rc, &br );
	}

	// - glyph (19x19 cell at the left) ---
	RECT	rcGlyph = { rc.left + 2, rc.top + 2, rc.left + 21, rc.top + 21 };
	rc.left += 23;					// the caption starts past the glyph

	HGLOBAL	hGlyph;
	if ( IsHighlighted() )			// CODBlendBtn::IsHighlighted (0x441D70)
		hGlyph = m_dibDisabled;		// +276
	else if ( bPressed )
		hGlyph = m_dibDown;			// +264
	else if ( bChecked )
		hGlyph = m_dibChecked;		// +260
	else if ( bHover )
		hGlyph = m_dibOver;			// +272
	else
		hGlyph = m_dibEmpty;		// +268

	if ( hGlyph )
	{
		RECT	rcSrc = { 0, 0, rcGlyph.right - rcGlyph.left,
						  rcGlyph.bottom - rcGlyph.top };
		DIB_BlitDib( memDC.GetSafeHdc(), &rcGlyph, hGlyph, &rcSrc );
	}

	// - caption ---
	char	szText[260];
	GetWindowText( szText, sizeof( szText ) );

	CFont*		pOldFont = memDC.SelectObject( &m_font );
	COLORREF	clrOld   = memDC.SetTextColor( m_clrDown );	// base +208 text colour
	if ( IsHighlighted() )
		memDC.SetTextColor( CB_TEXT_DIMMED );
	memDC.SetBkMode( TRANSPARENT );
	memDC.DrawText( szText, -1, &rc, m_textFlags | DT_VCENTER );	// +256
	memDC.SetTextColor( clrOld );
	memDC.SelectObject( pOldFont );

	// - present ---
	pItemDC->BitBlt( 0, 0, w, h, &memDC, 0, 0, SRCCOPY );
	memDC.SelectObject( pOldBmp );
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendCheckBox::OnLButtonDown (0x442620)

void CODBlendCheckBox::OnLButtonDown( UINT, CPoint )
{
	if ( IsHighlighted() )		// disabled / forced-dim: swallow it
		return;

	Snd_PlayMenuSound( UISND_GLOW );
	m_bCapturing = 1;
	SetFocus();
	SetCapture();
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendCheckBox::OnLButtonUp (0x442660)

void CODBlendCheckBox::OnLButtonUp( UINT /*nFlags*/, CPoint pt )
{
	if ( IsHighlighted() || !m_bCapturing )
		return;

	Snd_PlayMenuSound( UISND_GLOW );
	m_bCapturing = 0;
	ReleaseCapture();

	RECT	rc;
	GetClientRect( &rc );
	if ( !PtInRect( &rc, pt ) )
		return;

	m_bChecked = ( m_bChecked == 0 );			// +252 toggle

	CWnd*	pParent = GetParent();
	if ( pParent )
	{
		UINT	nID = GetDlgCtrlID();
		pParent->SendMessage( WM_COMMAND, (WPARAM)nID, (LPARAM)GetSafeHwnd() );
	}

	InvalidateRect( NULL, TRUE );
}

// CODBlendCheckBox::SetFontSize (0x442760, vtbl tail 0x4B0E68)
void CODBlendCheckBox::SetFontSize( int nSize, int nWeight )
{
	m_font.DeleteObject();
	m_font.Attach( ::CreateFontA( -nSize, 0, 0, 0, nWeight, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, FF_ROMAN, "Arial" ) );
}

// CODBlendCheckBox::SetCheck
void CODBlendCheckBox::SetCheck( int bChecked )
{
	m_bChecked = ( bChecked != 0 );
	if ( GetSafeHwnd() )
		InvalidateRect( NULL, TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendBtn::FreeSkinBitmaps (0x441DB0)
//
// drop both blended bitmaps so the next paint re-slices them from the
// strip.

void CODBlendBtn::FreeSkinBitmaps()
{
	m_bSkinDirty = 1;

	HGDIOBJ	hOverlay = m_bmpOverlay.Detach();
	if ( hOverlay )
		::DeleteObject( hOverlay );

	HGDIOBJ	hAlt = m_bmpOverlayAlt.Detach();
	if ( hAlt )
		::DeleteObject( hAlt );
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendCheckBox::SetTransparent (0x442740)
//
// hides the base method.

void CODBlendCheckBox::SetTransparent( BOOL bOn )
{
	m_bTransparent = bOn;
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendCheckBox::SetBkColor (0x442750)
//
// hides the base method.

void CODBlendCheckBox::SetBkColor( COLORREF clr )
{
	m_clrBg = clr;
}

BEGIN_MESSAGE_MAP( CODBlendStatic, CButton )
	//{{AFX_MSG_MAP(CODBlendStatic)
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_DRAWITEM()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODBlendStatic::OnMouseMove (0x4427B0)

void CODBlendStatic::OnMouseMove( UINT, CPoint )
{
	if ( !m_bFade )
	{
		if ( SetTimer( 1, 150, NULL ) )
			m_bFade = 1;
	}
	m_timeStart = engineapi.Sys_FloatTime();
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODBlendStatic::OnTimer (0x4427F0)

void CODBlendStatic::OnTimer( UINT_PTR nIDEvent )
{
	RECT	rc;
	POINT	pt;

	if ( nIDEvent == 1 )
	{
		m_timeCur = engineapi.Sys_FloatTime();
		::GetWindowRect( m_hWnd, &rc );
		GetCursorPos( &pt );
		if ( PtInRect( &rc, pt ) )
			m_timeStart = m_timeCur;

		if ( m_timeCur - m_timeStart >= (double)m_fadeEnd )
		{
			KillTimer( 1 );
			m_bFade = 0;
		}

		RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
	}
	Default();
}
