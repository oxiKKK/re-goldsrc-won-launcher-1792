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
// Purpose: the chat room list (Rooms_Load, CODRoomListCtrl).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// One room name slot in the array Rooms_Load hands back.
#define ROOMS_NAME_STRIDE	64

/*
==================
Rooms_Load (0x45C150)

Returns the loaded file pointer -- already freed once any room was parsed
(sic) -- or -1 when rooms.lst is absent.
==================
*/
char* Rooms_Load( char** ppArray, int* pnCount )
{
	FILE*	fp;
	CToken	tok( NULL );
	char*	pFile;
	char*	pNames;
	char*	p;
	int		nRooms = 0;
	int		cb;

	*ppArray = NULL;
	*pnCount = 0;

	if ( COM_FindFile( "rooms.lst", NULL, &fp ) == -1 )
		return (char*)-1;
	fclose( fp );

	pFile = (char*)COM_LoadMallocFile( "rooms.lst" );
	if ( !pFile )
		return NULL;

	tok.SetData( pFile );
	tok.SetCommentMode( 1 );

	// pass 1 -- count non-blank lines
	while ( TRUE )
	{
		tok.GetRemainder();
		if ( !strlen( tok.token ) )
			break;
		p = tok.GetData();				// skip leading CR/LF
		while ( *p == '\r' || *p == '\n' )
			p++;
		nRooms++;
		if ( !*p )
			break;
		tok.SetData( p );
	}

	if ( nRooms )
	{
		tok.SetData( pFile );			// rewind
		*pnCount = nRooms;
		cb = nRooms * ROOMS_NAME_STRIDE;
		pNames = new char[cb];
		*ppArray = pNames;
		memset( pNames, 0, cb );

		// pass 2 -- copy each room name into its slot
		while ( TRUE )
		{
			tok.GetRemainder();
			if ( !strlen( tok.token ) )
				break;
			p = tok.GetData();
			while ( *p == '\r' || *p == '\n' )
				p++;
			strcpy( pNames, tok.token );
			pNames += ROOMS_NAME_STRIDE;
			if ( !*p )
				break;
			tok.SetData( p );
		}
		free( pFile );
	}

	return pFile;
}

/////////////////////////////////////////////////////////////////////////////
// CODRoomListCtrl::AddRoomRow (0x45C320)
//
// Sorted insert: rooms absent from rooms.lst carry group -1, which the
// unsigned compare sinks below every listed room.

void CODRoomListCtrl::AddRoomRow( chatroom_t* pRoom )
{
	odrow_t*	pNew;
	chatroom_t*	pCur;
	int			pos;
	int			i;

	if ( !m_rows || !pRoom )
		return;

	pNew = AllocRow();				// from the free ring, not new
	if ( !pNew )
		return;
	pNew->record  = (char*)pRoom;
	pNew->flags = 0;

	for ( pos = 0; pos < m_nRows; pos++ )
	{
		pCur = (chatroom_t*)m_rows[pos]->record;
		if ( !pCur )
			break;
		if ( (unsigned)pCur->m_nGroup > (unsigned)pRoom->m_nGroup
			|| ( pCur->m_nGroup == pRoom->m_nGroup
				&& _stricmp( pCur->m_szName, pRoom->m_szName ) > 0 ) )
			break;
	}
	for ( i = m_nRows; i > pos; i-- )
		m_rows[i] = m_rows[i - 1];
	m_rows[pos] = pNew;
	m_nRows++;

	SelectItem( pos, 1 );
	if ( m_pScrollbar )
	{
		m_pScrollbar->SetRange( 0, m_nRows );
		m_pScrollbar->SetPos( m_curSel );
	}
	UpdateScrollbar( 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CODRoomListCtrl::DrawRow (0x45C420)
//
// Two lines per row: the name and player count across the top half, the
// topic across the bottom.  Rows lay out from the client top with no header
// term; adding one pushes the tail of the list past the bottom edge.

void CODRoomListCtrl::DrawRow( CDC* pDC, int iRow )
{
	odrow_t*	pRow;
	chatroom_t*	pRoom;
	CFont*		pOldFont;
	COLORREF	clrName;
	RECT		rcRow;
	RECT		rc;
	RECT		rcInfo;
	char		szNum[16];
	BOOL		bSel;
	int			iVis, halfH, x;

	pRow = m_rows ? m_rows[iRow] : NULL;
	if ( !pRow )
		return;
	pRoom = (chatroom_t*)pRow->record;
	if ( !pRoom )
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
	halfH        = m_rowHeight / 2;

	pDC->SetBkColor( m_clrRowBg );
	if ( !m_bTransparent )
		pDC->FillRect( &rcRow, bSel ? &m_brHighlight : &m_brBg );

	pDC->SetBkMode( TRANSPARENT );
	pOldFont = pDC->SelectObject( &m_headerFont );

	// ungrouped rooms get the accent regardless of selection
	clrName = ( pRoom->m_nGroup == -1 ) ? RGB( 255, 64, 0 )
			: ( bSel ? m_clrSelText : m_clrRowText );
	pDC->SetTextColor( clrName );

	// col 0: room name (top half)
	x = rcRow.left;
	if ( m_nCols > 0 )
	{
		::SetRect( &rc, x + 2, rcRow.top, x + m_cols[0].width, rcRow.top + halfH - 1 );
		pDC->DrawText( CODList_EllipsizeText( pDC, pRoom->m_szName, m_cols[0].width, 2 ),
			-1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );
		x += m_cols[0].width;
	}

	// col 1: player count (top half, no left inset)
	if ( m_nCols > 1 )
	{
		sprintf( szNum, "%i", pRoom->m_nPlayers );
		::SetRect( &rc, x, rcRow.top, x + m_cols[1].width, rcRow.top + halfH - 1 );
		pDC->DrawText( CODList_EllipsizeText( pDC, szNum, m_cols[1].width, 2 ),
			-1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );
	}

	// the topic, full width on the bottom half
	::SetRect( &rcInfo, rcRow.left + 2, rcRow.top + halfH, rcRow.right,
		m_rowHeight * ( iVis + 1 ) - 1 );
	if ( !bSel )								// a selected row keeps the name colour
		pDC->SetTextColor( RGB( 128, 128, 128 ) );
	pDC->DrawText( CODList_EllipsizeText( pDC, pRoom->m_szTopic, rcInfo.right - rcInfo.left, 2 ),
		-1, &rcInfo, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX );

	pDC->SelectObject( pOldFont );
}
