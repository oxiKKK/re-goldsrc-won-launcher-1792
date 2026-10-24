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
// Purpose: CDlgBase, the skinned dialog base and its slide animation.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// WM_SYSTIMER is an undocumented internal timer message, not in <winuser.h>.
// The modal loop treats it, like WM_SYSKEYDOWN, as a late-show trigger.
#ifndef WM_SYSTIMER
#define WM_SYSTIMER		0x0118
#endif

extern "C" DWORD __stdcall timeGetTime( void );		// from winmm

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::RunModalLoop (0x4099F0)

int CDlgBase::RunModalLoop( DWORD dwFlags )
{
	BOOL	bIdle = TRUE;			// MFC idle protocol latch
	LONG	lIdleCount = 0;
	BOOL	bShowIdle = ( dwFlags & MLF_SHOWONIDLE ) && !( GetStyle() & WS_VISIBLE );
	HWND	hWndParent = ::GetParent( m_hWnd );
#if defined(_MSC_VER) && (_MSC_VER < 1300)
	MSG*	pMsg = &( AfxGetThread()->m_msgCur );		// VC6: m_msgCur lives on CWinThread
#else
	MSG*	pMsg = &( AfxGetThreadState()->m_msgCur );
#endif
	int		nFrame;

	m_nFlags |= ( WF_MODALLOOP | WF_CONTINUEMODAL );

	RMLSetup();

	for ( ;; )
	{
		// frame burst until the dialog reports idle (or quits)
		do
		{
			nFrame = RMLPreIdle();
			if ( nFrame < 0 )
				goto ExitModal;
		} while ( nFrame > 0 );

		// the stock MFC idle protocol, once per idle stretch
		if ( bIdle )
		{
			while ( !::PeekMessage( pMsg, NULL, NULL, NULL, PM_NOREMOVE ) )
			{
				if ( bShowIdle )
				{
					ShowWindow( SW_SHOWNORMAL );
					UpdateWindow();
					bShowIdle = FALSE;
				}
				RMLIdle();
				VGui_Frame();
				if ( !( dwFlags & MLF_NOIDLEMSG ) && hWndParent != NULL && lIdleCount == 0 )
					::SendMessage( hWndParent, WM_ENTERIDLE, MSGF_DIALOGBOX, (LPARAM)m_hWnd );
				if ( ( dwFlags & MLF_NOKICKIDLE ) ||
					 !SendMessage( WM_KICKIDLE, MSGF_DIALOGBOX, lIdleCount++ ) )
				{
					// no more idle work wanted
					bIdle = FALSE;
					break;
				}
			}
		}
		RMLPrePump();

		// while the frame pump is on, keep ticking instead of blocking
		while ( m_bFramePump && !::PeekMessage( pMsg, NULL, NULL, NULL, PM_NOREMOVE ) )
		{
			RMLPostPump();
			do
			{
				nFrame = RMLPreIdle();
				if ( nFrame < 0 )
					goto ExitModal;
			} while ( nFrame > 0 );
		}

		// pump the queue (blocks here when idle and the pump flag is off)
		do
		{
			if ( !AfxGetThread()->PumpMessage() )
			{
				// WM_QUIT -- forward it and bail
				AfxPostQuitMessage( 0 );
				return -1;
			}
			RMLPump();

			// late show: certain messages force the window up
			if ( bShowIdle && ( pMsg->message == WM_SYSTIMER ||
								pMsg->message == WM_SYSKEYDOWN ) )
			{
				ShowWindow( SW_SHOWNORMAL );
				UpdateWindow();
				bShowIdle = FALSE;
			}

			if ( !ContinueModal() )
				goto ExitModal;

			if ( AfxGetThread()->IsIdleMessage( pMsg ) )
			{
				bIdle = TRUE;
				lIdleCount = 0;
			}
		} while ( ::PeekMessage( pMsg, NULL, NULL, NULL, PM_NOREMOVE ) );

		// queue just drained -- service deferred work, then frame again
		RMLPostPump();
	}

ExitModal:
	m_nFlags &= ~( WF_MODALLOOP | WF_CONTINUEMODAL );
	return m_nModalResult;
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::DoModal (0x409BF0)

#if defined(_MSC_VER) && (_MSC_VER < 1300)
int CDlgBase::DoModal()			// VC6: CDialog::DoModal returns int
#else
INT_PTR CDlgBase::DoModal()
#endif
{
	// resolve the dialog template exactly as CDialog does
	LPCDLGTEMPLATE lpDialogTemplate = m_lpDialogTemplate;
	HGLOBAL hDialogTemplate = m_hDialogTemplate;
	HINSTANCE hInst = AfxGetResourceHandle();
	if ( m_lpszTemplateName != NULL )
	{
		hInst = AfxFindResourceHandle( m_lpszTemplateName, RT_DIALOG );
		HRSRC hResource = ::FindResource( hInst, m_lpszTemplateName, RT_DIALOG );
		hDialogTemplate = ::LoadResource( hInst, hResource );
	}
	if ( hDialogTemplate != NULL )
		lpDialogTemplate = (LPCDLGTEMPLATE)::LockResource( hDialogTemplate );
	if ( lpDialogTemplate == NULL )
		return -1;

	// disable the parent for the modal stretch
	HWND hWndParent = PreModal();
	BOOL bEnableParent = FALSE;
	if ( hWndParent != NULL && ::IsWindowEnabled( hWndParent ) )
	{
		::EnableWindow( hWndParent, FALSE );
		bEnableParent = TRUE;
	}

	TRY
	{
		// create the dialog modeless, then run the frame-protocol loop over it
		if ( CreateDlgIndirect( lpDialogTemplate,
				CWnd::FromHandle( hWndParent ), hInst ) )
		{
			if ( m_nFlags & WF_CONTINUEMODAL )
			{
				DWORD dwFlags = MLF_SHOWONIDLE;
				if ( GetStyle() & DS_NOIDLEMSG )
					dwFlags |= MLF_NOIDLEMSG;
				m_nModalResult = RunModalLoop( dwFlags );
			}

			// hide before destruction so the teardown never paints
			if ( m_hWnd != NULL )
				SetWindowPos( NULL, 0, 0, 0, 0, SWP_HIDEWINDOW |
					SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER );
		}
	}
	END_TRY

	if ( bEnableParent )
		::EnableWindow( hWndParent, TRUE );
	if ( hWndParent != NULL && ::GetActiveWindow() == m_hWnd )
		::SetActiveWindow( hWndParent );

	DestroyWindow();
	PostModal();
	return m_nModalResult;
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::CDlgBase (0x409DA0)

CDlgBase::CDlgBase( UINT nIDTemplate, CWnd* pParent )
	: CDialog( nIDTemplate, pParent )
{
	m_hHeaderDib = NULL;
	m_nHdrPad    = 0;
	m_bFramePump = FALSE;

	// The binary stops there and leaves the transition context uninitialised,
	// because each page's own ctor fills in what it uses.  Not every page in
	// this partial tree does, and RestoreAfterModal only tests the three
	// pointers for null -- against stack garbage it runs the slide over bad
	// DCs and the parent page comes back black.  So zero them here.
	m_pdcParent   = NULL;
	m_pdcWork     = NULL;
	m_pdcSaved    = NULL;
	m_pbmWork     = NULL;
	m_pbmWorkOld  = NULL;
	m_pbmSaved    = NULL;
	m_pbmSavedOld = NULL;
	m_pWndSlide2  = NULL;
	m_hHdrDibDst  = NULL;
	m_hHdrDibOwn  = NULL;
	m_pSelfWnd    = NULL;
	m_pSlideWnd   = NULL;
	m_prcTargetHdr   = NULL;
	m_phTargetHdrDib = NULL;
	memset( &m_rcUnion, 0, sizeof( m_rcUnion ) );
	memset( &m_rcHdrDst, 0, sizeof( m_rcHdrDst ) );
	memset( &m_rcHdrOwn, 0, sizeof( m_rcHdrOwn ) );
	memset( &m_rcHeader, 0, sizeof( m_rcHeader ) );
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::~CDlgBase (0x409E00)

CDlgBase::~CDlgBase()
{
	FreeHeaderDib();
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::SetFramePump (0x409E50)

int CDlgBase::SetFramePump( int bOn )
{
	m_bFramePump = bOn;
	return bOn;
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::LoadHeaderBitmap (0x409E60)
//
// Loads gfx/shell/<name> and measures its rect; prcOverride replaces the
// measured one.

void* CDlgBase::LoadHeaderBitmap( const char* pszName, RECT* prcOverride )
{
	FreeHeaderDib();

	return Launcher_LoadHeaderBitmapFile( pszName, &m_hHeaderDib, &m_rcHeader, prcOverride );
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::FreeHeaderDib (0x409E90)

HGLOBAL CDlgBase::FreeHeaderDib()
{
	HGLOBAL	hDib = m_hHeaderDib;

	if ( hDib )
		hDib = GlobalFree( m_hHeaderDib );
	m_hHeaderDib = NULL;
	return hDib;
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::DrawDialogContent (0x409EB0)

void CDlgBase::DrawDialogContent( CDC* pDC )
{
	Launcher_CompositeDib( pDC, &m_rcHeader, m_hHeaderDib, NULL );
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::InitChildDialog (0x409ED0)

void CDlgBase::InitChildDialog( CDlgBase* pChildPage, CODBlendBtn* pSlideBtn )
{
	if ( !pChildPage )
		return;

	m_pSlideWnd      = pSlideBtn;
	m_prcTargetHdr   = &pChildPage->m_rcHeader;
	m_phTargetHdrDib = &pChildPage->m_hHeaderDib;

	if ( pSlideBtn )
	{
		PrepareTransition( 0, m_prcTargetHdr, *m_phTargetHdrDib, pSlideBtn );
		if ( m_pSelfWnd )
			RunSlide( 1 );
		FinishTransition( 0, 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::RestoreAfterModal (0x409F50)

void CDlgBase::RestoreAfterModal()
{
	if ( m_pSlideWnd && m_prcTargetHdr && m_phTargetHdrDib )
	{
		PrepareTransition( 1, m_prcTargetHdr, *m_phTargetHdrDib, m_pSlideWnd );
		if ( m_pSelfWnd )
			RunSlide( 0 );
		FinishTransition( 1, 1 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::PrepareTransition (0x409FC0)

void CDlgBase::PrepareTransition( int bRestore, RECT* prcTarget, HGLOBAL hHdrDib, CODBlendBtn* pSlideBtn )
{
	if ( g_bNoFly || !m_pSelfWnd )
		return;

	// reset the whole context block
	m_pdcParent = m_pdcWork = m_pdcSaved = NULL;
	m_pbmWork = m_pbmWorkOld = m_pbmSaved = m_pbmSavedOld = NULL;
	m_pWndSlide2 = NULL;
	memset( &m_rcUnion, 0, sizeof( m_rcUnion ) );
	memset( &m_rcHdrDst, 0, sizeof( m_rcHdrDst ) );
	memset( &m_rcHdrOwn, 0, sizeof( m_rcHdrOwn ) );
	m_hHdrDibDst = m_hHdrDibOwn = NULL;

	RECT	rcParent;
	::GetClientRect( m_pSelfWnd->GetSafeHwnd(), &rcParent );

	// slide-source window rect, parent-client space
	RECT	rcSrc;
	::GetWindowRect( pSlideBtn->GetSafeHwnd(), &rcSrc );
	m_pSelfWnd->ScreenToClient( &rcSrc );

	RECT	rcUnion;
	::UnionRect( &rcUnion, &rcSrc, prcTarget );
	m_rcUnion = rcUnion;
	m_rcHdrDst = *prcTarget;
	m_hHdrDibDst = hHdrDib;
	m_rcHdrOwn = m_rcHeader;
	m_hHdrDibOwn = m_hHeaderDib;

	// parent client DC + two union-sized buffers
	m_pdcParent = new CClientDC( m_pSelfWnd );

	int	w = rcUnion.right - rcUnion.left;
	int	h = rcUnion.bottom - rcUnion.top;

	m_pdcWork = new CDC;
	m_pdcWork->CreateCompatibleDC( m_pdcParent );
	m_pbmWork = new CBitmap;
	m_pbmWork->Attach( ::CreateCompatibleBitmap( m_pdcParent->GetSafeHdc(), w, h ) );
	m_pbmWorkOld = m_pdcWork->SelectObject( m_pbmWork );

	m_pdcSaved = new CDC;
	m_pdcSaved->CreateCompatibleDC( m_pdcParent );
	m_pbmSaved = new CBitmap;
	m_pbmSaved->Attach( ::CreateCompatibleBitmap( m_pdcParent->GetSafeHdc(), w, h ) );
	m_pbmSavedOld = m_pdcSaved->SelectObject( m_pbmSaved );

	m_pWndSlide2 = pSlideBtn;

	// snapshot the background under the union
	m_pdcSaved->BitBlt( 0, 0, w, h, m_pdcParent, rcUnion.left, rcUnion.top, SRCCOPY );

	// paint clean background over the target area of the saved copy
	RECT	rc = m_rcHdrDst;
	::OffsetRect( &rc, -rcUnion.left, -rcUnion.top );
	Launcher_BlitBackground( m_pdcSaved, &rc, &m_rcHdrDst );

	// pre-draw the header that should already be visible
	if ( bRestore )
	{
		if ( m_hHdrDibDst )
			Launcher_CompositeDib( m_pdcParent, &m_rcHdrDst, m_hHdrDibDst, NULL );
	}
	else
	{
		if ( m_hHdrDibOwn )
			Launcher_CompositeDib( m_pdcParent, &m_rcHdrOwn, m_hHdrDibOwn, NULL );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::FinishTransition (0x40A370)

void CDlgBase::FinishTransition( int bShow, int bForce )
{
	if ( g_bNoFly )
	{
		if ( g_nMenuShown && m_pSelfWnd && m_pSelfWnd->GetSafeHwnd() )
		{
			m_pSelfWnd->InvalidateRect( NULL, TRUE );
			m_pSelfWnd->UpdateWindow();
		}
		return;
	}

	if ( m_pdcWork )
	{
		m_pdcWork->SelectObject( m_pbmWorkOld );
		delete m_pbmWork;
		m_pdcWork->DeleteDC();
		delete m_pdcWork;
		m_pdcWork = NULL;
		m_pbmWork = NULL;
	}
	if ( m_pdcSaved )
	{
		m_pdcSaved->SelectObject( m_pbmSavedOld );
		delete m_pbmSaved;
		m_pdcSaved->DeleteDC();
		delete m_pdcSaved;
		m_pdcSaved = NULL;
		m_pbmSaved = NULL;
	}

	if ( bShow && m_pdcParent )
	{
		Launcher_BlitBackground( m_pdcParent, &m_rcHeader, &m_rcHeader );
		if ( m_hHdrDibDst )
			Launcher_CompositeDib( m_pdcParent, &m_rcHeader, m_hHeaderDib, NULL );
	}

	if ( m_pdcParent )
	{
		delete m_pdcParent;
		m_pdcParent = NULL;
	}

	if ( m_pWndSlide2 )
		::SendMessage( m_pWndSlide2->GetSafeHwnd(), WM_MOUSEMOVE, 0, 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::PreTranslateMessage (0x40A4A0)

BOOL CDlgBase::PreTranslateMessage( MSG* pMsg )
{
#ifdef LAUNCHER_FIXES
	if ( pMsg->message == WM_MOUSEWHEEL && Dlg_RouteMouseWheel( pMsg ) )
		return TRUE;
#endif

	if ( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN )
	{
		CWnd*			focus = CWnd::FromHandlePermanent( ::GetFocus() );
		CODBlendBtn*	btn = (CODBlendBtn*)AfxDynamicDownCast( RUNTIME_CLASS( CButton ), focus );
		if ( !btn )
			return CWnd::PreTranslateMessage( pMsg );
		::SendMessageA( m_hWnd, WM_COMMAND, btn->GetDlgCtrlID(), (LPARAM)btn->m_hWnd );
		return TRUE;
	}
	return CDialog::PreTranslateMessage( pMsg );
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::PaintSkinnedDialog (0x40A530)

void CDlgBase::PaintSkinnedDialog()
{
	if ( !m_pSelfWnd )
		return;

	CPaintDC	dc( m_pSelfWnd );
	RECT		rc;
	::GetClientRect( GetSafeHwnd(), &rc );

	CDC		mem;
	if ( !mem.CreateCompatibleDC( &dc ) )
		return;
	CBitmap	bmp;
	bmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
		rc.right - rc.left, rc.bottom - rc.top ) );
	CBitmap*	pOld = mem.SelectObject( &bmp );

	// NULL source rect: the full DIB is scaled to fill the client.
	Launcher_BlitBackground( &mem, &rc, NULL );

	// header content (skipped when m_nHdrPad set), then the per-page overlay
	if ( !m_nHdrPad )
		DrawDialogContent( &mem );
	DrawDialogOverlay( &mem, &rc );

	dc.BitBlt( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		&mem, 0, 0, SRCCOPY );
	mem.SelectObject( pOld );
}

#ifdef LAUNCHER_RE
// The retail build this tree reconstructs.  The original stores it nowhere the
// launcher can read, so it is a constant here.
#define LAUNCHER_RE_BUILD	1792

// Where the marker sits, and how far the plate is inflated past the glyphs.
#define MARKER_INSET_X		6
#define MARKER_INSET_Y		2
#define MARKER_PAD_X		3
#define MARKER_PAD_Y		2

/*
==================
Launcher_MarkerFont
==================
*/
static void Launcher_MarkerFont( CFont* pFont )
{
	pFont->CreateFont( -10, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, VARIABLE_PITCH, "Arial" );
}

/*
==================
Launcher_FormatBuildMarker
==================
*/
static void Launcher_FormatBuildMarker( char* pszOut, int cchOut )
{
	_snprintf( pszOut, cchOut - 1, "hl1792-re  build %i  proto %i  %s  %s",
			   LAUNCHER_RE_BUILD, g_nDefaultProtocol,
			   g_szPatchVersion[0] ? g_szPatchVersion : "1.1.0.8", __DATE__ );
	pszOut[cchOut - 1] = 0;
}

/*
==================
Launcher_PaintBuildMarker

Lays the marker down with its top-left corner at (x,y).  Shared by the dialog
pages and by the in-game overlay window below.
==================
*/
static void Launcher_PaintBuildMarker( CDC* pDC, int x, int y )
{
	CFont	font;
	Launcher_MarkerFont( &font );

	char	szMark[128];
	Launcher_FormatBuildMarker( szMark, sizeof( szMark ) );

	CFont*	pOldFont = pDC->SelectObject( &font );

	// The skin art under the marker is busy, so shrink-wrap the text and lay a
	// solid plate behind it instead of relying on contrast alone.
	CRect	tr( x, y, x + 454, y + 14 );
	pDC->DrawText( szMark, -1, &tr,
		DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_CALCRECT );

	CRect	plate( tr );
	plate.InflateRect( MARKER_PAD_X, MARKER_PAD_Y );
	pDC->FillSolidRect( &plate, RGB( 0, 0, 0 ) );

	int			oldBk  = pDC->SetBkMode( TRANSPARENT );
	COLORREF	oldClr = pDC->SetTextColor( RGB( 255, 220, 120 ) );

	pDC->DrawText( szMark, -1, &tr,
		DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_VCENTER );

	pDC->SetTextColor( oldClr );
	pDC->SetBkMode( oldBk );
	pDC->SelectObject( pOldFont );
}

/*
==================
Launcher_MeasureBuildMarker

The plate size, cached: the text only changes if the patch version does, and
that cannot happen while the engine is up.
==================
*/
static void Launcher_MeasureBuildMarker( SIZE* pSize )
{
	static SIZE	s_size;

	if ( !s_size.cx )
	{
		CFont	font;
		Launcher_MarkerFont( &font );

		char	szMark[128];
		Launcher_FormatBuildMarker( szMark, sizeof( szMark ) );

		HDC		hdcScreen = ::GetDC( NULL );
		CDC*	pDC       = CDC::FromHandle( hdcScreen );
		CFont*	pOldFont  = pDC->SelectObject( &font );

		CRect	tr( 0, 0, 1024, 32 );
		pDC->DrawText( szMark, -1, &tr,
			DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_CALCRECT );

		pDC->SelectObject( pOldFont );
		::ReleaseDC( NULL, hdcScreen );

		s_size.cx = tr.Width()  + 2 * MARKER_PAD_X;
		s_size.cy = tr.Height() + 2 * MARKER_PAD_Y;
	}

	*pSize = s_size;
}

/*
==================
Launcher_DrawBuildMarker

Not in the original: the LAUNCHER_RE build marker.
==================
*/
void Launcher_DrawBuildMarker( CDC* pDC )
{
	Launcher_PaintBuildMarker( pDC, MARKER_INSET_X, MARKER_INSET_Y );
}

/////////////////////////////////////////////////////////////////////////////
// The in-game copy of the marker.
//
// The engine owns mainwindow's client area outright and we have no hook into
// its present, so the marker cannot be composited into the frame.  What we can
// do is park a tiny topmost popup over the game window's top-left corner and
// keep re-asserting its z-order every frame.
//
// That reaches the cases where the engine draws into an ordinary window: any
// renderer in windowed mode, and OpenGL fullscreen, which GoldSrc runs as a
// plain topmost window rather than an exclusive mode.  It cannot reach
// exclusive-fullscreen software or Direct3D, where the primary surface is
// flipped behind GDI's back -- there the overlay would either not appear at all
// or fight the flip chain, so it stays hidden instead.

#define MARKER_CLASS	"HL1792ReBuildMarker"

static HWND	s_hMarkerWnd;
static RECT	s_rcMarkerLast;

/*
==================
Launcher_MarkerWndProc
==================
*/
static LRESULT CALLBACK Launcher_MarkerWndProc( HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
	case WM_PAINT:
		{
			PAINTSTRUCT	ps;
			HDC			hdc = ::BeginPaint( hWnd, &ps );

			// The window is the plate, so the text starts one pad in.
			Launcher_PaintBuildMarker( CDC::FromHandle( hdc ),
				MARKER_PAD_X, MARKER_PAD_Y );

			::EndPaint( hWnd, &ps );
		}
		return 0;

	// Never swallow a click meant for the game.
	case WM_NCHITTEST:
		return HTTRANSPARENT;

	case WM_ERASEBKGND:
		return TRUE;
	}

	return ::DefWindowProc( hWnd, uMsg, wParam, lParam );
}

/*
==================
Launcher_GameOverlayWanted
==================
*/
static int Launcher_GameOverlayWanted( void )
{
	if ( gDLLState != DLL_ACTIVE || gBackground )
		return 0;

	// Topmost is app-global, so drop the marker the moment we lose foreground
	// rather than floating it over whatever the user switched to.
	if ( !ActiveApp )
		return 0;

	if ( !mainwindow || !::IsWindow( mainwindow ) || !::IsWindowVisible( mainwindow ) )
		return 0;

	// See the note above: only the windowed cases composite.
	return gEngineModeWindowed || gEngineVidType == VT_OpenGL;
}

/*
==================
Launcher_UpdateGameOverlay
==================
*/
void Launcher_UpdateGameOverlay( void )
{
	if ( !Launcher_GameOverlayWanted() )
	{
		if ( s_hMarkerWnd && ::IsWindowVisible( s_hMarkerWnd ) )
			::ShowWindow( s_hMarkerWnd, SW_HIDE );
		return;
	}

	if ( !s_hMarkerWnd )
	{
		static int	s_bClassRegistered;

		if ( !s_bClassRegistered )
		{
			WNDCLASS	wc;

			memset( &wc, 0, sizeof( wc ) );
			wc.lpfnWndProc   = Launcher_MarkerWndProc;
			wc.hInstance     = AfxGetInstanceHandle();
			wc.hbrBackground = (HBRUSH)::GetStockObject( BLACK_BRUSH );
			wc.lpszClassName = MARKER_CLASS;

			if ( !::RegisterClass( &wc ) )
				return;

			s_bClassRegistered = 1;
		}

		// Owned by nobody: an owner would drag the marker along with the
		// launcher dialog's own show/hide, and the dialog is hidden in-game.
		s_hMarkerWnd = ::CreateWindowEx(
			WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
			MARKER_CLASS, MARKER_CLASS, WS_POPUP,
			0, 0, 0, 0, NULL, NULL, AfxGetInstanceHandle(), NULL );

		if ( !s_hMarkerWnd )
			return;

		::SetRectEmpty( &s_rcMarkerLast );
	}

	// Park it on the game window's client origin, inset the same as the dialog
	// copy, so the two land in the same place on screen.
	POINT	pt = { 0, 0 };
	SIZE	sz;
	RECT	rc;

	::ClientToScreen( mainwindow, &pt );
	Launcher_MeasureBuildMarker( &sz );

	rc.left   = pt.x + MARKER_INSET_X;
	rc.top    = pt.y + MARKER_INSET_Y;
	rc.right  = rc.left + sz.cx;
	rc.bottom = rc.top  + sz.cy;

	if ( !::EqualRect( &rc, &s_rcMarkerLast ) )
	{
		s_rcMarkerLast = rc;
		::SetWindowPos( s_hMarkerWnd, HWND_TOPMOST, rc.left, rc.top,
			rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER );
		::InvalidateRect( s_hMarkerWnd, NULL, TRUE );
	}

	if ( !::IsWindowVisible( s_hMarkerWnd ) )
	{
		::ShowWindow( s_hMarkerWnd, SW_SHOWNOACTIVATE );
	}
	else
	{
		// A mode switch or the engine's own topmost poke can bury us, so keep
		// claiming the top of the z-order.
		::SetWindowPos( s_hMarkerWnd, HWND_TOPMOST, 0, 0, 0, 0,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER );
	}
}

/*
==================
Launcher_DestroyGameOverlay
==================
*/
void Launcher_DestroyGameOverlay( void )
{
	if ( s_hMarkerWnd )
	{
		::DestroyWindow( s_hMarkerWnd );
		s_hMarkerWnd = NULL;
	}
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::DrawDialogOverlay (0x40C070)

void CDlgBase::DrawDialogOverlay( CDC* pDC, RECT* prc )
{
#ifdef LAUNCHER_RE
	// Every skinned page gets the marker; CHLMainDlg overrides this slot and
	// draws its own copy alongside the version string.
	Launcher_DrawBuildMarker( pDC );
#endif
}

/*
==================
BuildEaseCurve (0x40A740)
==================
*/
static float* BuildEaseCurve( int n )
{
	float*	curve = new float[n];
	float	n2 = (float)( n * n );

	memset( curve, 0, sizeof( float ) * n );
	for ( int i = 0; i < n; i++ )
	{
		float	d = (float)( n - 1 - i );
		curve[i] = 1.0f - d * d / n2;
	}
	return curve;
}

/////////////////////////////////////////////////////////////////////////////
// CDlgBase::RunSlide (0x40A7D0)

void CDlgBase::RunSlide( int bOpen )
{
	if ( g_bNoFly )
		return;
	if ( !m_pdcParent || !m_pdcWork || !m_pdcSaved || !m_prcTargetHdr )
		return;

	CODBlendBtn*	pSlide = m_pSlideWnd;
	HWND			hSlide = pSlide->GetSafeHwnd();
	RECT	rcBase = m_rcUnion;
	RECT	rcHdrDst = m_rcHdrDst;

	// slide-source rect, union-relative
	RECT	rcWnd;
	::GetWindowRect( hSlide, &rcWnd );
	m_pSelfWnd->ScreenToClient( &rcWnd );
	::OffsetRect( &rcWnd, -rcBase.left, -rcBase.top );

	int	wSnap = rcWnd.right - rcWnd.left;
	int	hSnap = rcWnd.bottom - rcWnd.top;
	if ( wSnap <= 0 || hSnap <= 0 )
		return;

	// snapshot of the slid window (black-filled first, so SRCPAINT merges cleanly)
	CDC		dcSnap;
	if ( !dcSnap.CreateCompatibleDC( m_pdcParent ) )
		return;
	CBitmap	bmSnap;
	bmSnap.Attach( ::CreateCompatibleBitmap( m_pdcParent->GetSafeHdc(), wSnap, hSnap ) );
	CBitmap* pOldSnap = dcSnap.SelectObject( &bmSnap );
	{
		RECT	rcClient;
		::GetClientRect( hSlide, &rcClient );
		CBrush	black( RGB( 0, 0, 0 ) );
		dcSnap.FillRect( &rcClient, &black );
		// The button draws its own face into the snapshot; the binary sends no
		// WM_PRINT anywhere.
		pSlide->m_bTwoBitmap = 1;
		pSlide->DrawButtonFace( &dcSnap, &bmSnap, &rcClient, 1, 1, 0 );
		pSlide->m_bTwoBitmap = 0;
	}

	// per-edge deltas from the window rect to the target rect (union space)
	RECT	rcTo = *m_prcTargetHdr;
	int		dL = rcTo.left   - rcWnd.left   - rcBase.left;
	int		dT = rcTo.top    - rcWnd.top    - rcBase.top;
	int		dR = rcTo.right  - rcWnd.right  - rcBase.left;
	int		dB = rcTo.bottom - rcWnd.bottom - rcBase.top;

	float*	curve = BuildEaseCurve( 20 );

	int		nFrom, nTo;
	if ( bOpen )
	{
		nFrom = 0;
		nTo = 20;
		Snd_PlayMenuSound( UISND_UPMENU );
	}
	else
	{
		nFrom = 19;
		nTo = -1;
		Snd_PlayMenuSound( UISND_DNMENU );
	}

	BOOL	bFirst = TRUE;
	RECT	rcPrev = { 0, 0, 0, 0 };
	double	t0 = (double)timeGetTime() * 0.001;

	// prime the work buffer with the saved background before the first frame
	m_pdcWork->BitBlt( 0, 0, rcBase.right - rcBase.left, rcBase.bottom - rcBase.top,
		m_pdcSaved, 0, 0, SRCCOPY );

	for ( ;; )
	{
		double	t = (double)timeGetTime() * 0.001 - t0;
		if ( t >= 0.2 )
			break;

		int	idx = nFrom + (int)( (double)( nTo - nFrom ) * ( t * 5.0 ) );
		if ( idx < 0 )   idx = 0;
		if ( idx > 20 )  idx = 20;
		float	f = curve[idx < 20 ? idx : 19];

		RECT	rcFrame;
		rcFrame.left   = rcWnd.left   + (LONG)( (double)dL * f );
		rcFrame.right  = rcWnd.right  + (LONG)( (double)dR * f );
		rcFrame.top    = rcWnd.top    + (LONG)( (double)dT * f );
		rcFrame.bottom = rcWnd.bottom + (LONG)( (double)dB * f );
		( (CRect*)&rcFrame )->NormalizeRect();
		if ( bFirst )
			rcPrev = rcFrame;

		RECT	rcDirty;
		::UnionRect( &rcDirty, &rcFrame, &rcPrev );

		// restore background under the dirty area, merge the stretched snapshot
		m_pdcWork->BitBlt( 0, 0, rcDirty.right - rcDirty.left, rcDirty.bottom - rcDirty.top,
			m_pdcSaved, rcDirty.left, rcDirty.top, SRCCOPY );
		m_pdcWork->StretchBlt( rcFrame.left - rcDirty.left, rcFrame.top - rcDirty.top,
			rcFrame.right - rcFrame.left, rcFrame.bottom - rcFrame.top,
			&dcSnap, 0, 0, wSnap, hSnap, SRCPAINT );
		m_pdcParent->BitBlt( rcBase.left + rcDirty.left, rcBase.top + rcDirty.top,
			rcDirty.right - rcDirty.left, rcDirty.bottom - rcDirty.top,
			m_pdcWork, 0, 0, SRCCOPY );

		rcPrev = rcFrame;
		bFirst = FALSE;
	}

	if ( curve )
		delete[] curve;
	dcSnap.SelectObject( pOldSnap );
	dcSnap.DeleteDC();

	// compose the final state into the saved buffer and show it
	RECT	rcFinal = rcTo;
	::OffsetRect( &rcFinal, -rcBase.left, -rcBase.top );
	Launcher_BlitBackground( m_pdcSaved, &rcFinal, &rcTo );
	if ( m_hHdrDibOwn && m_hHdrDibDst )
	{
		RECT	rcHdr = rcHdrDst;
		::OffsetRect( &rcHdr, -rcBase.left, -rcBase.top );
		Launcher_CompositeDib( m_pdcSaved, &rcHdr, m_hHdrDibDst, NULL );
	}
	m_pdcParent->BitBlt( rcBase.left, rcBase.top,
		rcBase.right - rcBase.left, rcBase.bottom - rcBase.top,
		m_pdcSaved, 0, 0, SRCCOPY );
}

#ifdef LAUNCHER_FIXES
/*
==================
LAUNCHER_FIXES: a real title bar, and pages that swap in place

Original behaviour: every launcher dialog -- the main menu and each of its pages
-- is a frameless WS_POPUP sized to exactly g_nLauncherDefW x g_nLauncherDefH and
centred on the screen by Dlg_CenterWindow.  The skinned "min"/"cls" statics in
the top-right corner stand in for a caption bar, so the window has no title, no
system menu to drag or an Alt+Space away, and nothing for the shell to label it
with.  And because a page is centred on the *screen* rather than over the menu it
came from, moving the launcher (once it can be moved at all) makes every page
jump back to the middle of the display, which reads as a new window opening.

With the fix the main launcher window gets WS_CAPTION and its title text, and is
positioned so that its *client* area still lands exactly where the old frameless
window sat -- so nothing inside the skin moves.  Pages are then placed over the
launcher's client rect instead of the screen centre, which makes clicking a menu
item swap the contents of the window that is already open rather than put up a
second one somewhere else.

Fullscreen (DirectDraw) mode is left alone: there the launcher owns the whole
display and a caption has nowhere to go.
==================
*/

/*
==================
Dlg_ApplyTitleBar (LAUNCHER_FIXES)

Give a top-level launcher dialog a caption and a title, keeping its client area
g_nLauncherDefW x g_nLauncherDefH and anchored where the frameless window used to
be, so the skin bitmaps and the hand-placed child controls stay put.
==================
*/
void  Dlg_ApplyTitleBar( CWnd* pDlg, const char* pszTitle )
{
	HWND	hWnd;

	if ( !pDlg )
		return;

	hWnd = pDlg->GetSafeHwnd();
	if ( !hWnd || !gEngineModeWindowed )
		return;

	SetWindowLongA( hWnd, GWL_STYLE,
		GetWindowLongA( hWnd, GWL_STYLE ) | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX );
	::SetWindowPos( hWnd, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED );

	if ( pszTitle )
		pDlg->SetWindowText( pszTitle );

	// Dlg_CenterWindow does the geometry: with the caption on, it grows the
	// window rect around a g_nLauncherDefW x g_nLauncherDefH client area.
	Dlg_CenterWindow( pDlg );
}

/*
==================
LAUNCHER_FIXES: the mouse wheel scrolls whatever is under the pointer

Original behaviour: Windows delivers WM_MOUSEWHEEL to the focused window, and of
all the owner-draw widgets only CODListCtrl looks at it -- through a
PreTranslateMessage filter, so even there it answers only while the list itself
holds the focus.  The drop-lists, the list boxes and the skinned scrollbars
never see the message at all, which leaves every one of them drag-only.

With the fix each dialog re-aims the wheel at the window under the pointer
before dispatch, and the four owner-draw widgets handle it: lists and scrollbars
scroll by the system's wheel-lines setting, a closed combo steps its selection.
==================
*/

/*
==================
Dlg_WheelScrollLines (LAUNCHER_FIXES)

Rows per wheel notch, from the system setting.  WHEEL_PAGESCROLL ("one screen at
a time") has no meaning for a fixed-height skin, so it reads as three.
==================
*/
int  Dlg_WheelScrollLines( void )
{
	UINT	lines = 3;

	::SystemParametersInfoA( SPI_GETWHEELSCROLLLINES, 0, &lines, 0 );

	if ( lines == WHEEL_PAGESCROLL || lines > 16 )
		lines = 3;
	if ( lines < 1 )
		lines = 1;

	return (int)lines;
}

/*
==================
Dlg_RouteMouseWheel (LAUNCHER_FIXES)

Hand the wheel to the widget the pointer is over.  Nonzero when that widget
claimed it, which is the dialog's cue to swallow the message.
==================
*/
int  Dlg_RouteMouseWheel( MSG* pMsg )
{
	POINT	pt;
	HWND	hHover;

	pt.x = (short)LOWORD( pMsg->lParam );
	pt.y = (short)HIWORD( pMsg->lParam );

	hHover = ::WindowFromPoint( pt );
	if ( !hHover || hHover == pMsg->hwnd )
		return 0;

	return ::SendMessageA( hHover, WM_MOUSEWHEEL, pMsg->wParam, pMsg->lParam ) != 0;
}

/*
==================
LAUNCHER_FIXES: the arrow keys move the selection, not the view

Original behaviour: CODListCtrl::OnKeyDown maps Up/Down and PageUp/PageDown onto
ScrollLineUp/ScrollPageUp, which move the visible window and leave the selected
row where it was.  A server, a save game or a mod can therefore only be picked
with the mouse, and there is no Home, no End, and no Enter.

With the fix those keys move the selection and scroll it into view, Home and End
jump to the ends of the list, and Enter opens the selected row exactly as a
double-click does.
==================
*/

/*
==================
LAUNCHER_FIXES: the thumb reaches the bottom of its track

Every owner-draw list hands its scrollbar SetRange( 0, <item count> ) and then
reads the bar's position back as the first visible row.  Those two do not agree
at the end of the list: the last row is on screen once the position reaches
count - page, but the bar keeps a further page of travel it can never turn into
scrolling.  So a list scrolled to its end leaves the thumb about its own height
short of the bottom, and the last stretch of the track is dead.

With the fix the bar takes its page size off its own maximum -- the thumb tracks
count - page instead of count -- so it lands flush at the bottom exactly when the
last row is showing, and every part of the track moves the list.  The thumb's
*size* still comes from page / count, which is what makes it proportional.
==================
*/

/*
==================
LAUNCHER_FIXES: a popup opens over the launcher, not over the desktop

Original behaviour: every prompt, login box, refresh box and download box places
itself with (SM_CXSCREEN - w) / 2 -- the middle of the primary display.  That was
the middle of the launcher too, back when the launcher was a frameless window
pinned to the screen centre and could not be moved.  Now that it can be dragged
anywhere, including onto a second monitor, a popup still opens in the middle of
the desktop, away from the window that raised it.

With the fix a popup is centred on the launcher's client area, and nudged back
if that would put any of it off the desktop.
==================
*/

/*
==================
Dlg_CenterPopup (LAUNCHER_FIXES)

Place a w x h popup in the middle of the launcher, falling back to the middle of
the primary display -- the original placement -- when there is no launcher window
to sit over.
==================
*/
void  Dlg_CenterPopup( CWnd* pDlg, int w, int h )
{
	POINT	pt = { 0, 0 };
	RECT	rcClient;
	int		x, y, vx, vy, vw, vh;

	if ( !pDlg )
		return;

	x = ( GetSystemMetrics( SM_CXSCREEN ) - w ) / 2;
	y = ( GetSystemMetrics( SM_CYSCREEN ) - h ) / 2;

	if ( gLauncherWnd && ::IsWindow( gLauncherWnd ) )
	{
		::GetClientRect( gLauncherWnd, &rcClient );
		::ClientToScreen( gLauncherWnd, &pt );

		x = pt.x + ( rcClient.right - rcClient.left - w ) / 2;
		y = pt.y + ( rcClient.bottom - rcClient.top - h ) / 2;

		vx = GetSystemMetrics( SM_XVIRTUALSCREEN );
		vy = GetSystemMetrics( SM_YVIRTUALSCREEN );
		vw = GetSystemMetrics( SM_CXVIRTUALSCREEN );
		vh = GetSystemMetrics( SM_CYVIRTUALSCREEN );

		if ( x + w > vx + vw )
			x = vx + vw - w;
		if ( y + h > vy + vh )
			y = vy + vh - h;
		if ( x < vx )
			x = vx;
		if ( y < vy )
			y = vy;
	}

	pDlg->MoveWindow( x, y, w, h, TRUE );
}

/*
==================
LAUNCHER_FIXES: the launcher opens where you left it

Original behaviour: Dlg_CenterWindow drops the launcher in the middle of the
display on every start, and nothing anywhere writes a window position.  The
frameless original could not be moved at all, so there was nothing to remember;
now that the window has a caption to drag it by, being put back in the centre of
the screen on the next run is a step backwards.

With the fix the position is written when the drag ends and read back on the
next start.  One that no longer lands on a monitor -- a display unplugged, a
resolution changed -- is discarded, and the window centres as before.
==================
*/
#define DLG_POS_UNSET	(-1000000)

/*
==================
Dlg_LoadWindowPos (LAUNCHER_FIXES)

The saved client-area origin, for the launcher window only -- a page is placed
over that window and must never take its own position from the profile.
Rejected unless enough of the window would land on the desktop to grab it by.
==================
*/
static int  Dlg_LoadWindowPos( CWnd* pDlg, int* px, int* py )
{
	int	x, y, vx, vy, vw, vh;

	if ( !gEngineModeWindowed )
		return 0;
	if ( !pDlg || pDlg->GetSafeHwnd() != gLauncherWnd )
		return 0;

	x = Launcher_GetProfileInt( "Settings", "Window X", DLG_POS_UNSET );
	y = Launcher_GetProfileInt( "Settings", "Window Y", DLG_POS_UNSET );
	if ( x == DLG_POS_UNSET || y == DLG_POS_UNSET )
		return 0;

	vx = GetSystemMetrics( SM_XVIRTUALSCREEN );
	vy = GetSystemMetrics( SM_YVIRTUALSCREEN );
	vw = GetSystemMetrics( SM_CXVIRTUALSCREEN );
	vh = GetSystemMetrics( SM_CYVIRTUALSCREEN );

	// Leave a grabbable strip of caption on the desktop in both axes.
	if ( x + g_nLauncherDefW < vx + 120 || x > vx + vw - 120 )
		return 0;
	if ( y < vy || y > vy + vh - 40 )
		return 0;

	*px = x;
	*py = y;
	return 1;
}

/*
==================
Dlg_SaveWindowPos (LAUNCHER_FIXES)
==================
*/
void  Dlg_SaveWindowPos( CWnd* pDlg )
{
	POINT	pt = { 0, 0 };

	if ( !pDlg || !pDlg->GetSafeHwnd() )
		return;
	if ( !gEngineModeWindowed )
		return;
	if ( pDlg->IsIconic() || pDlg->IsZoomed() )
		return;

	// Dlg_CenterWindow places the client origin, so that is what is stored.
	::ClientToScreen( pDlg->GetSafeHwnd(), &pt );

	Launcher_WriteProfileInt( "Settings", "Window X", pt.x );
	Launcher_WriteProfileInt( "Settings", "Window Y", pt.y );
}

#endif	// LAUNCHER_FIXES

/*
==================
Dlg_CenterWindow (0x40AED0)
==================
*/
void  Dlg_CenterWindow( CWnd* pDlg )
{
	int	x = 0, y = 0;

#ifdef LAUNCHER_FIXES
	// A page belongs inside the launcher it was opened from: put it over the
	// main window's client area so the contents change in place.
	if ( gLauncherWnd
	  && ::IsWindow( gLauncherWnd )
	  && pDlg->GetSafeHwnd() != gLauncherWnd )
	{
		POINT	pt = { 0, 0 };
		RECT	rcClient;

		::GetClientRect( gLauncherWnd, &rcClient );
		::ClientToScreen( gLauncherWnd, &pt );
		pDlg->MoveWindow( pt.x, pt.y,
			rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, TRUE );
		return;
	}
#endif

#ifdef LAUNCHER_FIXES
	// Fullscreen with no mode to switch to: the skin keeps its own size and is
	// centred on Shell_ShowBackdrop's black fill rather than left in the corner.
	if ( Shell_FullscreenActive() && !Shell_HasFullscreenMode() )
	{
		x = ( GetSystemMetrics( SM_CXSCREEN ) - g_nLauncherDefW ) / 2;
		y = ( GetSystemMetrics( SM_CYSCREEN ) - g_nLauncherDefH ) / 2;
		pDlg->MoveWindow( x, y, g_nLauncherDefW, g_nLauncherDefH, TRUE );
		return;
	}
#endif

	if ( gEngineModeWindowed && !CheckParm( "-tl", NULL ) )
	{
		x = ( GetSystemMetrics( SM_CXSCREEN ) - g_nLauncherDefW ) / 2;
		y = ( GetSystemMetrics( SM_CYSCREEN ) - g_nLauncherDefH ) / 2;
	}

#ifdef LAUNCHER_FIXES
	// The launcher window itself reopens where it was last dragged to.
	Dlg_LoadWindowPos( pDlg, &x, &y );
#endif

#ifdef LAUNCHER_FIXES
	// The launcher window carries a caption now, so (x,y,W,H) is where its
	// *client* area goes; grow the window rect around it.  Everything the skin
	// draws and every hand-placed control then stays exactly where it was.
	if ( pDlg->GetSafeHwnd() )
	{
		HWND	hWnd  = pDlg->GetSafeHwnd();
		LONG	style = GetWindowLongA( hWnd, GWL_STYLE );

		if ( style & WS_CAPTION )
		{
			RECT	rc;

			rc.left   = x;
			rc.top    = y;
			rc.right  = x + g_nLauncherDefW;
			rc.bottom = y + g_nLauncherDefH;
			AdjustWindowRectEx( &rc, style, FALSE, GetWindowLongA( hWnd, GWL_EXSTYLE ) );
			pDlg->MoveWindow( &rc, TRUE );
			return;
		}
	}
#endif

	pDlg->MoveWindow( x, y, g_nLauncherDefW, g_nLauncherDefH, TRUE );
}

/*
==================
SetWindowTextSafe (0x4989F0)
==================
*/
void  SetWindowTextSafe( CWnd* pWnd, const char* psz )
{
	if ( pWnd && pWnd->GetSafeHwnd() )
		pWnd->SetWindowText( psz ? psz : "" );
}
