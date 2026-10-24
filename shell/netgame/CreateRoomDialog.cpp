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
// Purpose: CCreateRoomDialog, the chat-room creation dialog.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Entries at 0x4AD180, base map 0x4B4398 = CDialog.
BEGIN_MESSAGE_MAP( CCreateRoomDialog, CDialog )
	//{{AFX_MSG_MAP(CCreateRoomDialog)
	ON_MESSAGE( WM_DISPLAYCHANGE, OnDisplayChange )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateRoomDialog::CCreateRoomDialog (0x406960)

CCreateRoomDialog::CCreateRoomDialog( CWnd* pParent )
	: CDlgBase( IDD_CREATEROOM, pParent )
{
	m_pSelfWnd = this;		// gates the slide transition
	LoadHeaderBitmap( "head_createroom", NULL );
	SetupButtonStrips();
}

/////////////////////////////////////////////////////////////////////////////
// CCreateRoomDialog::~CCreateRoomDialog (0x406A70)

CCreateRoomDialog::~CCreateRoomDialog()
{
}

/////////////////////////////////////////////////////////////////////////////
// CCreateRoomDialog::SetupButtonStrips (0x406B40)
//
// Slice the two buttons out of the loaded header strip.  Both faces are freed
// first, so a re-entry after a skin change re-slices rather than reusing the
// blend from the previous strip.

void CCreateRoomDialog::SetupButtonStrips()
{
	int	wh[2];

	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( wh );
	m_headerW      = wh[0];
	m_headerH      = wh[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnOK.FreeSkinBitmaps();
		m_btnOK.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_OK, m_headerLoaded );
		m_btnCancel.FreeSkinBitmaps();
		m_btnCancel.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_BACK, m_headerLoaded );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCreateRoomDialog::DoDataExchange (0x406BD0)
//
// The two entry fields are runtime children, so their text is pulled straight
// off the inner edit -- both ways round, not only on save.

void CCreateRoomDialog::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_CREATEROOM_ROOMPASSWORD, m_lblRoomPassword );
	DDX_Control( pDX, IDC_CREATEROOM_ROOMNAME,     m_lblRoomName );
	DDX_Control( pDX, IDCANCEL,                    m_btnCancel );
	DDX_Control( pDX, IDOK,                        m_btnOK );

	if ( m_editRoomName.m_hWnd && m_editRoomName.m_pEdit )
		m_editRoomName.m_pEdit->GetWindowText( m_strRoomName );

	if ( m_editRoomPassword.m_hWnd && m_editRoomPassword.m_pEdit )
		m_editRoomPassword.m_pEdit->GetWindowText( m_strRoomPassword );
}

/////////////////////////////////////////////////////////////////////////////
// CCreateRoomDialog::OnInitDialog (0x406C80)
//
// Caption, field, caption, field, then the two buttons stacked underneath --
// every row is one strip cell tall and the column is two cells wide.

BOOL CCreateRoomDialog::OnInitDialog()
{
	int		wh[2];
	RECT	rc;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	Launcher_HeaderSize( wh );

	int	cyCell = wh[1];
	int	xRight = 2 * wh[0] + 50;

	m_lblRoomName.MoveWindow( 50, 140, xRight - 50, cyCell, TRUE );
	m_lblRoomName.SetTransparent( TRUE );
	m_lblRoomName.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblRoomName.SetFontSize( 14, FW_HEAVY );
	m_lblRoomName.SetWindowText( Launcher_LoadString( IDS_CREATEROOM_ROOMNAME ) );

	rc.left   = 50;
	rc.top    = 172;
	rc.right  = xRight;
	rc.bottom = cyCell + 172;
	m_editRoomName.Create( WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
		&rc, this, IDC_CREATEROOM_NAMEEDIT );
	m_editRoomName.SetBorderColor( RGB( 56, 56, 56 ) );
	m_editRoomName.MoveWindow( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );
	m_editRoomName.SetActive( 1 );

	m_lblRoomPassword.MoveWindow( 50, 204, xRight - 50, cyCell, TRUE );
	m_lblRoomPassword.SetTransparent( TRUE );
	m_lblRoomPassword.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblRoomPassword.SetFontSize( 14, FW_HEAVY );
	m_lblRoomPassword.SetWindowText( Launcher_LoadString( IDS_CREATEROOM_ROOMPASSWORD ) );

	rc.left   = 50;
	rc.top    = 236;
	rc.right  = xRight;
	rc.bottom = cyCell + 236;
	m_editRoomPassword.Create( WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
		&rc, this, IDC_CREATEROOM_PASSWORDEDIT );
	m_editRoomPassword.SetBorderColor( RGB( 56, 56, 56 ) );
	m_editRoomPassword.MoveWindow( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );
	m_editRoomPassword.SetActive( 1 );

	m_btnOK.MoveWindow( 50, 268, wh[0], cyCell, TRUE );
	m_btnOK.SetTransparent( TRUE );
	m_btnOK.SetTextColor( RGB( 240, 176, 24 ) );
	m_btnOK.SetHasArrow( 0 );
	m_btnOK.SetFontSize( 14, FW_HEAVY );
	m_btnOK.SetLeftAlign();
	m_btnOK.SetWindowText( Launcher_LoadString( IDS_BTN_OK ) );

	m_btnCancel.MoveWindow( 50, 300,
		Launcher_StringHeight( IDS_CREATEROOM_OFFSET, 0 ) + wh[0], cyCell, TRUE );
	m_btnCancel.SetTransparent( TRUE );
	m_btnCancel.SetTextColor( RGB( 240, 176, 24 ) );
	m_btnCancel.SetHasArrow( 0 );
	m_btnCancel.SetFontSize( 14, FW_HEAVY );
	m_btnCancel.SetLeftAlign();
	m_btnCancel.SetWindowText( Launcher_LoadString( IDS_BTN_CANCEL ) );

	m_editRoomName.SetFocus();
	return FALSE;	// focus set explicitly
}

/////////////////////////////////////////////////////////////////////////////
// CCreateRoomDialog::OnActivateApp (0x406FE0)

void CCreateRoomDialog::OnActivateApp( BOOL bActive, DWORD /*dwThreadID*/ )
{
	ActiveApp = bActive;
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CCreateRoomDialog::RMLPreIdle (0x407000)
//
// The modal loop's per-pass hook: run one engine frame, and while the launcher
// still owns the foreground keep this dialog up and the engine window down.

int CCreateRoomDialog::RMLPreIdle()
{
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
// CCreateRoomDialog::OnDisplayChange (0x410010)
//
// Re-centre on the new desktop.  Not Dlg_CenterWindow: this one centres the
// window's own size on the screen rather than the launcher's design size.

LRESULT CCreateRoomDialog::OnDisplayChange( WPARAM, LPARAM )
{
	RECT	rc;

	::GetWindowRect( m_hWnd, &rc );

	int	w = rc.right - rc.left;
	int	h = rc.bottom - rc.top;

#ifdef LAUNCHER_FIXES
	Dlg_CenterPopup( this, w, h );
#else
	MoveWindow( ( ::GetSystemMetrics( SM_CXSCREEN ) - w ) / 2,
				( ::GetSystemMetrics( SM_CYSCREEN ) - h ) / 2, w, h, TRUE );
#endif
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CCreateRoomDialog::OnPaint (0x412860)

void CCreateRoomDialog::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CCreateRoomDialog::OnEraseBkgnd (0x412870)

BOOL CCreateRoomDialog::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}
