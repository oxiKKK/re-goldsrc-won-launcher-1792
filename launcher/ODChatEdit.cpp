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
// Purpose: CODChatEdit, the chat transcript control.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

static char	s_wrapScratch[2304];		// 0x4F39E8  word-wrap work buffer
static char	s_szChatFormat[2048];		// 0x4EB698  shared line-formatting scratch
static int	s_odChatPrevSel = -1;		// 0x4D0D40  line to repaint when the caret moves

static void ODChat_WrapLine( CDC* pDC, const char* pszText, int maxWidth,
	int startX, int* pCount, int* pWidth );
static void __stdcall ODChat_FreeLine( odchatline_t* pLine );

BEGIN_MESSAGE_MAP( CODChatEdit, CWnd )
	//{{AFX_MSG_MAP(CODChatEdit)
	ON_WM_NCDESTROY()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_VSCROLL()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	ON_WM_GETDLGCODE()
	ON_WM_CREATE()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::CODChatEdit (0x442890)

CODChatEdit::CODChatEdit()
{
	InitMembers();
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::~CODChatEdit (0x442930)

CODChatEdit::~CODChatEdit()
{
	ClearAllLines();
	delete[] m_lines;
}

/*
==================
ChatWnd_Printf (0x4429F0)

Formats through the shared scratch buffer, then hands the line to the control.
==================
*/
void ChatWnd_Printf( CODChatEdit* pWnd, const char* pszNick, const char* pszFormat, ... )
{
	va_list	va;

	memset( s_szChatFormat, 0, sizeof( s_szChatFormat ) );
	va_start( va, pszFormat );
	vsprintf( s_szChatFormat, pszFormat, va );
	va_end( va );

	if ( pWnd )
		pWnd->AddChatLine( 0, pszNick, s_szChatFormat );
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::AddChatLine (0x442A30)
//
// Word-wraps the body in the control's font and appends one ring record per
// display line; the first carries the "<nick>" prefix, the rest are marked as
// continuations.

void CODChatEdit::AddChatLine( int, const char* pszSpeaker, const char* pszText )
{
	odchatline_t	line;
	CString			str;
	CRect			rc;
	char			szBody[2048];
	char*			d;
	char*			p;
	const char*		s;
	CFont*			pOldFont;
	BOOL			bSelf;
	BOOL			bAtBottom;
	int				oldPos;
	int				count, width, len;

	if ( !pszText )
		return;

	// strip CR/LF/TAB from the body
	d = szBody;
	for ( s = pszText; *s; ++s )
		if ( *s != '\r' && *s != '\n' && *s != '\t' )
			*d++ = *s;
	*d = 0;

	memset( &line, 0, sizeof( line ) );
	line.prefixWidth = 0;

	// frame the "<speaker>" / "<speaker -> target>" prefix
	str.Empty();
	if ( pszSpeaker )
	{
		str = "<";
		str += pszSpeaker;
		if ( m_bWhisper )
		{
			str += " ";
			str += "-> ";
			str += m_szWhisperTarget;
		}
		str += ">";
	}

	len = str.GetLength();
	line.raw = new char[len + 1];
	if ( !line.raw )
	{
		Launcher_ShowMessageById( 0, IDS_CHATCTRL_NOMEM );
		PostQuitMessage( 0 );
		return;
	}
	strcpy( line.raw, str );
	line.raw[len] = 0;

	if ( m_nLines >= ODCHAT_MAX_LINES )
		ResetContent();

	// colorize: system text vs self/other speaker
	if ( pszSpeaker )
	{
		bSelf = ( _strcmpi( pszSpeaker, m_szSelfNick ) == 0 );
		if ( m_bWhisper )
			line.nameColor = m_clrWhisper;
		else
			line.nameColor = bSelf ? m_clrName : m_clrOther;
		// Only other people's lines carry the olive body colour.
		line.bodyColor = bSelf ? m_clrText : RGB( 116, 116, 56 );
		m_bWhisper = 0;
	}
	else
	{
		line.bodyColor = m_clrText;
	}

	// Autoscroll bookkeeping: remember whether the thumb was parked at the
	// bottom (so a new line keeps following) and the current scroll position.
	bAtBottom = IsScrolledToBottom();
	oldPos    = m_pScrollbar ? m_pScrollbar->GetPos() : 0;

	CClientDC	dc( this );
	pOldFont = dc.SelectObject( &m_font );

	GetClientRect( &rc );
	rc.right -= 22;

	count = 0;
	width = 0;

	if ( pszSpeaker )
	{
		ODChat_WrapLine( &dc, line.raw, rc.right - rc.left, 8, &count, &width );
		line.prefixWidth = width + 2;
	}

	ODChat_WrapLine( &dc, szBody, rc.right - rc.left - line.prefixWidth, 8,
		&count, &width );

	line.text = new char[count + 1];
	if ( !line.text )
	{
		delete[] line.raw;
		dc.SelectObject( pOldFont );
		Launcher_ShowMessageById( 0, IDS_CHATCTRL_NOTEXTMEM );
		PostQuitMessage( 0 );
		return;
	}
	strncpy( line.text, szBody, count );
	line.text[count] = 0;
	AppendLine( &line );

	// one continuation record per remaining segment
	if ( count != (int)strlen( szBody ) )
	{
		for ( p = &szBody[count + 1]; (int)strlen( p ) > 0; p += count + 1 )
		{
			line.flags      |= ODCHAT_LINE_CONTINUATION;
			line.prefixWidth = 0;
			line.raw         = NULL;

			ODChat_WrapLine( &dc, p, rc.right - rc.left, 8, &count, &width );
			line.text = new char[count + 1];
			if ( !line.text )
			{
				dc.SelectObject( pOldFont );
				Launcher_ShowMessageById( 0, IDS_CHATCTRL_NOTEXTMEM );
				PostQuitMessage( 0 );
				return;
			}
			strncpy( line.text, p, count );
			line.text[count] = 0;
			AppendLine( &line );
		}
	}

	dc.SelectObject( pOldFont );

	// Fit the scrollbar to the new line count, then keep following the bottom
	// if we were already pinned there.
	UpdateScrollbarVisibility();
	if ( m_pScrollbar )
		m_pScrollbar->SetPos( bAtBottom ? m_nLines : oldPos );

	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::SetSelfNick (0x442F20)

void CODChatEdit::SetSelfNick( const char* pszNick )
{
	if ( pszNick )
		lstrcpynA( m_szSelfNick, pszNick, sizeof( m_szSelfNick ) );
	else
		m_szSelfNick[0] = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::SetWhisperTarget (0x442F50)

void CODChatEdit::SetWhisperTarget( const char* pszTarget )
{
	if ( pszTarget )
		lstrcpynA( m_szWhisperTarget, pszTarget, sizeof( m_szWhisperTarget ) );
	else
		m_szWhisperTarget[0] = 0;

	m_bWhisper = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::Create (0x442FA0)

BOOL CODChatEdit::Create( DWORD dwStyle, RECT* prc, CWnd* pParent, UINT nID )
{
	WNDCLASSA	wc;
	RECT		rcDst;
	RECT		rcSb;

	m_rowHeight  = 15;
	m_pChatOwner = pParent;

	::CopyRect( &rcDst, prc );

	memset( &wc, 0, sizeof( wc ) );
	wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = AfxGetAfxWndProc();
	wc.hInstance     = AfxGetInstanceHandle();
	wc.hbrBackground = (HBRUSH)::GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "CODChatEditCls";
	wc.hCursor       = ::LoadCursorA( NULL, IDC_ARROW );
	if ( !AfxRegisterClass( &wc ) )
	{
		Launcher_ShowMessageById( 0, IDS_CHATCTRL_WINREGFAIL );
		return FALSE;
	}

	if ( !CreateEx( 0, "CODChatEditCls", "",
		dwStyle | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP,
		rcDst.left, rcDst.top, rcDst.right - rcDst.left, rcDst.bottom - rcDst.top,
		pParent ? pParent->GetSafeHwnd() : NULL, (HMENU)(UINT_PTR)nID, NULL ) )
		return FALSE;

	m_pScrollbar = new CODScrollBar;
	rcSb.left   = rcDst.right - rcDst.left - 19;
	rcSb.top    = 3;
	rcSb.right  = rcDst.right - rcDst.left - 3;
	rcSb.bottom = rcDst.bottom - rcDst.top - 3;
	if ( !m_pScrollbar->Create( this, &rcSb, m_rowHeight ) )
		return FALSE;

	m_pScrollbar->ShowWindow( SW_HIDE );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::InitMembers (0x4431A0)
//
// The binary leaves m_bWhisper and the two name buffers uninitialised; retail
// got away with it because a fresh heap block reads as zero.  A debug heap does
// not, so the first line drawn came out as "<nick -> 0xCD junk>".  Zero them.

void CODChatEdit::InitMembers()
{
	m_bWhisper      = 0;
	m_szWhisperTarget[0] = 0;
	m_szSelfNick[0] = 0;

	m_bAutoDelete   = 1;
	m_pChatOwner    = NULL;
	m_nLines        = 0;
	m_rowHeight     = 15;
	m_topLine       = 0;
	m_bHasScrollbar = 0;
	m_curLine       = -1;
	m_pScrollbar    = NULL;
	m_unk64         = 0;

	m_clrText       = RGB( 255, 255, 255 );
	m_clrName       = RGB( 192, 192, 192 );
	m_clrOther      = RGB( 127, 100, 56 );
	m_clrWhisper    = RGB( 255, 180, 24 );
	m_clrBorder     = RGB( 255, 150, 24 );
	m_clrFrame      = RGB( 56, 56, 56 );
	m_clrFocus      = RGB( 128, 128, 128 );

	m_brBack.Attach( ::CreateSolidBrush( RGB( 84, 45, 0 ) ) );

	m_lines = new odchatline_t[ODCHAT_RING_SLOTS];
	memset( m_lines, 0, ODCHAT_RING_SLOTS * sizeof( odchatline_t ) );

	m_brBlack.Attach( ::CreateSolidBrush( RGB( 0, 0, 0 ) ) );

	m_font.Attach( ::CreateFontA( -11, 0, 0, 0, 400, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, 2, "Arial" ) );
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::OnNcDestroy (0x4432B0)

void CODChatEdit::OnNcDestroy()
{
	CWnd::OnNcDestroy();
	if ( m_bAutoDelete )
		delete this;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::ScrollPageUp (0x4432E0)

void CODChatEdit::ScrollPageUp()
{
	int	line = GetCurSel() - GetVisibleRows();

	if ( line < 0 )
		line = 0;

	SetCurSel( line );
	if ( line < GetTopLine() )
		m_pScrollbar->SetPos( line );
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::ScrollPageDown (0x443320)

void CODChatEdit::ScrollPageDown()
{
	int	line = GetCurSel() + GetVisibleRows();

	if ( line >= GetLineCount() )
		line = GetLineCount() - 1;

	SetCurSel( line );
	if ( line > GetVisibleRows() + GetTopLine() - 1 )
		m_pScrollbar->SetPos( line );
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::ScrollLineUp (0x443380)

void CODChatEdit::ScrollLineUp()
{
	int	line;

	GetVisibleRows();

	line = GetCurSel() - 1;
	if ( line < 0 )
		line = 0;

	SetCurSel( line );
	if ( line < GetTopLine() )
		m_pScrollbar->SetPos( line );
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::ScrollLineDown (0x4433C0)

void CODChatEdit::ScrollLineDown()
{
	int	line;

	GetVisibleRows();

	line = GetCurSel() + 1;
	if ( line >= GetLineCount() )
		line = GetLineCount() - 1;

	SetCurSel( line );
	if ( line > GetVisibleRows() + GetTopLine() - 1 )
		m_pScrollbar->SetPos( line );
}

/*
==================
ODChat_WrapLine (0x443420)

Fits as much of pszText as will run to maxWidth, breaking at the last space.
==================
*/
static void ODChat_WrapLine( CDC* pDC, const char* pszText, int maxWidth,
	int startX, int* pCount, int* pWidth )
{
	const char*	pFit = pszText;
	int			len = lstrlenA( pszText );
	int			extent;
	int			brk, mark;
	SIZE		sz;

	if ( len )
	{
		::GetTextExtentPoint32A( pDC->GetSafeHdc(), pszText, len, &sz );
		extent = startX + sz.cx;

		if ( extent > maxWidth )
		{
			lstrcpyA( s_wrapScratch, pszText );

			extent = maxWidth;
			for ( brk = len - 1; brk > 0; --brk )
			{
				// walk back to the previous space; on none, hard-break at brk
				mark = brk;
				while ( s_wrapScratch[brk] != ' ' )
				{
					if ( --brk <= 0 )
					{
						brk = mark;
						break;
					}
				}

				s_wrapScratch[brk] = 0;
				::GetTextExtentPoint32A( pDC->GetSafeHdc(), s_wrapScratch, brk, &sz );
				extent = sz.cx + startX;
				if ( extent <= maxWidth )
					break;
			}
			pFit = s_wrapScratch;
		}
	}
	else
	{
		extent = startX;
	}

	*pCount = (int)strlen( pFit );
	*pWidth = extent;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::OnPaint (0x4434F0)

void CODChatEdit::OnPaint()
{
	CPaintDC	dc( this );
	CDC			mem;
	CRect		rc;
	CRect		rcB;
	CWnd*		pParent;
	COLORREF	clr;
	int			last, i;

	GetClientRect( &rc );
	if ( m_bHasScrollbar )
		rc.right -= 16;
	::ValidateRect( GetSafeHwnd(), &rc );
	rc.InflateRect( -3, -3 );

	SyncTopLine();

	if ( mem.CreateCompatibleDC( &dc ) )
	{
		CBitmap	bmp;
		CBitmap*	pOld;

		bmp.CreateCompatibleBitmap( &dc, rc.Width(), rc.Height() );
		pOld = mem.SelectObject( &bmp );

		CRect	buf( 0, 0, rc.Width(), rc.Height() );
		mem.FillRect( &buf, &m_brBlack );

		last = m_topLine + GetVisibleRows();
		if ( last > m_nLines )
			last = m_nLines;
		for ( i = m_topLine; i < last; i++ )
			if ( i >= 0 )
				DrawLine( &mem, i );

		dc.BitBlt( 3, 3, rc.Width(), rc.Height(), &mem, 0, 0, SRCCOPY );
		mem.SelectObject( pOld );

		// Three concentric frames: focus colour if the pane has the focus.
		GetClientRect( &rcB );
		clr = ( CWnd::FromHandle( ::GetFocus() ) == this ) ? m_clrFocus : m_clrFrame;
		for ( i = 0; i < 3; i++ )
		{
			CBrush	br( clr );
			dc.FrameRect( &rcB, &br );
			rcB.InflateRect( -1, -1 );
		}
	}

	// The pane owns its slice of the parent's background.
	pParent = CWnd::FromHandle( ::GetParent( GetSafeHwnd() ) );
	if ( pParent )
	{
		GetWindowRect( &rcB );
		pParent->ScreenToClient( &rcB );
		::ValidateRect( pParent->GetSafeHwnd(), &rcB );
	}

	if ( m_bHasScrollbar && m_pScrollbar )
	{
		GetClientRect( &rcB );
		rcB.InflateRect( -3, -3 );
		rcB.left = rcB.right - 16;
		ClientToScreen( &rcB );
		m_pScrollbar->ScreenToClient( &rcB );
		::InvalidateRect( m_pScrollbar->GetSafeHwnd(), &rcB, TRUE );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::DrawLine (0x4438C0)
//
// The "<nick>" prefix draws first in its own colour, clipped to prefixWidth;
// continuation lines and system lines start the body at x = 0.

void CODChatEdit::DrawLine( CDC* pMemDC, int iLine )
{
	odchatline_t*	pLine;
	CRect			rc;
	CRect			row;
	CFont*			pOld;
	char			szBuf[2304];
	int				y, xLeft;

	if ( iLine < 0 || iLine >= m_nLines || !m_lines )
		return;
	pLine = &m_lines[iLine];

	GetClientRect( &rc );
	rc.right  -= 6;
	rc.bottom -= 6;
	if ( m_bHasScrollbar )
		rc.right -= 16;

	y = ( iLine - m_topLine ) * m_rowHeight;

	row.SetRect( 0, y, rc.right - rc.left, y + m_rowHeight );
	pMemDC->FillRect( &row, &m_brBlack );

	pMemDC->SetBkMode( TRANSPARENT );
	pOld = pMemDC->SelectObject( &m_font );

	xLeft = 0;
	if ( pLine->flags >= 0 && pLine->raw && pLine->raw[0] )
	{
		CRect	pre( 2, y, pLine->prefixWidth, y + m_rowHeight );
		_snprintf( szBuf, sizeof( szBuf ) - 1, " %s", pLine->raw );
		szBuf[sizeof( szBuf ) - 1] = 0;
		pMemDC->SetTextColor( pLine->nameColor );
		pMemDC->DrawText( szBuf, -1, &pre, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );
		xLeft = pLine->prefixWidth;
	}

	if ( pLine->text )
	{
		CRect	body( xLeft + 2, y, rc.right - rc.left, y + m_rowHeight );
		_snprintf( szBuf, sizeof( szBuf ) - 1, " %s", pLine->text );
		szBuf[sizeof( szBuf ) - 1] = 0;
		pMemDC->SetTextColor( pLine->bodyColor );
		pMemDC->DrawText( szBuf, -1, &body, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );
	}

	pMemDC->SelectObject( pOld );
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::AppendLine (0x443AA0)

void CODChatEdit::AppendLine( const odchatline_t* pLine )
{
	if ( !m_lines || m_nLines >= ODCHAT_RING_SLOTS )
		return;

	memcpy( &m_lines[m_nLines], pLine, sizeof( odchatline_t ) );
	m_nLines++;
	SetCurSel( m_nLines - 1 );

	if ( m_pScrollbar )
	{
		m_pScrollbar->SetRange( 0, m_nLines );
		m_pScrollbar->SetPos( m_curLine );
	}

	UpdateScrollbarVisibility();
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::ClearAllLines (0x443B20)

void CODChatEdit::ClearAllLines()
{
	int	i;

	if ( m_lines )
	{
		for ( i = 0; i < ODCHAT_RING_SLOTS; i++ )
			ODChat_FreeLine( &m_lines[i] );
		memset( m_lines, 0, ODCHAT_RING_SLOTS * sizeof( odchatline_t ) );
	}
	m_nLines = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::ResetContent (0x443B80)

void CODChatEdit::ResetContent()
{
	ClearAllLines();
	SetCurSel( -1 );
	m_pScrollbar->SetRange( 0, 100 );
	m_pScrollbar->SetPos( 0 );
	UpdateScrollbarVisibility();
}

/*
==================
ODChat_FreeLine (0x443BC0)

Releases one ring slot's heap strings.
==================
*/
static void __stdcall ODChat_FreeLine( odchatline_t* pLine )
{
	if ( !pLine )
		return;

	delete[] pLine->text;
	delete[] pLine->raw;

	memset( pLine, 0, sizeof( odchatline_t ) );
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::SetCurSel (0x443C00)
//
// (sic) iLine is dropped: the binary only repaints the previously touched line
// and then clears the selection.

void CODChatEdit::SetCurSel( int )
{
	if ( s_odChatPrevSel != -1 && s_odChatPrevSel < m_nLines && s_odChatPrevSel >= 0 )
	{
		CClientDC	dc( this );
		DrawLine( &dc, s_odChatPrevSel );
	}

	m_curLine       = -1;
	s_odChatPrevSel = -1;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::GetVisibleRows (0x443C90)

int CODChatEdit::GetVisibleRows()
{
	CRect	rc;

	GetClientRect( &rc );
	rc.InflateRect( -3, -3 );

	if ( m_rowHeight )
		return (int)( (double)rc.Height() / (double)m_rowHeight + 0.5 );
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::UpdateScrollbarVisibility (0x443D00)

void CODChatEdit::UpdateScrollbarVisibility()
{
	if ( m_nLines <= GetVisibleRows() || m_bHasScrollbar )
	{
		if ( m_nLines > GetVisibleRows() )
			goto show;					// already shown and still overflowing

		if ( !m_bHasScrollbar )
			return;						// already hidden and still fits

		m_bHasScrollbar = 0;			// fits now -> retract the bar
		if ( m_pScrollbar )
			m_pScrollbar->ShowWindow( SW_HIDE );
	}
	else
	{
		m_bHasScrollbar = 1;			// overflowed -> bring the bar in
		if ( m_pScrollbar )
			m_pScrollbar->ShowWindow( SW_SHOWNA );
	}

	::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( GetSafeHwnd() );

show:
	if ( m_bHasScrollbar && m_pScrollbar )
		m_pScrollbar->ShowWindow( SW_SHOWNA );
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::GetCurSel (0x443DC0)

int CODChatEdit::GetCurSel()
{
	return m_curLine;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::GetLineCount (0x443DD0)

int CODChatEdit::GetLineCount()
{
	return m_nLines;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::GetTopLine (0x443DE0)

int CODChatEdit::GetTopLine()
{
	return m_topLine;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::SyncTopLine (0x443DF0)
//
// The companion scrollbar owns the offset; OnPaint adopts its position,
// clamped to the last page.

void CODChatEdit::SyncTopLine()
{
	int	visible = GetVisibleRows();
	int	maxTop;
	int	pos;

	if ( !m_nLines || !m_pScrollbar )
		return;

	maxTop = m_nLines - visible;
	if ( maxTop < 0 )
		maxTop = 0;

	pos = m_pScrollbar->GetPos();
	m_topLine = ( pos <= maxTop ) ? pos : maxTop;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::OnSize (0x443E30)

void CODChatEdit::OnSize( UINT, int cx, int cy )
{
	Default();

	if ( m_pScrollbar && m_pScrollbar->GetSafeHwnd() )
		m_pScrollbar->MoveWindow( cx - 19, 3, 16, cy - 6, TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::OnKeyDown (0x443E70)

void CODChatEdit::OnKeyDown( UINT nChar, UINT, UINT )
{
	HWND	hNext;

	switch ( nChar )
	{
	case VK_TAB:
		// (sic) both arms ask for the next item -- shift-tab does not go back
		if ( ::GetAsyncKeyState( VK_SHIFT ) )
			hNext = ::GetNextDlgTabItem( GetParent()->GetSafeHwnd(), GetSafeHwnd(), FALSE );
		else
			hNext = ::GetNextDlgTabItem( GetParent()->GetSafeHwnd(), GetSafeHwnd(), FALSE );

		if ( hNext )
			::SetFocus( hNext );
		break;

	case VK_PRIOR:
		ScrollPageUp();
		return;

	case VK_NEXT:
		ScrollPageDown();
		return;

	case VK_UP:
		ScrollLineUp();
		return;

	case VK_DOWN:
		ScrollLineDown();
		return;

	default:
		break;
	}

	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::IsScrolledToBottom (0x443F70)

BOOL CODChatEdit::IsScrolledToBottom()
{
	return m_nLines <= 0 || ( m_pScrollbar && m_pScrollbar->GetPos() == m_nLines );
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::OnCreate (0x443FA0)

int CODChatEdit::OnCreate( LPCREATESTRUCT )
{
	return (int)Default();
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::OnLButtonDown (0x428BD0)
//
// The chat pane ignores the mouse; the three button/move handlers folded to
// one empty body.

void CODChatEdit::OnLButtonDown( UINT, CPoint )
{
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::OnLButtonUp (0x428BD0)

void CODChatEdit::OnLButtonUp( UINT, CPoint )
{
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::OnMouseMove (0x428BD0)

void CODChatEdit::OnMouseMove( UINT, CPoint )
{
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::OnVScroll (0x44A0B0)
//
// The companion scrollbar owns the offset; the pane only has to repaint, and
// OnPaint reads the bar back through SyncTopLine.

void CODChatEdit::OnVScroll( UINT, UINT, CScrollBar* )
{
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::OnGetDlgCode (0x44A4D0)

UINT CODChatEdit::OnGetDlgCode()
{
	return DLGC_WANTALLKEYS;
}

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit::OnEraseBkgnd (0x4515E0)
//
// OnPaint covers the client, so hand the background back to the parent
// instead of erasing it here.

BOOL CODChatEdit::OnEraseBkgnd( CDC* )
{
	CWnd*	pParent = GetParent();
	RECT	rc;

	if ( pParent )
	{
		::GetWindowRect( GetSafeHwnd(), &rc );
		::MapWindowPoints( NULL, pParent->GetSafeHwnd(), (LPPOINT)&rc, 2 );
		::ValidateRect( pParent->GetSafeHwnd(), &rc );
	}
	return TRUE;
}
