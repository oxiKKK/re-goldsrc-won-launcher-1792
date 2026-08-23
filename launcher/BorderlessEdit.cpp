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
// Purpose: CBorderLessEdit and CInputEdit, the frameless skinned edit
//          controls.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

#define IDC_BORDERLESS_EDIT		101			// the inner CEdit's child id

BEGIN_MESSAGE_MAP( CBorderLessEdit, CWnd )
	//{{AFX_MSG_MAP(CBorderLessEdit)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_CHAR()
	ON_WM_SIZE()
	ON_EN_SETFOCUS( IDC_BORDERLESS_EDIT, OnEditFocusChanged )
	ON_EN_KILLFOCUS( IDC_BORDERLESS_EDIT, OnEditFocusChanged )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::CBorderLessEdit (0x402DB0)

CBorderLessEdit::CBorderLessEdit()
{
	HBRUSH	hbr;
	HFONT	hf;

	m_pEdit        = NULL;
	m_bActive      = 0;
	m_clrEditBk    = 0;
	m_clrBorder    = 0;
	m_clrFocus     = RGB( 128, 128, 128 );
	m_clrEditText  = RGB( 240, 127, 24 );
	m_bPassword    = 0;

	hbr = ::CreateSolidBrush( RGB( 0, 0, 0 ) );
	if ( hbr )
		m_brBack.Attach( hbr );

	hf = ::CreateFontA( -12, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, VARIABLE_PITCH, "Arial" );
	if ( hf )
		m_font.Attach( hf );
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::~CBorderLessEdit (0x402E90)

CBorderLessEdit::~CBorderLessEdit()
{
	delete m_pEdit;
	m_pEdit = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::Create (0x402F30)

BOOL CBorderLessEdit::Create( DWORD dwStyle, RECT* prc, CWnd* pParent, UINT nID )
{
	RECT		rcDst;
	WNDCLASSA	wc;
	DWORD		style;

	m_nID = nID;

	::CopyRect( &rcDst, prc );

	memset( &wc, 0, sizeof( wc ) );
	wc.style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	wc.lpfnWndProc   = AfxGetAfxWndProc();
	wc.hInstance     = AfxGetInstanceHandle();
	wc.hCursor       = ::LoadCursorA( NULL, IDC_IBEAM );
	wc.hbrBackground = (HBRUSH)::GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "ODBorderlessEdit";
	if ( !AfxRegisterClass( &wc ) )
	{
		Launcher_ShowMessageById( 0, IDS_BORDERLESS_REGFAIL );
		return FALSE;
	}

	if ( !CreateEx( WS_EX_NOPARENTNOTIFY, "ODBorderlessEdit", "",
		dwStyle | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		rcDst.left, rcDst.top, rcDst.right - rcDst.left, rcDst.bottom - rcDst.top,
		pParent ? pParent->GetSafeHwnd() : NULL, (HMENU)nID, NULL ) )
		return FALSE;

	m_pEdit = new CEdit;
	if ( !m_pEdit )
		return FALSE;

	style = ( dwStyle & ES_PASSWORD ) | WS_CHILD | WS_VISIBLE;
	if ( m_bAutoHScroll )
		style |= ES_AUTOHSCROLL;
	if ( m_bPassword )
		style |= ES_PASSWORD;

	CRect	rc( 0, 0, rcDst.right - rcDst.left, rcDst.bottom - rcDst.top );
	rc.InflateRect( -3, -3 );
	rc.top += 1;									// the edit sits 1px lower

	if ( !m_pEdit->Create( style, rc, this, IDC_BORDERLESS_EDIT ) )
		return FALSE;

	m_pEdit->ShowWindow( SW_RESTORE );
	m_pEdit->SetFont( &m_font );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::OnPaint (0x403120)

void CBorderLessEdit::OnPaint()
{
	CPaintDC	dc( this );
	CRect		rc;
	CBrush		brBlack( RGB( 0, 0, 0 ) );
	BOOL		bFocus;
	COLORREF	clrBorder;
	int			i;

	GetClientRect( &rc );

	for ( i = 0; i < 3; i++ )					// 3px border
	{
		bFocus = ( CWnd::FromHandle( ::GetFocus() ) == this ) ||
			( CWnd::FromHandle( ::GetFocus() ) == m_pEdit );
		clrBorder = bFocus ? m_clrFocus : m_clrBorder;

		CBrush	br( clrBorder );
		dc.FrameRect( &rc, &br );
		rc.InflateRect( -1, -1 );
	}

	dc.FillRect( &rc, &brBlack );

	// NOTE(ox): 0x403120 derefs +92 unguarded, because in the binary every object
	// reaching this paint went through Create() and owns an inner edit.  CRoomDialog's
	// +224/+528 controls are DDX-attached and are 304-byte owner-draw controls in the
	// original (+224 -> +528 -> +832), not CBorderLessEdit -- until those are retyped
	// they land here with a null m_pEdit, so guard rather than fault.
	if ( m_pEdit )
	{
		m_pEdit->Invalidate( TRUE );
		m_pEdit->UpdateWindow();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::OnEraseBkgnd (0x4032B0)

BOOL CBorderLessEdit::OnEraseBkgnd( CDC* pDC )
{
	CRect		rc;
	CBrush		brBlack( RGB( 0, 0, 0 ) );
	BOOL		bFocus;
	COLORREF	clrBorder;
	int			i;

	GetClientRect( &rc );

	for ( i = 0; i < 3; i++ )					// 3px border
	{
		bFocus = ( CWnd::FromHandle( ::GetFocus() ) == this ) ||
			( CWnd::FromHandle( ::GetFocus() ) == m_pEdit );
		clrBorder = bFocus ? m_clrFocus : m_clrBorder;

		CBrush	br( clrBorder );
		pDC->FrameRect( &rc, &br );
		rc.InflateRect( -1, -1 );
	}

	pDC->FillRect( &rc, &brBlack );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::OnCtlColor (0x403400)
//
// Reflected from the inner CEdit.

HBRUSH CBorderLessEdit::OnCtlColor( CDC* pDC, CWnd* /*pWnd*/, UINT /*nCtlColor*/ )
{
	pDC->SetTextColor( m_clrEditText );
	pDC->SetBkColor( m_clrEditBk );
	return (HBRUSH)m_brBack.GetSafeHandle();
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::OnSetFocus (0x403430)

void CBorderLessEdit::OnSetFocus( CWnd* /*pOldWnd*/ )
{
	Default();

	if ( m_bActive && m_pEdit )
		m_pEdit->SetFocus();
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::SetBorderColor (0x403450)

void CBorderLessEdit::SetBorderColor( COLORREF clr )
{
	m_clrBorder = clr;
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::OnChar (0x403460)

void CBorderLessEdit::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( m_pEdit && m_pEdit->GetSafeHwnd() )
		m_pEdit->SendMessage( WM_CHAR, nChar, MAKELPARAM( nRepCnt, nFlags ) );
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::SetEditTextColor (0x4034A0)

void CBorderLessEdit::SetEditTextColor( COLORREF clr )
{
	m_clrEditText = clr;
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::OnSize (0x4034B0)
//
// The inner edit tracks the frame.

void CBorderLessEdit::OnSize( UINT /*nType*/, int cx, int cy )
{
	Default();

	CRect	rc( 0, 0, cx, cy );
	rc.InflateRect( -3, -3 );
	rc.top += 1;

	if ( m_pEdit && m_pEdit->GetSafeHwnd() )
		m_pEdit->MoveWindow( rc.left, rc.top, rc.Width(), rc.Height(), TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::SetActive (0x403530)

void CBorderLessEdit::SetActive( int bActive )
{
	m_bActive = bActive;
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::SetText (0x403540)

void CBorderLessEdit::SetText( const char* psz )
{
	if ( m_pEdit && m_pEdit->GetSafeHwnd() )
		m_pEdit->SetWindowText( psz );
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::SetAutoHScroll (0x403560)

void CBorderLessEdit::SetAutoHScroll()
{
	m_bAutoHScroll = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::SetPasswordMode (0x403570)

void CBorderLessEdit::SetPasswordMode()
{
	m_bPassword = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::OnKillFocus (0x420320)

void CBorderLessEdit::OnKillFocus( CWnd* /*pNewWnd*/ )
{
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::OnEditFocusChanged (0x441DF0)
//
// Repaints the border when the inner edit gains or loses focus.

void CBorderLessEdit::OnEditFocusChanged()
{
	Invalidate();
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit::OnLButtonDown (0x455E00)

void CBorderLessEdit::OnLButtonDown( UINT /*nFlags*/, CPoint /*point*/ )
{
	Default();
}
