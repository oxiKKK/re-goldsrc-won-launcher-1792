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
// Purpose: the Audio options page (CAudioDlg, IDD 162, "head_audio").
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

UINT	g_uiScrollMsg;		// RegisterWindowMessage( "HL_WM_SCROLL" )


BEGIN_MESSAGE_MAP( CAudioDlg, CDialog )
	//{{AFX_MSG_MAP(CAudioDlg)
	ON_MESSAGE( WM_DISPLAYCHANGE, &CAudioDlg::OnDisplayChange )
	ON_REGISTERED_MESSAGE( g_uiScrollMsg, OnSliderScroll )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_COMMAND( IDC_AUDIO_HIGHQUALITY, OnHighQuality )
	ON_COMMAND( IDC_AUDIO_USECD, OnUseCD )
	ON_WM_ACTIVATEAPP()
	ON_COMMAND( IDC_AUDIO_A3D, OnA3D )
	ON_COMMAND( IDC_AUDIO_EAX, OnEAX )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::CAudioDlg (0x4020A0)

CAudioDlg::CAudioDlg( CWnd* pParent )
	: CDlgBase( IDD_AUDIO, pParent )
{
	int	dims[2];

	m_pSelfWnd = this;		// gates the slide transition
	LoadHeaderBitmap( "head_audio", NULL );
	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	m_savedVidRestart = force_mode_set;

	if ( m_headerLoaded )
		m_btnDone.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DONE, m_headerLoaded );
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::~CAudioDlg (0x402200)

CAudioDlg::~CAudioDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::DoDataExchange (0x4022D0)

void CAudioDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_AUDIO_EAX, m_checkEAX );
	DDX_Control( pDX, IDC_AUDIO_CDHINT, m_lblCDHint );
	DDX_Control( pDX, IDC_AUDIO_A3D, m_checkA3D );
	DDX_Control( pDX, IDC_AUDIO_VOLUME, m_lblVolume );
	DDX_Control( pDX, IDC_AUDIO_SUITVOL, m_lblSuitVol );
	DDX_Control( pDX, IDC_AUDIO_USECD, m_checkCDMusic );
	DDX_Control( pDX, IDC_AUDIO_HIGHQUALITY, m_checkHighQuality );
	DDX_Control( pDX, IDOK, m_btnDone );
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnInitDialog (0x402380)

BOOL CAudioDlg::OnInitDialog()
{
	RECT	rc = { 0, 0, 100, 100 };
	RECT	hint;
	int		nWrap;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	m_lblVolume.SetFontSize( 14, FW_HEAVY );
	m_lblVolume.SetWindowText( Launcher_LoadString( IDS_AUDIO_VOLUME ) );
	m_lblSuitVol.SetFontSize( 14, FW_HEAVY );
	m_lblSuitVol.SetWindowText( Launcher_LoadString( IDS_AUDIO_SUITVOL ) );

	m_checkHighQuality.SetFontSize( 12, FW_HEAVY );
	SetWindowTextSafe( &m_checkHighQuality, Launcher_LoadString( IDS_AUDIO_HIGHQUALITY ) );
	m_checkCDMusic.SetFontSize( 12, FW_HEAVY );
	SetWindowTextSafe( &m_checkCDMusic, Launcher_LoadString( IDS_AUDIO_USECD ) );
	m_checkA3D.SetFontSize( 12, FW_HEAVY );
	SetWindowTextSafe( &m_checkA3D, Launcher_LoadString( IDS_AUDIO_A3D ) );
	m_checkEAX.SetFontSize( 12, FW_HEAVY );
	SetWindowTextSafe( &m_checkEAX, Launcher_LoadString( IDS_AUDIO_EAX ) );

	// the two sliders are built by hand (no template controls)
	m_pVolumeSlider = new CODSlider;
	m_pVolumeSlider->Create( this, &rc );
	m_pSuitVolSlider = new CODSlider;
	m_pSuitVolSlider->Create( this, &rc );

	if ( !g_pServerBrowser )
	{
		// no player config to edit
		Launcher_ShowMessageById( 0, IDS_AUDIO_NOPROFILE );
		OnCancel();
		return 1;
	}

	CGameClientConfig*	cfg = &g_pServerBrowser->m_playerConfig;
	m_volume = cfg->volume * 100.0f;
	m_suitVolume = cfg->suitvolume * 100.0f;
	m_bHighQuality = ( cfg->hisound != 0.0f );
	m_bCDMusic = ( cfg->bgmvolume != 0.0f );
	m_bA3D = (int)cfg->s_a3d;
	m_bEAX = (int)cfg->s_eax;

	m_pVolumeSlider->SetRange( 0, 100 );
	m_pVolumeSlider->SetPos( (int)m_volume );
	m_pSuitVolSlider->SetRange( 0, 100 );
	m_pSuitVolSlider->SetPos( (int)m_suitVolume );

	m_checkHighQuality.m_bChecked = m_bHighQuality;
	m_checkHighQuality.InvalidateRect( NULL, TRUE );
	m_checkCDMusic.m_bChecked = m_bCDMusic;
	m_checkCDMusic.InvalidateRect( NULL, TRUE );
	m_checkA3D.m_bChecked = m_bA3D;
	m_checkA3D.InvalidateRect( NULL, TRUE );
	m_checkEAX.m_bChecked = m_bEAX;
	m_checkEAX.InvalidateRect( NULL, TRUE );

	// the check boxes paint owner-draw
	m_checkHighQuality.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkCDMusic.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkA3D.ModifyStyle( 0, BS_OWNERDRAW );
	m_checkEAX.ModifyStyle( 0, BS_OWNERDRAW );

	// lay the page out off the string-metric table; German builds
	// push the block right and down
	int		x = Launcher_StringHeight( IDS_AUDIO_OFFSET, 0 ) + 50;
	int		w = m_headerW;
	int		h = m_headerH;
	int		bWide = Launcher_StringHeight( IDS_SPANISH, 0 );
	int		xOfs = Launcher_StringHeight( IDS_AUDIO_OFFSET, 1 );
	int		yTop = bWide ? 400 : 140;

	if ( bWide )
		m_btnDone.MoveWindow( x, yTop, w, h, TRUE );
	else
		m_btnDone.MoveWindow( x + xOfs, yTop, w - xOfs, h, TRUE );
	SetWindowTextSafe( &m_btnDone, Launcher_LoadString( IDS_BTN_DONE ) );

	// Only the Done button sits at the page's left margin; the labels, sliders
	// and check boxes are all indented past it by the header width.
	int	hintLeft;
	int	hintRight;
	if ( bWide )
	{
		hintLeft  = x + w - 40;
		hintRight = x + w + 150;
	}
	else
	{
		x += w;
		hintLeft  = x;
		hintRight = x + xOfs + 190;
		m_lblVolume.MoveWindow( x, 140, xOfs + 190, h, TRUE );
	}

	// the CD hint sits under the CD check box, HELP_COLORed
	nWrap = Launcher_StringHeight( IDS_GERMAN, 0 );
	hint.left   = hintLeft;
	hint.top    = 140;
	hint.right  = hintRight;
	hint.bottom = h + 140;

	::OffsetRect( &hint, Launcher_StringHeight( IDS_AUDIO_OFFSET, 2 ) + 190, 0 );
	hint.bottom += 15;
	hint.bottom += h * nWrap;
	hint.top    += h * nWrap;
	hint.left   += Launcher_StringHeight( IDS_AUDIO_OFFSET, 3 );
	hint.right  += 33 * nWrap;

	m_lblCDHint.MoveWindow( hint.left, hint.top,
		hint.right - hint.left, hint.bottom - hint.top, TRUE );
	m_lblCDHint.SetTransparent( TRUE );
	m_lblCDHint.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblCDHint.SetFontSize( 11, FW_NORMAL );
	m_lblCDHint.SetWindowText( Launcher_LoadString( IDS_AUDIO_CDHINT ) );

	if ( bWide )
		m_lblVolume.MoveWindow( x, 140, 300, h, TRUE );
	m_pVolumeSlider->MoveWindow( x, 159, w, h, TRUE );
	m_lblSuitVol.MoveWindow( x, 191, 150 * bWide + 200, h, TRUE );
	m_pSuitVolSlider->MoveWindow( x, 210, w, h, TRUE );

	m_checkCDMusic.MoveWindow( x, 242, 2 * w, h, TRUE );
	m_checkCDMusic.SetFontSize( 12, FW_HEAVY );
	m_checkHighQuality.MoveWindow( x, 274, 2 * w, h, TRUE );
	m_checkHighQuality.SetFontSize( 12, FW_HEAVY );
	m_checkA3D.MoveWindow( x, 306, 2 * w, h, TRUE );
	m_checkA3D.SetFontSize( 12, FW_HEAVY );
	m_checkEAX.MoveWindow( x, 338, 2 * w, h, TRUE );
	m_checkEAX.SetFontSize( 12, FW_HEAVY );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::SliderScrolled (0x402B70)

void CAudioDlg::SliderScrolled( int nSBCode, int nPos, CObject* pObj )
{
	CWnd*	pSlider = DYNAMIC_DOWNCAST( CWnd, pObj );

	if ( pSlider && pSlider->GetSafeHwnd() == m_pVolumeSlider->GetSafeHwnd() )
	{
		if ( nSBCode == SB_ENDSCROLL )
			m_volume = (float)nPos;
		return;
	}

	if ( pSlider && pSlider->GetSafeHwnd() == m_pSuitVolSlider->GetSafeHwnd() )
	{
		if ( nSBCode == SB_ENDSCROLL )
			m_suitVolume = (float)nPos;
		return;
	}

	CWnd::OnVScroll( nSBCode, nPos, (CScrollBar*)pObj );
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnHighQuality (0x402C20)

void CAudioDlg::OnHighQuality()
{
	m_bHighQuality = ( m_checkHighQuality.m_bChecked != 0 );
	force_mode_set = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnOK (0x402C40)

void CAudioDlg::OnOK()
{
	CGameClientConfig*	cfg = &g_pServerBrowser->m_playerConfig;

	cfg->bgmvolume = m_bCDMusic ? 1.0f : 0.0f;
	cfg->volume = m_volume * 0.01f;
	cfg->suitvolume = m_suitVolume * 0.01f;
	cfg->hisound = m_bHighQuality ? 1.0f : 0.0f;
	cfg->s_a3d = m_bA3D ? 1.0f : 0.0f;
	cfg->s_eax = m_bEAX ? 1.0f : 0.0f;

	Launcher_SavePlayerInfoTo( "Player", &g_pServerBrowser->m_playerConfig );
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnCancel (0x402D00)

void CAudioDlg::OnCancel()
{
	force_mode_set = m_savedVidRestart;
	OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnUseCD (0x402D20)

void CAudioDlg::OnUseCD()
{
	m_bCDMusic = ( m_checkCDMusic.m_bChecked != 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnSliderScroll (0x402D40)
//
// wParam is MAKEWPARAM( SB_code, pos ), lParam the sender.

LRESULT CAudioDlg::OnSliderScroll( WPARAM wParam, LPARAM lParam )
{
	SliderScrolled( LOWORD( wParam ), HIWORD( wParam ), (CObject*)lParam );
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnA3D (0x402D70)

void CAudioDlg::OnA3D()
{
	m_bA3D = ( m_checkA3D.m_bChecked != 0 );
	force_mode_set = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnEAX (0x402D90)

void CAudioDlg::OnEAX()
{
	m_bEAX = ( m_checkEAX.m_bChecked != 0 );
	force_mode_set = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnActivateApp (0x406FE0)

void CAudioDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnPaint (0x412860)

void CAudioDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnEraseBkgnd (0x412870)

BOOL CAudioDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAudioDlg::OnDisplayChange (0x453D00)

LRESULT CAudioDlg::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}
