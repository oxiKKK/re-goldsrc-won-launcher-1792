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
// Purpose: CInputDlg, the skinned modal text-entry dialog (IDD 200).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Control ids for the IDD 200 template (launcher.rc).
#define IDC_INPUT_EDIT		103

BEGIN_MESSAGE_MAP( CInputEdit, CBorderLessEdit )
	//{{AFX_MSG_MAP(CInputEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP( CInputDlg, CDialog )
	ON_MESSAGE( WM_DISPLAYCHANGE, &CInputDlg::OnDisplayChange )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInputEdit::CInputEdit (0x41B250)

CInputEdit::CInputEdit( CWnd* pOwner )
	: CBorderLessEdit()
{
	m_pOwnerDlg = pOwner;
}

/////////////////////////////////////////////////////////////////////////////
// CInputEdit::PreTranslateMessage (0x41B2A0)
//
// ENTER fires the owner dialog's OK button rather than reaching the dialog
// manager.

BOOL CInputEdit::PreTranslateMessage( MSG* pMsg )
{
	CWnd*		pItem;
	CButton*	pBtn;

	if ( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && m_pOwnerDlg )
	{
		pItem = m_pOwnerDlg->GetDlgItem( IDOK );
		pBtn  = (CButton*)AfxDynamicDownCast( RUNTIME_CLASS( CButton ), pItem );
		if ( pBtn )
		{
			::SendMessageA( m_pOwnerDlg->GetSafeHwnd(), WM_COMMAND, IDOK,
				(LPARAM)pBtn->GetSafeHwnd() );
			return TRUE;
		}
	}

	return CWnd::PreTranslateMessage( pMsg );
}

/////////////////////////////////////////////////////////////////////////////
// CInputEdit::~CInputEdit (0x41B270)

CInputEdit::~CInputEdit()
{
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::CInputDlg (0x41B320)

CInputDlg::CInputDlg( CWnd* pParent )
	: CDlgPopupBase( IDD, pParent )
{
	// 0x41B3AB -- the popup binds the base painter to itself.  Without it
	// CDlgPopupBase::OnPaint no-ops, so WM_PAINT is never validated (nothing
	// calls BeginPaint) and the dialog strobes the default white face.
	SetPaintWnd( this );
	m_bPasswordMode = 0;
	InitMembers();
	m_pInput        = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::~CInputDlg (0x41B410)

CInputDlg::~CInputDlg()
{
	if ( m_pInput )
		delete m_pInput;
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::DoDataExchange (0x41B540)

void CInputDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDOK,             m_btnOK );
	DDX_Control( pDX, IDCANCEL,         m_btnCancel );
	DDX_Control( pDX, IDC_INPUT_STATIC, m_promptLabel );	// 1149

	// CDialog::OnOK runs UpdateData( TRUE ) first, so this is where the entered
	// text reaches m_strInput -- the class overrides no OnOK.
	if ( m_pInput && m_pInput->GetSafeHwnd() && m_pInput->m_pEdit )
		m_pInput->m_pEdit->GetWindowText( m_strInput );
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::InitMembers (0x41B4B0)

void CInputDlg::InitMembers()
{
	int	wh[2];

	if ( Launcher_HeaderLoaded() )
	{
		Launcher_HeaderSize( wh );
		Launcher_HeaderStride();
		m_btnOK.SetDIBData( CSize( wh[0], wh[1] ), BTNSTRIP_OK, Launcher_HeaderLoaded() );
		m_btnCancel.SetDIBData( CSize( wh[0], wh[1] ), BTNSTRIP_BACK, Launcher_HeaderLoaded() );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::OnPaint (0x41B5B0)

void CInputDlg::OnPaint()
{
	CDlgPopupBase::OnPaint();

	if ( m_promptLabel.GetSafeHwnd() )
		m_promptLabel.SetWindowText( m_strPrompt );
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::OnEraseBkgnd (0x41B5E0)

BOOL CInputDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	CDlgPopupBase::OnPaint();

	if ( m_promptLabel.GetSafeHwnd() )
		m_promptLabel.SetWindowText( m_strPrompt );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::OnDisplayChange (0x41B610)

LRESULT CInputDlg::OnDisplayChange( WPARAM, LPARAM )
{
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::OnInitDialog (0x41B620)

BOOL CInputDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

#ifdef LAUNCHER_FIXES
	Dlg_CenterPopup( this, 400, 190 );
#else
	// Centre on the primary display (binary uses raw screen metrics: 400x190).
	int	x = ( GetSystemMetrics( SM_CXSCREEN ) - 400 ) / 2;
	int	y = ( GetSystemMetrics( SM_CYSCREEN ) - 190 ) / 2;
	MoveWindow( x, y, 400, 190, TRUE );
#endif

	// Entry field: a stock CEdit has no black face or 3px frame.
	if ( !m_pInput )
		m_pInput = new CInputEdit( this );
	if ( m_bPasswordMode )
		m_pInput->SetPasswordMode();
	if ( !m_pInput->GetSafeHwnd() )
	{
		CRect	rcInit( 0, 0, 100, 100 );
		m_pInput->Create( 0, &rcInit, this, IDC_INPUT_EDIT );
	}

	// Button cell size comes from the header strip geometry (w-50 x h).
	int	wh[2] = { 100, 25 };
	Launcher_HeaderSize( wh );
	int	btnW = wh[0] - 50;
	int	btnH = wh[1];
	if ( btnW <= 0 ) btnW = 100;
	if ( btnH <= 0 ) btnH = 25;

	m_promptLabel.MoveWindow( 10, 10, 370, 118, TRUE );
	m_pInput->MoveWindow( 10, 128, 370, 25, TRUE );
	m_btnOK.MoveWindow( 2 * ( 190 - btnW ), 158, btnW, btnH, TRUE );
	m_btnCancel.MoveWindow( btnW + 2 * ( 190 - btnW ), 158, btnW, btnH, TRUE );

	m_pInput->ShowWindow( SW_RESTORE );
	m_pInput->SetActive( 1 );

	// Prompt label: larger font, opaque INPUT_BG_COLOR fill, scheme text colour, then
	// the prompt itself (0x41B7AF..0x41B7E2).  SetTransparent(0) is what makes the top
	// of the dialog a flat panel -- left transparent, the label's 370x118 area shows
	// the page skin through it.  And the text has to go in via SetWindowText: that is
	// what CODStatic::OnPaint draws, not the window caption.
	m_promptLabel.SetFontSize( 18, FW_HEAVY );
	m_promptLabel.SetTransparent( 0 );
	m_promptLabel.SetTextColor( Scheme_GetColor( "INPUT_TEXT_COLOR" ) );
	m_promptLabel.m_clrBgnd = Scheme_GetColor( "INPUT_BG_COLOR" );
	m_promptLabel.SetWindowText( (LPCTSTR)m_strPrompt );

	// Both buttons fill the panel themselves; left transparent they blend the page
	// skin behind the dialog into their faces.
	CODBlendBtn*	pButtons[2] = { &m_btnCancel, &m_btnOK };
	for ( int i = 0; i < 2; i++ )
	{
		pButtons[i]->SetFontSize( 12, FW_HEAVY );
		pButtons[i]->SetTransparent( 0 );
		pButtons[i]->m_clrDown   = RGB( 240, 180, 24 );		// 0x18B4F0
		pButtons[i]->m_clrBg     = Scheme_GetColor( "INPUT_BG_COLOR" );
		pButtons[i]->m_bHasArrow = 0;
	}

	// Button captions (string ids from the binary: 0xFB = Cancel, 0x118 = OK).
	SetWindowTextSafe( &m_btnCancel, Launcher_LoadString( IDS_BTN_CANCEL ) );
	SetWindowTextSafe( &m_btnOK,     Launcher_LoadString( IDS_BTN_OK ) );

	m_pInput->SetFocus();
	return FALSE;	// focus set explicitly (binary returns FALSE)
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::SetPrompt (0x41B8C0)

void CInputDlg::SetPrompt( const char* psz )
{
	m_strPrompt = psz ? psz : "";
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::RMLPreIdle (0x41B8D0)

int CInputDlg::RMLPreIdle()
{
	Eng_Frame( gBackground );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::SetPasswordMode (0x41B8F0)

void CInputDlg::SetPasswordMode( int bOn )
{
	m_bPasswordMode = bOn;		// +688
}

/////////////////////////////////////////////////////////////////////////////
// CInputDlg::OnActivateApp (0x406FE0)

void CInputDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}
