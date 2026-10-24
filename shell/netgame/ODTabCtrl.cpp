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
// Purpose: CODTabCtrl, the owner-draw tab strip.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The tab whose face must repaint when the selection moves off it.
static int	g_odTabPrevSel = -1;		// 0x4D1118

BEGIN_MESSAGE_MAP( CODTabCtrl, CWnd )
	//{{AFX_MSG_MAP(CODTabCtrl)
	ON_WM_NCDESTROY()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_ERASEBKGND()
	ON_WM_GETDLGCODE()
	ON_WM_CREATE()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::CODTabCtrl (0x450BE0)

CODTabCtrl::CODTabCtrl()
{
	InitMembers();
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::~CODTabCtrl (0x450C70)

CODTabCtrl::~CODTabCtrl()
{
	if ( m_pTabs )
		delete[] m_pTabs;
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::Create (0x450D40)

BOOL CODTabCtrl::Create( DWORD dwStyle, RECT* prc, CWnd* pParent, UINT nID )
{
	RECT		rc;
	WNDCLASSA	wc;

	::CopyRect( &rc, prc );
	m_pTabParent = pParent;

	memset( &wc, 0, sizeof( wc ) );
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc   = AfxGetAfxWndProc();
	wc.hInstance     = AfxGetInstanceHandle();
	wc.hbrBackground = (HBRUSH)::GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "CODTabCtrlCls";
	wc.hCursor       = ::LoadCursorA( NULL, IDC_ARROW );
	if ( !AfxRegisterClass( &wc ) )
	{
		Launcher_ShowMessageById( 0, IDS_ODTAB_REGFAIL );
		return FALSE;
	}

	return CreateEx( WS_EX_NOPARENTNOTIFY, "CODTabCtrlCls", "",
		dwStyle | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		pParent ? pParent->GetSafeHwnd() : NULL, (HMENU)nID, NULL ) != 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::InitMembers (0x450E40)

void CODTabCtrl::InitMembers()
{
	EnableStackedTabs( 1 );				// the video page overrides to 0
	m_bAutoDelete = 1;
	m_pTabParent  = NULL;
	m_nTabCount   = 0;
	m_nCurSel     = -1;
	m_clrTextNorm = RGB( 255, 180, 24 );
	m_clrTextSel  = RGB( 255, 255, 255 );
	m_clrLine     = RGB( 127, 127, 127 );

	m_brHot.Attach( ::CreateSolidBrush( RGB( 84, 45, 0 ) ) );

	m_pTabs = new tabentry_t[MAX_OD_TABS];
	memset( m_pTabs, 0, MAX_OD_TABS * sizeof( tabentry_t ) );

	m_brBg.Attach( ::CreateSolidBrush( RGB( 0, 0, 0 ) ) );

	m_fontNorm.Attach( ::CreateFontA( -11, 0, 0, 0, FW_NORMAL, 0, 0, 0,
		ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
		VARIABLE_PITCH | FF_DONTCARE, "Arial" ) );

	m_fontSel.Attach( ::CreateFontA( -20, 0, 0, 0, FW_NORMAL, 0, 0, 0,
		ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
		VARIABLE_PITCH | FF_DONTCARE, "Arial" ) );
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::OnNcDestroy (0x450F20)

void CODTabCtrl::OnNcDestroy()
{
	CWnd::OnNcDestroy();
	if ( m_bAutoDelete )
		delete this;
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::OnPaint (0x450F40)

void CODTabCtrl::OnPaint()
{
	CPaintDC	dc( this );
	CDC			mem;
	CBitmap		bmp;
	CBitmap*	pOld;
	RECT		client, rcDst, rcSrc;
	int			i;

	::GetClientRect( GetSafeHwnd(), &client );

	if ( !mem.CreateCompatibleDC( &dc ) )
		return;

	bmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
		client.right - client.left, client.bottom - client.top ) );
	pOld = mem.SelectObject( &bmp );

	// dst maps through our own client, src through the parent's.
	::GetWindowRect( GetSafeHwnd(), &rcDst );
	ScreenToClient( &rcDst );
	::GetWindowRect( GetSafeHwnd(), &rcSrc );
	if ( GetParent() )
		GetParent()->ScreenToClient( &rcSrc );
	Launcher_CopyParentBackground( &mem, &rcDst, &rcSrc );

	for ( i = 0; i < m_nTabCount; i++ )
		DrawTab( &mem, i );

	dc.BitBlt( 0, 0, client.right - client.left, client.bottom - client.top,
		&mem, 0, 0, SRCCOPY );
	mem.SelectObject( pOld );
	mem.DeleteDC();
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::DrawTab (0x451150)

void CODTabCtrl::DrawTab( CDC* pDC, int iTab )
{
	RECT		client, rc;
	tabentry_t*	pTab;
	const char*	pszText = NULL;
	CFont*		pOldFont;
	CPen		pen;
	CPen*		pOldPen;
	int			bLast  = 0;
	int			cyText = 10;
	int			nCenter = 0;

	if ( iTab >= m_nTabCount )
		return;

	::GetClientRect( GetSafeHwnd(), &client );	// (sic) fetched and never read
	GetTabRect( iTab, &rc, &bLast, &cyText );

	pTab = &m_pTabs[iTab];
	if ( !pTab )
		return;
	if ( pTab->bIsPtr )
		pszText = pTab->pszPtr;
	else
		pszText = pTab->szText;
	if ( !pszText )
		return;

	pDC->SetBkMode( TRANSPARENT );

	if ( iTab == m_nCurSel )
	{
		pDC->SetTextColor( m_clrTextSel );
		pOldFont = pDC->SelectObject( &m_fontSel );
	}
	else
	{
		nCenter = DT_CENTER;
		pDC->SetTextColor( m_clrTextNorm );
		pOldFont = pDC->SelectObject( &m_fontNorm );
	}

	// Unselected tabs are indented 2px.
	if ( iTab != m_nCurSel )
		rc.left += 2;
	pDC->DrawText( pszText, -1, &rc, nCenter | DT_SINGLELINE | DT_VCENTER );
	if ( iTab != m_nCurSel )
		rc.left -= 2;

	// A separator down the tab's right edge, on every tab but the last.
	if ( !bLast )
	{
		pen.Attach( ::CreatePen( PS_SOLID, 1, m_clrLine ) );
		pOldPen = pDC->SelectObject( &pen );

		if ( m_bStacked )
			pDC->MoveTo( rc.right - 1, rc.bottom + 1 );
		else
			pDC->MoveTo( rc.right - 1, rc.bottom - cyText + 2 );
		pDC->LineTo( rc.right - 1, rc.bottom - 1 );

		pDC->SelectObject( pOldPen );
	}

	pDC->SelectObject( pOldFont );
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::OnLButtonDown (0x451330)

void CODTabCtrl::OnLButtonDown( UINT /*nFlags*/, CPoint pt )
{
	RECT	client, rc;
	int		iHit = -1;
	int		bLast, i;

	::GetClientRect( GetSafeHwnd(), &client );
	if ( !::PtInRect( &client, pt ) )
		return;

	for ( i = 0; i < m_nTabCount; i++ )
	{
		GetTabRect( i, &rc, &bLast, NULL );
		if ( ::PtInRect( &rc, pt ) )
		{
			iHit = i;
			break;
		}
	}

	if ( iHit < m_nTabCount && iHit >= 0 )
	{
		SetCurSel( iHit, 1 );
		InvalidateRect( NULL, TRUE );
		UpdateWindow();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::AddTabPtr (0x4513E0)

void CODTabCtrl::AddTabPtr( const char* pszText )
{
	tabentry_t*	pTab = &m_pTabs[m_nTabCount];

	pTab->pszPtr = pszText;
	pTab->bIsPtr = 1;

	MeasureTab( pszText, &pTab->cxSmall, &pTab->cxBig );

	m_nTabCount++;
	SetCurSel( m_nTabCount - 1, 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::AddTab (0x451440)

void CODTabCtrl::AddTab( const char* pszText )
{
	tabentry_t*	pTab = &m_pTabs[m_nTabCount];

	strncpy( pTab->szText, pszText, sizeof( pTab->szText ) );
	pTab->szText[sizeof( pTab->szText ) - 1] = 0;
	pTab->bIsPtr = 0;

	MeasureTab( pTab->szText, &pTab->cxSmall, &pTab->cxBig );

	m_nTabCount++;
	SetCurSel( m_nTabCount - 1, 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::SetCurSel (0x4514C0)

void CODTabCtrl::SetCurSel( int iTab, int bNotify )
{
	int	nCtrlID;

	if ( iTab == -1 )
	{
		if ( g_odTabPrevSel != -1 && g_odTabPrevSel < m_nTabCount && g_odTabPrevSel >= 0 )
		{
			CClientDC	dc( this );
			DrawTab( &dc, g_odTabPrevSel );
		}
		m_nCurSel      = -1;
		g_odTabPrevSel = -1;
		return;
	}

	if ( iTab >= 0 && iTab < m_nTabCount )
	{
		g_odTabPrevSel = m_nCurSel;
		m_nCurSel      = iTab;

		nCtrlID = GetDlgCtrlID();
		if ( bNotify && GetParent() )
			::SendMessageA( GetParent()->GetSafeHwnd(),
				WM_COMMAND, MAKEWPARAM( nCtrlID, 1 ), (LPARAM)GetSafeHwnd() );

		InvalidateRect( NULL, TRUE );
		UpdateWindow();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::OnEraseBkgnd (0x4515E0)

BOOL CODTabCtrl::OnEraseBkgnd( CDC* /*pDC*/ )
{
	RECT	rc;
	CWnd*	pParent;

	::GetWindowRect( GetSafeHwnd(), &rc );
	pParent = GetParent();
	if ( pParent )
	{
		pParent->ScreenToClient( &rc );
		::ValidateRect( pParent->GetSafeHwnd(), &rc );
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::GetCurSel (0x451650)

int CODTabCtrl::GetCurSel()
{
	return m_nCurSel;
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::GetTabText (0x451660)

const char* CODTabCtrl::GetTabText( int iTab )
{
	tabentry_t*	pTab;

	if ( iTab < 0 || iTab >= m_nTabCount )
		return NULL;

	pTab = &m_pTabs[iTab];
	return pTab->bIsPtr ? pTab->pszPtr : pTab->szText;
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::GetTabRectSingleRow (0x451690)

void CODTabCtrl::GetTabRectSingleRow( int iTab, RECT* prc, int* pbLast, int* pcyText )
{
	RECT		client;
	tabentry_t*	pTab;
	int			x, i;

	::GetClientRect( GetSafeHwnd(), &client );

	*pbLast = 0;
	if ( pcyText )
		*pcyText = 10;
	if ( m_nCurSel < 0 )
		return;

	x = 0;
	for ( i = 0; i < iTab; i++ )
	{
		if ( i == m_nCurSel )
			x += m_pTabs[i].cxBig + 10;
		else
			x += m_pTabs[i].cxSmall;
	}

	pTab = &m_pTabs[iTab];
	if ( !pTab )
		return;

	prc->left   = x;
	prc->right  = x;
	prc->top    = client.top;
	prc->bottom = client.bottom;
	if ( pcyText )
		*pcyText = client.bottom - client.top;

	if ( iTab == m_nCurSel )
	{
		// right grows from the unshifted x, so the +5 does not widen the tab
		prc->right  = prc->left + pTab->cxBig + 10;
		prc->left  += 5;
	}
	else
	{
		prc->top   += 5;
		prc->right += pTab->cxSmall;
	}

	*pbLast = ( iTab == m_nTabCount - 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::GetTabRect (0x451780)
//
// The stacked layout leaves pcyText alone; the caller's seed value stands.

void CODTabCtrl::GetTabRect( int iTab, RECT* prc, int* pbLast, int* pcyText )
{
	RECT		client;
	tabentry_t*	pSel;
	tabentry_t*	pTab;
	tabentry_t*	pWalk;
	int			x, i;

	if ( !m_bStacked )
	{
		GetTabRectSingleRow( iTab, prc, pbLast, pcyText );
		return;
	}

	::GetClientRect( GetSafeHwnd(), &client );

	*pbLast = 0;
	if ( m_nCurSel < 0 )
		return;

	pSel = &m_pTabs[m_nCurSel];
	if ( !pSel )
		return;

	x = pSel->cxBig + 10;

	if ( iTab == m_nCurSel )
	{
		prc->left   = 0;
		prc->top    = client.top;
		prc->right  = x;
		prc->bottom = client.bottom;
		*pbLast     = 0;
		return;
	}

	pWalk = m_pTabs;
	for ( i = 0; i <= iTab; i++, pWalk++ )
	{
		if ( i != m_nCurSel )
		{
			if ( i == iTab )
				break;
			if ( pWalk )
				x += pWalk->cxSmall;
		}
	}

	pTab = &m_pTabs[iTab];
	if ( !pTab )
		return;

	prc->left   = x;
	prc->right  = x + pTab->cxSmall;
	prc->top    = client.top + 5;
	prc->bottom = client.bottom;

	// The selected tab is skipped by the walk above, so the strip's last drawn
	// slot is one short once iTab has passed it.
	if ( iTab <= m_nCurSel )
		*pbLast = ( iTab == m_nTabCount - 2 );
	else
		*pbLast = ( iTab == m_nTabCount - 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::MeasureTab (0x4518B0)

void CODTabCtrl::MeasureTab( const char* pszText, int* pcxSmall, int* pcxBig )
{
	CClientDC	dc( this );
	CFont*		pOld;
	RECT		rc = { 0, 0, 0, 0 };

	*pcxSmall = 0;
	*pcxBig   = 0;
	if ( !pszText )
		return;

	pOld = dc.SelectObject( &m_fontSel );
	dc.DrawText( pszText, -1, &rc, DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_CENTER );
	*pcxBig = rc.right - rc.left + 10;

	dc.SelectObject( &m_fontNorm );
	dc.DrawText( pszText, -1, &rc, DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_CENTER );
	*pcxSmall = rc.right - rc.left + 10;

	dc.SelectObject( pOld );
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::EnableStackedTabs (0x4519C0)

void CODTabCtrl::EnableStackedTabs( int bStacked )
{
	m_bStacked = bStacked;
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::OnKeyDown (0x4519D0)

void CODTabCtrl::OnKeyDown( UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/ )
{
	HWND	hNext;

	if ( nChar == VK_TAB )
	{
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
	}

	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::OnCreate (0x443FA0)
//
// Folded onto the shared default-handler stub.

int CODTabCtrl::OnCreate( LPCREATESTRUCT /*lpcs*/ )
{
	return (int)Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::GetTabCount (0x44A350)

int CODTabCtrl::GetTabCount()
{
	return m_nTabCount;
}

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl::OnGetDlgCode (0x44A4D0)

UINT CODTabCtrl::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}
