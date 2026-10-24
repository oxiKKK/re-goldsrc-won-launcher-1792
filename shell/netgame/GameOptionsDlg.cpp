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
// Purpose: the multiplayer / advanced game-options page (CGameOptionsDlg, IDD
//          175 = 0xAF, "head_advanced").
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"


// The input/mouse options edit named fields of the player config block.  (AUTOAIM
// maps to the sv_aim cvar field; the rest are 1:1.)
static CGameClientConfig* GameOpt_Config()
{
	return &g_pServerBrowser->m_playerConfig;
}

BEGIN_MESSAGE_MAP( CGameOptionsDlg, CDialog )
	ON_MESSAGE( WM_DISPLAYCHANGE, &CGameOptionsDlg::OnDisplayChange )
	ON_REGISTERED_MESSAGE( g_uiScrollMsg, OnSliderScroll )	// HL_WM_SCROLL (slider)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED( 1064, OnCrosshair )
	ON_BN_CLICKED( 1065, OnReverse )
	ON_BN_CLICKED( 1035, OnJoystick )
	ON_BN_CLICKED( 1061, OnMouseLook )
	ON_BN_CLICKED( 1027, OnLookSpring )
	ON_BN_CLICKED( 1028, OnLookStrafe )
	ON_BN_CLICKED( 1062, OnMouseFilter )
	ON_WM_ACTIVATEAPP()
	ON_BN_CLICKED( 1063, OnAutoaim )
	ON_BN_CLICKED( 1066, OnJoystickLook )
	ON_BN_CLICKED( 1067, OnConsole )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::CGameOptionsDlg (0x411400)

CGameOptionsDlg::CGameOptionsDlg( CWnd* pParent )
	: CDlgBase( IDD_OPTS, pParent )
{
	int	dims[2];

	m_pSelfWnd = this;		// +204 -- gates the slide transition
	LoadHeaderBitmap( "head_advanced", 0 );

	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
		m_btnDone.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DONE, m_headerLoaded );
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::~CGameOptionsDlg (0x411640)

CGameOptionsDlg::~CGameOptionsDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::DoDataExchange (0x4117F0)

void CGameOptionsDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_OPTS_CONSOLEHELP, m_lblConsole );
	DDX_Control( pDX, IDC_OPTS_JLOOKHELP, m_lblJLook );
	DDX_Control( pDX, IDC_OPTS_SENSITIVITYHELP, m_lblSensHelp );
	DDX_Control( pDX, IDC_OPTS_REVERSEHELP, m_lblReverse );
	DDX_Control( pDX, IDC_OPTS_MLOOKHELP, m_lblMLook );
	DDX_Control( pDX, IDC_OPTS_MFILTERHELP, m_lblMFilter );
	DDX_Control( pDX, IDC_OPTS_LOOKSTRAFEHELP, m_lblLookStrafe );
	DDX_Control( pDX, IDC_OPTS_LOOKSPRINGHELP, m_lblLookSpring );
	DDX_Control( pDX, IDC_OPTS_CROSSHAIRHELP, m_lblCrosshair );
	DDX_Control( pDX, IDC_OPTS_JOYSTICKHELP, m_lblJoystick );
	DDX_Control( pDX, IDC_OPTS_AUTOAIMHELP, m_lblAutoaim );

	DDX_Control( pDX, IDC_OPTS_JLOOK, m_checkJLook );
	DDX_Control( pDX, IDC_OPTS_JOYSTICK, m_checkJoystick );
	DDX_Control( pDX, IDC_OPTS_REVERSE, m_checkReverse );
	DDX_Control( pDX, IDC_OPTS_MLOOK, m_checkMLook );
	DDX_Control( pDX, IDC_OPTS_MFILTER, m_checkMFilter );
	DDX_Control( pDX, IDC_OPTS_LOOKSTRAFE, m_checkLookStrafe );
	DDX_Control( pDX, IDC_OPTS_LOOKSPRING, m_checkLookSpring );
	DDX_Control( pDX, IDC_OPTS_CROSSHAIR, m_checkCrosshair );
	DDX_Control( pDX, IDC_OPTS_AUTOAIM, m_checkAutoaim );
	DDX_Control( pDX, IDC_OPTS_CONSOLE, m_checkConsole );
	DDX_Control( pDX, IDOK, m_btnDone );
}

// A check box + its help label.
static void GameOpt_Row( CODBlendCheckBox* check, int bChecked,
	CODStatic* label, unsigned int helpId )
{
	check->ModifyStyle( 0, BS_OWNERDRAW );
	check->m_bChecked = bChecked;
	check->InvalidateRect( NULL, TRUE );

	label->SetTransparent( 1 );
	label->SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	label->SetFontSize( 11, FW_NORMAL );
	label->SetWindowText( Launcher_LoadString( helpId ) );
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnInitDialog (0x4119A0)

BOOL CGameOptionsDlg::OnInitDialog()
{
	int	dims[2];
	int	w, h, ctlX, rowW, y;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	// Owner-draw all ten check boxes + caption them.
	m_checkConsole.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkAutoaim.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkReverse.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkMLook.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkMFilter.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkLookStrafe.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkLookSpring.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkCrosshair.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkJoystick.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkJLook.ModifyStyle( 0, BS_OWNERDRAW );

	SetWindowTextSafe( &m_checkJoystick,   Launcher_LoadString( IDS_OPTS_JOYSTICK ) );	// 0xB0
	SetWindowTextSafe( &m_checkCrosshair,  Launcher_LoadString( IDS_OPTS_CROSSHAIR ) );	// 0xAA
	SetWindowTextSafe( &m_checkReverse,    Launcher_LoadString( IDS_OPTS_REVERSE ) );	// 0xAB
	SetWindowTextSafe( &m_checkMLook,      Launcher_LoadString( IDS_OPTS_MLOOK ) );		// 0xAC
	SetWindowTextSafe( &m_checkMFilter,    Launcher_LoadString( IDS_OPTS_MFILTER ) );	// 0xAF
	SetWindowTextSafe( &m_checkLookStrafe, Launcher_LoadString( IDS_OPTS_LOOKSTRAFE ) );	// 0xAE
	SetWindowTextSafe( &m_checkLookSpring, Launcher_LoadString( IDS_OPTS_LOOKSPRING ) );	// 0xAD
	SetWindowTextSafe( &m_checkAutoaim,    Launcher_LoadString( IDS_OPTS_AUTOAIM ) );	// 0x18D
	SetWindowTextSafe( &m_checkConsole,    Launcher_LoadString( IDS_OPTS_CONSOLE ) );	// 0x231
	SetWindowTextSafe( &m_checkJLook,      Launcher_LoadString( IDS_OPTS_JLOOK ) );		// 0x1F1

	// Defaults if no config is present (sensitivity 3.0).
	m_bCrosshair = TRUE;  m_bReverseMouse = FALSE; m_bJoystick = FALSE;
	m_bMouseLook = TRUE;  m_bLookSpring = TRUE;     m_bLookStrafe = FALSE;
	m_bMouseFilter = FALSE; m_bAutoaim = TRUE;      m_bConsole = FALSE;
	m_bJoystickLook = TRUE; m_sensitivity = 3.0f;

	if ( !g_pServerBrowser )
	{
		Launcher_ShowMessageById( 0, IDS_AUDIO_NOPROFILE );	// 0x48
		OnCancel();
		return TRUE;
	}

	// Read the current values out of the player config.
	CGameClientConfig*	cfg = GameOpt_Config();
	m_sensitivity   = cfg->sensitivity;	// == g_pServerBrowser float[2638]
	m_bCrosshair    = ( cfg->crosshair    != 0.0f );
	m_bReverseMouse = ( cfg->m_pitch         < 0.0f );
	m_bJoystick     = ( cfg->joystick     != 0.0f );
	m_bMouseLook    = ( cfg->mlook    != 0.0f );
	m_bLookSpring   = ( cfg->lookspring   != 0.0f );
	m_bLookStrafe   = ( cfg->lookstrafe   != 0.0f );
	m_bMouseFilter  = ( cfg->m_filter  != 0.0f );
	m_bAutoaim      = ( cfg->sv_aim      != 0.0f );
	m_bConsole      = ( cfg->console      != 0.0f );
	m_bJoystickLook = ( cfg->jlook != 0.0f );

	// Reflect the flags into the owner-draw check boxes + help labels.
	GameOpt_Row( &m_checkReverse,    m_bReverseMouse, &m_lblReverse,    IDS_OPTS_REVERSEHELP );	// 0xB2
	GameOpt_Row( &m_checkMLook,      m_bMouseLook,    &m_lblMLook,      IDS_OPTS_MLOOKHELP );		// 0xB3
	GameOpt_Row( &m_checkLookSpring, m_bLookSpring,   &m_lblLookSpring, IDS_OPTS_LOOKSPRINGHELP );	// 0xB4
	GameOpt_Row( &m_checkLookStrafe, m_bLookStrafe,   &m_lblLookStrafe, IDS_OPTS_LOOKSTRAFEHELP );	// 0xB5
	GameOpt_Row( &m_checkMFilter,    m_bMouseFilter,  &m_lblMFilter,    IDS_OPTS_MFILTERHELP );	// 0xB6
	GameOpt_Row( &m_checkJoystick,   m_bJoystick,     &m_lblJoystick,   IDS_OPTS_JOYSTICKHELP );	// 0xB7
	GameOpt_Row( &m_checkJLook,      m_bJoystickLook, &m_lblJLook,      IDS_OPTS_JLOOKHELP );		// 0x1F0
	GameOpt_Row( &m_checkAutoaim,    m_bAutoaim,      &m_lblAutoaim,    IDS_OPTS_AUTOAIMHELP );	// 0x18E
	GameOpt_Row( &m_checkCrosshair,  m_bCrosshair,    &m_lblCrosshair,  IDS_OPTS_CROSSHAIRHELP );	// 0xB1

	// Lay the rows out: each check box on the left, its help label on the right.
	Launcher_HeaderSize( dims );
	w = dims[0];
	h = dims[1];
	ctlX = w + 60;
	rowW = ( g_nLauncherDefW - 10 ) - ctlX;

	// The first four rows sit on a plain 30px grid; each help label starts 6px
	// into its row and is two pixels taller than the row, so a description that
	// wraps still has somewhere to put its second line.
	struct { CODBlendCheckBox* check; CODStatic* label; } rows[] =
	{
		{ &m_checkCrosshair,  &m_lblCrosshair  },
		{ &m_checkReverse,    &m_lblReverse    },
		{ &m_checkMLook,      &m_lblMLook      },
		{ &m_checkLookSpring, &m_lblLookSpring },
	};
	y = 140;
	for ( int i = 0; i < (int)ARRAYSIZE( rows ); i++ )
	{
		rows[i].check->MoveWindow( 50, y, w, h, TRUE );
		rows[i].label->MoveWindow( ctlX, y + 6, rowW, h + 2, TRUE );
		y += 30;
	}

	// From here the layout comes out of the dialog's offset string rather than
	// the grid: IDS_GAMEOPTIONS_OFFSET carries "<base> <strafeY> <strafeH> <gap>",
	// and IDS_GERMAN the extra line height a localised description needs.
	int	nBaseY   = Launcher_StringHeight( IDS_GAMEOPTIONS_OFFSET, 0 ) + 260;
	int	nStrafeY = Launcher_StringHeight( IDS_GAMEOPTIONS_OFFSET, 1 );
	int	nStrafeH = Launcher_StringHeight( IDS_GAMEOPTIONS_OFFSET, 2 );
	int	nGap     = Launcher_StringHeight( IDS_GAMEOPTIONS_OFFSET, 3 );
	int	nWrap    = Launcher_StringHeight( IDS_GERMAN, 0 );

	// Look strafe: its own indent, and a description tall enough to wrap.
	m_checkLookStrafe.MoveWindow( 50, nBaseY + nStrafeY,
		( w + 5 * ( nWrap + 10 ) ) - 50, h - nStrafeY, TRUE );
	m_lblLookStrafe.MoveWindow( ctlX, nBaseY + 6, rowW, nStrafeH + h + 2, TRUE );

	y = nBaseY + nGap + 30;

	struct { CODBlendCheckBox* check; CODStatic* label; } rows2[] =
	{
		{ &m_checkMFilter,  &m_lblMFilter  },
		{ &m_checkJoystick, &m_lblJoystick },
		{ &m_checkJLook,    &m_lblJLook    },
	};
	for ( int i = 0; i < (int)ARRAYSIZE( rows2 ); i++ )
	{
		rows2[i].check->MoveWindow( 50, y, w, h, TRUE );
		rows2[i].label->MoveWindow( ctlX, y + 6, rowW, h + 2, TRUE );
		y += 30;
	}

	// Auto-aim closes the list; its description is the one allowed to run long.
	m_checkAutoaim.MoveWindow( 50, y, w, h, TRUE );
	m_lblAutoaim.MoveWindow( ctlX, y + 6, rowW, h + 8 * nWrap - 6, TRUE );

	// The console toggle is built by the template but never shown.
	m_checkConsole.ShowWindow( SW_HIDE );
	m_lblConsole.ShowWindow( SW_HIDE );

	y += 30;

	// The sensitivity slider sits below the rows; its help label uses the
	// IDC 1096 ("sensitivity") slot.
	CRect	rc( 50, y, 50 + w, y + h );
	m_pSensSlider = new CODSlider;
	m_pSensSlider->Create( this, &rc );
	m_pSensSlider->SetRange( 1, 200 );
	m_pSensSlider->SetPos( (int)( m_sensitivity * 10.0f ) );

	m_lblSensHelp.MoveWindow( ctlX, y + 6, rowW, h + 2, TRUE );
	m_lblSensHelp.SetTransparent( 1 );
	m_lblSensHelp.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblSensHelp.SetFontSize( 11, FW_NORMAL );
	m_lblSensHelp.SetWindowText( Launcher_LoadString( IDS_OPTS_SENSITIVITYHELP ) );	// 0xB8

	// The Done button caption + the slider's enabled state follow mouse-look.
	m_btnDone.MoveWindow( 50, y + 30, w, h, TRUE );
	SetWindowTextSafe( &m_btnDone, Launcher_LoadString( IDS_BTN_DONE ) );	// 0xF7
	HighlightLookOptions( m_bMouseLook );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::SliderScrolled (0x412930)

void CGameOptionsDlg::SliderScrolled( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	CWnd*	pWnd = (CWnd*)AfxDynamicDownCast( RUNTIME_CLASS( CWnd ), pScrollBar );

	if ( pWnd && m_pSensSlider
	  && pWnd->GetSafeHwnd() == m_pSensSlider->GetSafeHwnd() )
	{
		m_sensitivity = (float)nPos * 0.1f;
		return;
	}

	CWnd::OnVScroll( nSBCode, nPos, pScrollBar );
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnConsole (0x412880)

void CGameOptionsDlg::OnConsole()
{
	m_bConsole = m_checkConsole.m_bChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnAutoaim (0x412890)

void CGameOptionsDlg::OnAutoaim()
{
	m_bAutoaim = m_checkAutoaim.m_bChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnCrosshair (0x4128A0)

void CGameOptionsDlg::OnCrosshair()
{
	m_bCrosshair = m_checkCrosshair.m_bChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnReverse (0x4128B0)

void CGameOptionsDlg::OnReverse()
{
	m_bReverseMouse = m_checkReverse.m_bChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnJoystick (0x4128C0)

void CGameOptionsDlg::OnJoystick()
{
	m_bJoystick = m_checkJoystick.m_bChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnMouseLook (0x4128D0)

void CGameOptionsDlg::OnMouseLook()
{
	m_bMouseLook = m_checkMLook.m_bChecked;
	HighlightLookOptions( m_bMouseLook );
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnLookSpring (0x4128F0)

void CGameOptionsDlg::OnLookSpring()
{
	m_bLookSpring = m_checkLookSpring.m_bChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnLookStrafe (0x412900)

void CGameOptionsDlg::OnLookStrafe()
{
	m_bLookStrafe = m_checkLookStrafe.m_bChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnMouseFilter (0x412910)

void CGameOptionsDlg::OnMouseFilter()
{
	m_bMouseFilter = m_checkMFilter.m_bChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnJoystickLook (0x412920)

void CGameOptionsDlg::OnJoystickLook()
{
	m_bJoystickLook = m_checkJLook.m_bChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnOK (0x4129B0)

void CGameOptionsDlg::OnOK()
{
	CGameClientConfig*	cfg = GameOpt_Config();

	// Reverse mouse: keep the pitch's sign in step with the check box.
	float	pitch = cfg->m_pitch;
	if ( ( m_bReverseMouse && pitch > 0.0f ) || ( !m_bReverseMouse && pitch < 0.0f ) )
		cfg->m_pitch = -cfg->m_pitch;

	cfg->lookstrafe  = m_bLookStrafe  ? 1.0f : 0.0f;
	cfg->lookspring  = m_bLookSpring  ? 1.0f : 0.0f;
	cfg->crosshair   = m_bCrosshair   ? 1.0f : 0.0f;
	cfg->joystick    = m_bJoystick    ? 1.0f : 0.0f;
	cfg->m_filter = m_bMouseFilter ? 1.0f : 0.0f;
	cfg->mlook   = m_bMouseLook   ? 1.0f : 0.0f;
	cfg->sensitivity = m_sensitivity;
	cfg->sv_aim     = m_bAutoaim     ? 1.0f : 0.0f;
	cfg->jlook= m_bJoystickLook? 1.0f : 0.0f;

	Launcher_SavePlayerInfoTo( "Player", cfg );

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnPaint (0x412860)

void CGameOptionsDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnEraseBkgnd (0x412870)

BOOL CGameOptionsDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnActivateApp (0x406FE0)

void CGameOptionsDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnDisplayChange (0x453D00)

LRESULT CGameOptionsDlg::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::HighlightLookOptions (0x412B10)

void CGameOptionsDlg::HighlightLookOptions( BOOL bOn )
{
	if ( bOn )
	{
		m_checkLookSpring.SetHighlight( TRUE );
		m_checkLookStrafe.SetHighlight( TRUE );
	}
	else
	{
		m_checkLookSpring.SetHighlight( FALSE );
		m_checkLookStrafe.SetHighlight( FALSE );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGameOptionsDlg::OnSliderScroll (0x412B60)

LRESULT CGameOptionsDlg::OnSliderScroll( WPARAM wParam, LPARAM lParam )
{
	SliderScrolled( LOWORD( wParam ), HIWORD( wParam ), (CScrollBar*)lParam );
	return 1;
}
