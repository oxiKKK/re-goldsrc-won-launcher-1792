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
// Purpose: CPlayerInfo and CODPlayerListCtrl's row drawing.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// CPlayerInfo::Save (0x451C70)
void CPlayerInfo::Save( int iIndex, FILE* fp )
{
	if ( !fp )
		return;

	fprintf( fp, "%splayer %02i\r\n", "\t\t\t", iIndex );
	fprintf( fp, "%s{\r\n", "\t\t\t" );
	fprintf( fp, "%s\t\"%s\" \"%s\"\r\n", "\t\t\t", "name",   (LPCSTR)m_strName );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t\t\t", "id",     m_iId );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t\t\t", "colors", m_iColors );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t\t\t", "frags",  m_iFrags );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t\t\t", "time",   m_iTime );
	fprintf( fp, "%s}\r\n", "\t\t\t" );
}

// CPlayerInfo::SetKey (0x451D50)
void CPlayerInfo::SetKey( const char* pszKey, const char* pszValue )
{
	if ( !_strcmpi( pszKey, "name" ) )		m_strName = pszValue;
	else if ( !_strcmpi( pszKey, "id" ) )		m_iId     = atoi( pszValue );
	else if ( !_strcmpi( pszKey, "colors" ) )	m_iColors = atoi( pszValue );
	else if ( !_strcmpi( pszKey, "frags" ) )	m_iFrags  = atoi( pszValue );
	else if ( !_strcmpi( pszKey, "time" ) )	m_iTime   = atoi( pszValue );
}

// CPlayerInfo::Parse (0x451E20)
BOOL CPlayerInfo::Parse( char** ppBuf )
{
	CString	strKey;
	CString	strVal;
	CToken	tok( *ppBuf );

	tok.SetQuoteMode( TRUE );
	tok.SetCommentMode( TRUE );

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
		return FALSE;

	if ( strcmp( tok.token, "{" ) )
	{
		Launcher_ErrorMessageBox( 0, "Expecting '{', got '%s'", tok.token );
		return FALSE;
	}

	for ( ;; )
	{
		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			break;
		if ( !_strcmpi( tok.token, "}" ) )
		{
			*ppBuf = tok.GetData();
			return TRUE;
		}

		strKey = tok.token;
		tok.ParseNextToken();
		strVal = tok.token;
		SetKey( strKey, strVal );
	}
	return FALSE;
}

// CPlayerInfo::CPlayerInfo (0x452000)
CPlayerInfo::CPlayerInfo( const char* pszName )
{
	m_strName      = pszName;	// call sites pass "unknown"
	m_iId          = 0;
	m_iColors      = 0;
	m_iFrags       = 0;
	m_iTime        = 0;
	m_dConnTime    = 0.0;
}

// CPlayerInfo::~CPlayerInfo (0x452090)
CPlayerInfo::~CPlayerInfo()
{
	// m_strName releases through its own dtor.
}

/////////////////////////////////////////////////////////////////////////////
// CODPlayerListCtrl::DrawRow (0x4520A0)
//
// Four cells: id, name, frags and connected time.  The last three are drawn
// only when the row carries a name.

void CODPlayerListCtrl::DrawRow( CDC* pDC, int iRow )
{
	RECT	rc;

	GetClientRect( &rc );
	if ( m_bHasScrollbar )
		rc.right -= 16;

	int	vis = iRow - m_topRow;
	if ( vis < 0 )
		return;

	odrow_t*		pRec    = m_rows[iRow];
	CPlayerInfo*	pPlayer = (CPlayerInfo*)pRec->record;
	if ( !pPlayer )
		return;

	int	bSel = ( pRec->flags & 1 );

	rc.top    = vis * m_rowHeight;
	rc.bottom = m_rowHeight + vis * m_rowHeight - 1;

	pDC->SetBkColor( m_clrRowBg );
	if ( !m_bTransparent )
		pDC->FillRect( &rc, bSel ? &m_brHighlight : &m_brBg );

	if ( bSel )
	{
		pDC->SetTextColor( m_clrSelText );
		if ( m_bTransparent )
		{
			// Transparent lists let the page art through, so the selection bar is
			// OR-ed over whatever is already on screen.
			CDC	mem;

			if ( mem.CreateCompatibleDC( pDC ) )
			{
				CBitmap		bmp;
				CBitmap*	pOldBmp;
				CBrush		brSel( RGB( 80, 56, 24 ) );
				CRect		rcBuf( 0, 0, rc.right - rc.left, rc.bottom - rc.top );

				bmp.CreateCompatibleBitmap( pDC, rcBuf.Width(), rcBuf.Height() );
				pOldBmp = mem.SelectObject( &bmp );
				mem.FillRect( &rcBuf, &brSel );
				pDC->BitBlt( rc.left, rc.top, rcBuf.Width(), rcBuf.Height(),
					&mem, 0, 0, SRCPAINT );
				mem.SelectObject( pOldBmp );
			}
		}
	}
	else
	{
		pDC->SetTextColor( m_clrRowText );
	}

	pDC->SetBkMode( TRANSPARENT );

	CFont*	pOldFont = pDC->SelectObject( &m_headerFont );
	pDC->SetBkColor( m_clrRowBg );		// (sic) set a second time

	char	szBuf[64];
	RECT	rcCell;
	int		x = 0;

	sprintf( szBuf, "%i", pPlayer->m_iId );
	rcCell.left   = x + 2;
	rcCell.top    = rc.top;
	rcCell.right  = x + m_cols[0].width;
	rcCell.bottom = rc.bottom;
	pDC->DrawText( CODList_EllipsizeText( pDC, szBuf, m_cols[0].width, 2 ), -1, &rcCell,
		DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
	x += m_cols[0].width;

	if ( pPlayer->m_strName )
	{
		const char*	pszName = pPlayer->m_strName;

		rcCell.left  = x + 2;
		rcCell.right = x + m_cols[1].width;
		pDC->DrawText( CODList_EllipsizeText( pDC, pszName, m_cols[1].width, 2 ), -1, &rcCell,
			DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
		x += m_cols[1].width;

		sprintf( szBuf, "%i", pPlayer->m_iFrags );
		rcCell.left  = x + 2;
		rcCell.right = x + m_cols[2].width;
		pDC->DrawText( CODList_EllipsizeText( pDC, szBuf, m_cols[2].width, 2 ), -1, &rcCell,
			DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
		x += m_cols[2].width;

		int	t = pPlayer->m_iTime;
		int	h = t / 3600;	t %= 3600;
		int	m = t / 60;		int s = t % 60;

		if ( h )
			sprintf( szBuf, "%ih %02i:%02i", h, m, s );
		else
			sprintf( szBuf, "%02i:%02i", m, s );

		rcCell.left  = x + 2;
		rcCell.right = x + m_cols[3].width;
		pDC->DrawText( CODList_EllipsizeText( pDC, szBuf, m_cols[3].width, 2 ), -1, &rcCell,
			DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
	}

	pDC->SelectObject( pOldFont );
}
