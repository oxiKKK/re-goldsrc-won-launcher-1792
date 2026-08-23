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
// Purpose: CODIRCUserListCtrl, the chat user list.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

BEGIN_MESSAGE_MAP( CODIRCUserListCtrl, CODListCtrl )
	//{{AFX_MSG_MAP(CODIRCUserListCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CODIRCUserListCtrl::CODIRCUserListCtrl (0x448F20)

CODIRCUserListCtrl::CODIRCUserListCtrl( CWnd* pOwner )
{
	m_pOwnerDlg = pOwner;
}

/////////////////////////////////////////////////////////////////////////////
// CODIRCUserListCtrl::~CODIRCUserListCtrl (0x448F60)

CODIRCUserListCtrl::~CODIRCUserListCtrl()
{
}

/////////////////////////////////////////////////////////////////////////////
// CODIRCUserListCtrl::AddRow (0x448F70)
//
// Sorted insert by nick; the row pool is the only bound on the roster.

void CODIRCUserListCtrl::AddRow( CChatUser* pUser )
{
	odrow_t*	row;
	CChatUser*	pExisting;
	int			iAt = -1;
	int			i;

	if ( !m_rows )
		return;

	row = AllocRow();				// from the free ring, not new
	if ( !row )
		return;
	row->record  = (char*)pUser;
	row->flags = 0;

	// Find the insertion slot: first existing nick that sorts >= the newcomer.
	for ( i = 0; i < m_nRows; i++ )
	{
		pExisting = (CChatUser*)m_rows[i]->record;
		if ( _strcmpi( pExisting->m_szNick, pUser->m_szNick ) > 0 )
		{
			iAt = i;
			break;
		}
	}

	if ( iAt < 0 )
	{
		m_rows[m_nRows] = row;			// append
		iAt = m_nRows;
	}
	else
	{
		for ( i = m_nRows; i > iAt; i-- )	// shift tail down one
			m_rows[i] = m_rows[i - 1];
		m_rows[iAt] = row;
	}
	m_nRows++;

	SelectItem( iAt, 1 );
	if ( m_pScrollbar )
	{
		m_pScrollbar->SetRange( 0, m_nRows );
		m_pScrollbar->SetPos( m_curSel );
	}
	UpdateScrollbar( 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CODIRCUserListCtrl::DrawRow (0x449070)
//
// The page toggles this control's header off, so rows lay out from the client
// top with no header term.

void CODIRCUserListCtrl::DrawRow( CDC* pDC, int iRow )
{
	odrow_t*	pRow;
	CChatUser*	pUser;
	CFont*		pOldFont;
	RECT		rcRow;
	RECT		rc;
	char		szText[260];
	BOOL		bSel;
	int			iVis;

	pRow = m_rows ? m_rows[iRow] : NULL;
	if ( !pRow )
		return;
	pUser = (CChatUser*)pRow->record;
	if ( !pUser )
		return;

	bSel = ( pRow->flags & 1 );

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

	pDC->SetBkMode( TRANSPARENT );
	pDC->SetTextColor( bSel ? m_clrSelText : m_clrRowText );
	pOldFont = pDC->SelectObject( &m_headerFont );

	sprintf( szText, " %s", NET_CleanServerName( pUser->m_szNick ) );
	::SetRect( &rc, rcRow.left + 2, rcRow.top, rcRow.right, rcRow.bottom );
	pDC->DrawText( szText, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );

	pDC->SelectObject( pOldFont );
}
