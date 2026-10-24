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
// Purpose: the owner-draw combo boxes (CODComboBox, CODDriverComboBox,
//          CODColorComboBox).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The drop list and the hidden edit, by control id.
enum { ODCB_ID_LIST = 101, ODCB_ID_EDIT = 102 };

BEGIN_MESSAGE_MAP( CODComboBox, CComboBox )
	//{{AFX_MSG_MAP(CODComboBox)
	ON_WM_GETDLGCODE()
	ON_WM_NCDESTROY()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_CTLCOLOR()
	ON_WM_KEYDOWN()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_VSCROLL()
	ON_WM_NCCALCSIZE()
	ON_WM_CREATE()
	ON_LBN_SELCHANGE( ODCB_ID_LIST, OnNotifyParent )
	ON_EN_KILLFOCUS( ODCB_ID_EDIT, OnEditCommit )
	ON_EN_UPDATE( ODCB_ID_EDIT, OnEditUpdate )
	ON_WM_PARENTNOTIFY()
	//}}AFX_MSG_MAP
#ifdef LAUNCHER_FIXES
	ON_WM_MOUSEWHEEL()
#endif
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::CODComboBox (0x443FB0)

CODComboBox::CODComboBox()
{
	char	path[MAX_PATH];
	HBRUSH	hText;
	HBRUSH	hHot;
	HBITMAP	hFrame;
	HFONT	hFace;
	HFONT	hRow;

	m_pList       = NULL;
	m_bTracking   = 0;
	m_dropHeight  = 60;
	m_curText     = "";
	m_bEditable   = 0;
	m_pEdit       = NULL;
	m_bEditOpen   = 0;
	m_bEditing    = 0;
	m_curSel      = -1;
	m_bAutoDelete = 1;
	m_pOwner      = NULL;
	m_rowHeight   = 15;
	m_bDropped    = 0;
	memset( &m_rcClosed, 0, sizeof( m_rcClosed ) );

	m_clrFrame      = RGB( 56, 56, 56 );
	m_clrFrameFocus = RGB( 128, 128, 128 );
	m_clrText       = RGB( 255, 127, 24 );
	m_clrBk         = 0;

	sprintf( path, "%s%s.bmp", "gfx/shell/", "sm_dnarw" );
	m_dnArrow  = DIB_LoadBitmapFile( path );
	sprintf( path, "%s%s.bmp", "gfx/shell/", "sm_dnarf" );
	m_dnArrowF = DIB_LoadBitmapFile( path );

	hText = ::CreateSolidBrush( RGB( 0, 0, 0 ) );
	if ( hText )
		m_brText.Attach( hText );
	hHot = ::CreateSolidBrush( RGB( 84, 45, 0 ) );
	if ( hHot )
		m_brHot.Attach( hHot );

	hFrame = ::LoadBitmapA( AfxGetInstanceHandle(), MAKEINTRESOURCEA( IDB_COMBO_FRAME ) );
	if ( hFrame )
		m_frame.Attach( hFrame );

	hFace = ::CreateFontA( -12, 0, 0, 0, 400, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, 2, "Arial" );
	if ( hFace )
		m_faceFont.Attach( hFace );
	hRow = ::CreateFontA( -11, 0, 0, 0, 400, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, 2, "Arial" );
	if ( hRow )
		m_textFont.Attach( hRow );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::~CODComboBox (0x444070)
//
// Neither child is freed here: the drop list and the hidden edit are child
// windows with m_bAutoDelete set, so they delete themselves from their own
// OnNcDestroy when this combo's window goes away.

CODComboBox::~CODComboBox()
{
	if ( m_dnArrow )
		::GlobalFree( m_dnArrow );
	m_dnArrow = NULL;
	if ( m_dnArrowF )
		::GlobalFree( m_dnArrowF );
	m_dnArrowF = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::DrawRow (0x4441A0)

void CODComboBox::DrawRow( CDC* pDC, int iRow )
{
	RECT		client;
	RECT		rc;
	CFont*		pOldFont;
	const char*	pszText;
	int			vis;

	if ( !m_pList )
		return;

	::GetClientRect( m_pList->GetSafeHwnd(), &client );
	client.bottom -= 6;
	client.right  -= 6;
	if ( m_pList->HasScrollbar() )
		client.right -= 16;

	vis = iRow - m_pList->GetTopIndex();
	if ( vis < 0 )
		return;

	pszText = m_pList->GetText( iRow );
	if ( !pszText )
		return;

	rc.left   = 0;
	rc.top    = vis * m_rowHeight;
	rc.bottom = rc.top + m_rowHeight;
	rc.right  = client.right - client.left;

	pDC->SetTextColor( m_clrText );
	pDC->SetBkMode( TRANSPARENT );
	pOldFont = pDC->SelectObject( &m_textFont );
	pDC->SetBkColor( m_clrBk );
	if ( iRow == m_pList->GetCurSel() )
		pDC->FillRect( &rc, &m_brHot );
	else
		pDC->FillRect( &rc, &m_brText );

	rc.left += 2;
	pDC->DrawText( pszText, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );
	rc.left -= 2;
	pDC->SelectObject( pOldFont );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::Create (0x444300)

BOOL CODComboBox::Create( DWORD dwStyle, RECT* prc, CWnd* pParent, UINT nID )
{
	WNDCLASSA	wc;
	RECT		rc;
	RECT		rcList;
	int			bottom;

	::CopyRect( &rc, prc );
	m_pOwner = pParent;

	memset( &wc, 0, sizeof( wc ) );
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc   = AfxGetAfxWndProc();
	wc.hInstance     = AfxGetInstanceHandle();
	wc.hbrBackground = (HBRUSH)::GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "CODComboBoxCls";
	wc.hCursor       = ::LoadCursorA( NULL, IDC_ARROW );
	if ( !AfxRegisterClass( &wc ) )
	{
		Launcher_ShowMessageById( 0, IDS_ODCOMBO_REGFAIL );
		return FALSE;
	}

	bottom = m_bEditable ? rc.bottom : rc.top + 21;
	if ( !CreateEx( WS_EX_NOPARENTNOTIFY, "CODComboBoxCls", "",
		dwStyle | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP,
		rc.left, rc.top, rc.right - rc.left, bottom - rc.top,
		pParent ? pParent->GetSafeHwnd() : NULL, (HMENU)(UINT_PTR)nID, NULL ) )
		return FALSE;

	::SetRect( &m_rcClosed, rc.left, rc.top, rc.right, bottom );

	m_pList = new CODListBox;
	if ( !m_pList )
		return FALSE;

	rcList.left   = 0;
	rcList.top    = 15;
	rcList.right  = rc.right - rc.left;
	rcList.bottom = m_dropHeight + 15;
	if ( !m_pList->Create( m_bEditable ? ( WS_CHILD | WS_VISIBLE ) : WS_CHILD,
		&rcList, this, ODCB_ID_LIST ) )
		return FALSE;

	m_pList->m_bTransparent = 1;		// the list never paints itself
	m_pList->m_pOwnerCombo = this;		// it dirties us instead

	m_pEdit = new CEdit;
	if ( !m_pEdit )
		return FALSE;

	CRect	rcEdit( 1, 3, rc.right - rc.left - 21, 21 );
	if ( !m_pEdit->Create( WS_CHILD, rcEdit, this, ODCB_ID_EDIT ) )
		return FALSE;
	m_pEdit->ShowWindow( SW_HIDE );
	::SendMessageA( m_pEdit->GetSafeHwnd(), WM_SETFONT,
		(WPARAM)m_faceFont.GetSafeHandle(), TRUE );

	ShowDrop( m_bEditable );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnNcDestroy (0x444720)

void CODComboBox::OnNcDestroy()
{
	if ( m_pEdit )
		delete m_pEdit;
	m_pEdit = NULL;

	::DestroyCaret();

	CComboBox::OnNcDestroy();
	if ( m_bAutoDelete )
		delete this;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnPaint (0x444760)
//
// Paint fetches and releases its own DC, so the client is validated by hand.

void CODComboBox::OnPaint()
{
	Paint();
	::ValidateRect( GetSafeHwnd(), NULL );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnNotifyParent (0x444780)

void CODComboBox::OnNotifyParent()
{
	CWnd*	pParent = GetParent();

	::SendMessageA( pParent ? pParent->GetSafeHwnd() : NULL, WM_COMMAND,
		MAKEWPARAM( GetDlgCtrlID(), CBN_SELCHANGE ), (LPARAM)GetSafeHwnd() );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnLButtonDown (0x4447D0)
//
// The closed face opens the drop and takes the capture; a click in the open
// list picks a row; anything else collapses, and when this combo is neither
// dropped nor captured the click is handed to whichever sibling it landed on.

void CODComboBox::OnLButtonDown( UINT nFlags, CPoint pt )
{
	BOOL		bCaptured;
	RECT		rc;
	RECT		rcHit;
	RECT		rcParent;
	POINT		ptChild;
	CWnd*		pFirst;
	CWnd*		pSib;
	int			sel;
	const char*	psz;
	char		cls[256];

	bCaptured = ( CWnd::GetCapture() == this );

	GetClientRect( &rc );
	rc.bottom = rc.top + 15;

	if ( pt.x >= 0 && pt.x <= rc.right - rc.left
		&& pt.y >= 0 && pt.y <= rc.bottom - rc.top )
	{
		if ( m_bDropped && !m_bEditable )
		{
			if ( bCaptured )
				::ReleaseCapture();
			ShowDrop( FALSE );
			::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
			::UpdateWindow( GetSafeHwnd() );
			if ( GetParent() )
			{
				::GetWindowRect( m_pList->GetSafeHwnd(), &rcParent );
				GetParent()->ScreenToClient( &rcParent );
				::InvalidateRect( GetParent()->GetSafeHwnd(), &rcParent, TRUE );
				::UpdateWindow( GetParent()->GetSafeHwnd() );
			}
			SetFocus();
			return;
		}

		// the editable face: show the hidden CEdit and seed it
		if ( m_bEditOpen && !m_bEditing )
		{
			::GetWindowRect( m_pEdit->GetSafeHwnd(), &rcHit );
			ScreenToClient( &rcHit );
			if ( ::PtInRect( &rcHit, pt ) )
			{
				m_pEdit->ShowWindow( SW_RESTORE );
				m_pEdit->SetFocus();
				m_bEditing = 1;
				sel = GetCurSel();
				if ( sel != -1 )
				{
					psz = GetString( sel );
					if ( psz )
					{
						m_pEdit->SetWindowText( psz );
						::SendMessageA( m_pEdit->GetSafeHwnd(), EM_SETSEL, 0, -1 );
						::SendMessageA( m_pEdit->GetSafeHwnd(), EM_SCROLLCARET, 0, 0 );
						::InvalidateRect( m_pEdit->GetSafeHwnd(), NULL, TRUE );
						::UpdateWindow( m_pEdit->GetSafeHwnd() );
					}
				}
				SetFocus();
				return;
			}
		}

		ShowDrop( TRUE );
		::InvalidateRect( m_pList->GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( m_pList->GetSafeHwnd() );
		::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( GetSafeHwnd() );
		SetCapture();
		SetFocus();
		return;
	}

	if ( m_bDropped )
	{
		::GetWindowRect( m_pList->GetSafeHwnd(), &rcHit );
		ScreenToClient( &rcHit );
		if ( ::PtInRect( &rcHit, pt ) )
		{
			if ( bCaptured )
				::ReleaseCapture();

			m_curSel = m_pList->GetCurSel();

			// forward the click to the list in its own client space
			ptChild.x = pt.x;
			ptChild.y = pt.y;
			::ClientToScreen( GetSafeHwnd(), &ptChild );
			::ScreenToClient( m_pList->GetSafeHwnd(), &ptChild );
			m_pList->OnLButtonDown( nFlags, ptChild );

			if ( bCaptured )
				SetCapture();

			if ( m_pList->GetCurSel() != m_curSel )
				::SendMessageA( GetParent()->GetSafeHwnd(), WM_COMMAND,
					MAKEWPARAM( GetDlgCtrlID(), CBN_SELCHANGE ), (LPARAM)GetSafeHwnd() );

			// a click in the 16px scrollbar gutter leaves the drop open
			rcHit.left = rcHit.right - 16;
			if ( m_pList->HasScrollbar() && ::PtInRect( &rcHit, pt ) )
			{
				::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
				::UpdateWindow( GetSafeHwnd() );
				if ( GetParent() )
				{
					::GetWindowRect( m_pList->GetSafeHwnd(), &rcParent );
					GetParent()->ScreenToClient( &rcParent );
					::InvalidateRect( GetParent()->GetSafeHwnd(), &rcParent, TRUE );
					::UpdateWindow( GetParent()->GetSafeHwnd() );
				}
				return;
			}
		}

		if ( bCaptured )
			::ReleaseCapture();
		if ( m_bEditable )
			return;

		ShowDrop( FALSE );
		::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( GetSafeHwnd() );
		if ( GetParent() )
		{
			::GetWindowRect( m_pList->GetSafeHwnd(), &rcParent );
			GetParent()->ScreenToClient( &rcParent );
			::InvalidateRect( GetParent()->GetSafeHwnd(), &rcParent, TRUE );
			::UpdateWindow( GetParent()->GetSafeHwnd() );
		}
		return;
	}

	if ( bCaptured )
	{
		if ( m_bEditable )
			return;
		::ReleaseCapture();
		ShowDrop( FALSE );
		::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( GetSafeHwnd() );
		if ( GetParent() )
		{
			::GetWindowRect( m_pList->GetSafeHwnd(), &rcParent );
			GetParent()->ScreenToClient( &rcParent );
			::InvalidateRect( GetParent()->GetSafeHwnd(), &rcParent, TRUE );
			::UpdateWindow( GetParent()->GetSafeHwnd() );
		}
		return;
	}

	// closed and uncaptured: hand the click to whichever sibling it landed on
	pFirst = GetParent()->GetWindow( GW_CHILD );
	pSib   = pFirst;
	while ( pSib )
	{
		if ( pSib != this && ::IsWindowVisible( pSib->GetSafeHwnd() )
			&& pSib->IsWindowEnabled() )
		{
			::GetClassNameA( pSib->GetSafeHwnd(), cls, sizeof( cls ) );
			::GetWindowRect( pSib->GetSafeHwnd(), &rcHit );
			pSib->ScreenToClient( &rcHit );
			if ( !_strcmpi( cls, "CODComboBoxCls" )
				&& !( (CODComboBox*)pSib )->IsDropped() )
				rcHit.bottom = rcHit.top + 15;

			ptChild.x = pt.x;
			ptChild.y = pt.y;
			::ClientToScreen( GetSafeHwnd(), &ptChild );
			::ScreenToClient( pSib->GetSafeHwnd(), &ptChild );
			if ( ::PtInRect( &rcHit, ptChild ) )
			{
				::SendMessageA( pSib->GetSafeHwnd(), WM_LBUTTONDOWN, nFlags,
					MAKELPARAM( ptChild.x, ptChild.y ) );
				pSib->SetFocus();
			}
		}

		pSib = pSib->GetWindow( GW_HWNDNEXT );
		if ( pSib == pFirst )
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::AddItem (0x444ED0)

int CODComboBox::AddItem( void* pRecord )
{
	if ( !m_pList )
		return -1;

	m_pList->AddStringPtr( pRecord );
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
	return m_pList->GetCount() - 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::AddString (0x444F00)

int CODComboBox::AddString( const char* psz )
{
	if ( !m_pList )
		return -1;

	m_pList->AddString( psz );
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
	return m_pList->GetCount() - 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::SetCurSel (0x444F30)
//
// Mirrors the row text into m_curText while the hidden edit is up.

void CODComboBox::SetCurSel( int i )
{
	const char*	psz;
	int			sel;

	if ( !m_pList )
		return;

	m_pList->SetCurSel( i );
	if ( m_bEditOpen )
	{
		sel = m_pList->GetCurSel();
		if ( sel != -1 )
		{
			psz = m_pList->GetText( sel );
			if ( psz )
				m_curText = psz;
		}
	}
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnEraseBkgnd (0x444F90)

BOOL CODComboBox::OnEraseBkgnd( CDC* pDC )
{
	CDC			mem;
	CBitmap		bmp;
	CBitmap*	pOld;
	RECT		rc;
	RECT		full;

	GetClientRect( &rc );
	rc.right  -= 16;
	rc.bottom  = rc.top + 15;

	if ( !mem.CreateCompatibleDC( pDC ) )
		return TRUE;

	bmp.Attach( ::CreateCompatibleBitmap( pDC->GetSafeHdc(),
		rc.right - rc.left, rc.bottom - rc.top ) );
	pOld = mem.SelectObject( &bmp );

	::SetRect( &full, 0, 0, rc.right - rc.left, rc.bottom - rc.top );
	mem.FillRect( &full, &m_brText );
	pDC->BitBlt( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		&mem, 0, 0, SRCCOPY );
	mem.SelectObject( pOld );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::GetString (0x445140)

const char* CODComboBox::GetString( int i )
{
	return m_pList ? m_pList->GetText( i ) : NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::GetCount (0x445150)

int CODComboBox::GetCount()
{
	return m_pList ? m_pList->GetCount() : 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::GetCurSel (0x445160)

int CODComboBox::GetCurSel()
{
	return m_pList ? m_pList->GetCurSel() : -1;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::SetAutoDelete (0x445170)

void CODComboBox::SetAutoDelete( int bAuto )
{
	m_bAutoDelete = bAuto;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::FindString (0x445180)

int CODComboBox::FindString( const char* psz )
{
	return m_pList->FindString( psz );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::Paint (0x445190)

void CODComboBox::Paint()
{
	CClientDC	dc( this );
	CDC			mem;
	CDC			rowMem;
	RECT		rc;
	RECT		band;
	RECT		arrow;
	BOOL		bFocus;
	char		text[256];
	const char*	psz;
	int			sel;
	int			visRows, first, last, iRow, i;

	::GetClientRect( GetSafeHwnd(), &rc );
	::InflateRect( &rc, -3, -3 );
	rc.bottom = rc.top + 15;

	bFocus = ( CWnd::GetFocus() == this );

	// the closed-face band
	if ( mem.CreateCompatibleDC( &dc ) )
	{
		CBitmap		bmp;
		CBitmap*	pOld;
		CFont*		pOldFace;

		bmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
			rc.right - rc.left, rc.bottom - rc.top ) );
		pOld = mem.SelectObject( &bmp );

		::SetRect( &band, 0, 0, rc.right - rc.left, rc.bottom - rc.top );
		mem.FillRect( &band, &m_brText );
		mem.SetBkMode( TRANSPARENT );
		mem.SetTextColor( m_clrText );
		pOldFace = mem.SelectObject( &m_faceFont );

		sel = m_pList->GetCurSel();
		if ( sel != -1 && !m_bEditing )
		{
			psz = m_pList->GetText( sel );
			if ( psz )
			{
				strncpy( text, psz, sizeof( text ) );
				text[sizeof( text ) - 1] = 0;
				mem.DrawText( text, -1, &band, DT_VCENTER | DT_NOPREFIX );
			}
		}
		if ( m_bEditing )
		{
			::InvalidateRect( m_pEdit->GetSafeHwnd(), NULL, TRUE );
			::UpdateWindow( m_pEdit->GetSafeHwnd() );
		}

		::SetRect( &arrow, band.right - 15, band.top, band.right, band.bottom );
		DrawArrow( &mem, &arrow );

		mem.SelectObject( pOldFace );
		dc.BitBlt( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			&mem, 0, 0, SRCCOPY );
		mem.SelectObject( pOld );
	}

	// the focus frame: three nested rectangles
	if ( m_bDropped )
	{
		::GetClientRect( GetSafeHwnd(), &rc );
		for ( i = 0; i < 3; ++i )
		{
			CBrush	br( bFocus ? m_clrFrameFocus : m_clrFrame );
			dc.FrameRect( &rc, &br );
			::InflateRect( &rc, -1, -1 );
		}

		// then the open rows
		if ( m_pList->HasScrollbar() )
			rc.right -= 16;
		rc.top += 15;
		visRows = m_pList->GetVisibleRows();

		if ( rowMem.CreateCompatibleDC( &dc ) )
		{
			CBitmap		rowBmp;
			CBitmap*	pOldRow;
			RECT		rowFull;

			rowBmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
				rc.right - rc.left, rc.bottom - rc.top ) );
			pOldRow = rowMem.SelectObject( &rowBmp );

			::SetRect( &rowFull, 0, 0, rc.right - rc.left, rc.bottom - rc.top );
			rowMem.FillRect( &rowFull, &m_brText );

			m_pList->ClampTopIndex();
			first = m_pList->GetTopIndex();
			last  = first + visRows;
			if ( m_pList->GetCount() < last )
				last = m_pList->GetCount();
			for ( iRow = first; iRow < last; ++iRow )
			{
				if ( iRow >= 0 )
					DrawRow( &rowMem, iRow );
			}

			dc.BitBlt( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				&rowMem, 0, 0, SRCCOPY );
			rowMem.SelectObject( pOldRow );
			::ValidateRect( GetSafeHwnd(), &rc );
		}
	}
	else
	{
		for ( i = 0; i < 3; ++i )
		{
			::InflateRect( &rc, 1, 1 );
			CBrush	br( bFocus ? m_clrFrameFocus : m_clrFrame );
			dc.FrameRect( &rc, &br );
		}
		::ValidateRect( GetSafeHwnd(), &rc );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnLButtonUp (0x445820)

void CODComboBox::OnLButtonUp( UINT nFlags, CPoint pt )
{
	CWnd*	pParent;
	CWnd*	pSib;
	RECT	band;
	RECT	rcSib;
	POINT	scr;

	if ( CWnd::GetCapture() == this )
		::ReleaseCapture();

	::GetClientRect( GetSafeHwnd(), &band );
	band.bottom = band.top + 15;

	if ( ( pt.x < 0 || pt.x > band.right - band.left || (UINT)pt.y >= 16 ) && m_bDropped )
	{
		pParent = GetParent();
		if ( pParent )
		{
			for ( pSib = pParent->GetWindow( GW_CHILD ); pSib;
				pSib = pSib->GetWindow( GW_HWNDNEXT ) )
			{
				if ( pSib == this )
					continue;
				if ( !::IsWindowVisible( pSib->GetSafeHwnd() ) )
					continue;
				if ( !pSib->IsWindowEnabled() )
					continue;

				::GetWindowRect( pSib->GetSafeHwnd(), &rcSib );
				pSib->ScreenToClient( &rcSib );
				scr = pt;
				::ClientToScreen( GetSafeHwnd(), &scr );
				::ScreenToClient( pSib->GetSafeHwnd(), &scr );
				if ( ::PtInRect( &rcSib, scr ) )
					::SendMessageA( pSib->GetSafeHwnd(), WM_LBUTTONDOWN, nFlags,
						MAKELPARAM( scr.x, scr.y ) );
			}
		}
	}
	else
	{
		SetFocus();
		Default();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::DrawArrow (0x4459D0)

void CODComboBox::DrawArrow( CDC* pDC, RECT* prc )
{
	POINT	pt;
	RECT	band;
	RECT	src;
	HGLOBAL	hDib;

	::GetCursorPos( &pt );
	::ScreenToClient( GetSafeHwnd(), &pt );

	::GetClientRect( GetSafeHwnd(), &band );
	::InflateRect( &band, -3, -3 );
	band.bottom = band.top + 15;
	band.left   = band.right - 15;

	hDib = ::PtInRect( &band, pt ) ? m_dnArrowF : m_dnArrow;
	if ( hDib )
	{
		::SetRect( &src, 0, 0, prc->right - prc->left, prc->bottom - prc->top );
		DIB_BlitDib( pDC->GetSafeHdc(), prc, hDib, &src );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnMouseMove (0x445AA0)

void CODComboBox::OnMouseMove( UINT, CPoint pt )
{
	POINT	cur;
	RECT	band;
	RECT	rcList;
	POINT	scr;

	::GetCursorPos( &cur );
	::ScreenToClient( GetSafeHwnd(), &cur );

	::GetClientRect( GetSafeHwnd(), &band );
	::InflateRect( &band, -3, -3 );
	band.bottom = band.top + 15;
	band.left   = band.right - 15;

	if ( !m_bTracking && ::PtInRect( &band, cur ) )
	{
		m_bTracking = 1;
		::SetTimer( GetSafeHwnd(), 1, 50, NULL );
	}

	CWnd::GetCapture();
	if ( m_bDropped )
	{
		::GetWindowRect( m_pList->GetSafeHwnd(), &rcList );
		ScreenToClient( &rcList );
		if ( ::PtInRect( &rcList, pt ) )
		{
			m_curSel = m_pList->GetCurSel();
			scr = pt;
			::ClientToScreen( GetSafeHwnd(), &scr );
			::ScreenToClient( m_pList->GetSafeHwnd(), &scr );
		}
	}
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnTimer (0x445BE0)

void CODComboBox::OnTimer( UINT_PTR nIDEvent )
{
	POINT	cur;
	RECT	band;

	if ( nIDEvent == 1 )
	{
		::GetCursorPos( &cur );
		::ScreenToClient( GetSafeHwnd(), &cur );

		::GetClientRect( GetSafeHwnd(), &band );
		::InflateRect( &band, -3, -3 );
		band.bottom = band.top + 15;
		band.left   = band.right - 15;

		if ( !::PtInRect( &band, cur ) && m_bTracking )
		{
			m_bTracking = 0;
			::KillTimer( GetSafeHwnd(), 1 );
		}

		CClientDC	dc( this );
		DrawArrow( &dc, &band );
	}
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::Collapse (0x445CF0)

void CODComboBox::Collapse()
{
	BOOL	bCaptured = ( CWnd::GetCapture() == this );
	RECT	rc;

	if ( !m_bDropped )
		return;
	if ( bCaptured )
		::ReleaseCapture();
	if ( m_bEditable )
		return;

	ShowDrop( FALSE );
	::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( GetSafeHwnd() );

	if ( GetParent() )
	{
		::GetWindowRect( m_pList->GetSafeHwnd(), &rc );
		GetParent()->ScreenToClient( &rc );
		::InvalidateRect( GetParent()->GetSafeHwnd(), &rc, TRUE );
		::UpdateWindow( GetParent()->GetSafeHwnd() );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnKillFocus (0x445DD0)

void CODComboBox::OnKillFocus( CWnd* pNewWnd )
{
	CWnd*	pParent;
	CWnd*	pSib;
	RECT	rcSelf;
	RECT	rcSib;
	RECT	tmp;
	BOOL	bClear;
	char	cls[256];

	UNUSED_ALWAYS( pNewWnd );

	Default();

	if ( m_pEdit && m_bEditOpen )
	{
		m_pEdit->GetWindowText( m_curText );
		if ( m_curText.GetLength() == 0 )
			SetCurSel( m_pList->GetCurSel() );
	}

	Collapse();

	::GetWindowRect( GetSafeHwnd(), &rcSelf );
	rcSelf.bottom = rcSelf.top + 15;
	pParent = GetParent();
	if ( pParent )
		pParent->ScreenToClient( &rcSelf );

	bClear = TRUE;
	if ( pParent )
	{
		for ( pSib = pParent->GetWindow( GW_CHILD ); pSib;
			pSib = pSib->GetWindow( GW_HWNDNEXT ) )
		{
			if ( pSib == this )
				continue;
			if ( !::IsWindowVisible( pSib->GetSafeHwnd() ) )
				continue;
			if ( !pSib->IsWindowEnabled() )
				continue;

			::GetClassNameA( pSib->GetSafeHwnd(), cls, sizeof( cls ) );
			::GetWindowRect( pSib->GetSafeHwnd(), &rcSib );
			pParent->ScreenToClient( &rcSib );
			if ( !_strcmpi( cls, "CODComboBoxCls" ) )
			{
				if ( !( (CODComboBox*)pSib )->IsDropped() )
					rcSib.bottom = rcSib.top + 15;
				if ( ::IntersectRect( &tmp, &rcSib, &rcSelf ) )
					bClear = FALSE;
			}
		}
	}

	if ( bClear )
	{
		::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( GetSafeHwnd() );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnEditCommit (0x445FA0)

void CODComboBox::OnEditCommit()
{
	char	text[64];

	::HideCaret( m_pEdit->GetSafeHwnd() );
	if ( !m_bEditing )
		return;

	m_pEdit->ShowWindow( SW_HIDE );
	m_bEditing = 0;

	m_pEdit->GetWindowText( text, sizeof( text ) );
	if ( FindString( text ) == -1 && strlen( text ) )
	{
		AddString( text );
		::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( GetSafeHwnd() );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnCtlColor (0x446030)

HBRUSH CODComboBox::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	CComboBox::OnCtlColor( pDC, pWnd, nCtlColor );

	pDC->SetTextColor( m_clrText );
	pDC->SetBkColor( m_clrBk );
	return (HBRUSH)m_brText.GetSafeHandle();
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnEditUpdate (0x446080)

void CODComboBox::OnEditUpdate()
{
	if ( m_bEditOpen && m_pEdit )
		m_pEdit->GetWindowText( m_curText );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::SetRowHeight (0x4460A0)

void CODComboBox::SetRowHeight( int h )
{
	m_rowHeight = h;
	m_pList->SetRowHeight( h );
	::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( GetSafeHwnd() );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::GetScrollbar (0x4460E0)

CODScrollBar* CODComboBox::GetScrollbar()
{
	return m_pList->GetScrollbar();
}

#ifdef LAUNCHER_FIXES
/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnMouseWheel (LAUNCHER_FIXES)
//
// Open, the drop list is its own window and takes the wheel itself; closed, the
// face steps through the items the way a stock combo does.

BOOL CODComboBox::OnMouseWheel( UINT /*nFlags*/, short zDelta, CPoint /*pt*/ )
{
	int	sel;

	if ( m_bDropped || m_bEditing || !m_pList || !zDelta )
		return FALSE;

	sel = GetCurSel() + ( ( zDelta > 0 ) ? -1 : 1 );
	if ( sel < 0 || sel >= GetCount() )
		return TRUE;

	SetCurSel( sel );
	return TRUE;
}

#endif	// LAUNCHER_FIXES

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnKeyDown (0x4460F0)
//
// (sic) both Tab arms ask for the next item -- shift-tab does not go back.

void CODComboBox::OnKeyDown( UINT nChar, UINT, UINT )
{
	CWnd*	pParent;
	HWND	hNext;

	switch ( nChar )
	{
	case VK_TAB:
		pParent = GetParent();
		hNext = ::GetNextDlgTabItem( pParent ? pParent->GetSafeHwnd() : NULL,
			GetSafeHwnd(), FALSE );
		if ( hNext )
			::SetFocus( hNext );
		break;

	case VK_RETURN:
		if ( m_bDropped )
			Collapse();
		break;

	case VK_PRIOR:
		m_pList->NavPageUp();
		break;

	case VK_NEXT:
		m_pList->NavPageDown();
		break;

	case VK_UP:
		m_pList->NavLineUp();
		break;

	case VK_DOWN:
		m_pList->NavLineDown();
		break;

	default:
		Default();
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnSize (0x446210)

void CODComboBox::OnSize( UINT nType, int cx, int cy )
{
	CComboBox::OnSize( nType, cx, cy );

	if ( !m_pList || !m_pEdit )
		return;

	if ( m_pList->GetSafeHwnd() )
		m_pList->MoveWindow( 0, 15, cx, cy - 15, TRUE );
	if ( m_pEdit->GetSafeHwnd() )
		m_pEdit->MoveWindow( 1, 3, cx - 22, 18, TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::DropDown (0x446270)

void CODComboBox::DropDown()
{
	if ( m_bDropped )
		return;

	ShowDrop( TRUE );
	::InvalidateRect( m_pList->GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( m_pList->GetSafeHwnd() );
	::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( GetSafeHwnd() );
	SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::IsDropped (0x4462D0)

int CODComboBox::IsDropped()
{
	return m_bDropped;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::SetFaceColor (0x4462F0)

void CODComboBox::SetFaceColor( COLORREF clr )
{
	HBRUSH	hbr;

	m_clrBk = clr;
	m_brText.DeleteObject();
	hbr = ::CreateSolidBrush( clr );
	if ( hbr )
		m_brText.Attach( hbr );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::ShowDrop (0x446320)

void CODComboBox::ShowDrop( int bShow )
{
	CWnd*	pParent;
	RECT	rc;
	LONG	ex;

	::GetWindowRect( GetSafeHwnd(), &rc );
	pParent = GetParent();
	if ( pParent )
		pParent->ScreenToClient( &rc );

	if ( ::EqualRect( &rc, &m_rcClosed ) && bShow )
	{
		rc.bottom += m_dropHeight;
		m_bDropped = 1;
		MoveTo( &rc, TRUE );
	}
	else if ( !::EqualRect( &rc, &m_rcClosed ) && !bShow )
	{
		rc = m_rcClosed;
		m_bDropped = 0;
		MoveTo( &rc, TRUE );
	}

	ex = ::GetWindowLongA( GetSafeHwnd(), GWL_EXSTYLE );
	if ( bShow )
	{
		m_pList->ShowWindow( SW_RESTORE );
		ex &= ~WS_EX_TRANSPARENT;
		m_bDropped = 1;
		::SetWindowLongA( GetSafeHwnd(), GWL_EXSTYLE, ex );
		SetWindowPos( &wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOCOPYBITS );
		m_pList->SetWindowPos( &wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOCOPYBITS );
	}
	else
	{
		m_pList->ShowWindow( SW_HIDE );
		ex |= WS_EX_TRANSPARENT;
		m_bDropped = 0;
		::SetWindowLongA( GetSafeHwnd(), GWL_EXSTYLE, ex );
		SetWindowPos( &wndBottom, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOCOPYBITS );
		m_pList->SetWindowPos( &wndBottom, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOCOPYBITS );
	}

	::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( GetSafeHwnd() );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::MoveTo (0x4464C0)

void CODComboBox::MoveTo( RECT* prc, int bRepaint )
{
	RECT	rc;
	int		bottom;

	::CopyRect( &rc, prc );
	bottom = m_bEditable ? rc.bottom : rc.top + 21;
	if ( m_bDropped )
		bottom += m_dropHeight;

	::SetRect( &m_rcClosed, rc.left, rc.top, rc.right, bottom );
	MoveWindow( m_rcClosed.left, m_rcClosed.top,
		m_rcClosed.right - m_rcClosed.left,
		m_rcClosed.bottom - m_rcClosed.top, bRepaint );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnGetMinMaxInfo (0x446540)
//
// The closed face is clamped to a 15px band; once dropped the list sizes
// itself.

void CODComboBox::OnGetMinMaxInfo( MINMAXINFO* lpMMI )
{
	int	cy;

	if ( !m_bDropped )
	{
		cy = m_rowHeight;
		if ( cy < 15 )
			cy = 15;
		lpMMI->ptMaxSize.y = cy;
	}
	CComboBox::OnGetMinMaxInfo( lpMMI );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnParentNotify (0x446570)

void CODComboBox::OnParentNotify( UINT message, LPARAM lParam )
{
	CComboBox::OnParentNotify( message, lParam );
	if ( message == WM_LBUTTONDOWN )
		OnLButtonDown( 0, CPoint( LOWORD( lParam ), HIWORD( lParam ) ) );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnNcCalcSize (0x4465B0)

void CODComboBox::OnNcCalcSize( BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp )
{
	CComboBox::OnNcCalcSize( bCalcValidRects, lpncsp );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnCreate (0x443FA0)

int CODComboBox::OnCreate( LPCREATESTRUCT lpcs )
{
	return CComboBox::OnCreate( lpcs );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnGetDlgCode (0x44A4D0)

UINT CODComboBox::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnSetFocus (0x450640)
//
// Folded with CODSlider::OnKillFocus.

void CODComboBox::OnSetFocus( CWnd* pOldWnd )
{
	CComboBox::OnSetFocus( pOldWnd );
	::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( GetSafeHwnd() );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::OnVScroll (0x497E7C)
//
// Folded onto CWnd::OnVScroll.

void CODComboBox::OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	CComboBox::OnVScroll( nSBCode, nPos, pScrollBar );
}

/////////////////////////////////////////////////////////////////////////////
// CODComboBox::PreSubclassWindow
//
// NOTE(ox): not in the binary -- see the header.  Builds the same children a
// Created combo gets, so the "list exists by paint time" invariant holds for a
// DDX-attached one too.

void CODComboBox::PreSubclassWindow()
{
	CRect	rc;
	RECT	rcList;

	CWnd::PreSubclassWindow();

	if ( m_pList )
		return;

	GetClientRect( &rc );
	m_rcClosed = rc;

	m_pList = new CODListBox;
	if ( !m_pList )
		return;

	::SetRect( &rcList, 0, 15, rc.right - rc.left, m_dropHeight + 15 );
	m_pList->Create( WS_CHILD, &rcList, this, ODCB_ID_LIST );
	m_pList->m_bTransparent = 1;
	m_pList->m_pOwnerCombo = this;

	m_pEdit = new CEdit;
	if ( m_pEdit )
	{
		CRect	rcEdit( 1, 3, rc.right - rc.left - 21, 21 );
		m_pEdit->Create( WS_CHILD, rcEdit, this, ODCB_ID_EDIT );
		m_pEdit->ShowWindow( SW_HIDE );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODDriverComboBox::CODDriverComboBox (0x4465C0)

CODDriverComboBox::CODDriverComboBox()
{
}

BEGIN_MESSAGE_MAP( CODDriverComboBox, CODComboBox )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODDriverComboBox::DrawRow (0x446620)
//
// The row record is a 160-byte label + description pair; the row paints the
// description.

void CODDriverComboBox::DrawRow( CDC* pDC, int iRow )
{
	const drivrow_t*	pRecord;
	RECT				client;
	RECT				rc;
	CFont*				pOldFont;
	int					vis;

	if ( !m_pList )
		return;

	::GetClientRect( m_pList->GetSafeHwnd(), &client );
	client.bottom -= 6;
	client.right  -= 6;
	if ( m_pList->HasScrollbar() )
		client.right -= 16;

	vis = iRow - m_pList->GetTopIndex();
	if ( vis < 0 )
		return;

	pRecord = (const drivrow_t*)m_pList->GetText( iRow );
	if ( !pRecord || !pRecord->desc[0] )
		return;

	rc.left   = 0;
	rc.top    = vis * m_rowHeight;
	rc.bottom = rc.top + m_rowHeight;
	rc.right  = client.right - client.left;

	pDC->SetTextColor( m_clrText );
	pDC->SetBkMode( TRANSPARENT );
	pOldFont = pDC->SelectObject( &m_textFont );
	pDC->SetBkColor( m_clrBk );
	if ( iRow == m_pList->GetCurSel() )
		pDC->FillRect( &rc, &m_brHot );
	else
		pDC->FillRect( &rc, &m_brText );

	rc.left += 2;
	pDC->DrawText( pRecord->desc, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );
	rc.left -= 2;
	pDC->SelectObject( pOldFont );
}

/////////////////////////////////////////////////////////////////////////////
// CODDriverComboBox::Paint (0x446790)

void CODDriverComboBox::Paint()
{
	CClientDC			dc( this );
	CDC					mem;
	CDC					rowMem;
	const drivrow_t*	pRecord;
	RECT				rc;
	RECT				band;
	RECT				arrow;
	BOOL				bFocus;
	int					sel;
	int					visRows, first, last, iRow, i;

	::GetClientRect( GetSafeHwnd(), &rc );
	::InflateRect( &rc, -3, -3 );
	rc.bottom = rc.top + 15;

	bFocus = ( CWnd::GetFocus() == this );

	if ( mem.CreateCompatibleDC( &dc ) )
	{
		CBitmap		bmp;
		CBitmap*	pOld;
		CFont*		pOldFace;

		bmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
			rc.right - rc.left, rc.bottom - rc.top ) );
		pOld = mem.SelectObject( &bmp );

		::SetRect( &band, 0, 0, rc.right - rc.left, rc.bottom - rc.top );
		mem.FillRect( &band, &m_brText );
		mem.SetBkMode( TRANSPARENT );
		mem.SetTextColor( m_clrText );
		pOldFace = mem.SelectObject( &m_faceFont );

		sel = m_pList->GetCurSel();
		if ( sel != -1 && !m_bEditing )
		{
			pRecord = (const drivrow_t*)m_pList->GetText( sel );
			if ( pRecord )
				mem.DrawText( pRecord->desc, -1, &band, DT_VCENTER | DT_NOPREFIX );
		}
		if ( m_bEditing )
		{
			::InvalidateRect( m_pEdit->GetSafeHwnd(), NULL, TRUE );
			::UpdateWindow( m_pEdit->GetSafeHwnd() );
		}

		::SetRect( &arrow, band.right - 15, band.top, band.right, band.bottom );
		DrawArrow( &mem, &arrow );

		mem.SelectObject( pOldFace );
		dc.BitBlt( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			&mem, 0, 0, SRCCOPY );
		mem.SelectObject( pOld );
	}

	if ( m_bDropped )
	{
		::GetClientRect( GetSafeHwnd(), &rc );
		for ( i = 0; i < 3; ++i )
		{
			CBrush	br( bFocus ? m_clrFrameFocus : m_clrFrame );
			dc.FrameRect( &rc, &br );
			::InflateRect( &rc, -1, -1 );
		}

		if ( m_pList->HasScrollbar() )
			rc.right -= 16;
		rc.top += 15;
		visRows = m_pList->GetVisibleRows();

		if ( rowMem.CreateCompatibleDC( &dc ) )
		{
			CBitmap		rowBmp;
			CBitmap*	pOldRow;
			RECT		rowFull;

			rowBmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
				rc.right - rc.left, rc.bottom - rc.top ) );
			pOldRow = rowMem.SelectObject( &rowBmp );

			::SetRect( &rowFull, 0, 0, rc.right - rc.left, rc.bottom - rc.top );
			rowMem.FillRect( &rowFull, &m_brText );

			m_pList->ClampTopIndex();
			first = m_pList->GetTopIndex();
			last  = first + visRows;
			if ( m_pList->GetCount() < last )
				last = m_pList->GetCount();
			for ( iRow = first; iRow < last; ++iRow )
			{
				if ( iRow >= 0 )
					DrawRow( &rowMem, iRow );
			}

			dc.BitBlt( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				&rowMem, 0, 0, SRCCOPY );
			rowMem.SelectObject( pOldRow );
			::ValidateRect( GetSafeHwnd(), &rc );
		}
	}
	else
	{
		for ( i = 0; i < 3; ++i )
		{
			::InflateRect( &rc, 1, 1 );
			CBrush	br( bFocus ? m_clrFrameFocus : m_clrFrame );
			dc.FrameRect( &rc, &br );
		}
		::ValidateRect( GetSafeHwnd(), &rc );
	}
}

/*
==================
Color_NameToRGB (0x455020)

The eight player colours settings.scr offers, by name.
==================
*/
COLORREF Color_NameToRGB( const char* pszName )
{
	if ( !_strcmpi( pszName, "orange" ) )	return RGB( 255, 180,  24 );
	if ( !_strcmpi( pszName, "blue"   ) )	return RGB(   0,  60, 255 );
	if ( !_strcmpi( pszName, "ltblue" ) )	return RGB(   0, 167, 255 );
	if ( !_strcmpi( pszName, "green"  ) )	return RGB(   0, 167,   0 );
	if ( !_strcmpi( pszName, "red"    ) )	return RGB( 255,  73,   0 );
	if ( !_strcmpi( pszName, "brown"  ) )	return RGB( 123,  73,   0 );
	if ( !_strcmpi( pszName, "ltgray" ) )	return RGB( 100, 100, 100 );
	if ( !_strcmpi( pszName, "dkgray" ) )	return RGB(  36,  36,  36 );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODColorComboBox::CODColorComboBox (0x453690)
//
// The base ctor plus a vftable swap; the compiler inlined it into its owner.

CODColorComboBox::CODColorComboBox()
{
}

/////////////////////////////////////////////////////////////////////////////
// CODColorComboBox::CurrentSwatch (0x455110)

COLORREF CODColorComboBox::CurrentSwatch()
{
	const char*	psz;
	char		szName[256];
	int			sel;

	sel = m_pList->GetCurSel();
	if ( sel == -1 )
		return 0;

	psz = m_pList->GetText( sel );
	if ( !psz )
		return 0;

	strncpy( szName, psz, sizeof( szName ) );
	szName[sizeof( szName ) - 1] = 0;
	return Color_NameToRGB( szName );
}

/////////////////////////////////////////////////////////////////////////////
// CODColorComboBox::Paint (0x455170)
//
// The face is the selected colour as a swatch, inset short of the arrow
// gutter.

void CODColorComboBox::Paint()
{
	CClientDC	dc( this );
	CDC			mem;
	CDC			rowMem;
	RECT		rc;
	RECT		full;
	RECT		rcSwatch;
	RECT		arrow;
	BOOL		bFocus;
	int			visRows, first, last, iRow, i;

	GetClientRect( &rc );
	::InflateRect( &rc, -3, -3 );
	rc.bottom = rc.top + 15;

	bFocus = ( CWnd::GetFocus() == this );

	if ( mem.CreateCompatibleDC( &dc ) )
	{
		CBitmap		bmp;
		CBitmap*	pOld;
		CFont*		pOldFace;

		bmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
			rc.right - rc.left, rc.bottom - rc.top ) );
		pOld = mem.SelectObject( &bmp );

		::SetRect( &full, 0, 0, rc.right - rc.left, rc.bottom - rc.top );
		mem.FillRect( &full, &m_brText );
		mem.SetBkMode( TRANSPARENT );
		mem.SetTextColor( m_clrText );
		pOldFace = mem.SelectObject( &m_faceFont );

		rcSwatch = full;
		::InflateRect( &rcSwatch, -3, -3 );
		rcSwatch.right -= 20;

		CBrush	brSwatch( CurrentSwatch() );
		mem.FillRect( &rcSwatch, &brSwatch );

		::SetRect( &arrow, full.right - 15, full.top, full.right, full.bottom );
		DrawArrow( &mem, &arrow );

		mem.SelectObject( pOldFace );
		dc.BitBlt( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			&mem, 0, 0, SRCCOPY );
		mem.SelectObject( pOld );
	}

	if ( m_bDropped )
	{
		GetClientRect( &rc );
		for ( i = 0; i < 3; ++i )
		{
			CBrush	br( bFocus ? m_clrFrameFocus : m_clrFrame );
			dc.FrameRect( &rc, &br );
			::InflateRect( &rc, -1, -1 );
		}

		if ( m_pList->HasScrollbar() )
			rc.right -= 16;
		rc.top += 15;
		visRows = m_pList->GetVisibleRows();

		if ( rowMem.CreateCompatibleDC( &dc ) )
		{
			CBitmap		rowBmp;
			CBitmap*	pOldRow;
			RECT		rowFull;

			rowBmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
				rc.right - rc.left, rc.bottom - rc.top ) );
			pOldRow = rowMem.SelectObject( &rowBmp );

			::SetRect( &rowFull, 0, 0, rc.right - rc.left, rc.bottom - rc.top );
			rowMem.FillRect( &rowFull, &m_brText );

			m_pList->ClampTopIndex();
			first = m_pList->GetTopIndex();
			last  = first + visRows;
			if ( m_pList->GetCount() < last )
				last = m_pList->GetCount();
			for ( iRow = first; iRow < last; ++iRow )
			{
				if ( iRow >= 0 )
					DrawRow( &rowMem, iRow );
			}

			dc.BitBlt( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
				&rowMem, 0, 0, SRCCOPY );
			rowMem.SelectObject( pOldRow );
			::ValidateRect( GetSafeHwnd(), &rc );
		}

		if ( GetScrollbar() )
			::UpdateWindow( GetScrollbar()->GetSafeHwnd() );
	}
	else
	{
		for ( i = 0; i < 3; ++i )
		{
			::InflateRect( &rc, 1, 1 );
			CBrush	br( bFocus ? m_clrFrameFocus : m_clrFrame );
			dc.FrameRect( &rc, &br );
		}
		::ValidateRect( GetSafeHwnd(), &rc );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODColorComboBox::DrawRow (0x455830)
//
// The row fill with the named colour as a swatch inset into it.

void CODColorComboBox::DrawRow( CDC* pDC, int iRow )
{
	const char*	pszText;
	RECT		client;
	RECT		rc;
	RECT		rcSwatch;
	int			vis;

	::GetClientRect( m_pList->GetSafeHwnd(), &client );
	client.bottom -= 6;
	client.right  -= 6;
	if ( m_pList->HasScrollbar() )
		client.right -= 16;

	vis = iRow - m_pList->GetTopIndex();
	if ( vis < 0 )
		return;

	rc.left   = 0;
	rc.top    = vis * m_rowHeight;
	rc.bottom = rc.top + m_rowHeight;
	rc.right  = client.right - client.left;

	pszText = m_pList->GetText( iRow );
	if ( !pszText )
		return;

	rcSwatch = rc;
	::InflateRect( &rcSwatch, -5, -5 );
	rcSwatch.right -= 5;

	pDC->FillRect( &rc, iRow == m_pList->GetCurSel() ? &m_brHot : &m_brText );

	CBrush	brSwatch( Color_NameToRGB( pszText ) );
	pDC->FillRect( &rcSwatch, &brSwatch );
}
