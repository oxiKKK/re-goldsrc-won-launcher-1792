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
// Purpose: CRoomDialog, the chat-room list shell, CRoomListCtrl and
//          CRoomStatic.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Entries at 0x4B3120, base map 0x4B4398 = CDialog.
BEGIN_MESSAGE_MAP( CRoomDialog, CDialog )
	//{{AFX_MSG_MAP(CRoomDialog)
	ON_MESSAGE( WM_DISPLAYCHANGE, OnDisplayChange )
	ON_WM_PAINT()
	ON_COMMAND( IDC_ROOM_CREATEGAME, OnCreateGame )
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
	ON_COMMAND( IDC_ROOM_PERMANENT, OnTogglePermanent )
	ON_COMMAND( IDC_ROOM_CREATE_ROOM, OnCreateGame )
	ON_CONTROL( LBN_DBLCLK, IDC_ROOM_LIST, OnRoomListSelect )
	ON_COMMAND( IDC_ROOM_USER, OnToggleUserCreated )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Entries at 0x4B33E0, base map 0x4B5778 = CListCtrl.
BEGIN_MESSAGE_MAP( CRoomListCtrl, CListCtrl )
	//{{AFX_MSG_MAP(CRoomListCtrl)
	ON_WM_SIZE()
	ON_WM_DRAWITEM()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_MESSAGE( LVM_SETTEXTCOLOR,   OnSetTextColor )
	ON_MESSAGE( LVM_SETTEXTBKCOLOR, OnSetBkColor )
	ON_MESSAGE( LVM_SETBKCOLOR,     OnSetBkColor )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Entries at 0x4B34B0, base map 0x4B23B0 = CODStatic.
BEGIN_MESSAGE_MAP( CRoomStatic, CODStatic )
	//{{AFX_MSG_MAP(CRoomStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::CRoomDialog (0x45C8F0)

CRoomDialog::CRoomDialog( CNetGameDlg* pSheet, CWnd* pParent )
	: CDlgBase( IDD_ROOM, pParent )
{
	// Neither m_pRoomList nor the two filter flags are seeded here --
	// OnInitDialog creates the list and reads the flags back out of the
	// profile.
	m_pSelfWnd      = this;		// the page points the slide at itself
	m_pSheet        = pSheet;
	m_pRoomListHead = pSheet->m_pRoomList;
	m_pszPickedRoom = NULL;
	m_clrListText   = RGB( 127, 127, 127 );
	m_clrListInfo   = RGB( 127, 127, 127 );
	m_clrListBg     = RGB( 63, 63, 63 );
	m_clrListSel    = RGB( 192, 128, 63 );

	LoadHeaderBitmap( "head_rooms", NULL );
	InitButtonStrips();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::InitButtonStrips (0x45CA30)

void CRoomDialog::InitButtonStrips()
{
	int	wh[2];

	m_hStripDib = Launcher_HeaderLoaded();
	Launcher_HeaderSize( wh );
	m_stripWH[0]   = wh[0];
	m_stripWH[1]   = wh[1];
	m_nStripStride = Launcher_HeaderStride();

	if ( m_hStripDib )
	{
		m_btnJoin.FreeSkinBitmaps();
		m_btnJoin.SetDIBData( CSize( m_stripWH[0], m_stripWH[1] ), BTNSTRIP_JOIN, m_hStripDib );
		m_btnCreate.FreeSkinBitmaps();
		m_btnCreate.SetDIBData( CSize( m_stripWH[0], m_stripWH[1] ), BTNSTRIP_CREATE_GAME, m_hStripDib );
		m_btnCancel.FreeSkinBitmaps();
		m_btnCancel.SetDIBData( CSize( m_stripWH[0], m_stripWH[1] ), BTNSTRIP_BACK, m_hStripDib );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::~CRoomDialog (0x45CAE0)

CRoomDialog::~CRoomDialog()
{
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::DoDataExchange (0x45CBA0)

void CRoomDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_ROOM_USER,          m_lblUserCreated );
	DDX_Control( pDX, IDC_ROOM_PERMANENT,     m_lblPermanent );
	DDX_Control( pDX, IDCANCEL,               m_btnCancel );
	DDX_Control( pDX, IDC_ROOM_CREATE_ROOM,   m_btnCreate );
	DDX_Control( pDX, IDOK,                   m_btnJoin );
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::OnOK (0x45CC20)

void CRoomDialog::OnOK()
{
	// Fall back to row 0 when nothing is selected, so closing the page with an
	// empty selection still joins the top room.
	m_pszPickedRoom = NULL;

	int	sel = m_pRoomList->GetCurSel();
	if ( sel == -1 )
		sel = 0;

	chatroom_t*	pRoom = (chatroom_t*)m_pRoomList->GetItemData( sel );
	if ( pRoom )
		m_pszPickedRoom = pRoom->m_szName;

	Launcher_WriteProfileInt( "Settings", "Show Permanent",    m_bShowPermanent   != 0 );
	Launcher_WriteProfileInt( "Settings", "Show User-Created", m_bShowUserCreated != 0 );
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::OnInitDialog (0x45CCA0)
//
// The room list fills the right-hand panel; Join / Create room / Cancel stack
// down the left edge with the two filter toggles under them.

BOOL CRoomDialog::OnInitDialog()
{
	int			wh[2];
	odcolumn_t	col;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	// Every control is placed off the header-strip cell size, not fixed
	// numbers.
	Launcher_HeaderSize( wh );
	Launcher_HeaderSize( wh );

	int	cw = wh[0];		// cell width  -> button width
	int	ch = wh[1];		// cell height -> button height

	// Owner-draw room list, 2 columns, to the RIGHT of the button column.  It
	// is created with no style bits at all -- WS_VISIBLE arrives later, with
	// the page's own show.
	RECT	rcList;

	rcList.left   = cw + 60;
	rcList.top    = 140;
	rcList.right  = g_nLauncherDefW - 20;
	rcList.bottom = g_nLauncherDefH - 20;

	m_pRoomList = new CODRoomListCtrl;
	m_pRoomList->SetRowHeight( 30 );
	m_pRoomList->Create( 0, rcList, this, IDC_ROOM_LIST );
	// SetTransparent(0) is what makes the list an opaque black panel for its
	// whole height; left at the ctor default of 1 it shows the parent skin
	// everywhere no row happens to be drawn.
	m_pRoomList->SetTransparent( 0 );
	m_pRoomList->SetHeaderTransparent( 1 );
	m_pRoomList->SetDrawFrame( 1 );

	Launcher_LoadStringInto( col.title, IDS_ROOM_NAMECOL );
	col.width = 150;
	m_pRoomList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_ROOM_PEOPLECOL );
	col.width = 75;
	m_pRoomList->AddColumn( &col );

	// Join / Create room / Cancel, 32px apart down the left edge.  Join keeps
	// the caption the template gave it.
	m_btnJoin.MoveWindow( 50, 140, cw, ch, TRUE );
	m_btnCreate.MoveWindow( 50, 172, cw, ch, TRUE );
	m_btnCreate.SetWindowText( Launcher_LoadString( IDS_BTN_CREATE ) );

	int	nCancelPad = Launcher_StringHeight( IDS_ROOM_OFFSET, 0 );	// locale widener
	m_btnCancel.MoveWindow( 50, 204, nCancelPad + cw, ch, TRUE );
	m_btnCancel.SetWindowText( Launcher_LoadString( IDS_BTN_CANCEL ) );

	// The two filter toggles sit at the bottom of the same column.
	int	xToggle = Launcher_StringHeight( IDS_FRENCH, 0 ) ? 30 : 50;
	int	yToggle = g_nLauncherDefH - 84;

	m_lblPermanent.MoveWindow( xToggle, yToggle, cw + 50 - xToggle, ch, TRUE );
	m_lblPermanent.SetWindowText( Launcher_LoadString( IDS_ROOM_PERMANENT ) );
	m_lblUserCreated.MoveWindow( xToggle, yToggle + 32, cw + 50 - xToggle, ch, TRUE );
	m_lblUserCreated.SetWindowText( Launcher_LoadString( IDS_ROOM_USER ) );

	// The profile value is written straight back before it is narrowed, so a
	// stored 0 stays 0 and anything else is remembered verbatim.
	int	nPermanent = Launcher_GetProfileInt( "Settings", "Show Permanent", 1 );
	Launcher_WriteProfileInt( "Settings", "Show Permanent", nPermanent );
	m_bShowPermanent = ( nPermanent == 1 );

	int	nUserCreated = Launcher_GetProfileInt( "Settings", "Show User-Created", 1 );
	Launcher_WriteProfileInt( "Settings", "Show User-Created", nUserCreated );
	m_bShowUserCreated = ( nUserCreated == 1 );

	// Reflect the filters onto the boxes and make them owner-draw so the glyph
	// paints.  BM_SETCHECK would repaint through the template's own class,
	// which is not the one doing the drawing.
	m_lblPermanent.m_bChecked = m_bShowPermanent;
	::InvalidateRect( m_lblPermanent.m_hWnd, NULL, TRUE );
	m_lblUserCreated.m_bChecked = m_bShowUserCreated;
	::InvalidateRect( m_lblUserCreated.m_hWnd, NULL, TRUE );

	::SetWindowLong( m_lblPermanent.GetSafeHwnd(), GWL_STYLE,
		m_lblPermanent.GetStyle() | BS_OWNERDRAW );
	::SetWindowLong( m_lblUserCreated.GetSafeHwnd(), GWL_STYLE,
		m_lblUserCreated.GetStyle() | BS_OWNERDRAW );

	ShowWindow( SW_RESTORE );
	UpdateWindow();

	// The dialog re-runs the directory fetch itself against the sheet, so the
	// list it shows is current rather than whatever the chat page last cached.
	// The argument lands in the frame and is never read. (sic)
	if ( m_pSheet->FetchRoomList( 0 ) )
	{
		m_pSheet->SetCurrentRoom( NULL );
		PopulateRooms();
	}
	else
	{
		Launcher_ShowMessageById( 0, IDS_CHAT_NOROOMLIST );	// "Could not obtain room list"
		OnOK();
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::GetPickedRoomName (0x45D140)

const char* CRoomDialog::GetPickedRoomName()
{
	return m_pszPickedRoom;
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::OnCreateGame (0x45D150)

void CRoomDialog::OnCreateGame()
{
	CCreateRoomDialog	dlg( this );
	if ( dlg.DoModal() != IDOK )
		return;

	// The entered name, clamped to the directory record's 64-byte name field.
	char	szName[64];

	memset( szName, 0, sizeof( szName ) );
	strncpy( szName, (LPCTSTR)dlg.m_strRoomName, sizeof( szName ) - 1 );
	szName[sizeof( szName ) - 1] = 0;

	if ( !strlen( szName ) )
	{
		Launcher_ShowMessageById( 0, IDS_ROOM_BADNAME );
		::InvalidateRect( m_hWnd, NULL, TRUE );
		::UpdateWindow( m_hWnd );
		return;
	}

	chatroom_t*	pRoom = m_pRoomListHead->FindByName( szName );
	if ( pRoom )
	{
		// The room already exists: select its row, then Join.
		m_pszPickedRoom = NULL;

		int	n = m_pRoomList->GetRowCount();
		for ( int i = 0; i < n; i++ )
		{
			if ( pRoom == m_pRoomList->GetItemData( i ) )
			{
				m_pRoomList->SelectItem( i, 1 );
				break;
			}
		}
		OnOK();
	}
	else
	{
		// A new room: build the create-room command and spawn the WON chat
		// server.
		char	szArgs[256];

		if ( !dlg.m_strRoomPassword.IsEmpty() )
			sprintf( szArgs, "%s -pass %s", szName, (LPCTSTR)dlg.m_strRoomPassword );
		else
			strcpy( szArgs, szName );

		if ( m_pSheet->LaunchChatServer( szArgs ) )
		{
			// Join the room the factory just spawned.
			if ( m_pSheet->m_pSelfIdentity )
				m_pSheet->JoinRoom( m_pSheet->m_pSelfIdentity );
		}
		CDialog::OnOK();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::RMLPreIdle (0x45D350)

int CRoomDialog::RMLPreIdle()
{
	if ( m_pSheet )
		m_pSheet->Pump();

	Launcher_SyncEngineWindow( this );

	if ( Eng_Frame( gBackground ) && !gBackground )
		return 1;

	if ( Launcher_AppOwnsForeground() )
	{
		ShowWindow( SW_SHOWNORMAL );
		::ShowWindow( mainwindow, SW_HIDE );
	}

	IN_HideMouse();
	::ClipCursor( NULL );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::MatchSavedRoom (0x45D3D0)
//
// Stamp a room with its index in rooms.lst, or -1 when it is not listed.

void CRoomDialog::MatchSavedRoom( const char* pSavedNames, int nCount, chatroom_t* pRoom )
{
	if ( !pRoom )
		return;

	const char*	pName   = pSavedNames;
	const char*	pszRoom = pRoom->m_szName;

	if ( pName && nCount > 0 )
	{
		int	i = 0;
		while ( 1 )
		{
			if ( !_strcmpi( pName, pszRoom ) )
			{
				pRoom->m_nGroup = i;				// exact match -> saved index
				return;
			}
			if ( !_strnicmp( pName, "Lobby", strlen( "Lobby" ) )
			  && !_strnicmp( pName, pszRoom, strlen( pName ) ) )
				break;								// "Lobby*" prefix match
			++i;
			pName += 64;
			if ( i >= nCount )
			{
				pRoom->m_nGroup = -1;				// exhausted -> no match
				return;
			}
		}
		pRoom->m_nGroup = i;
	}
	else
	{
		pRoom->m_nGroup = -1;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::PopulateRooms (0x45D480)

void CRoomDialog::PopulateRooms()
{
	// Re-seed off the sheet every time: the fetch that fed this call may have
	// replaced the list wholesale.
	CRoomList*	pHead = m_pSheet->m_pRoomList;

	m_pRoomListHead = pHead;
	if ( !pHead || pHead->m_pNext == pHead )
		return;

	m_pRoomList->ResetContent();

	// rooms.lst supplies the canonical ordering: MatchSavedRoom stamps each
	// room's m_nGroup, which is what AddRoomRow sorts on.  Rooms absent from it
	// are user-created (-1).
	char*	pSaved = NULL;
	int		nSaved = 0;

	Rooms_Load( &pSaved, &nSaved );		// frees the file itself; *pSaved is ours

	chatroom_t*	pFirstEmptyLobby = NULL;
	int			nLobbies       = 0;
	BOOL		bLobbyOccupied = FALSE;
	const int	nLobbyLen      = (int)strlen( "Lobby" );

	for ( chatroom_t* p = pHead->m_pNext; p != pHead; p = p->m_pNext )
	{
		p->m_bHidden = 0;
		if ( pSaved )
			MatchSavedRoom( pSaved, nSaved, p );

		// The two toggles filter on that index, not on a separate category
		// field.
		if ( !m_bShowPermanent && p->m_nGroup != -1 )
			p->m_bHidden = 1;
		if ( !m_bShowUserCreated && p->m_nGroup == -1 )
			p->m_bHidden = 1;

		// Lobbies are pooled: every empty one is hidden up front and one is put
		// back below only if no lobby has anybody in it.
		if ( p->m_nGroup != -1 && !_strnicmp( p->m_szName, "Lobby", nLobbyLen ) )
		{
			nLobbies++;
			if ( p->m_nPlayers )
			{
				bLobbyOccupied = TRUE;
			}
			else
			{
				if ( !pFirstEmptyLobby )
					pFirstEmptyLobby = p;
				p->m_bHidden = 1;
			}
		}
	}

	if ( pSaved )
		delete[] pSaved;

	// With no occupied lobby, show exactly one -- the first empty one, or a
	// random pick when they are all empty.
	if ( m_bShowPermanent && !bLobbyOccupied && nLobbies > 0 )
	{
		if ( pFirstEmptyLobby )
		{
			pFirstEmptyLobby->m_bHidden = 0;
		}
		else
		{
			int	iPick = rand() % nLobbies;
			int	i     = 0;

			for ( chatroom_t* p = pHead->m_pNext; p != pHead; p = p->m_pNext )
			{
				if ( _strnicmp( p->m_szName, "Lobby", nLobbyLen ) )
					continue;
				if ( i == iPick )
				{
					p->m_bHidden = 0;
					break;
				}
				i++;
			}
		}
	}

	for ( chatroom_t* p = pHead->m_pNext; p != pHead; p = p->m_pNext )
		if ( !p->m_bHidden )
			m_pRoomList->AddRoomRow( p );

	m_pRoomList->SelectItem( 0, 1 );		// the first row starts selected
	m_pRoomList->RefitScrollbar();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::OnRoomListSelect (0x45D6A0)
//
// A double click joins.

void CRoomDialog::OnRoomListSelect()
{
	OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::OnCancel (0x45D6B0)

void CRoomDialog::OnCancel()
{
	Launcher_WriteProfileInt( "Settings", "Show Permanent",    m_bShowPermanent   != 0 );
	Launcher_WriteProfileInt( "Settings", "Show User-Created", m_bShowUserCreated != 0 );
	CDialog::OnCancel();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::OnTogglePermanent (0x45D700)

void CRoomDialog::OnTogglePermanent()
{
	// Copy the box's own state into the filter flag rather than inverting it --
	// the control has already toggled itself by the time we run.
	m_bShowPermanent = m_lblPermanent.GetCheck();
	PopulateRooms();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::OnToggleUserCreated (0x45D720)

void CRoomDialog::OnToggleUserCreated()
{
	m_bShowUserCreated = m_lblUserCreated.GetCheck();
	PopulateRooms();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomListCtrl::OnSize (0x45D740)

void CRoomListCtrl::OnSize( UINT /*nType*/, int cx, int /*cy*/ )
{
	m_nWidth = cx;
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomListCtrl::OnPaint (0x45D750)
//
// The report view only fills as far as its last column, so widen the update
// region over the dead strip to its right and let the default paint cover the
// whole client width.

void CRoomListCtrl::OnPaint()
{
	if ( ( GetStyle() & LVS_TYPEMASK ) == LVS_REPORT )
	{
		RECT	rcItem;

		GetItemRect( 0, &rcItem, LVIR_BOUNDS );
		if ( rcItem.right < m_nWidth )
		{
			CPaintDC	dc( this );
			RECT		rc;

			dc.GetClipBox( &rc );
			if ( rcItem.right - 1 < rc.left )
				rc.left = rcItem.right - 1;
			rc.right = m_nWidth;
			InvalidateRect( &rc, FALSE );
		}
	}
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomListCtrl::OnDrawItem (0x45D810)

void CRoomListCtrl::OnDrawItem( int /*nIDCtl*/, LPDRAWITEMSTRUCT lpDrawItemStruct )
{
	DrawItem( lpDrawItemStruct );
}

/////////////////////////////////////////////////////////////////////////////
// CRoomListCtrl::OnSetTextColor (0x45D820)

LRESULT CRoomListCtrl::OnSetTextColor( WPARAM /*wParam*/, LPARAM lParam )
{
	m_clrText = (COLORREF)lParam;
	return Default();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomListCtrl::OnSetBkColor (0x45D830)
//
// LVM_SETBKCOLOR and LVM_SETTEXTBKCOLOR share one cached colour.

LRESULT CRoomListCtrl::OnSetBkColor( WPARAM /*wParam*/, LPARAM lParam )
{
	m_clrBk = (COLORREF)lParam;
	return Default();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomListCtrl::OnEraseBkgnd (0x45D840)

BOOL CRoomListCtrl::OnEraseBkgnd( CDC* pDC )
{
	RECT	rcClient;

	GetClientRect( &rcClient );

	CBrush	br( m_clrBk );
	pDC->FillRect( &rcClient, &br );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRoomStatic::CRoomStatic (0x45D8D0)

CRoomStatic::CRoomStatic()
{
	m_nBannerState = 0;
	m_banner.Empty();
	m_clrShadow = RGB( 100, 100, 100 );
}

/////////////////////////////////////////////////////////////////////////////
// CRoomStatic::OnEraseBkgnd (0x45D9C0)

BOOL CRoomStatic::OnEraseBkgnd( CDC* /*pDC*/ )
{
	OnPaint();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRoomStatic::OnPaint (0x45D9D0)
//
// Two draws, not one: the "Room:" caption in the shadow colour inside a fixed
// 125px column, then the value after it.

void CRoomStatic::OnPaint()
{
	CPaintDC	dc( this );
	RECT		rcClient;

	GetClientRect( &rcClient );

	CDC		mem;
	if ( !mem.CreateCompatibleDC( &dc ) )
		return;

	int			w = rcClient.right - rcClient.left;
	int			h = rcClient.bottom - rcClient.top;
	CBitmap		bmp;

	bmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(), w, h ) );

	CBitmap*	pOldBmp = mem.SelectObject( &bmp );

	if ( m_bTransparent )
	{
		RECT	rcDst, rcSrc;

		::GetWindowRect( m_hWnd, &rcDst );
		ScreenToClient( &rcDst );
		::GetWindowRect( m_hWnd, &rcSrc );
		if ( GetParent() )
			GetParent()->ScreenToClient( &rcSrc );
		Launcher_CopyParentBackground( &mem, &rcDst, &rcSrc );
	}
	else
	{
		CBrush	bg( m_clrBgnd );
		mem.FillRect( &rcClient, &bg );
	}

	CFont*	pOldFont = mem.SelectObject( &m_hStaticFont );
	mem.SetBkMode( TRANSPARENT );

	DRAWTEXTPARAMS	dtp;

	dtp.cbSize       = sizeof( dtp );
	dtp.iTabLength   = 4;
	dtp.iLeftMargin  = 0;
	dtp.iRightMargin = 0;

	RECT	rcText = rcClient;

	rcText.left += m_szOffsets.cx;
	rcText.top  += m_szOffsets.cy;

	int	xRight = rcText.right;

	// caption: fixed 125px column, shadow colour
	rcText.right = rcText.left + 125;
	mem.SetTextColor( m_clrShadow );
	mem.DrawTextEx( (LPSTR)Launcher_LoadString( IDS_MULTI_CHATROOMCAPTION ), -1,
		&rcText, DT_WORDBREAK | DT_VCENTER, &dtp );

	// value: everything after the caption column
	rcText.left += 125;
	rcText.right = xRight;
	mem.SetTextColor( m_clrText );
	mem.DrawTextEx( (LPSTR)(LPCSTR)strText, -1, &rcText,
		( m_bCenterText ? DT_CENTER : DT_LEFT ) | DT_WORDBREAK | DT_VCENTER, &dtp );

	dc.BitBlt( rcClient.left, rcClient.top, w, h, &mem, 0, 0, SRCCOPY );

	mem.SelectObject( pOldFont );
	mem.SelectObject( pOldBmp );
	mem.DeleteDC();
	ValidateRect( &rcClient );
}

/////////////////////////////////////////////////////////////////////////////
// CRoomStatic::SetText (0x45DD40)

void CRoomStatic::SetText( const char* psz )
{
	m_banner = psz ? psz : "";
	if ( GetSafeHwnd() )
		CODStatic::SetWindowText( m_banner );
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::OnActivateApp (0x406FE0)

void CRoomDialog::OnActivateApp( BOOL bActive, DWORD /*dwThreadID*/ )
{
	ActiveApp = bActive;
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::OnPaint (0x412860)

void CRoomDialog::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::OnEraseBkgnd (0x412870)

BOOL CRoomDialog::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog::OnDisplayChange (0x453D00)

LRESULT CRoomDialog::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}
