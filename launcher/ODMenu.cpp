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
// Purpose: CODMenu, the skinned owner-draw popup menu.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The owner-draw item-text scratch (byte_4F4BF4 in the binary).
static char	g_szODMenuText[0x104];	// 0x4F4BF4

/////////////////////////////////////////////////////////////////////////////
// CODMenu::~CODMenu (0x448BF0)

CODMenu::~CODMenu()
{
	m_font.DeleteObject();
}

/////////////////////////////////////////////////////////////////////////////
// CODMenu::CODMenu (0x44CC70)

CODMenu::CODMenu( const char* pszFace, int nSize, int nWeight )
{
	m_clrText	 = RGB( 255, 255, 255 );
	m_clrBg		 = 0;
	m_clrSelText = RGB( 255, 182, 24 );
	m_clrSelBg	 = RGB( 80, 56, 24 );

	HFONT	hf = ::CreateFontA( -nSize, 0, 0, 0, nWeight, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, 2, pszFace );
	if ( hf )
		m_font.Attach( hf );
}

/////////////////////////////////////////////////////////////////////////////
// CODMenu::DrawItem (0x44CD30)

void CODMenu::DrawItem( LPDRAWITEMSTRUCT lpDIS )
{
	CDC*	pDC = CDC::FromHandle( lpDIS->hDC );

	RECT	rcText = lpDIS->rcItem;
	RECT	rcFill = lpDIS->rcItem;

	::GetBoundsRect( lpDIS->hDC, &rcText, DCB_RESET );
	pDC->SetBkColor( m_clrBg );
	pDC->SetTextColor( m_clrText );
	strncpy( g_szODMenuText, (const char*)lpDIS->itemData, 0x104 );
	g_szODMenuText[0x103] = 0;

	rcText.left  += 4;
	rcText.right -= 4;

	CFont*	pOldFont = pDC->SelectObject( &m_font );
	int		oldMode  = pDC->SetBkMode( TRANSPARENT );

	BOOL	bSel	= ( lpDIS->itemState & ODS_SELECTED ) || ( lpDIS->itemState & ODS_FOCUS );
	UINT	state	= ::GetMenuState( (HMENU)m_hMenu, lpDIS->itemID, MF_BYCOMMAND );
	BOOL	bGray	= ( state & MF_GRAYED );

	COLORREF	oldText, oldBk;
	if ( bSel )
	{
		oldText = pDC->SetTextColor( m_clrSelText );
		oldBk	= pDC->SetBkColor( m_clrSelBg );
		CBrush	br( m_clrSelBg );
		pDC->FillRect( &rcFill, &br );
	}
	else
	{
		oldText = pDC->SetTextColor( m_clrText );
		oldBk	= pDC->SetBkColor( m_clrBg );
		CBrush	br( m_clrBg );
		pDC->FillRect( &rcFill, &br );
	}

	if ( bGray )
		pDC->SetTextColor( RGB( 127, 127, 127 ) );

	pDC->DrawText( g_szODMenuText, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );

	pDC->SetBkMode( oldMode );
	pDC->SetBkColor( oldBk );
	pDC->SetTextColor( oldText );
	pDC->SelectObject( pOldFont );
}

/////////////////////////////////////////////////////////////////////////////
// CODMenu::MeasureItem (0x44CF50)

void CODMenu::MeasureItem( LPMEASUREITEMSTRUCT lpMIS )
{
	CWnd*	pMain = AfxGetMainWnd();
	HWND	hMain = pMain ? pMain->GetSafeHwnd() : NULL;
	HDC		hdc = ::GetDC( hMain );
	CDC*	pDC = CDC::FromHandle( hdc );

	CFont*	pOld = pDC->SelectObject( &m_font );
	TEXTMETRIC	tm;
	pDC->GetTextMetrics( &tm );
	pDC->SelectObject( pOld );
	::ReleaseDC( hMain, hdc );

	const char*	psz = (const char*)lpMIS->itemData;
	lpMIS->itemHeight = 12;
	if ( psz && strlen( psz ) )
	{
		int	len = lstrlenA( psz );
		lpMIS->itemWidth = (int)( 10.0 + (double)len * (double)tm.tmAveCharWidth * 1.3 );
	}
	else
	{
		lpMIS->itemWidth = 20;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODRuleListCtrl::DrawRow (0x44D270)
//
// key | value

void CODRuleListCtrl::DrawRow( CDC* pDC, int iRow )
{
	odrow_t*		pRec;
	CServerRule*	pRule;
	CFont*			pOldFont;
	RECT			rcRow;
	RECT			rc;
	BOOL			bSel;
	int				iVis, x;

	pRec = m_rows ? m_rows[iRow] : NULL;
	if ( !pRec )
		return;
	pRule = (CServerRule*)pRec->record;
	if ( !pRule )
		return;

	bSel = ( pRec->flags & 1 );

	GetClientRect( &rcRow );
	if ( m_bHasScrollbar )
		rcRow.right -= 16;
	iVis = iRow - m_topRow;
	if ( iVis < 0 )
		return;

	rcRow.top    = iVis * m_rowHeight;
	rcRow.bottom = m_rowHeight + rcRow.top - 1;

	pDC->SetBkColor( m_clrRowBg );
	if ( !m_bTransparent )
		pDC->FillRect( &rcRow, bSel ? &m_brHighlight : &m_brBg );

	pDC->SetTextColor( bSel ? m_clrSelText : m_clrRowText );
	pDC->SetBkMode( TRANSPARENT );
	pDC->SetBkColor( m_clrRowBg );
	pOldFont = pDC->SelectObject( &m_headerFont );

	x = 0;
	if ( m_nCols > 0 )
	{
		::SetRect( &rc, x + 2, rcRow.top, x + m_cols[0].width, rcRow.bottom );
		pDC->DrawText( CODList_EllipsizeText( pDC, pRule->m_strKey, m_cols[0].width, 2 ),
			-1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );
		x += m_cols[0].width;
	}
	if ( m_nCols > 1 && (LPCSTR)pRule->m_strValue )
	{
		::SetRect( &rc, x + 2, rcRow.top, x + m_cols[1].width, rcRow.bottom );
		pDC->DrawText( CODList_EllipsizeText( pDC, pRule->m_strValue, m_cols[1].width, 2 ),
			-1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );
	}

	pDC->SelectObject( pOldFont );
}

/////////////////////////////////////////////////////////////////////////////
// CODPingComboBox::CODPingComboBox (0x44D020)

CODPingComboBox::CODPingComboBox()
{
	HBRUSH	hbr;

	m_clrRow   = RGB( 255, 255, 255 );
	m_clrRowBk = RGB( 63, 63, 63 );

	hbr = ::CreateSolidBrush( RGB( 63, 63, 63 ) );
	if ( hbr )
		m_brRow.Attach( hbr );
}

/////////////////////////////////////////////////////////////////////////////
// CODPingComboBox::DrawRow (0x44D0D0)

void CODPingComboBox::DrawRow( CDC* pDC, int iRow )
{
	const char*	pszText;
	RECT		client;
	RECT		rc;
	CFont*		pOldFont;
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

	if ( iRow == m_pList->GetCurSel() )
		pDC->FillRect( &rc, &m_brHot );
	else
		pDC->FillRect( &rc, &m_brText );

	pDC->SetBkMode( TRANSPARENT );
	pDC->SetTextColor( m_clrRow );
	pOldFont = pDC->SelectObject( &m_textFont );

	rc.left += 2;
	pDC->DrawText( pszText, -1, &rc, DT_VCENTER | DT_NOPREFIX );
	rc.left -= 2;

	pDC->SelectObject( pOldFont );
}

BEGIN_MESSAGE_MAP( CODRuleListCtrl, CODListCtrl )
	//{{AFX_MSG_MAP(CODRuleListCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODRuleListCtrl::CODRuleListCtrl (0x44D220)

CODRuleListCtrl::CODRuleListCtrl()
{
}

/////////////////////////////////////////////////////////////////////////////
// CODRuleListCtrl::~CODRuleListCtrl (0x44D260)

CODRuleListCtrl::~CODRuleListCtrl()
{
}

BEGIN_MESSAGE_MAP( CODSaveGameListCtrl, CODListCtrl )
	//{{AFX_MSG_MAP(CODSaveGameListCtrl)
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODSaveGameListCtrl::OnLButtonDblClk (0x44D690)
//
// Tells the owning page a row was chosen; the page loads or overwrites it.

void CODSaveGameListCtrl::OnLButtonDblClk( UINT, CPoint )
{
	CWnd*	pParent = CWnd::FromHandle( ::GetParent( GetSafeHwnd() ) );

	::SendMessageA( pParent ? pParent->GetSafeHwnd() : NULL, WM_COMMAND,
		MAKEWPARAM( GetDlgCtrlID(), LBN_DBLCLK ), (LPARAM)GetSafeHwnd() );
}

/////////////////////////////////////////////////////////////////////////////
// CODSaveGameListCtrl::DrawRow (0x44D6E0)
//
// Three cells: the timestamp, the comment (replaced by the "quick save" /
// "autosave" string when the record is one of those) and the elapsed time.

void CODSaveGameListCtrl::DrawRow( CDC* pDC, int iRow )
{
	odrow_t*	pRow;
	savegame_t*	pRec;
	CFont*		pOldFont;
	const char*	pszComment;
	RECT		rcRow;
	RECT		rc;
	char		szBuf[260];
	BOOL		bSel;
	int			iVis, x;

	GetClientRect( &rcRow );
	if ( m_bHasScrollbar )
		rcRow.right -= 16;

	iVis = iRow - m_topRow;
	if ( iVis < 0 )
		return;

	pRow = m_rows[iRow];
	pRec = (savegame_t*)pRow->record;
	if ( !pRec )
		return;

	bSel = ( pRow->flags & 1 );

	rcRow.top    = iVis * m_rowHeight;
	rcRow.bottom = m_rowHeight + rcRow.top - 1;

	pDC->SetBkColor( m_clrRowBg );
	if ( !m_bTransparent )
		pDC->FillRect( &rcRow, bSel ? &m_brHighlight : &m_brBg );

	if ( bSel )
	{
		pDC->SetTextColor( m_clrSelText );
		if ( m_bTransparent )
		{
			CDC			memDC;
			CBitmap		bmp;
			CBitmap*	pOldBmp;
			CBrush		brSel( RGB( 80, 56, 24 ) );
			CRect		rcBuf( 0, 0, rcRow.right - rcRow.left, rcRow.bottom - rcRow.top );

			if ( memDC.CreateCompatibleDC( pDC ) )
			{
				bmp.CreateCompatibleBitmap( pDC, rcBuf.Width(), rcBuf.Height() );
				pOldBmp = memDC.SelectObject( &bmp );
				memDC.FillRect( &rcBuf, &brSel );
				pDC->BitBlt( rcRow.left, rcRow.top, rcBuf.Width(), rcBuf.Height(),
					&memDC, 0, 0, SRCPAINT );
				memDC.SelectObject( pOldBmp );
			}
		}
	}
	else
	{
		pDC->SetTextColor( m_clrRowText );
	}

	pDC->SetBkMode( TRANSPARENT );
	pDC->SetBkColor( m_clrRowBg );
	pOldFont = pDC->SelectObject( &m_headerFont );

	// the timestamp
	x = 0;
	::SetRect( &rc, x + 2, iVis * m_rowHeight, m_cols[0].width,
		m_rowHeight + iVis * m_rowHeight - 1 );
	pDC->DrawText( CODList_EllipsizeText( pDC, pRec->date, m_cols[0].width, 2 ),
		-1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );
	x = m_cols[0].width;

	// the comment, or what kind of save this is
	pszComment = pRec->comment;
	if ( pRec->bQuicksave )
	{
		Launcher_LoadStringInto( szBuf, IDS_SAVELOAD_QUICKLISTTEXT, pRec->comment );
		pszComment = szBuf;
	}
	else if ( pRec->bAutosave )
	{
		Launcher_LoadStringInto( szBuf, IDS_SAVELOAD_AUTOLISTITEM, pRec->comment );
		pszComment = szBuf;
	}

	::SetRect( &rc, x + 2, iVis * m_rowHeight, x + m_cols[1].width,
		m_rowHeight + iVis * m_rowHeight - 1 );
	pDC->DrawText( CODList_EllipsizeText( pDC, pszComment, m_cols[1].width, 2 ),
		-1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );
	x += m_cols[1].width;

	// the elapsed time
	::SetRect( &rc, x + 2, iVis * m_rowHeight, x + m_cols[2].width,
		m_rowHeight + iVis * m_rowHeight - 1 );
	pDC->DrawText( CODList_EllipsizeText( pDC, pRec->elapsed, m_cols[2].width, 2 ),
		-1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );

	pDC->SelectObject( pOldFont );
}

BEGIN_MESSAGE_MAP( CODPingComboBox, CODComboBox )
	//{{AFX_MSG_MAP(CODPingComboBox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
