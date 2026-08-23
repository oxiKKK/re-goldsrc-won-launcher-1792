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
// Purpose: the Controls / key-binding dialog (CKeyboardDlg, IDD 0xA3 = 163)
//          and CODKeySearchComboBox.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// g_nLauncherDefW/H come from vid.h; COM_LoadMallocFile from common/common.h;
// Joy_GetPressedButton from joystick.h (included above).

// The "Player" config-engine entry points OnUseDefaults drives are declared in
// Profile.h (int return, char* block) -- pulled in via NetGame.h above.

// The key captured by PreTranslateMessage on its way into the pick cell (the
// binary keeps it in a global the kb_keys parser also uses as scratch).
static char		s_szPickedKey[256];

// Kb_TranslateKey's own static return buffer (binary byte_4E2BC8); the engine
// key name is rebuilt here on every call.
static char		s_szKeyName[256];

// VK -> kb_keys.lst key-name translation used while capturing (GetKeyNameText
// + the special cases; 0x41DD80; defined below at its address position).
const char*	Kb_TranslateKey( int bExtended, UINT nVirtKey, int* pKeyNum );

// File-local helpers, forward-declared so the global ascending-address ordering
// of the definitions below stays define-before-use regardless of caller order.
static void Kb_StyleButton( CODBlendBtn* btn, const char* pszCaption );			// (inlined button-style triple)
static int  Kb_FindKeyIndex( const char* pszKey, const kbkey_t* keys );
static void Kb_WriteBinding( kbbinding_t* bindings, int idx, const char* pszCommand, const kbkey_t* keys );

// Entries at 0x4AEBC8, base map 0x4B4398 = CDialog.
BEGIN_MESSAGE_MAP( CKeyboardDlg, CDialog )
	ON_MESSAGE( WM_DISPLAYCHANGE, &CKeyboardDlg::OnDisplayChange )
	ON_WM_PAINT()		// (shared skin thunk)
	ON_WM_ERASEBKGND()		// (shared skin thunk)
	ON_WM_CTLCOLOR()
	ON_WM_KEYDOWN()		// (folded stub)
	ON_COMMAND( IDC_KEYBOARD_CANCEL, OnCancelPage )		// 25
	ON_COMMAND( IDC_KEYBOARD_USEDEFAULTS, OnUseDefaults )		// 21
	ON_COMMAND( IDC_KEYBOARD_ADVANCED, OnAdvancedOptions )		// 34
	ON_WM_TIMER()
	ON_CONTROL( CBN_SELCHANGE, IDC_KEYBOARD_KEYSEARCH, OnSearchSelChange )
	ON_CONTROL( LBN_SELCHANGE, IDC_KEYBOARD_BINDLIST, OnBindListSelChange )
	ON_WM_ACTIVATEAPP()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP( CODKeySearchComboBox, CODComboBox )
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

// Entries at 0x4AEB48, base map 0x4B1B40 = CODListCtrl.
BEGIN_MESSAGE_MAP( CODKeyBindingCtrl, CODListCtrl )
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	ON_WM_PAINT()
END_MESSAGE_MAP()

// The two paint entries in the message map are thin thunks (0x412860 /
/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnPaint (0x412870)
//
//
// ) that forward to the shared skinned-dialog paint.

void CKeyboardDlg::OnPaint()
{
	PaintSkinnedDialog();
}

BOOL CKeyboardDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CODKeySearchComboBox::CODKeySearchComboBox (0x41BE30)

CODKeySearchComboBox::CODKeySearchComboBox( CODKeyBindingCtrl* pList, CKeyboardDlg* pOwner )
{
	m_pList2   = pList;			// +196
	m_pOwner   = pOwner;		// +200
	m_curSel2  = -1;			// +204
	m_editable = 'a';			// +208
}

/////////////////////////////////////////////////////////////////////////////
// CODKeySearchComboBox::~CODKeySearchComboBox (0x453940)

CODKeySearchComboBox::~CODKeySearchComboBox()
{
}

/////////////////////////////////////////////////////////////////////////////
// CODKeySearchComboBox::OnKeyDown (0x41BE80)

void CODKeySearchComboBox::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	switch ( nChar )
	{
	case VK_ESCAPE:
		m_pList2->m_bPickPending = 0;
		ShowWindow( SW_HIDE );
		m_curSel2 = -1;
		m_pList2->SetFocus();
		return;

	case VK_RETURN:
		m_pList2->m_bPickPending = 1;
		m_pOwner->OnSearchSelChange();
		ShowWindow( SW_HIDE );
		m_curSel2 = -1;
		m_pList2->SetFocus();
		return;

	case VK_DOWN:
		if ( !m_bDropped )
		{
			DropDown();
			return;
		}
		break;					// drop already open -> default scroll

	case VK_BACK:
	case VK_DELETE:
		if ( ::IsWindowVisible( m_hWnd ) )
		{
			m_pList2->m_bPickPending = 0;
			ShowWindow( SW_HIDE );
			m_curSel2 = -1;
			m_pList2->SetFocus();

			int		sel = m_pList2->GetCurSel();
			int		col = m_pList2->m_iCurCol;		// +1932
			const char*	key = GetString( sel );
			if ( key && key[0] )
			{
				CODKeyBindingRow*	row = (CODKeyBindingRow*)m_pList2->GetItemData( sel );
				if ( row )
				{
					if ( col == 1 )
						row->ClearPrimary();
					else
						row->ClearAlternate();
					m_pOwner->RememberClearedKey( key );
				}
			}
		}
		return;

	default:
		// Type-ahead: letters and digits jump to the next matching item.
		if ( ( nChar >= 'A' && nChar <= 'Z' ) || ( nChar >= 'a' && nChar <= 'z' )
			|| nChar == '0' || ( nChar >= '1' && nChar <= '9' ) )
		{
			char	want = (char)nChar;
			if ( nChar >= 'a' && nChar <= 'z' )
				want = (char)toupper( (char)nChar );

			int		start = ( m_curSel2 == -1 ) ? 0 : m_curSel2 + 1;
			if ( want != m_editable )
			{
				m_curSel2  = -1;
				start      = 0;
				m_editable = want;
			}

			int		i = start;
			for ( ; i < GetCount(); i++ )
			{
				const char*	item = GetString( i );
				if ( !item || !item[0] )
					continue;
				char	c = item[0];
				if ( c >= 'a' && c <= 'z' )
				{
					if ( (char)toupper( c ) == want )
						break;
				}
				else if ( c == want )
				{
					break;
				}
			}
			if ( i < GetCount() )
			{
				m_curSel2 = i;
				SetCurSel( i );
			}
			if ( i == GetCount() )
				m_curSel2 = -1;
		}
		break;
	}

	CODComboBox::OnKeyDown( nChar, nRepCnt, nFlags );
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingRow (0x41C120)
//
//
// all four string slots and the chain link start empty.

CODKeyBindingRow::CODKeyBindingRow()
{
	m_pszCommand    = NULL;		// +4
	m_pszText       = NULL;		// +0
	m_pszPrimaryKey = NULL;		// +8
	m_pszAltKey     = NULL;		// +12
	m_clrPrimary    = RGB( 255, 255, 255 );	// +16
	m_clrAlt        = RGB( 255, 255, 255 );	// +20
	m_pNext         = NULL;		// +24
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingRow::~CODKeyBindingRow (0x41C140)
//
//
// The row must already be unlinked; the dialog drains m_pRows in its own
// destructor first.

CODKeyBindingRow::~CODKeyBindingRow()
{
	if ( m_pNext )
		Launcher_ShowMessageById( 0, IDS_CONTROLS_DELETE_LINKED );		// still on a chain
	if ( m_pszCommand )		free( m_pszCommand );
	if ( m_pszText )		free( m_pszText );
	if ( m_pszPrimaryKey )	free( m_pszPrimaryKey );
	if ( m_pszAltKey )		free( m_pszAltKey );
}

COLORREF	CODKeyBindingRow::GetTextColor()	{ return m_clrPrimary; }
const char*	CODKeyBindingRow::GetCommand()		{ return m_pszCommand    ? m_pszCommand    : ""; }
const char*	CODKeyBindingRow::GetText()			{ return m_pszText       ? m_pszText       : ""; }
const char*	CODKeyBindingRow::GetPrimaryKey()	{ return m_pszPrimaryKey ? m_pszPrimaryKey : ""; }
const char*	CODKeyBindingRow::GetAltKey()		{ return m_pszAltKey     ? m_pszAltKey     : ""; }

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingRow::ClearAlternate (0x41C1F0)
//
//
// drop the alternate key.

void CODKeyBindingRow::ClearAlternate()
{
	if ( m_pszAltKey )
		free( m_pszAltKey );
	m_pszAltKey = NULL;
	m_clrAlt    = RGB( 255, 255, 255 );
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingRow::ClearPrimary (0x41C220)
//
//
// drop the primary; if an alternate exists, promote it to primary.

void CODKeyBindingRow::ClearPrimary()
{
	if ( m_pszPrimaryKey )
		free( m_pszPrimaryKey );
	m_pszPrimaryKey = NULL;
	m_clrPrimary    = RGB( 255, 255, 255 );

	if ( m_pszAltKey )
	{
		m_pszPrimaryKey = m_pszAltKey;				// promote
		m_clrPrimary    = m_clrAlt;
		m_pszAltKey     = NULL;
		m_clrAlt        = RGB( 255, 255, 255 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingRow::SetKey (0x41C260)
//
//
// set one slot.  Primary is always overwritten; alternate is taken
// only when it differs (case-insensitively) from the primary.  Stored upper-cased.

void CODKeyBindingRow::SetKey( const char* pszKey, COLORREF clr, int bAlternate )
{
	if ( bAlternate )
	{
		if ( m_pszAltKey )
			free( m_pszAltKey );
		m_pszAltKey = NULL;
		if ( !m_pszPrimaryKey || !pszKey || _strcmpi( pszKey, m_pszPrimaryKey ) != 0 )
		{
			m_pszAltKey = (char*)malloc( strlen( pszKey ) + 1 );
			strcpy( m_pszAltKey, pszKey );
			_strupr( m_pszAltKey );
			m_clrAlt = clr;
		}
	}
	else
	{
		if ( m_pszPrimaryKey )
			free( m_pszPrimaryKey );
		m_pszPrimaryKey = (char*)malloc( strlen( pszKey ) + 1 );
		strcpy( m_pszPrimaryKey, pszKey );
		_strupr( m_pszPrimaryKey );
		m_clrPrimary = clr;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingRow::AssignKey (0x41C350)
//
//
// fill the first free slot: if a primary is set, copy it down into
// the alternate first, then the new key becomes the primary.

void CODKeyBindingRow::AssignKey( const char* pszKey, COLORREF clr )
{
	if ( m_pszAltKey )
	{
		free( m_pszAltKey );
		m_pszAltKey = NULL;
	}
	if ( m_pszPrimaryKey && m_pszPrimaryKey[0] )
	{
		m_pszAltKey = (char*)malloc( strlen( m_pszPrimaryKey ) + 1 );
		strcpy( m_pszAltKey, m_pszPrimaryKey );
		_strupr( m_pszAltKey );
		m_clrAlt = m_clrPrimary;
	}
	if ( m_pszPrimaryKey )
		free( m_pszPrimaryKey );
	m_pszPrimaryKey = (char*)malloc( strlen( pszKey ) + 1 );
	strcpy( m_pszPrimaryKey, pszKey );
	_strupr( m_pszPrimaryKey );
	m_clrPrimary = clr;
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingRow::Init (0x41C420)
//
//
// (re)initialise the record: copy the label, command and primary
// key, drop any alternate, reset both colours to white.

void CODKeyBindingRow::Init( const char* pszText, const char* pszCommand,
							 const char* pszKey, COLORREF clr )
{
	if ( m_pszText )		free( m_pszText );
	m_pszText = (char*)malloc( strlen( pszText ) + 1 );
	strcpy( m_pszText, pszText );

	if ( m_pszCommand )		free( m_pszCommand );
	m_pszCommand = (char*)malloc( strlen( pszCommand ) + 1 );
	strcpy( m_pszCommand, pszCommand );

	if ( m_pszPrimaryKey )	free( m_pszPrimaryKey );
	m_pszPrimaryKey = (char*)malloc( strlen( pszKey ) + 1 );
	strcpy( m_pszPrimaryKey, pszKey );

	m_clrPrimary = clr;								// +16
	if ( m_pszAltKey )
	{
		free( m_pszAltKey );
		m_pszAltKey = NULL;							// +12
	}
	m_clrAlt = RGB( 255, 255, 255 );				// +20
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::SelectPrevRow (0x41C550)
//
// UP

void CODKeyBindingCtrl::SelectPrevRow()
{
	GetVisibleRows();		// (parity; result unused)

	int		sel = GetCurSel() - 1;
	if ( sel < 0 )
		sel = 0;
	if ( sel < m_topRow )
		m_pScrollbar->LineUp();

	SelectItem( sel, 1 );							// vtbl+200

	if ( sel < m_topRow )
	{
		int		prev;
		do
		{
			prev = m_pScrollbar->GetPos();
			m_pScrollbar->LineUp();
		}
		while ( m_pScrollbar->GetPos() != prev && sel < m_topRow );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::SelectNextRow (0x41C5D0)
//
// DOWN

void CODKeyBindingCtrl::SelectNextRow()
{
	GetVisibleRows();

	int		sel = GetCurSel() + 1;
	if ( sel >= GetRowCount() )
		sel = GetRowCount() - 1;
	if ( sel > GetVisibleRows() + m_topRow - 1 )
		m_pScrollbar->LineDown();

	SelectItem( sel, 1 );							// vtbl+200

	for ( int top = GetVisibleRows(); sel > top + m_topRow - 1; top = GetVisibleRows() )
	{
		int		prev = m_pScrollbar->GetPos();
		m_pScrollbar->LineDown();
		if ( m_pScrollbar->GetPos() == prev )
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::SelectPrevPage (0x41C680)
//
// PAGE UP

void CODKeyBindingCtrl::SelectPrevPage()
{
	int		page = GetVisibleRows();

	int		sel = GetCurSel() - page;
	if ( sel < 0 )
		sel = 0;
	if ( sel < m_topRow )
		m_pScrollbar->LineUp();

	SelectItem( sel, 1 );							// vtbl+200

	if ( sel < m_topRow )
	{
		int		prev;
		do
		{
			prev = m_pScrollbar->GetPos();
			m_pScrollbar->LineUp();
		}
		while ( m_pScrollbar->GetPos() != prev && sel < m_topRow );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::SelectNextPage (0x41C700)
//
// PAGE DOWN

void CODKeyBindingCtrl::SelectNextPage()
{
	int		page = GetVisibleRows();

	int		sel = page + GetCurSel();
	if ( sel >= GetRowCount() )
		sel = GetRowCount() - 1;
	if ( sel > GetVisibleRows() + m_topRow - 1 )
		m_pScrollbar->LineDown();

	SelectItem( sel, 1 );							// vtbl+200

	for ( int top = GetVisibleRows(); sel > top + m_topRow - 1; top = GetVisibleRows() )
	{
		int		prev = m_pScrollbar->GetPos();
		m_pScrollbar->LineDown();
		if ( m_pScrollbar->GetPos() == prev )
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl (0x41C7B0)

CODKeyBindingCtrl::CODKeyBindingCtrl( CKeyboardDlg* pOwner )
{
	m_pOwnerDlg    = pOwner;			// +1908
	m_clrCell      = RGB( 156, 96, 0 );			// +1912
	m_clrCellHot   = RGB( 232, 184, 0 );			// +1916
	m_pSearchCombo = NULL;				// +1920
	m_iPickRow     = -1;				// +1924
	m_iPickCol     = -1;				// +1928
	m_iCurCol      = 0;					// +1932
	m_bPickPending = 0;					// +1936

	HFONT	hCell = ::CreateFontA( -12, 0, 0, 0, FW_HEAVY, 0, 0, 0, 0,
		OUT_TT_PRECIS, 0, PROOF_QUALITY, FF_SWISS, "Arial" );
	if ( hCell )
		m_cellFont.Attach( hCell );		// +1940
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::~CODKeyBindingCtrl (0x41C890)

CODKeyBindingCtrl::~CODKeyBindingCtrl()
{
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::OnLButtonDown (0x41C8F0)

void CODKeyBindingCtrl::OnLButtonDown( UINT nFlags, CPoint pt )
{
	RECT	rc;
	int		col;

	CODListCtrl::OnLButtonDown( nFlags, pt );

	int		sel = GetCurSel();
	if ( !HitTestCell( sel, pt.x, &rc, &col ) )
		return;

	m_iCurCol = col;								// +1932
	::InvalidateRect( m_hWnd, NULL, TRUE );
	if ( col >= 1 && PtInRect( &rc, pt ) )
		BeginCellEdit( sel, col );
	else
		::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::OnLButtonDblClk (0x41C990)

void CODKeyBindingCtrl::OnLButtonDblClk( UINT nFlags, CPoint pt )
{
	RECT	rc;
	int		col;

	CODListCtrl::OnLButtonDown( nFlags, pt );

	int		sel = GetCurSel();
	if ( !HitTestCell( sel, pt.x, &rc, &col ) )
		return;

	m_iCurCol = col;								// +1932
	::InvalidateRect( m_hWnd, NULL, TRUE );

	if ( col < 1 || !PtInRect( &rc, pt ) )
	{
		::UpdateWindow( m_hWnd );
		return;
	}

	if ( !BeginCellEdit( sel, col ) )
		return;

	// Land the cursor on a key column before capturing.
	if ( !m_iCurCol )
	{
		CycleColRight();
		if ( !m_iCurCol )
			return;
	}

	m_pOwnerDlg->SetCapturing( 1 );
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::AttachSearchCombo (0x41CA70)
//
//
// remember the search combo the cell-edit drops over a key cell.

void CODKeyBindingCtrl::AttachSearchCombo( CODKeySearchComboBox* pCombo )
{
	m_pSearchCombo = pCombo;
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::CycleColLeft (0x41CA80)
//
//
// move the cell-column cursor one column left, wrapping from the
// first key column back to the last (the column count comes from the base list).

void CODKeyBindingCtrl::CycleColLeft()
{
	if ( --m_iCurCol < 1 )
		m_iCurCol = GetColumnCount() - 1;
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::CycleColRight (0x41CAC0)
//
//
// move the cell-column cursor one column right, wrapping past the
// last column back to the first key column (1).

void CODKeyBindingCtrl::CycleColRight()
{
	if ( ++m_iCurCol >= GetColumnCount() )
		m_iCurCol = 1;
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::BeginCellEdit (0x41CB00)
//
//
// latch a key cell for editing.

int CODKeyBindingCtrl::BeginCellEdit( int iRow, int iCol )
{
	CODKeyBindingRow*	row = (CODKeyBindingRow*)GetItemData( iRow );
	if ( !row )
		return 0;

	if ( !_strcmpi( row->GetCommand(), "blank" ) )
	{
		Snd_PlayMenuSound( UISND_DENY1 );
		return 0;
	}

	m_iPickRow = iRow;		// +1924
	m_iPickCol = iCol;		// +1928
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::OnKeyDown (0x41CB60)

void CODKeyBindingCtrl::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	switch ( nChar )
	{
	case VK_BACK:		// 8
	case VK_DELETE:		// 46
	{
		Snd_PlayMenuSound( UISND_DENY1 );
		m_bPickPending = 0;							// +1936
		SetFocus();

		int		sel = GetCurSel();
		int		col = m_iCurCol;					// +1932
		if ( col < 1 )
			break;

		CODKeyBindingRow*	row = (CODKeyBindingRow*)GetItemData( sel );
		if ( !row )
			break;

		const char*	key = ( col == 1 ) ? row->GetPrimaryKey() : row->GetAltKey();
		if ( !key || !key[0] )
			break;

		m_pOwnerDlg->SetDirty();
		m_pOwnerDlg->RememberClearedKey( key );
		if ( col == 1 )
			row->ClearPrimary();
		else
			row->ClearAlternate();

		::InvalidateRect( m_hWnd, NULL, TRUE );
		::UpdateWindow( m_hWnd );
		break;
	}

	case VK_RETURN:		// 13
		if ( BeginCellEdit( GetCurSel(), m_iCurCol ) )
		{
			if ( !m_iCurCol )
			{
				CycleColRight();
				if ( !m_iCurCol )
					break;
			}
			m_pOwnerDlg->SetCapturing( 1 );
			::InvalidateRect( m_hWnd, NULL, TRUE );
			::UpdateWindow( m_hWnd );
		}
		break;

	case VK_PRIOR:		// 33
		SelectPrevPage();
		break;

	case VK_NEXT:		// 34
		SelectNextPage();
		break;

	case VK_LEFT:		// 37
		CycleColLeft();
		break;

	case VK_UP:			// 38
		SelectPrevRow();
		break;

	case VK_RIGHT:		// 39
		CycleColRight();
		break;

	case VK_DOWN:		// 40
		SelectNextRow();
		break;

	default:
		CODListCtrl::OnKeyDown( nChar, nRepCnt, nFlags );
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::DrawRow (0x41CD60)

void CODKeyBindingCtrl::DrawRow( CDC* pDC, int iRow )
{
	RECT	client;
	GetClientRect( &client );
	if ( m_bHasScrollbar )
		client.right -= 16;

	int		vis = iRow - m_topRow;
	if ( vis < 0 )
		return;

	odrow_t*	pRow = m_rows[iRow];
	CODKeyBindingRow*	row = (CODKeyBindingRow*)pRow->record;
	if ( !row )
		return;

	int		bSelected = pRow->flags & 1;

	RECT	rc;
	rc.left   = client.left;
	rc.right  = client.right;
	rc.top    = vis * m_rowHeight;
	rc.bottom = m_rowHeight + vis * m_rowHeight - 1;

	pDC->SetBkColor( m_clrRowBg );			// +1772

	// Row background.
	if ( !m_bTransparent )
	{
		CBrush	bg( bSelected ? m_clrHighlight : m_clrRowBg );	// +1780 / +1772
		pDC->FillRect( &rc, &bg );
	}
	COLORREF	clrText = bSelected ? m_clrSelText : m_clrRowText;	// +1776 / +1764

	pDC->SetBkMode( TRANSPARENT );
	CFont*	pOldFont = pDC->SelectObject( &m_headerFont );	// +1824 (row font)

	CKeyboardDlg*	pOwner = m_pOwnerDlg;
	int		capturing = pOwner->IsCapturing();

	// - column 0: the action label ---
	const char*	label = row->GetText();
	if ( !label )
	{
		pDC->SelectObject( pOldFont );
		return;
	}

	RECT	cell;
	cell.left   = 0;
	cell.top    = vis * m_rowHeight;
	cell.bottom = m_rowHeight + vis * m_rowHeight - 1;
	cell.right  = m_cols[0].width;							// +92

	if ( bSelected && m_iCurCol == 0 )
	{
		if ( capturing )
		{
			CBrush	hot( RGB( 56, 56, 56 ) );
			pDC->FillRect( &cell, &hot );
			pDC->SetTextColor( RGB( 255, 180, 56 ) );
		}
		else
		{
			CBrush	hot( m_clrCell );						// +1912
			pDC->FillRect( &cell, &hot );
			pDC->SetTextColor( m_clrCellHot );				// +1916
		}
	}
	else
	{
		pDC->SetTextColor( clrText );
	}

	char	buf[300];
	const char*	fit = CODList_EllipsizeText( pDC, label, m_cols[0].width, 2 );
	cell.left += 4;
	wsprintfA( buf, " %s", fit );
	pDC->DrawText( buf, -1, &cell, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
	cell.left = cell.right - 5;
	{
		CBrush	bg( m_clrRowBg );
		pDC->FillRect( &cell, &bg );
	}

	// - column 1: the primary key ---
	const char*	keyPrimary = pOwner->GetKeyDisplayName( row->GetPrimaryKey() );
	if ( !keyPrimary )
	{
		pDC->SelectObject( pOldFont );
		return;
	}

	int		col1Right = m_cols[1].width + m_cols[0].width;	// +128 + +92
	cell.left   = m_cols[0].width;
	cell.top    = vis * m_rowHeight;
	cell.bottom = m_rowHeight + vis * m_rowHeight - 1;
	cell.right  = col1Right;

	if ( bSelected && m_iCurCol == 1 )
	{
		if ( capturing )
		{
			CBrush	hot( RGB( 56, 56, 56 ) );
			pDC->FillRect( &cell, &hot );
			pDC->SetTextColor( RGB( 255, 180, 56 ) );
		}
		else
		{
			CBrush	hot( m_clrCell );
			pDC->FillRect( &cell, &hot );
			pDC->SetTextColor( m_clrCellHot );
		}
	}
	else
	{
		pDC->SetTextColor( clrText );
	}

	const char*	fit1 = CODList_EllipsizeText( pDC, keyPrimary, m_cols[1].width, 2 );
	cell.left += 4;
	pDC->SetTextColor( row->GetTextColor() );		// (primary key colour)
	wsprintfA( buf, " %s", fit1 );
	pDC->DrawText( buf, -1, &cell, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
	cell.left = cell.right - 5;
	{
		CBrush	bg( m_clrRowBg );
		pDC->FillRect( &cell, &bg );
	}

	// - column 2: the alternate key ---
	const char*	keyAlt = pOwner->GetKeyDisplayName( row->GetAltKey() );		// /
	if ( keyAlt )
	{
		cell.top    = vis * m_rowHeight;
		cell.bottom = m_rowHeight + vis * m_rowHeight - 1;
		cell.left   = col1Right;
		cell.right  = m_cols[2].width + col1Right;			// +164

		if ( bSelected && m_iCurCol == 2 )
		{
			if ( capturing )
			{
				CBrush	hot( RGB( 56, 56, 56 ) );
				pDC->FillRect( &cell, &hot );
				pDC->SetTextColor( RGB( 255, 180, 56 ) );
			}
			else
			{
				CBrush	hot( m_clrCell );
				pDC->FillRect( &cell, &hot );
				pDC->SetTextColor( m_clrCellHot );
			}
		}
		else
		{
			pDC->SetTextColor( clrText );
		}

		const char*	fit2 = CODList_EllipsizeText( pDC, keyAlt, m_cols[2].width, 2 );
		cell.left += 4;
		pDC->SetTextColor( row->GetAltColor() );		// (alternate key colour)
		wsprintfA( buf, " %s", fit2 );
		pDC->DrawText( buf, -1, &cell, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
		cell.left = cell.right - 5;
		CBrush	bg( m_clrRowBg );
		pDC->FillRect( &cell, &bg );
	}

	pDC->SelectObject( pOldFont );
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::CKeyboardDlg (0x41D3F0)

CKeyboardDlg::CKeyboardDlg( CWnd* pParent )
	: CDlgBase( IDD_KEYBOARD, pParent )
{
	int	dims[2];

	m_bCapturing = 0;
	m_bDirty = 0;
	m_pRows = NULL;
	m_pSelfWnd = this;		// +204 -- gates the slide transition
	LoadHeaderBitmap( "head_controls", 0 );

	m_pBindList = NULL;
	m_pSearchCombo = NULL;
	m_bkBrush.CreateSolidBrush( RGB( 0, 0, 0 ) );

	memset( m_keys, 0, sizeof( m_keys ) );
	m_nKeys = 0;
	memset( m_clearedKeys, 0, sizeof( m_clearedKeys ) );
	m_nClearedKeys = 0;

	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnOK.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_OK, m_headerLoaded );		// +808
		m_btnCancel.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_BACK, m_headerLoaded );	// +568
		m_btnDefaults.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DEFAULTS, m_headerLoaded );	// +1048
		m_btnAdvanced.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_ADVANCED, m_headerLoaded );	// +328
	}
}

// CKeyboardDlg::~CKeyboardDlg (0x41D5A0 -> body 0x41DC90)
CKeyboardDlg::~CKeyboardDlg()
{
	CODKeyBindingRow*	row = m_pRows;

	while ( row )
	{
		CODKeyBindingRow*	next = row->m_pNext;
		row->m_pNext = NULL;
		delete row;
		row = next;
	}
	m_pRows = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::DoDataExchange (0x41D5C0)

void CKeyboardDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_CONTROLS_KEYHELP, m_lblHelp );			// 1149 (+228)
	DDX_Control( pDX, IDC_KEYBOARD_ADVANCED, m_btnAdvanced );	// 34   (+328)
	DDX_Control( pDX, IDC_KEYBOARD_CANCEL, m_btnCancel );				// 25   (+568)
	DDX_Control( pDX, IDOK, m_btnOK );								// 1    (+808)
	DDX_Control( pDX, IDC_KEYBOARD_USEDEFAULTS, m_btnDefaults );		// 21   (+1048)
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::FindRowByCommand (0x41D630)

CODKeyBindingRow* CKeyboardDlg::FindRowByCommand( const char* pszCommand )
{
	for ( CODKeyBindingRow* row = m_pRows; row; row = row->m_pNext )
	{
		if ( !_strcmpi( pszCommand, row->GetCommand() ) )
			return row;
	}
	return NULL;
}

// One left-aligned skin button.
static void Kb_StyleButton( CODBlendBtn* btn, const char* pszCaption )
{
	btn->m_bHasArrow = 0;
	btn->m_textFlags &= ~DT_CENTER;
	btn->SetFontSize( 14, FW_HEAVY );		// vtbl+188
	btn->m_clrDown = RGB( 240, 180, 20 );
	SetWindowTextSafe( btn, pszCaption );
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnInitDialog (0x41D670)

BOOL CKeyboardDlg::OnInitDialog()
{
	RECT		rc;
	odcolumn_t	col;
	int			dims[2];

	CDialog::OnInitDialog();

	Joy_Detect();
	if ( joy_avail )
		Joy_AdvancedUpdate();
	Dlg_CenterWindow( this );

	// Layout metrics: wide string tables (IDS 0x1EE) pull the button column in.
	int		bWide = Launcher_StringHeight( IDS_SPANISH, 0 );
	int		x = bWide ? 20 : 50;
	Launcher_HeaderSize( dims );
	int		w = dims[0];
	int		h = dims[1];
	int		listLeft = bWide ? ( w + 70 ) : ( x + w + 20 );
	int		right = g_nLauncherDefW - 35;
	int		bottom = g_nLauncherDefH - 50;

	Kb_StyleButton( &m_btnCancel, Launcher_LoadString( IDS_BTN_REVERT ) );		// 0x10A
	Kb_StyleButton( &m_btnDefaults, Launcher_LoadString( IDS_BTN_RESTORE ) );	// 0x10B
	Kb_StyleButton( &m_btnAdvanced, Launcher_LoadString( IDS_BTN_ADVANCED ) );	// 0x109

	// The binding list, a key-binding-aware CODListCtrl.
	m_pBindList = new CODKeyBindingCtrl( this );
	rc.left = listLeft; rc.top = 140; rc.right = right; rc.bottom = bottom - 10;
	m_pBindList->Create( 0, rc, this, IDC_KEYBOARD_BINDLIST );
	m_pBindList->SetRowHeight( 20 );

	m_btnDefaults.MoveWindow( x, 140, w, h, TRUE );
	m_btnAdvanced.MoveWindow( x, 172, w, h, TRUE );
	m_btnOK.MoveWindow( x, 204, w, h, TRUE );
	SetWindowTextSafe( &m_btnOK, Launcher_LoadString( IDS_BTN_OK ) );		// 0x118
	m_btnCancel.MoveWindow( x, 236, w, h, TRUE );

	// The help label under the buttons.
	m_lblHelp.MoveWindow( x + 6, 268, w, bottom - 258, TRUE );
	m_lblHelp.SetTransparent( 1 );		// targets m_lblHelp
	m_lblHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblHelp.SetFontSize( 11, FW_NORMAL );
	m_lblHelp.SetWindowText( Launcher_LoadString( IDS_CONTROLS_KEYHELP ) );		// 0x4A

	// Columns: Action / key / alternate (the last two split the remainder).
	int		keyColW = ( right - listLeft - 170 ) / 2;
	Launcher_LoadStringInto( col.title, IDS_BINDING_ACTIONHEADING );			// 0x56
	col.width = 170;
	m_pBindList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_BINDING_PRIMARYHEADING );			// 0x57
	col.width = keyColW;
	m_pBindList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_BINDING_ALTERNATEHEADING );			// 0x58
	col.width = keyColW;
	m_pBindList->AddColumn( &col );

	// The key-search combo (0xD4 bytes), id 113, riding the binding list.
	m_pSearchCombo = new CODKeySearchComboBox( m_pBindList, this );
	m_pSearchCombo->SetDropHeight( 120 );
	rc.left = 0; rc.top = 0; rc.right = 15; rc.bottom = 100;
	m_pSearchCombo->Create( 0, &rc, this, IDC_KEYBOARD_KEYSEARCH );
	// It rides the selected cell and is only shown for the duration of a key
	// capture; Create always ORs WS_VISIBLE, so hide it again immediately.
	m_pSearchCombo->ShowWindow( SW_HIDE );
	m_pSearchCombo->SetFrameColor( m_pBindList->m_clrCell );
	m_pSearchCombo->SetTextColor( m_pBindList->m_clrCellHot );
	m_pSearchCombo->SetFaceColor( m_pBindList->m_clrCell );

	// kb_keys.lst (the key table + search pool), then kb_act.lst (the rows).
	LoadKeyList();
	m_pSearchCombo->SetCurSel( 0 );
	LoadActionList();

	if ( !g_pServerBrowser )
	{
		// no player config to edit
		Launcher_ShowMessageById( 0, IDS_AUDIO_NOPROFILE );		// 0x48
		OnCancel();
		return TRUE;
	}

	// Mirror the config's 256 binding slots into the action rows.
	kbbinding_t*	bindings = (kbbinding_t*)( (char*)&g_pServerBrowser->m_playerConfig );
	for ( int i = 0; i < 256; i++ )
	{
		kbbinding_t*	e = &bindings[i];
		if ( !_strnicmp( e->szKeyName, "<UNKNOWN", strlen( "<UNKNOWN" ) ) )
			continue;
		if ( !e->szKeyName[0] || !e->pszCommand )
			continue;

		CODKeyBindingRow*	row = FindRowByCommand( e->pszCommand );
		if ( row )
			row->AssignKey( e->szKeyName, GetKeyColor( e->szKeyName ) );
	}

	m_pBindList->SelectItem( 0, 1 );
	m_pBindList->RefitScrollbar();
	m_pBindList->SetDrawFrame( 1 );
	m_pBindList->SetHeaderTransparent( 1 );
	m_pBindList->SetTransparent( 0 );
	m_pBindList->AttachSearchCombo( m_pSearchCombo );
	m_pBindList->SetFocus();

	return FALSE;						// focus set manually
}

/*
==================
Kb_TranslateKey (0x41DD80)
==================
*/
const char* Kb_TranslateKey( int bExtended, UINT nVirtKey, int* pKeyNum )
{
	s_szKeyName[0] = 0;

	// Letters and digits translate to themselves.
	if ( ( nVirtKey >= 'A' && nVirtKey <= 'Z' ) || ( nVirtKey >= '0' && nVirtKey <= '9' ) )
	{
		sprintf( s_szKeyName, "%c", (char)nVirtKey );
		*pKeyNum = nVirtKey;
		return s_szKeyName;
	}

	// VK_F1 (0x70) .. VK_F12 (0x7B) -> "F1".."F12".
	if ( nVirtKey >= VK_F1 && nVirtKey <= VK_F12 )
	{
		sprintf( s_szKeyName, "F%i", nVirtKey - ( VK_F1 - 1 ) );
		*pKeyNum = nVirtKey + 23;
		return s_szKeyName;
	}

	// Numeric-keypad range (VK_NUMPAD0 0x60 ..
	if ( nVirtKey >= VK_NUMPAD0 && nVirtKey <= VK_DIVIDE )
	{
		switch ( nVirtKey )
		{
		case VK_NUMPAD0:	sprintf( s_szKeyName, "KP_INS" );			*pKeyNum = 170; return s_szKeyName;
		case VK_NUMPAD1:	sprintf( s_szKeyName, "KP_END" );			*pKeyNum = 166; return s_szKeyName;
		case VK_NUMPAD2:	sprintf( s_szKeyName, "KP_DOWNARROW" );		*pKeyNum = 167; return s_szKeyName;
		case VK_NUMPAD3:	sprintf( s_szKeyName, "KP_PGDN" );			*pKeyNum = 168; return s_szKeyName;
		case VK_NUMPAD4:	sprintf( s_szKeyName, "KP_LEFTARROW" );		*pKeyNum = 163; return s_szKeyName;
		case VK_NUMPAD5:	sprintf( s_szKeyName, "KP_5" );				*pKeyNum = 164; return s_szKeyName;
		case VK_NUMPAD6:	sprintf( s_szKeyName, "KP_RIGHTARROW" );	*pKeyNum = 165; return s_szKeyName;
		case VK_NUMPAD7:	sprintf( s_szKeyName, "KP_HOME" );			*pKeyNum = 160; return s_szKeyName;
		case VK_NUMPAD8:	sprintf( s_szKeyName, "KP_UPARROW" );		*pKeyNum = 161; return s_szKeyName;
		case VK_NUMPAD9:	sprintf( s_szKeyName, "KP_PGUP" );			*pKeyNum = 162; return s_szKeyName;
		case VK_ADD:		sprintf( s_szKeyName, "KP_PLUS" );			*pKeyNum = 174; return s_szKeyName;
		case VK_SUBTRACT:	sprintf( s_szKeyName, "KP_MINUS" );			*pKeyNum = 173; return s_szKeyName;
		case VK_DECIMAL:	sprintf( s_szKeyName, "KP_DEL" );			*pKeyNum = 171; return s_szKeyName;
		case VK_DIVIDE:		sprintf( s_szKeyName, "KP_SLASH" );			*pKeyNum = 172; return s_szKeyName;
		default:			break;	// 0x6A / 0x6C fall through to the shared table
		}
	}

	// The shared navigation / edit / OEM table.
	switch ( nVirtKey )
	{
	case VK_BACK:		sprintf( s_szKeyName, "BACKSPACE" );	*pKeyNum = 127; break;
	case VK_TAB:		sprintf( s_szKeyName, "TAB" );			*pKeyNum = 9;   break;
	case VK_RETURN:
		// The keypad ENTER comes through with the extended-key flag set.
		if ( bExtended )
		{
			sprintf( s_szKeyName, "KP_ENTER" );
			*pKeyNum = 169;
		}
		else
		{
			sprintf( s_szKeyName, "ENTER" );
			*pKeyNum = 13;
		}
		break;
	case VK_SHIFT:		sprintf( s_szKeyName, "SHIFT" );		*pKeyNum = 134; break;
	case VK_CONTROL:	sprintf( s_szKeyName, "CTRL" );			*pKeyNum = 133; break;
	case VK_MENU:		sprintf( s_szKeyName, "ALT" );			*pKeyNum = 132; break;
	case VK_PAUSE:		sprintf( s_szKeyName, "PAUSE" );		*pKeyNum = 255; break;
	case VK_CAPITAL:	sprintf( s_szKeyName, "CAPSLOCK" );		*pKeyNum = 175; break;
	case VK_ESCAPE:		sprintf( s_szKeyName, "ESCAPE" );		*pKeyNum = 27;  break;
	case VK_SPACE:		sprintf( s_szKeyName, "SPACE" );		*pKeyNum = 32;  break;
	case VK_PRIOR:		sprintf( s_szKeyName, "PGUP" );			*pKeyNum = 150; break;
	case VK_NEXT:		sprintf( s_szKeyName, "PGDN" );			*pKeyNum = 149; break;
	case VK_END:		sprintf( s_szKeyName, "END" );			*pKeyNum = 152; break;
	case VK_HOME:		sprintf( s_szKeyName, "HOME" );			*pKeyNum = 151; break;
	case VK_LEFT:		sprintf( s_szKeyName, "LEFTARROW" );	*pKeyNum = 130; break;
	case VK_UP:			sprintf( s_szKeyName, "UPARROW" );		*pKeyNum = 128; break;
	case VK_RIGHT:		sprintf( s_szKeyName, "RIGHTARROW" );	*pKeyNum = 131; break;
	case VK_DOWN:		sprintf( s_szKeyName, "DOWNARROW" );	*pKeyNum = 129; break;
	case VK_INSERT:		sprintf( s_szKeyName, "INS" );			*pKeyNum = 147; break;
	case VK_DELETE:		// 0x2E
	case VK_DECIMAL:			// keypad '.' when NOT extended (VK_DECIMAL) -- also "DEL"
		sprintf( s_szKeyName, "DEL" );
		*pKeyNum = 148;
		break;
	case VK_MULTIPLY:	sprintf( s_szKeyName, "*" );			*pKeyNum = 42;  break;
	case VK_ADD:		sprintf( s_szKeyName, "+" );			*pKeyNum = 43;  break;
	case VK_SUBTRACT:
	case VK_OEM_MINUS:	sprintf( s_szKeyName, "-" );			*pKeyNum = 45;  break;
	case VK_DIVIDE:
	case VK_OEM_2:		sprintf( s_szKeyName, "/" );			*pKeyNum = 47;  break;
	case VK_OEM_1:		sprintf( s_szKeyName, ";" );			*pKeyNum = 59;  break;
	case VK_OEM_PLUS:	sprintf( s_szKeyName, "=" );			*pKeyNum = 61;  break;
	case VK_OEM_COMMA:	sprintf( s_szKeyName, "," );			*pKeyNum = 44;  break;
	case VK_OEM_PERIOD:	sprintf( s_szKeyName, "." );			*pKeyNum = 46;  break;
	case VK_OEM_3:		sprintf( s_szKeyName, "`" );			*pKeyNum = 126; break;
	case VK_OEM_4:		sprintf( s_szKeyName, "[" );			*pKeyNum = 91;  break;
	case VK_OEM_5:		sprintf( s_szKeyName, "\\" );			*pKeyNum = 92;  break;
	case VK_OEM_6:		sprintf( s_szKeyName, "]" );			*pKeyNum = 93;  break;
	case VK_OEM_7:		sprintf( s_szKeyName, "'" );			*pKeyNum = 39;  break;
	default:
		sprintf( s_szKeyName, "<UNKNOWN KEYNUM>" );
		*pKeyNum = 0;
		break;
	}

	return s_szKeyName;
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::PreTranslateMessage (0x41E720)

BOOL CKeyboardDlg::PreTranslateMessage( MSG* pMsg )
{
	if ( !m_bCapturing )
	{
#ifdef LAUNCHER_FIXES
		// Not capturing, so a wheel roll is a scroll and not a binding.  This page
		// never chains to CDlgBase::PreTranslateMessage, so it re-aims it itself.
		if ( pMsg->message == WM_MOUSEWHEEL && Dlg_RouteMouseWheel( pMsg ) )
			return TRUE;
#endif

		if ( pMsg->message == WM_KEYDOWN )
		{
			if ( pMsg->wParam == VK_RETURN )
			{
				// The binary tests CODBlendBtn's own runtime class (0x4B4C80).
				CWnd*			focus = CWnd::FromHandlePermanent( ::GetFocus() );
				CODBlendBtn*	btn = (CODBlendBtn*)AfxDynamicDownCast( RUNTIME_CLASS( CButton ), focus );
				if ( !btn )
					return CWnd::PreTranslateMessage( pMsg );
				::SendMessageA( m_hWnd, WM_COMMAND, btn->GetDlgCtrlID(), (LPARAM)btn->m_hWnd );
				return TRUE;
			}
			if ( pMsg->wParam == VK_ESCAPE )
			{
				OnCancelPage();
				return TRUE;
			}
		}
		return CDialog::PreTranslateMessage( pMsg );
	}

	// - capturing the next input as a key name ---

	if ( Joy_GetPressedButton( s_szPickedKey ) )		// ("JOY*/AUX*/POV*")
		SetCapturing( 0 );

	if ( pMsg->message == guMouseWheelMsg )		// registered MSWHEEL_ROLLMSG
	{
		strcpy( s_szPickedKey, (int)pMsg->wParam > 0 ? "MWHEELUP" : "MWHEELDOWN" );
		SetCapturing( 0 );
	}
	if ( pMsg->message == WM_MOUSEWHEEL )			// 0x20A
	{
		strcpy( s_szPickedKey, (short)HIWORD( pMsg->wParam ) > 0 ? "MWHEELUP" : "MWHEELDOWN" );
		SetCapturing( 0 );
	}
	if ( pMsg->message == WM_XBUTTONDOWN )			// 0x20B
	{
		if ( pMsg->wParam & MK_XBUTTON1 )					// MK_XBUTTON1
		{
			strcpy( s_szPickedKey, "MOUSE4" );
			SetCapturing( 0 );
		}
		else if ( pMsg->wParam & MK_XBUTTON2 )				// MK_XBUTTON2
		{
			strcpy( s_szPickedKey, "MOUSE5" );
			SetCapturing( 0 );
		}
	}

	if ( pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN )
	{
		int		bExtended = ( pMsg->lParam >> 24 ) & 1;

		// ALT chords with the arrow keys stand in for the extra mouse buttons.
		if ( pMsg->message == WM_SYSKEYDOWN && GetAsyncKeyState( VK_LEFT ) )
		{
			strcpy( s_szPickedKey, "MOUSE4" );
		}
		else if ( pMsg->message == WM_SYSKEYDOWN && GetAsyncKeyState( VK_RIGHT ) )
		{
			strcpy( s_szPickedKey, "MOUSE5" );
		}
		else
		{
			int		bValid = 0;
			strcpy( s_szPickedKey, Kb_TranslateKey( bExtended, pMsg->wParam, &bValid ) );
			if ( !_strcmpi( s_szPickedKey, "ESCAPE" ) || !_strnicmp( s_szPickedKey, "<UNK", 4 ) )
			{
				// abandon the pick
				m_pBindList->InvalidateRect( NULL, TRUE );
				memset( s_szPickedKey, 0, sizeof( s_szPickedKey ) );
				SetCapturing( 0 );
				return TRUE;
			}
			if ( !bValid )
				memset( s_szPickedKey, 0, sizeof( s_szPickedKey ) );
		}
		SetCapturing( 0 );
	}

	if ( pMsg->message == WM_LBUTTONDOWN )			// 0x201
	{
		strcpy( s_szPickedKey, "MOUSE1" );
		SetCapturing( 0 );
	}
	if ( pMsg->message == WM_MBUTTONDOWN )			// 0x207
	{
		strcpy( s_szPickedKey, "MOUSE3" );
		SetCapturing( 0 );
	}
	if ( pMsg->message == WM_RBUTTONDOWN )			// 0x204
	{
		strcpy( s_szPickedKey, "MOUSE2" );
		SetCapturing( 0 );
	}
	if ( pMsg->message == guMouseWheelMsg )		// (checked again, as in the binary)
	{
		strcpy( s_szPickedKey, (int)pMsg->wParam > 0 ? "MWHEELUP" : "MWHEELDOWN" );
		SetCapturing( 0 );
	}

	if ( !m_bCapturing && s_szPickedKey[0] )
		CommitPickedKey( s_szPickedKey );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnBindListSelChange (0x41EB80)
//

void CKeyboardDlg::OnBindListSelChange()
{
	int		sel = m_pBindList->GetCurSel();
	if ( sel != -1 )
		m_pBindList->GetItemData( sel );
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnSearchSelChange (0x41EBB0)
//
// id 113, CBN_SELCHANGE

void CKeyboardDlg::OnSearchSelChange()
{
	int		sel = m_pSearchCombo->GetCurSel();
	if ( sel == -1 )
		return;
	const char*	key = m_pSearchCombo->GetString( sel );
	if ( !key )
		return;

	if ( !m_pBindList->m_pSearchCombo )
		return;
	if ( !::IsWindowVisible( m_pBindList->m_pSearchCombo->GetSafeHwnd() ) )
		return;
	if ( !m_pBindList->m_bPickPending )
		return;
	m_pBindList->m_bPickPending = 0;

	int		pickRow = m_pBindList->m_iPickRow;			// +1924
	int		pickCol = m_pBindList->m_iPickCol;			// +1928 (1 = primary, 2 = alternate)
	if ( pickRow < 0 )
		return;
	CODKeyBindingRow*	target = (CODKeyBindingRow*)m_pBindList->GetItemData( pickRow );
	if ( !target )
		return;

	int		rows = m_pBindList->GetRowCount();
	for ( int i = 0; i < rows; i++ )
	{
		CODKeyBindingRow*	row = (CODKeyBindingRow*)m_pBindList->GetItemData( i );
		if ( row == target )
		{
			// the target's OTHER cell loses a duplicate of the picked key
			if ( pickCol == 1 )
			{
				if ( !_strcmpi( row->GetAltKey(), key ) )
				{
					row->ClearPrimary();		// (as in the binary)
					RememberClearedKey( key );
				}
			}
			else if ( !_strcmpi( row->GetPrimaryKey(), key ) )
			{
				row->ClearAlternate();
				RememberClearedKey( key );
			}
		}
		else
		{
			if ( !_strcmpi( row->GetPrimaryKey(), key ) )
			{
				row->ClearPrimary();
				RememberClearedKey( key );
			}
			if ( !_strcmpi( row->GetAltKey(), key ) )
			{
				row->ClearAlternate();
				RememberClearedKey( key );
			}
		}
	}

	// The cell's old key goes away too, then the pick lands.
	RememberClearedKey( pickCol == 1 ? target->GetPrimaryKey() : target->GetAltKey() );
	target->SetKey( key, GetKeyColor( key ), pickCol == 2 );

	m_pBindList->InvalidateRect( NULL, TRUE );
	m_pBindList->UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnCtlColor (0x41ED90)

HBRUSH CKeyboardDlg::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH	hbr = CDialog::OnCtlColor( pDC, pWnd, nCtlColor );

	if ( nCtlColor <= CTLCOLOR_EDIT )
	{
		pDC->SetTextColor( RGB( 255, 127, 24 ) );
		pDC->SetBkMode( TRANSPARENT );
		pDC->SetBkColor( RGB( 0, 0, 0 ) );
		return (HBRUSH)m_bkBrush.GetSafeHandle();
	}
	return hbr;
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::LoadActionList (0x41EDF0)

void CKeyboardDlg::LoadActionList()
{
	char		command[256];
	char		text[256];

	char*	file = (char*)COM_LoadMallocFile( "gfx/shell/kb_act.lst" );
	if ( !file )
	{
		Launcher_ShowMessageById( 0, IDS_CONTROLS_KBLIST_EMPTY );		// 0xD
		return;
	}

	CToken	tok( file );
	tok.SetQuoteMode( 1 );

	for ( ;; )
	{
		tok.ParseNextToken();					// bind command ("+attack")
		if ( !strlen( tok.token ) )
			break;
		strcpy( command, tok.token );

		tok.ParseNextToken();					// action text ("Attack")
		if ( !strlen( tok.token ) )
		{
			Launcher_ShowMessageById( 0, IDS_CONTROLS_KBLIST_PARSEERROR );	// 0xE
			break;
		}
		strcpy( text, tok.token );

		CODKeyBindingRow*	row = new CODKeyBindingRow;		// 0x1C bytes
		row->Init( text, command, "", RGB( 255, 255, 255 ) );
		row->m_pNext = m_pRows;
		m_pRows = row;
	}
	free( file );

	// The chain built up reversed -- restore file order.
	CODKeyBindingRow*	prev = NULL;
	CODKeyBindingRow*	row = m_pRows;
	while ( row )
	{
		CODKeyBindingRow*	next = row->m_pNext;
		row->m_pNext = prev;
		prev = row;
		row = next;
	}
	m_pRows = prev;

	m_pBindList->ResetContent();
	for ( row = m_pRows; row; row = row->m_pNext )
		m_pBindList->AddRow( row );	// vtbl+196 -- the record IS the row object

}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::GetKeyColor (0x41EFF0)

COLORREF CKeyboardDlg::GetKeyColor( const char* pszKey )
{
	for ( int i = 0; i < m_nKeys; i++ )
	{
		if ( !_strcmpi( pszKey, m_keys[i].szName ) )
			return m_keys[i].color;
	}
	return RGB( 255, 255, 255 );
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::LoadKeyList (0x41F060)

void CKeyboardDlg::LoadKeyList()
{
	char		display[256];

	char*	file = (char*)COM_LoadMallocFile( "gfx/shell/kb_keys.lst" );
	if ( !file )
	{
		Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_EMPTY );		// 0xF
		return;
	}

	CToken	tok( file );
	tok.SetQuoteMode( 1 );

	for ( ;; )
	{
		tok.ParseNextToken();					// index column (unused)
		if ( !strlen( tok.token ) )
			break;

		tok.ParseNextToken();					// engine key name
		if ( !strlen( tok.token ) )
		{
			Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_PARSEERROR );	// 0x10
			break;
		}
		strcpy( s_szPickedKey, tok.token );
		if ( _strnicmp( s_szPickedKey, "<UNK", 4 ) )
			m_pSearchCombo->AddString( s_szPickedKey );

		tok.ParseNextToken();					// display name
		if ( !strlen( tok.token ) )
		{
			Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_PARSEERROR );
			break;
		}
		strcpy( display, tok.token );

		tok.ParseNextToken();					// COLOR keyword, or filler
		if ( !strlen( tok.token ) )
		{
			Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_PARSEERROR );
			break;
		}

		if ( !_strnicmp( tok.token, "COLOR", 5 ) )
		{
			int		r, g, b;
			tok.ParseNextToken();
			r = atoi( tok.token );
			tok.ParseNextToken();
			g = atoi( tok.token );
			tok.ParseNextToken();
			b = atoi( tok.token );
			m_keys[m_nKeys].color = RGB( r, g, b );
		}
		else
		{
			m_keys[m_nKeys].color = RGB( 240, 180, 24 );
		}

		strcpy( m_keys[m_nKeys].szName, s_szPickedKey );
		strcpy( m_keys[m_nKeys].szDisplay, display );

		if ( ++m_nKeys > 256 )
		{
			Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_OVERFLOW );	// 0x11
			break;
		}
	}

	free( file );
}

/*
==================
Kb_FindKeyIndex (0x41F360)

: key name -> kb_keys.lst index (the binding-block
slot), -1 when unknown.
==================
*/
static int Kb_FindKeyIndex( const char* pszKey, const kbkey_t* keys )
{
	for ( int i = 0; i < 256; i++ )
	{
		if ( !_strcmpi( pszKey, keys[i].szName ) )
			return i;
	}
	return -1;
}

/*
==================
Kb_WriteBinding (0x41F3A0)

: put one bound command into the player config's
binding slot (fresh malloc'd command + the canonical key name).
==================
*/
static void Kb_WriteBinding( kbbinding_t* bindings, int idx, const char* pszCommand, const kbkey_t* keys )
{
	kbbinding_t*	e = &bindings[idx];

	if ( e->pszCommand )
	{
		free( e->pszCommand );
		e->pszCommand = NULL;
	}

	int		len = (int)strlen( pszCommand ) + 1;
	e->pszCommand = (char*)malloc( len );
	if ( !e->pszCommand )
		Launcher_ShowMessageById( 0, IDS_PROFILE_ALLOCFAIL );	// 0x47
	memset( e->pszCommand, 0, len );
	strcpy( e->pszCommand, pszCommand );
	e->cmdLen = len;

	memcpy( e->szKeyName, keys[idx].szName, strlen( keys[idx].szName ) + 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnOK (0x41F470)

void CKeyboardDlg::OnOK()
{
	kbbinding_t*	bindings = (kbbinding_t*)( (char*)&g_pServerBrowser->m_playerConfig );

	for ( int i = 0; i < m_nClearedKeys; i++ )
	{
		int		idx = Kb_FindKeyIndex( m_clearedKeys[i].szName, m_keys );
		if ( idx == -1 )
			continue;

		kbbinding_t*	e = &bindings[idx];
		if ( e->pszCommand && e->pszCommand[0] )
		{
			free( e->pszCommand );
			e->pszCommand = NULL;
			e->cmdLen = 0;
		}
	}

	int		rows = m_pBindList->GetRowCount();
	for ( int i = 0; i < rows; i++ )
	{
		CODKeyBindingRow*	row = (CODKeyBindingRow*)m_pBindList->GetItemData( i );
		if ( !row )
			continue;

		if ( strlen( row->GetPrimaryKey() ) )
		{
			int		idx = Kb_FindKeyIndex( row->GetPrimaryKey(), m_keys );
			if ( idx != -1 )
				Kb_WriteBinding( bindings, idx, row->GetCommand(), m_keys );
		}
		if ( strlen( row->GetAltKey() ) )
		{
			int		idx = Kb_FindKeyIndex( row->GetAltKey(), m_keys );
			if ( idx != -1 )
				Kb_WriteBinding( bindings, idx, row->GetCommand(), m_keys );
		}
	}

	Launcher_SavePlayerInfoTo( "Player", bindings );
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::RememberClearedKey (0x41F5F0)

void CKeyboardDlg::RememberClearedKey( const char* pszKey )
{
	if ( !pszKey || !pszKey[0] )
		return;

	for ( int i = 0; i < m_nClearedKeys; i++ )
	{
		if ( !_strcmpi( pszKey, m_clearedKeys[i].szName ) )
			return;							// already recorded
	}
	strcpy( m_clearedKeys[m_nClearedKeys++].szName, pszKey );
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnCancelPage (0x41F680)
//
// cmd 25, and ESC outside a capture

void CKeyboardDlg::OnCancelPage()
{
	if ( m_bDirty )
	{
		CPromptDlg	prompt( 2, NULL );
		prompt.SetMessage( Launcher_LoadString( IDS_CONTROLS_CANCELPROMPT ) );	// 0xED
		if ( prompt.DoModal() != IDOK )
			return;
	}
	OnCancel();
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnUseDefaults (0x41F8B0)
//
// cmd 21

void CKeyboardDlg::OnUseDefaults()
{
	m_bDirty = 1;

	char*	config = (char*)&g_pServerBrowser->m_playerConfig;
	PlayerConfig_LoadDefaults( config );
	PlayerConfig_ApplyDefaults( config );

	// Free the current rows, then rebuild them from kb_act.lst.
	CODKeyBindingRow*	row = m_pRows;
	while ( row )
	{
		CODKeyBindingRow*	next = row->m_pNext;
		row->m_pNext = NULL;
		delete row;
		row = next;
	}
	m_pRows = NULL;
	LoadActionList();

	kbbinding_t*	bindings = (kbbinding_t*)config;
	for ( int i = 0; i < 256; i++ )
	{
		kbbinding_t*	e = &bindings[i];
		if ( !_strnicmp( e->szKeyName, "<UNKNOWN", strlen( "<UNKNOWN" ) ) )
			continue;
		if ( !e->szKeyName[0] || !e->pszCommand )
			continue;

		CODKeyBindingRow*	bound = FindRowByCommand( e->pszCommand );
		if ( bound )
			bound->AssignKey( e->szKeyName, GetKeyColor( e->szKeyName ) );
	}

	m_pBindList->SelectItem( 0, 1 );
	m_pBindList->RefitScrollbar();
	m_pBindList->InvalidateRect( NULL, TRUE );
	m_pBindList->UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnAdvancedOptions (0x41F9C0)
//
// cmd 34

void CKeyboardDlg::OnAdvancedOptions()
{
	CGameOptionsDlg	page;		// /
	InitChildDialog( &page, &m_btnAdvanced );
	page.DoModal();
	RestoreAfterModal();
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::CommitPickedKey (0x41FA50)

void CKeyboardDlg::CommitPickedKey( char* pszKey )
{
	m_bDirty = 1;

	int		pickRow = m_pBindList->m_iPickRow;			// +1924
	int		pickCol = m_pBindList->m_iPickCol;			// +1928
	if ( pickRow < 0 )
		return;
	CODKeyBindingRow*	target = (CODKeyBindingRow*)m_pBindList->GetItemData( pickRow );
	if ( !target )
		return;

	int		rows = m_pBindList->GetRowCount();
	for ( int i = 0; i < rows; i++ )
	{
		CODKeyBindingRow*	row = (CODKeyBindingRow*)m_pBindList->GetItemData( i );
		if ( row == target )
		{
			if ( pickCol == 1 )
			{
				if ( !_strcmpi( row->GetAltKey(), pszKey ) )
				{
					RememberClearedKey( row->GetPrimaryKey() );
					row->ClearPrimary();
				}
			}
			else if ( !_strcmpi( row->GetPrimaryKey(), pszKey ) )
			{
				RememberClearedKey( row->GetAltKey() );
				row->ClearAlternate();
			}
		}
		else
		{
			if ( !_strcmpi( row->GetPrimaryKey(), pszKey ) )
			{
				row->ClearPrimary();
				RememberClearedKey( pszKey );
			}
			if ( !_strcmpi( row->GetAltKey(), pszKey ) )
			{
				row->ClearAlternate();
				RememberClearedKey( pszKey );
			}
		}
	}

	RememberClearedKey( pickCol == 1 ? target->GetPrimaryKey() : target->GetAltKey() );
	target->SetKey( pszKey, GetKeyColor( pszKey ), pickCol == 2 );

	m_pBindList->InvalidateRect( NULL, TRUE );
	m_pBindList->UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::SetCapturing (0x41FC00)

void CKeyboardDlg::SetCapturing( int bOn )
{
	m_bCapturing = bOn;
	if ( bOn )
	{
		SetTimer( 0, 50, NULL );
		Snd_PlayMenuSound( UISND_SELECT1 );
	}
	else
	{
		KillTimer( 0 );
		Snd_PlayMenuSound( UISND_SELECT2 );
	}

	if ( m_pBindList )
		m_pBindList->m_bWheelScroll = ( m_bCapturing == 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::IsCapturing (0x41FC60)
//
//
// the cell flash follows the owner page's capture latch.

int CKeyboardDlg::IsCapturing()
{
	return m_bCapturing;
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::OnPaint (0x41FC70)
//
// the base list paint with the capture
// prompt drawn over it; the scroll offset is taken from the companion scrollbar.

void CODKeyBindingCtrl::OnPaint()
{
	CPaintDC	dc( this );
	CRect		rc;
	GetClientRect( &rc );
	if ( m_bHasScrollbar )
		rc.right -= 16;

	if ( m_bHeaderVisible )
	{
		rc.top += m_headerHeight;
		DrawHeader( &dc );
	}

	if ( m_bDrawFrame )
	{
		for ( int pass = 0; pass < 3; pass++ )
		{
			CBrush	frame( GetFocus() == this ? m_clrFrameFocus : m_clrFrame );
			dc.FrameRect( &rc, &frame );
			rc.DeflateRect( 1, 1 );
		}
	}

	int	visible = GetVisibleRows();
	if ( m_nRows )
	{
		int	maxTop = m_nRows - visible;
		if ( maxTop < 0 )
			maxTop = 0;

		int	pos = m_pScrollbar->GetPos();
		m_topRow = ( pos <= maxTop ) ? pos : maxTop;
	}

	CDC	mem;
	if ( !mem.CreateCompatibleDC( &dc ) )
		return;

	CBitmap		bmp;
	bmp.CreateCompatibleBitmap( &dc, rc.Width(), rc.Height() );
	CBitmap*	pOldBmp = mem.SelectObject( &bmp );

	CRect	bufRc( 0, 0, rc.Width(), rc.Height() );
	if ( m_bTransparent )
	{
		CRect	dst, src;
		GetClientRect( &dst );
		GetWindowRect( &src );
		if ( GetParent() )
			GetParent()->ScreenToClient( &src );
		if ( m_bHeaderVisible )
		{
			dst.bottom -= m_headerHeight;
			src.top    += m_headerHeight;
		}
		if ( m_bHasScrollbar )
		{
			dst.right -= 16;
			src.right -= 16;
		}
		Launcher_BlitBackground( &mem, &dst, &src );
	}
	else
	{
		mem.FillRect( &bufRc, &m_brBg );
	}

	int	last = m_topRow + visible;
	if ( last > m_nRows )
		last = m_nRows;
	for ( int i = m_topRow; i < last; i++ )
		if ( i >= 0 )
			DrawRow( &mem, i );

	dc.BitBlt( rc.left, rc.top, rc.Width(), rc.Height(), &mem, 0, 0, SRCCOPY );
	mem.SelectObject( pOldBmp );
	mem.DeleteDC();

	DrawCapturePrompt( &dc, 0, 0 );
	::ValidateRect( m_hWnd, &rc );
}

/////////////////////////////////////////////////////////////////////////////
// CODKeyBindingCtrl::DrawCapturePrompt (0x420090)
//
// while the page is capturing,
// ring the cell under the keyboard cursor and hang a callout off it.

void CODKeyBindingCtrl::DrawCapturePrompt( CDC* pDC, int, int )
{
	if ( !m_pOwnerDlg->IsCapturing() )
		return;

	CRect	rc;
	if ( !GetCellRect( GetCurSel(), m_iCurCol, &rc ) )
		return;

	rc.bottom += 2;
	rc.left++;
	rc.top++;
	rc.InflateRect( -2, -2 );

	for ( int pass = 0; pass < 3; pass++ )
	{
		CBrush	frame( RGB( 255, 180, 56 ) );	// 0x38B4FF
		pDC->FrameRect( &rc, &frame );
		rc.InflateRect( 1, 1 );
	}
	rc.InflateRect( 2, 2 );

	CRect	rcClient;
	GetClientRect( &rcClient );
	if ( rc.top <= ( rcClient.top + rcClient.bottom ) / 2 )
		rc.OffsetRect( 0, 30 );
	else
		rc.OffsetRect( 0, -30 );

	rc.left  = m_cols[0].width - 50 - Launcher_StringHeight( IDS_KEYBOARD_OFFSET, 0 );
	rc.right = rcClient.right - 21;

	CBrush	fill( RGB( 56, 56, 56 ) );			// 0x383838
	pDC->FillRect( &rc, &fill );
	CBrush	edge( RGB( 255, 180, 56 ) );		// 0x38B4FF
	pDC->FrameRect( &rc, &edge );

	pDC->SetTextColor( RGB( 255, 180, 56 ) );
	pDC->SetBkMode( TRANSPARENT );
	CFont*	pOldFont = pDC->SelectObject( &m_cellFont );

	char	szPrompt[128];
	Launcher_LoadStringInto( szPrompt, IDS_BINDING_PROMPT );

	rc.top  += 6;
	rc.left += 4;
	pDC->DrawText( szPrompt, -1, &rc, DT_VCENTER | DT_NOPREFIX );

	pDC->SelectObject( pOldFont );
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnDisplayChange (0x453D00)
//
// a mode change re-centres the page
// at the launcher's default size.

LRESULT CKeyboardDlg::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnKeyDown (0x455E00)
//
// folded stub

void CKeyboardDlg::OnKeyDown( UINT /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/ )
{
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnActivateApp (0x406FE0)

void CKeyboardDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::OnTimer (0x420320)
//
// forwards to the default proc; the 50 ms
// capture tick only keeps the pump alive.

void CKeyboardDlg::OnTimer( UINT_PTR nIDEvent )
{
	CDialog::OnTimer( nIDEvent );
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::SetDirty (0x420330)
//
//
// the OD list marks the page dirty when a cell edit starts.

void CKeyboardDlg::SetDirty()
{
	m_bDirty = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDlg::GetKeyDisplayName (0x420340)

const char* CKeyboardDlg::GetKeyDisplayName( const char* pszKey )
{
	for ( int i = 0; i < 256; i++ )
	{
		if ( !_strcmpi( pszKey, m_keys[i].szName ) )
			return m_keys[i].szDisplay;
	}
	return "";
}

COLORREF	CODKeyBindingRow::GetAltColor()		{ return m_clrAlt; }

