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
// Purpose: CVideoDlg, the Video options page (IDD 0xA1 = 161) -- screen size,
//          gamma and glare, with the gamma preview.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

static void	Vid_GetCtlParentRect( CWnd* pDlg, CWnd* pCtl, RECT* prc );

BEGIN_MESSAGE_MAP( CVideoDlg, CDialog )
	//{{AFX_MSG_MAP(CVideoDlg)
	ON_MESSAGE( WM_DISPLAYCHANGE, &CVideoDlg::OnDisplayChange )
	ON_REGISTERED_MESSAGE( g_uiScrollMsg, OnSliderScroll )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
	ON_COMMAND( IDC_SKIP_SPRITE, OnSpriteSkipCheck )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::CVideoDlg (0x46A220)

CVideoDlg::CVideoDlg( CWnd* pParent )
	: CDlgBase( IDD_VIDEO, pParent )
{
	char	szPath[256];
	LPBITMAPINFOHEADER	pDIB;
	int		dims[2];

	m_pSelfWnd = this;
	m_hGammaDib = NULL;

	sprintf( szPath, "gfx/shell/gamma.bmp" );
	m_hGammaDib = DIB_LoadBitmapFile( szPath );
	if ( m_hGammaDib )
	{
		pDIB = (LPBITMAPINFOHEADER)GlobalLock( m_hGammaDib );
		m_rcGamma.left = 0;
		m_rcGamma.top = 0;
		m_rcGamma.right = DIB_Width( pDIB );
		m_rcGamma.bottom = DIB_Height( pDIB );
		GlobalUnlock( m_hGammaDib );
	}
	else
	{
		Launcher_ShowMessageById( 0, IDS_GAMMA_LOADFAIL );
		m_rcGamma.left = 0;
		m_rcGamma.top = 0;
		m_rcGamma.right = 100;
		m_rcGamma.bottom = 100;
	}

	LoadHeaderBitmap( "head_vidoptions", NULL );
	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
		m_btnDone.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DONE, m_headerLoaded );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::~CVideoDlg (0x46A440)

CVideoDlg::~CVideoDlg()
{
	if ( m_hGammaDib )
		GlobalFree( m_hGammaDib );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::DoDataExchange (0x46A520)

void CVideoDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_SKIP_SPRITE, m_checkSpriteSkip );
	DDX_Control( pDX, IDC_VIDEO_GAMMAHELP, m_lblGammaHelp );
	DDX_Control( pDX, IDC_VIDEO_GLAREHELP, m_lblGlareHelp );
	DDX_Control( pDX, IDC_VIDEO_SCREENSIZE, m_lblScreenSize );
	DDX_Control( pDX, IDC_VIDEO_GAMMA, m_lblGamma );
	DDX_Control( pDX, IDC_VIDEO_GLARE, m_lblGlare );
	DDX_Control( pDX, IDC_VIDEO_GAMMAIMAGE, m_imgGamma );
	DDX_Control( pDX, IDOK, m_btnDone );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::OnInitDialog (0x46A5D0)

BOOL CVideoDlg::OnInitDialog()
{
	RECT	rc;
	CGameClientConfig*	cfg;
	int		dims[2];
	int		w, h, gammaW, gammaH, right, ofs, helpX, helpW, y;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	// The image control is a geometry anchor only -- DrawGammaPreview paints
	// the preview onto the dialog, so the control itself stays hidden.
	m_imgGamma.ShowWindow( SW_HIDE );

	rc.left = 0;
	rc.top = 0;
	rc.right = 100;
	rc.bottom = 100;
	m_pScreenSizeSlider = new CODSlider;
	m_pScreenSizeSlider->Create( this, &rc );
	m_pGammaSlider = new CODSlider;
	m_pGammaSlider->Create( this, &rc );
	m_pGlareSlider = new CODSlider;
	m_pGlareSlider->Create( this, &rc );

	if ( !g_pServerBrowser )
	{
		Launcher_ShowMessageById( 0, IDS_VIDEO_NOPROFILE );
		OnCancel();
		return TRUE;
	}

	cfg = &g_pServerBrowser->m_playerConfig;
	m_screenSize = cfg->viewsize * 0.1f;
	m_gamma = cfg->gamma * 10.0f;
	m_glare = cfg->brightness * 10.0f;
	BuildGammaRamp();

	m_pScreenSizeSlider->SetRange( 3, 12 );
	m_pScreenSizeSlider->SetPos( (int)m_screenSize );
	m_pGammaSlider->SetRange( 18, 30 );
	m_pGammaSlider->SetPos( (int)m_gamma );
	m_pGlareSlider->SetRange( 0, 10 );
	m_pGlareSlider->SetPos( (int)m_glare );

	Launcher_HeaderSize( dims );
	w = dims[0];
	h = dims[1];
	gammaW = m_rcGamma.right - m_rcGamma.left;
	gammaH = m_rcGamma.bottom - m_rcGamma.top;
	right = g_nLauncherDefW - 70;
	ofs = Launcher_StringHeight( IDS_VIDEODLG_OFFSET, 0 );

	// The three label/slider rows down the left edge.
	m_lblScreenSize.MoveWindow( 50, 140, w + ofs, h, TRUE );
	m_lblScreenSize.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblScreenSize.SetFontSize( 14, FW_HEAVY );
	m_lblScreenSize.SetWindowText( Launcher_LoadString( IDS_VIDEO_SCREENSIZE ) );
	m_imgGamma.MoveWindow( w + 90, 140, gammaW, gammaH, TRUE );
	m_pScreenSizeSlider->MoveWindow( 50, 159, w, h, TRUE );

	m_lblGamma.MoveWindow( 50, 191, w, h, TRUE );
	m_lblGamma.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblGamma.SetFontSize( 14, FW_HEAVY );
	m_lblGamma.SetWindowText( Launcher_LoadString( IDS_VIDEO_GAMMA ) );
	m_pGammaSlider->MoveWindow( 50, 210, w, h, TRUE );

	m_lblGlare.MoveWindow( 50, 242, w + ofs, h, TRUE );
	m_lblGlare.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblGlare.SetFontSize( 14, FW_HEAVY );
	m_lblGlare.SetWindowText( Launcher_LoadString( IDS_VIDEO_GLARE ) );
	m_pGlareSlider->MoveWindow( 50, 261, w, h, TRUE );

	m_btnDone.MoveWindow( 50, 293, w, h, TRUE );
	SetWindowTextSafe( &m_btnDone, Launcher_LoadString( IDS_BTN_DONE ) );

	// Two HELP_COLORed paragraphs, laid out and then hidden -- the page ships
	// without them.
	helpX = w + 90;
	helpW = right - helpX;
	m_lblGammaHelp.MoveWindow( helpX, gammaH + 146, helpW, 44, TRUE );
	m_lblGammaHelp.SetTransparent( TRUE );
	m_lblGammaHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblGammaHelp.SetFontSize( 11, FW_NORMAL );
	m_lblGammaHelp.SetWindowText( Launcher_LoadString( IDS_VIDEO_GAMMAHELP ) );
	m_lblGammaHelp.ShowWindow( SW_HIDE );

	m_lblGlareHelp.MoveWindow( helpX, gammaH + 49, helpW, 2 * h - 6, TRUE );
	m_lblGlareHelp.SetTransparent( TRUE );
	m_lblGlareHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblGlareHelp.SetFontSize( 11, FW_NORMAL );
	m_lblGlareHelp.SetWindowText( Launcher_LoadString( IDS_VIDEO_GLAREHELP ) );
	m_lblGlareHelp.ShowWindow( SW_HIDE );

	// The sprite-skip check box along the bottom.
	y = g_nLauncherDefH - 50;
	Launcher_HeaderSize( dims );
	m_bSpriteSkip = ( cfg->d_spriteskip != 0.0f );
	SetWindowLongA( m_checkSpriteSkip.GetSafeHwnd(), GWL_STYLE,
		m_checkSpriteSkip.GetStyle() | BS_OWNERDRAW );
	m_checkSpriteSkip.m_textFlags = DT_WORDBREAK;
	m_checkSpriteSkip.MoveWindow( 50, y, 2 * w, dims[1], TRUE );
	m_checkSpriteSkip.m_bChecked = m_bSpriteSkip;
	::InvalidateRect( m_checkSpriteSkip.GetSafeHwnd(), NULL, TRUE );
	SetWindowTextSafe( &m_checkSpriteSkip, Launcher_LoadString( IDS_SPRITE_SKIP ) );

	return TRUE;
}

/*
==================
Vid_ComputeScreenRect (0x46ABB0)

Place the engine's viewport inside prcOuter for a 30..120 percent screen size,
leaving room for the status bar below it.
==================
*/
static void Vid_ComputeScreenRect( int sizePct, RECT* prcOuter, RECT* prcInner )
{
	int		outerW = prcOuter->right - prcOuter->left;
	int		outerH = prcOuter->bottom - prcOuter->top;
	int		bar = 0;
	float	scale, size;
	int		w, h;

	if ( sizePct < 30 )
		sizePct = 30;
	if ( sizePct > 120 )
		sizePct = 120;
	if ( sizePct < 120 )
		bar = (int)( (float)outerH * ( sizePct >= 110 ? 0.12f : 0.24f ) );

	size = (float)sizePct;
	if ( size > 100.0f )
		size = 100.0f;
	scale = size * 0.01f;

	w = (int)( (float)outerW * scale );
	if ( w < 96 )
	{
		scale = 96.0f / (float)outerW;
		w = 96;
	}
	w &= ~7;

	h = (int)( (float)outerH * scale );
	if ( h > outerH - bar )
		h = outerH - bar;
	h &= ~1;

	prcInner->left   = ( outerW - w ) / 2;
	prcInner->top    = ( outerH - bar - h ) / 2;
	prcInner->right  = prcInner->left + w;
	prcInner->bottom = prcInner->top + h;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::SliderScrolled (0x46ACE0)

void CVideoDlg::SliderScrolled( int nSBCode, int nPos, CObject* pObj )
{
	CWnd*	pSlider = DYNAMIC_DOWNCAST( CWnd, pObj );

	if ( pSlider && pSlider->GetSafeHwnd() == m_pScreenSizeSlider->GetSafeHwnd() )
	{
		if ( nSBCode == SB_ENDSCROLL || nSBCode == SB_THUMBTRACK )
			RedrawGammaImage();
		if ( nSBCode == SB_ENDSCROLL )
			m_screenSize = (float)nPos;
		return;
	}

	if ( pSlider && pSlider->GetSafeHwnd() == m_pGammaSlider->GetSafeHwnd() )
	{
		if ( nSBCode == SB_ENDSCROLL || nSBCode == SB_THUMBTRACK )
		{
			m_gamma = (float)nPos;
			BuildGammaRamp();
			RedrawGammaImage();
		}
		return;
	}

	if ( pSlider && pSlider->GetSafeHwnd() == m_pGlareSlider->GetSafeHwnd() )
	{
		if ( nSBCode == SB_ENDSCROLL || nSBCode == SB_THUMBTRACK )
		{
			m_glare = (float)nPos;
			BuildGammaRamp();
			RedrawGammaImage();
		}
		return;
	}

	CWnd::OnVScroll( nSBCode, nPos, (CScrollBar*)pObj );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::BuildGammaRamp (0x46AE20)

void CVideoDlg::BuildGammaRamp()
{
	float	gamma = m_gamma * 0.1f;
	float	glare = m_glare * 0.1f;
	float	exponent, knee;
	double	f, v;
	int		i, level;

	if ( gamma < 1.0f )
		gamma = 1.0f;
	else if ( gamma > 3.0f )
		gamma = 3.0f;
	exponent = 2.5f / gamma;

	if ( glare <= 0.0f )
		knee = 0.125f;
	else if ( glare <= 1.0f )
		knee = 0.125f - glare * glare * 0.075f;
	else
		knee = 0.05f;

	for ( i = 0; i < 256; i++ )
	{
		f = pow( (double)i * 0.0039215689, exponent );		// i / 255
		if ( glare > 1.0f )
			f = f * glare;
		if ( f > knee )
			v = ( f - knee ) / ( 1.0 - knee ) * 0.875 + 0.125;
		else
			v = f / knee * 0.125;

		level = (int)( v * 255.0 );
		if ( level < 0 )
			level = 0;
		if ( level > 255 )
			level = 255;
		m_gammaTable[i] = (BYTE)level;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::DrawGammaDib (0x46AF70)
//
// Blit the preview bitmap through m_gammaTable and stretch it into prcDest.

void CVideoDlg::DrawGammaDib( HGLOBAL hDib, CDC* pDC, RECT* prcDest )
{
	CDC			dc;					// (sic) constructed and never used
	CDC			memDC;
	CBitmap		bmp;
	CBitmap*	pOldBmp;
	BITMAPINFO	bmi;
	LPBITMAPINFOHEADER	pDIB;
	BYTE		*pBits, *pOut, *pSrc, *pDst;
	int			w, h, stride, y, x;

	pDIB = (LPBITMAPINFOHEADER)GlobalLock( hDib );
	if ( !pDIB )
		return;
	w = DIB_Width( pDIB );
	h = DIB_Height( pDIB );
	GlobalUnlock( hDib );

	if ( !memDC.CreateCompatibleDC( pDC ) )
		return;
	if ( !bmp.CreateCompatibleBitmap( pDC, w, h ) )
		return;
	pOldBmp = memDC.SelectObject( &bmp );

	DIB_BlitDib( memDC.GetSafeHdc(), &m_rcGamma, hDib, &m_rcGamma );

	memset( &bmi, 0, sizeof( bmi ) );
	bmi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	stride = ( 3 * w + 3 ) & ~3;

	pBits = new BYTE[h * stride + 40];
	GetDIBits( memDC.GetSafeHdc(), (HBITMAP)bmp.GetSafeHandle(), 0, h, pBits, &bmi, DIB_RGB_COLORS );
	pOut = new BYTE[h * stride + 40];

	for ( y = 0; y < h; y++ )
	{
		pSrc = pBits + y * stride;
		pDst = pOut + y * stride;
		for ( x = 0; x < w; x++ )
		{
			*pDst++ = m_gammaTable[*pSrc++];
			*pDst++ = m_gammaTable[*pSrc++];
			*pDst++ = m_gammaTable[*pSrc++];
		}
	}

	SetStretchBltMode( memDC.GetSafeHdc(), COLORONCOLOR );
	SetDIBitsToDevice( memDC.GetSafeHdc(), 0, 0, w, h, 0, 0, 0, h, pOut, &bmi, DIB_RGB_COLORS );

	SetStretchBltMode( pDC->GetSafeHdc(), COLORONCOLOR );
	StretchBlt( pDC->GetSafeHdc(), prcDest->left, prcDest->top,
		prcDest->right - prcDest->left, prcDest->bottom - prcDest->top,
		memDC.GetSafeHdc(), 0, 0, w, h, SRCCOPY );

	memDC.SelectObject( pOldBmp );
	memDC.DeleteDC();
	delete[] pBits;
	delete[] pOut;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::DrawGammaPreview (0x46B310)

void CVideoDlg::DrawGammaPreview( CDC* pDC )
{
	RECT	rcOuter, rcInner;

	Vid_GetCtlParentRect( this, &m_imgGamma, &rcOuter );
	Vid_ComputeScreenRect( 10 * m_pScreenSizeSlider->GetPos(), &rcOuter, &rcInner );

	FrameRect( pDC->GetSafeHdc(), &rcOuter, CBrush( RGB( 255, 0, 0 ) ) );
	InflateRect( &rcOuter, -1, -1 );
	FillRect( pDC->GetSafeHdc(), &rcOuter, CBrush( RGB( 63, 63, 63 ) ) );

	// The binary round-trips rcInner through the parent's ScreenToClient /
	// ClientToScreen (0x49C97A / 0x49C9B6); reproducing that here puts the
	// preview in the wrong place, so keep the offset.  Do not "fix" this.
	OffsetRect( &rcInner, rcOuter.left, rcOuter.top );
	DrawGammaDib( m_hGammaDib, pDC, &rcInner );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::OnOK (0x46B450)

void CVideoDlg::OnOK()
{
	CGameClientConfig*	cfg = &g_pServerBrowser->m_playerConfig;

	cfg->viewsize = m_screenSize * 10.0f;
	cfg->gamma = m_gamma * 0.1f;
	cfg->brightness = m_glare * 0.1f;
	cfg->d_spriteskip = m_bSpriteSkip ? 1.0f : 0.0f;
	Launcher_SavePlayerInfoTo( "Player", &g_pServerBrowser->m_playerConfig );

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::RedrawGammaImage (0x46B4D0)

void CVideoDlg::RedrawGammaImage()
{
	RECT		rcImg, rcClient;
	CDC*		pDC;
	CDC			memDC;
	CBitmap		bmp;
	CBitmap*	pOldBmp;

	pDC = GetDC();
	::GetWindowRect( m_imgGamma.GetSafeHwnd(), &rcImg );
	ScreenToClient( &rcImg );
	::GetClientRect( GetSafeHwnd(), &rcClient );

	if ( memDC.CreateCompatibleDC( pDC ) )
	{
		bmp.CreateCompatibleBitmap( pDC, rcClient.right - rcClient.left,
			rcClient.bottom - rcClient.top );
		pOldBmp = memDC.SelectObject( &bmp );
		DrawGammaPreview( &memDC );
		SetStretchBltMode( pDC->GetSafeHdc(), COLORONCOLOR );
		StretchBlt( pDC->GetSafeHdc(), rcImg.left, rcImg.top,
			rcImg.right - rcImg.left, rcImg.bottom - rcImg.top,
			memDC.GetSafeHdc(), rcImg.left, rcImg.top,
			rcImg.right - rcImg.left, rcImg.bottom - rcImg.top, SRCCOPY );
		memDC.SelectObject( pOldBmp );
		memDC.DeleteDC();
	}
	ReleaseDC( pDC );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::OnSliderScroll (0x46B6B0)
//
// wParam is MAKEWPARAM( SB_code, pos ), lParam the sender.

LRESULT CVideoDlg::OnSliderScroll( WPARAM wParam, LPARAM lParam )
{
	SliderScrolled( LOWORD( wParam ), HIWORD( wParam ), (CObject*)lParam );
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::OnSpriteSkipCheck (0x46B6E0)

void CVideoDlg::OnSpriteSkipCheck()
{
	m_bSpriteSkip = m_checkSpriteSkip.m_bChecked;
}

/*
==================
Vid_GetCtlParentRect (0x46B6F0)

A child control's rect in the client space of the dialog's parent, where the
skin paint composites.
==================
*/
static void Vid_GetCtlParentRect( CWnd* pDlg, CWnd* pCtl, RECT* prc )
{
	if ( pDlg && pCtl )
	{
		::GetWindowRect( pCtl->GetSafeHwnd(), prc );
		CWnd::FromHandle( ::GetParent( pDlg->GetSafeHwnd() ) )->ScreenToClient( prc );
	}
	else
	{
		prc->left = 0;
		prc->top = 0;
		prc->right = 100;
		prc->bottom = 50;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::DrawDialogOverlay (0x46B750)

void CVideoDlg::DrawDialogOverlay( CDC* pDC, RECT* /*prc*/ )
{
	DrawGammaPreview( pDC );
#ifdef LAUNCHER_RE
	Launcher_DrawBuildMarker( pDC );		// this slot replaces CDlgBase's, which draws it
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::OnActivateApp (0x406FE0)

void CVideoDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::OnPaint (0x412860)

void CVideoDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::OnEraseBkgnd (0x412870)

BOOL CVideoDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoDlg::OnDisplayChange (0x497E09)
//
// The binary binds the entry straight to MFC 4.2's own
// CWnd::OnDisplayChange( WPARAM, LPARAM ).  A modern MFC spells that
// handler void( UINT, int, int ), so it cannot be bound here; this
// forwarder reaches the same default handling.

LRESULT CVideoDlg::OnDisplayChange( WPARAM, LPARAM )
{
	return Default();
}
