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
// Purpose: the player-identity / "Customize" page (CPlayerProfileDlg).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// g_pCurrentMod is declared in mod.h (mod_t*).
void				Palette_FillHSVBand( BYTE* pDst, BYTE* pSrc, int iHue, int yTop, int yBot );	// 0x457730 (defined below)

// Quantiser state for AveragePixels (0x456AF0).  The shipping build's
// AveragePixels point-samples, so everything but s_pixdata is write-only.
static int		s_avgReserved;			// unk_4F4DFC
static int		s_colorUsed[256];		// unk_4F4E00  (all 1 = every entry usable)
static int		s_numColors;			// unk_4F5200
static int		s_avgR, s_avgG, s_avgB;	// unk_4F5204 / 4F5208 / 4F520C
static float	s_palFloat[256][3];		// unk_4F5210  the logo palette, 0..1
static BYTE		s_pixdata[256];			// unk_4F5E10  one mip block's source pixels

static BYTE	g_palModelWork[768];	// unk_4F5F10
static BYTE	g_palModelBase[768];	// unk_4F6210
static BYTE	g_palLogoBase[768];		// unk_4F6510

// WAD3 on-disk layout (id's wadlib/bspfile types).
typedef struct
{
	char	identification[4];		// "WAD3"
	int		numlumps;
	int		infotableofs;
} wadinfo_t;

typedef struct
{
	int		filepos;
	int		disksize;
	int		size;
	char	type;					// 0x40 = TYP_MIPTEX
	char	compression;
	char	pad1, pad2;
	char	name[16];
} lumpinfo_t;

typedef struct
{
	char	name[16];
	int		width;
	int		height;
	int		offsets[4];
} miptex_t;

// Static template rects (binary literals): the model preview frame and the logo
// invalidate region; "nomodels" liblist key gating the preview blit.
static const RECT	s_rcModelPreview = { 410, 160, 580, 356 };	// stru_4D13B0
static const RECT	s_rcLogoPreview = { 212, 226, 311, 325 };	// stru_4D1370

// The scanned model list (0x4F6810).
static mapinfo_t*	g_pModelList = 0;

// The mixer the page opens for the transmit slider (0x4F6814).  It is held open
// for the life of the page and released by OnOK.
static IMixerControls*	s_pMixerControls;

static void	Logo_WriteDecalWad( const char* pszLogo, HGLOBAL hDib, COLORREF clr );	// 0x456D70 (defined below)

// Entries at 0x4B28D8, base map 0x4B4398 = CDialog.  The nickname edit reports
// through three of them, so ApplyToConfig runs on every keystroke.
BEGIN_MESSAGE_MAP( CPlayerProfileDlg, CDialog )
	//{{AFX_MSG_MAP(CPlayerProfileDlg)
	ON_MESSAGE( WM_DISPLAYCHANGE, OnDisplayChange )
	ON_REGISTERED_MESSAGE( g_uiScrollMsg, OnSliderScroll )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_ACTIVATE()
	ON_WM_ACTIVATEAPP()
	ON_BN_CLICKED( IDC_OPTS_HIMODELS,   OnHiModels )
	ON_BN_CLICKED( IDC_OPTS_VOCENABLE,  OnVoiceEnable )
	ON_BN_CLICKED( IDC_PROFILE_LEFT,    OnModelPrev )
	ON_BN_CLICKED( IDC_PROFILE_RIGHT,   OnModelNext )
	ON_BN_CLICKED( IDC_PROFILE_LOGO_PREV,  OnLogoPrev )
	ON_BN_CLICKED( IDC_PROFILE_LOGO_NEXT, OnLogoNext )
	ON_BN_CLICKED( IDC_BTN_SETINFO,     OnSetInfo )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CPlayerProfileDlg::CPlayerProfileDlg (0x453690)
CPlayerProfileDlg::CPlayerProfileDlg( CWnd* pParent )
	: CDlgBase( IDD_PROFILE, pParent )		// 0xAE
{
	m_pSelfWnd      = this;		// gates the slide transition
	m_unk3888       = 0;
	m_pLogoNames    = NULL;
	m_nLogoNamesLen = 0;
	m_szLogo[0]     = 0;
	m_logoCount     = 0;
	m_hLogoDib      = NULL;
	m_szModel[0]    = 0;
	m_modelCount    = 0;
	m_hModelDib     = NULL;
	m_bConfigChanged = 0;

	// the snapshot OnInitDialog reads from and OnCancel restores
	m_pSavedConfig = new CServerBrowser;
	ServerBrowser_CopyConfig( m_pSavedConfig, g_pServerBrowser );

	m_brBlack.CreateSolidBrush( RGB( 0, 0, 0 ) );
	m_pNameEdit = NULL;

	LoadHeaderBitmap( "head_customize", NULL );
	m_pUserDesc = NULL;
	CacheHeaderMetrics();
}

// CPlayerProfileDlg::~CPlayerProfileDlg (0x453960)
CPlayerProfileDlg::~CPlayerProfileDlg()
{
	if ( m_pLogoNames )
	{
		delete[] m_pLogoNames;
		m_nLogoNamesLen = 0;
	}

	COM_FreeMapList( &g_pModelList );

	delete m_pSavedConfig;
	m_pSavedConfig = NULL;

	// The four sliders are not freed: each is a child window with m_bAutoDelete
	// set and frees itself from its own OnNcDestroy.
}

// CPlayerProfileDlg::DoDataExchange (0x453B90)
void CPlayerProfileDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_AUDIO_MICVOL,      m_lblMicVol );			// 32789 +224
	DDX_Control( pDX, IDC_AUDIO_MILES,       m_lblMiles );			// 1224  +320
	DDX_Control( pDX, IDC_AUDIO_SPEAKVOL,    m_lblSpeakVol );			// 32790 +416
	DDX_Control( pDX, IDC_OPTS_VOCENABLE,    m_chkVoice );		// 32791 +512
	DDX_Control( pDX, IDC_BTN_SETINFO,       m_btnSetInfo );		// 1220  +816
	DDX_Control( pDX, IDC_OPTS_HIMODELS,     m_chkHiModels );	// 1043  +1056
	DDX_Control( pDX, IDC_PROFILE_NICKNAME,  m_lblNickname );	// 1082  +1360
	DDX_Control( pDX, IDC_PROFILE_LOGO,      m_lblLogo );		// 1183  +1456
	DDX_Control( pDX, IDC_PROFILE_LOGOCOLOR, m_lblLogoColor );	// 1206  +1552
	DDX_Control( pDX, IDC_PROFILE_MODEL,     m_lblModel );		// 1077  +1984
	DDX_Control( pDX, IDC_PROFILE_COLOR,     m_lblColor );		// 1079  +2080
	DDX_Control( pDX, IDC_PROFILE_LEFT,      m_btnModelPrev );	// 22    +2464
	DDX_Control( pDX, IDC_PROFILE_RIGHT,     m_btnModelNext );	// 30    +2704
	DDX_Control( pDX, IDC_PROFILE_LOGO_PREV,    m_btnLogoPrev );	// 35    +2944
	DDX_Control( pDX, IDC_PROFILE_LOGO_NEXT,   m_btnLogoNext );	// 38    +3184
	DDX_Control( pDX, IDOK,                  m_btnDone );		// 1     +3424
}

// CPlayerProfileDlg::RMLPreIdle (0x453CC0) -- CDlgBase frame slot 56
int CPlayerProfileDlg::RMLPreIdle()
{
	Launcher_SyncEngineWindow( this );
	if ( Eng_Frame( gBackground ) && !gBackground )
		return 1;

	IN_HideMouse();
	::ClipCursor( NULL );
	return 0;
}

// CPlayerProfileDlg::OnDisplayChange (0x453D00)
LRESULT CPlayerProfileDlg::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}

// SetupLabel (0x453D10)
void SetupLabel( CODStatic* pLbl, const RECT* prc, UINT uID )
{
	pLbl->SetTransparent( TRUE );
	pLbl->SetTextColor( RGB( 255, 255, 255 ) );	// white, not HELP_COLOR
	pLbl->SetFontSize( 14, FW_NORMAL );
	pLbl->SetWindowText( Launcher_LoadString( uID ) );
	pLbl->MoveWindow( prc->left, prc->top, prc->right - prc->left,
		prc->bottom - prc->top, TRUE );
}

// SetupArrowButton (0x457170) -- the paging arrows the option pages hang off
// their own header, built as owner-draw BUTTONs so they get WM_DRAWITEM.
CODBitmapButton* SetupArrowButton( const RECT* prc, CWnd* pParent,
	const char* pszNormal, const char* pszDown, const char* pszFocus, UINT nID )
{
	CODBitmapButton*	pBtn = new CODBitmapButton;

	pBtn->CreateGlyph( WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, *prc, pParent, nID );
	pBtn->SetSkin( pszNormal, pszDown, pszFocus );
	pBtn->MoveWindow( prc->left, prc->top, prc->right - prc->left,
		prc->bottom - prc->top, TRUE );
	return pBtn;
}

// PopulateColorCombo (0x453D80)
static void PopulateColorCombo( CODColorComboBox* pCombo )
{
	pCombo->SetRowHeight( 18 );
	pCombo->SetAutoDelete( 0 );
	pCombo->AddString( "Orange" );
	pCombo->AddString( "Blue" );
	pCombo->AddString( "Ltblue" );
	pCombo->AddString( "Green" );
	pCombo->AddString( "Red" );
	pCombo->AddString( "Brown" );
	pCombo->AddString( "Ltgray" );
	pCombo->AddString( "Dkgray" );
	pCombo->SetCurSel( 0 );
}

// CPlayerProfileDlg::RecolourLogoDib (0x453E10)
void CPlayerProfileDlg::RecolourLogoDib( COLORREF clr )
{
	HGLOBAL	hDib = m_hLogoDib;
	if ( !hDib )
		return;

	BYTE	r = GetRValue( clr );
	BYTE	g = GetGValue( clr );
	BYTE	b = GetBValue( clr );

	BYTE*	pDib = (BYTE*)GlobalLock( hDib );
	if ( DIB_NumColors( (LPBITMAPINFOHEADER)pDib ) == 256 )
	{
		m_logoColorWord = clr;

		BOOL	bInfo = ( *(DWORD*)pDib == 40 );
		BYTE*	pTri  = pDib + 13;
		BYTE*	pQuad = pDib + 41;
		double	flIdx = 0.0;

		for ( int i = 0; i < 256; i++ )
		{
			double	flScale = flIdx * ( 1.0 / 256.0 );
			BYTE	vG = (BYTE)( (double)g * flScale );
			BYTE	vR = (BYTE)( (double)r * flScale );
			BYTE	vB = (BYTE)( (double)b * flScale );
			if ( bInfo )
			{
				pQuad[1] = vR;
				pQuad[0] = vG;
				pQuad[-1] = vB;
			}
			else
			{
				pTri[1] = vR;
				pTri[0] = vG;
				pTri[-1] = vB;
			}
			flIdx += 1.0;
			pQuad += 4;
			pTri += 3;
		}
	}

	GlobalUnlock( m_hLogoDib );
}

// CPlayerProfileDlg::OnInitDialog (0x453F40)
BOOL CPlayerProfileDlg::OnInitDialog()
{
	s_pMixerControls = CreateMixerControls();

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	if ( !g_pServerBrowser )
	{
		Launcher_ShowMessageById( 0, IDS_AUDIO_NOPROFILE );
		OnCancel();
		return TRUE;
	}

	// The template carries both switches as plain checkboxes; owner-draw is
	// OR'd on so CODBlendCheckBox paints them, and both captions wrap.
	::SetWindowLong( m_chkHiModels.m_hWnd, GWL_STYLE,
		m_chkHiModels.GetStyle() | BS_OWNERDRAW );
	m_chkHiModels.m_textFlags = DT_WORDBREAK;
	m_chkHiModels.SetWindowText( Launcher_LoadString( IDS_OPTS_HIMODELS ) );
	::SetWindowLong( m_chkVoice.m_hWnd, GWL_STYLE,
		m_chkVoice.GetStyle() | BS_OWNERDRAW );
	m_chkVoice.m_textFlags = DT_WORDBREAK;
	m_chkVoice.SetWindowText( Launcher_LoadString( IDS_AUDIO_ENABLEVOICE ) );

	m_bHiModels    = 0;
	m_bVoiceEnable = 0;

	// the five transparent customize labels
	static const RECT	rcNickname  = { 212, 137, 395, 160 };	// unk_4D1330
	static const RECT	rcLogo      = { 212, 200, 380, 217 };	// unk_4D1350
	static const RECT	rcLogoColor = { 320, 226, 380, 240 };	// unk_4D1390
	static const RECT	rcModel     = { 410, 137, 580, 160 };	// unk_4D13C0
	static const RECT	rcColor     = { 410, 400, 480, 417 };	// unk_4D13D0

	SetupLabel( &m_lblNickname,  &rcNickname,  IDS_PROFILE_NICKNAME );
	SetupLabel( &m_lblLogo,      &rcLogo,      IDS_PROFILE_LOGO );
	SetupLabel( &m_lblLogoColor, &rcLogoColor, IDS_PROFILE_LOGOCOLOR );
	SetupLabel( &m_lblModel,     &rcModel,     IDS_PROFILE_MODEL );
	SetupLabel( &m_lblColor,     &rcColor,     IDS_PROFILE_COLOR );

	// the model + logo step buttons
	// CODBlendBtn paints its window text; the buttons are made transparent so their
	// owner-draw face shows the skin, not a white client fill
	m_btnLogoPrev.SetBkColor( RGB( 56, 56, 56 ) );
	m_btnLogoPrev.SetTextColor( RGB( 240, 180, 24 ) );
	m_btnLogoPrev.SetHasArrow( 0 );
	m_btnLogoPrev.SetFontSize( 12, FW_NORMAL );
	m_btnLogoPrev.SetLeftAlign();
	m_btnLogoPrev.SetWindowText( Launcher_LoadString( IDS_PREVIOUS ) );			// 0x1A3
	m_btnLogoPrev.MoveWindow( 215, 325, 270 - 215, 350 - 325, TRUE );			// dword_4D1360
	m_btnLogoNext.SetBkColor( RGB( 56, 56, 56 ) );
	m_btnLogoNext.SetTextColor( RGB( 240, 180, 24 ) );
	m_btnLogoNext.SetHasArrow( 0 );
	m_btnLogoNext.SetFontSize( 12, FW_NORMAL );
	m_btnLogoNext.SetRightAlign();
	m_btnLogoNext.SetWindowText( Launcher_LoadString( IDS_NEXT ) );				// 0x1A4
	m_btnLogoNext.MoveWindow( 270, 325, 308 - 270, 350 - 325, TRUE );			// dword_4D1380
	m_btnModelPrev.SetBkColor( RGB( 56, 56, 56 ) );
	m_btnModelPrev.SetTextColor( RGB( 240, 180, 24 ) );
	m_btnModelPrev.SetHasArrow( 0 );
	m_btnModelPrev.SetFontSize( 12, FW_NORMAL );
	m_btnModelPrev.SetLeftAlign();
	m_btnModelPrev.SetWindowText( Launcher_LoadString( IDS_PREVIOUS ) );			// 0x1A3
	m_btnModelPrev.MoveWindow( 413, 356, 480 - 413, 381 - 356, TRUE );			// dword_4D1420
	m_btnModelNext.SetBkColor( RGB( 56, 56, 56 ) );
	m_btnModelNext.SetTextColor( RGB( 240, 180, 24 ) );
	m_btnModelNext.SetHasArrow( 0 );
	m_btnModelNext.SetFontSize( 12, FW_NORMAL );
	m_btnModelNext.SetRightAlign();
	m_btnModelNext.SetWindowText( Launcher_LoadString( IDS_NEXT ) );				// 0x1A4
	m_btnModelNext.MoveWindow( 480, 356, 577 - 480, 381 - 356, TRUE );			// dword_4D1430

	// the colour picker; its closed face paints the selected swatch
	if ( !m_colorCombo.m_hWnd )
	{
		RECT	rcCombo = { 320, 250, 380, 346 };

		m_colorCombo.SetDropHeight( 90 );
		m_colorCombo.Create( 0, &rcCombo, this, IDC_PROFILE_COLORCOMBO );
	}
	PopulateColorCombo( &m_colorCombo );

	// the player-name edit
	if ( !m_pNameEdit )
	{
		m_pNameEdit = new CBorderLessEdit;
		RECT	rcEdit = { 212, 160, 395, 184 };	// X/Y = dword_4D1340..dword_4D134C
		m_pNameEdit->Create( 0, &rcEdit, this, IDC_PROFILE_NAMEEDIT );
		// normal border colour, poked straight after Create
		m_pNameEdit->SetBorderColor( RGB( 56, 56, 56 ) );
		m_pNameEdit->MoveWindow( 212, 160, 395 - 212, 184 - 160, TRUE );
	}

	// Header-strip cell metrics drive the voice/Done/hi-models layout.
	int	hw  = m_headerW;
	int	hh  = m_headerH;
	int	v53 = Launcher_StringHeight( 0x1EE, 0 );	// locale layout flag

	// hi-models checkbox
	{
		int	x = v53 ? 10 : 50;
		int	w = Launcher_StringHeight( 0x1E7, 0 ) + hw + ( 150 * v53 );
		m_chkHiModels.MoveWindow( x, 221, w, hh + 9, TRUE );
	}

	// The two voice sliders.  Neither pointer is zeroed by the constructor and
	// neither new is guarded: OnInitDialog runs once per instance.
	{
		RECT	rc = { 0, 0, 100, 100 };

		m_pVoiceXmit = new CODSlider;
		m_pVoiceXmit->Create( this, &rc );
		m_pVoiceRecv = new CODSlider;
		m_pVoiceRecv->Create( this, &rc );
	}
	// Transmit comes off the mixer, receive out of the config.
	float	flMic = 1.0f;
	if ( s_pMixerControls )
		s_pMixerControls->GetValue( IMixerControls::MIXER_CONTROL_MICVOLUME, &flMic );
	m_flVoiceXmit = flMic * 100.0f;
	m_flVoiceRecv = g_pServerBrowser->m_playerConfig.voice_scale * 100.0f;

	m_pVoiceXmit->SetRange( 0, 100 );
	m_pVoiceXmit->SetPos( (int)m_flVoiceXmit );
	m_pVoiceRecv->SetRange( 0, 100 );
	m_pVoiceRecv->SetPos( (int)m_flVoiceRecv );

	// The two model-colour sliders, same shape.
	{
		RECT	rc = { 410, 420, 580, 445 };

		m_pModelColor0 = new CODSlider;
		m_pModelColor0->Create( this, &rc );
		m_pModelColor0->SetRange( 1, 255 );

		::SetRect( &rc, 410, 440, 580, 470 );
		m_pModelColor1 = new CODSlider;
		m_pModelColor1->Create( this, &rc );
		m_pModelColor1->SetRange( 1, 255 );
	}

	// Done / SetInfo owner-draw buttons
	m_btnDone.SetTransparent( TRUE );
	m_btnDone.SetTextColor( RGB( 240, 180, 24 ) );
	m_btnDone.SetHasArrow( 0 );
	m_btnDone.SetFontSize( 12, FW_HEAVY );
	m_btnDone.SetLeftAlign();
	m_btnDone.SetWindowText( Launcher_LoadString( IDS_BTN_DONE ) );
	m_btnSetInfo.SetTransparent( TRUE );
	m_btnSetInfo.SetTextColor( RGB( 240, 180, 24 ) );
	m_btnSetInfo.SetHasArrow( 0 );
	m_btnSetInfo.SetFontSize( 12, FW_HEAVY );
	m_btnSetInfo.SetLeftAlign();
	m_btnSetInfo.SetWindowText( Launcher_LoadString( IDS_BTN_SETINFO ) );
	{
		RECT	rc;
		rc.left = v53 ? 10 : 50;
		if ( Launcher_StringHeight( 0x1EF, 0 ) )
			rc.left -= 5;
		rc.right  = Launcher_StringHeight( 0x1E7, 1 ) + hw + rc.left;
		rc.top    = 140;
		rc.bottom = hh + 140;
		if ( Launcher_StringHeight( 0x1F2, 0 ) || Launcher_StringHeight( 0x1EF, 0 ) )
			::OffsetRect( &rc, -15, 0 );
		m_btnDone.MoveWindow( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );
		::OffsetRect( &rc, 0, 26 );
		m_btnSetInfo.MoveWindow( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );
	}

	LoadFromConfig( m_pSavedConfig );

	ScanLogoList();
	ScanModelList();

	m_pModelColor0->SetPos( m_modelHue1 );
	m_pModelColor1->SetPos( m_modelHue0 );

	// no spray picked yet: fall back to the first scanned logo
	if ( !strcmp( m_szLogo, "None" ) && m_logoCount )
		strcpy( m_szLogo, m_pLogoNames );

	m_logoIdx  = LogoIndexByName();
	m_hLogoDib = LoadPreviewDib( 0, m_szLogo );
	RefreshLogoPreview();

	if ( !m_szModel[0] )
		strcpy( m_szModel, "gordon" );

	// ModelIndexByName rewrites m_szModel to the scanned entry's full BMP path
	m_modelIdx  = ModelIndexByName();
	m_hModelDib = LoadPreviewDib( 1, m_szModel );

	// the model help caption
	{
		char	szBase[260];
		COM_FileBase( m_szModel, szBase );
		m_lblModel.SetWindowText( Launcher_FormatString( Launcher_LoadString( IDS_MODEL_NAME ), szBase ) );
		m_lblModel.SetFontSize( 13, FW_NORMAL );
	}

	RefreshModelPreview();

	// mod liblist overrides
	if ( g_pCurrentMod )
	{
		const char*	p = g_pCurrentMod->GetKey( "nomodels" );
		if ( p && *p && atoi( p ) )
		{
			m_btnModelPrev.ShowWindow( SW_HIDE );
			m_btnModelNext.ShowWindow( SW_HIDE );
			m_lblModel.ShowWindow( SW_HIDE );
			m_lblColor.ShowWindow( SW_HIDE );
			m_pModelColor0->ShowWindow( SW_HIDE );
			m_pModelColor1->ShowWindow( SW_HIDE );
		}
		p = g_pCurrentMod->GetKey( "nohimodel" );
		if ( p && *p && atoi( p ) )
			m_chkHiModels.ShowWindow( SW_HIDE );
	}

	// - the voice cluster, positioned bottom-centre of the customize page (binary
	// @0x454abe..0x454ce7).
	{
		int	x = 413 + 150 * v53 - 200;		// v40
		int	yBase = 400 - hh + 1;			// v39 / Y[0]
		int	y = yBase;

		// "Voice Transmit Volume" label (this+224): white, 14/400, caption 0x24C.
		m_lblMicVol.SetTextColor( RGB( 255, 255, 255 ) );
		m_lblMicVol.SetFontSize( 14, FW_NORMAL );
		m_lblMicVol.SetWindowText( Launcher_LoadString( IDS_AUDIO_VOICETRANSMIT ) );	// 0x24C
		m_lblMicVol.MoveWindow( x, y, ( 130 * v53 + 175 ), ( 400 + 1 ) - y, TRUE );

		// voice transmit slider (this+918): MoveWindow(x, y+19, hw, hh).
		y += 19;
		m_pVoiceXmit->MoveWindow( x, y, hw, hh, TRUE );

		// "Voice Receive Volume" label (this+416): white, 14/400, caption 0x24D.
		y += 32;
		m_lblSpeakVol.SetTextColor( RGB( 255, 255, 255 ) );
		m_lblSpeakVol.SetFontSize( 14, FW_NORMAL );
		m_lblSpeakVol.SetWindowText( Launcher_LoadString( IDS_AUDIO_VOICERECEIVE ) );	// 0x24D
		m_lblSpeakVol.MoveWindow( x, y, ( 150 * v53 + 200 ), hh, TRUE );

		// voice receive slider (this+919): MoveWindow(x, y+19, hw, hh).
		y += 19;
		m_pVoiceRecv->MoveWindow( x, y, hw, hh, TRUE );

		// "Enable voice in this mod" checkbox (this+512): MoveWindow(50, yBase, 150, 30).
		m_chkVoice.MoveWindow( 50, yBase, 150, 30, TRUE );

		// Miles copyright help line (this+320): HELP_COLOR, 11/400, caption 0x24E, at
		// (50, yBase+50, 150, 70).
		m_lblMiles.SetTransparent( TRUE );
		m_lblMiles.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
		m_lblMiles.SetFontSize( 11, FW_NORMAL );
		m_lblMiles.SetWindowText( Launcher_LoadString( IDS_AUDIO_MILESCOPYRIGHT ) );	// 0x24E
		m_lblMiles.MoveWindow( 50, yBase + 50, 150, 70, TRUE );
	}

	return TRUE;
}

// CPlayerProfileDlg::PaintModelPreview (0x454D10)
void CPlayerProfileDlg::PaintModelPreview( HDC hdc )
{
	if ( g_pCurrentMod )
	{
		char*	pKey = g_pCurrentMod->GetKey( "nomodels" );
		if ( pKey && *pKey && atoi( pKey ) )
			return;
	}

	RECT	rcDst = s_rcModelPreview;
	::InflateRect( &rcDst, -3, -3 );

	HGLOBAL	hDib = m_hModelDib;
	if ( hDib )
	{
		void*	pDib = GlobalLock( hDib );
		RECT	rcSrc;
		rcSrc.left = 0;
		rcSrc.top = 0;
		rcSrc.right = DIB_Width( (LPBITMAPINFOHEADER)pDib );
		rcSrc.bottom = DIB_Height( (LPBITMAPINFOHEADER)pDib );
		GlobalUnlock( m_hModelDib );
		DIB_BlitDib( hdc, &rcDst, hDib, &rcSrc );
	}
	else
	{
		::PatBlt( hdc, rcDst.left, rcDst.top,
			rcDst.right - rcDst.left, rcDst.bottom - rcDst.top, BLACKNESS );
	}
}

// CPlayerProfileDlg::LoadFromConfig (0x454E00)
void CPlayerProfileDlg::LoadFromConfig( CServerBrowser* pCfg )
{
	if ( !pCfg )
		return;

	m_pNameEdit->SetText( "" );
	m_pNameEdit->SetText( pCfg->GetPlayerName() );

	int	iColor = m_colorCombo.FindString( pCfg->m_szLogoColor );
	m_colorCombo.SetCurSel( iColor == -1 ? 0 : iColor );
	::InvalidateRect( m_colorCombo.m_hWnd, NULL, TRUE );
	::UpdateWindow( m_colorCombo.m_hWnd );

	strcpy( m_szLogo, pCfg->m_szLogoName );

	// the model cluster comes off the live document, not the snapshot
	m_modelHue1 = g_pServerBrowser->m_playerConfig.topcolor;
	m_modelHue0 = g_pServerBrowser->m_playerConfig.bottomcolor;
	strcpy( m_szModel, g_pServerBrowser->m_playerConfig.model );
}

// CPlayerProfileDlg::OnOK (0x454F10)
void CPlayerProfileDlg::OnOK()
{
	ApplyToConfig();

	g_pServerBrowser->m_playerConfig.voice_scale = m_flVoiceRecv * 0.01f;
	Launcher_SavePlayerInfoTo( "Player", &g_pServerBrowser->m_playerConfig );
	Launcher_WriteProfileString( "Settings", "Logo",
		g_pServerBrowser->m_szLogoName );
	Launcher_WriteProfileString( "Settings", "Logo Color",
		g_pServerBrowser->m_szLogoColor );

	// The mixer is opened for the life of the page only; the transmit slider is
	// the one setting that lands outside the config.
	if ( s_pMixerControls )
	{
		s_pMixerControls->SetValue( IMixerControls::MIXER_CONTROL_MICVOLUME, m_flVoiceXmit * 0.01f );
		s_pMixerControls->Release();
		s_pMixerControls = NULL;
	}

	CDialog::OnOK();
}

// CPlayerProfileDlg::OnCtlColor (0x454FC0) -- only the message-box and edit
// classes get the page's black face; everything else keeps MFC's brush.
HBRUSH CPlayerProfileDlg::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH	hbr = CDialog::OnCtlColor( pDC, pWnd, nCtlColor );

	if ( nCtlColor <= CTLCOLOR_EDIT )
	{
		pDC->SetTextColor( RGB( 255, 135, 24 ) );
		pDC->SetBkMode( TRANSPARENT );
		pDC->SetBkColor( RGB( 0, 0, 0 ) );
		return m_brBlack;
	}

	return hbr;
}

// CPlayerProfileDlg::ApplyToConfig (0x455E10) -- push the page's controls back
// into the live document and rewrite the spray WAD.
void CPlayerProfileDlg::ApplyToConfig()
{
	CServerBrowser	cfg;

	ServerBrowser_CopyConfig( &cfg, g_pServerBrowser );

	CString	strName;
	if ( m_pNameEdit->m_pEdit )
		m_pNameEdit->m_pEdit->GetWindowText( strName );

	cfg.m_playerConfig.name[0] = 0;
	if ( strName.GetLength() )
		sprintf( cfg.m_playerConfig.name, strName );

	cfg.m_playerConfig.topcolor    = m_modelHue1;
	cfg.m_playerConfig.bottomcolor = m_modelHue0;
	cfg.m_playerConfig.cl_himodels = m_bHiModels ? 1.0f : 0.0f;
	cfg.m_playerConfig.voice_modenable = m_bVoiceEnable ? 1.0f : 0.0f;

	strcpy( cfg.m_playerConfig.name, cfg.GetPlayerName() );

	// "<...>/player/<name>/..." -> "<name>"
	char	szModel[260];
	strcpy( szModel, m_szModel );

	const char*	pszModel = "gordon";
	char*	p = strstr( szModel, "player" );
	if ( p )
	{
		p += strlen( "player" ) + 1;
		char*	pEnd = strstr( p, "/" );
		if ( !pEnd )
			pEnd = strstr( p, "\\" );
		if ( pEnd )
			*pEnd = 0;
		pszModel = p;
	}
	strcpy( cfg.m_playerConfig.model, pszModel );

	Launcher_SavePlayerInfoTo( "Player", &cfg.m_playerConfig );

	int	iColor = m_colorCombo.GetCurSel();
	if ( iColor != -1 )
		strcpy( cfg.m_szLogoColor, m_colorCombo.GetString( iColor ) );
	strcpy( cfg.m_szLogoName, m_szLogo );

	Logo_WriteDecalWad( m_szLogo, m_hLogoDib, m_colorCombo.CurrentSwatch() );

	m_bConfigChanged = CGameConfig_IsEqual( &cfg, g_pServerBrowser );
	ServerBrowser_CopyConfig( g_pServerBrowser, &cfg );
	CFG_FreeBindings( cfg.m_playerConfig.m_binds );
}

// CPlayerProfileDlg::OnCancel (0x456110)
void CPlayerProfileDlg::OnCancel()
{
	if ( m_bConfigChanged )
	{
		CPromptDlg	dlg( 2, NULL );
		dlg.SetMessage( Launcher_LoadString( IDS_PROFILE_CANCELPROMPT ) );	// 242
		if ( dlg.DoModal() == IDOK )
			ApplyToConfig();
		else
			ServerBrowser_CopyConfig( g_pServerBrowser, m_pSavedConfig );
	}

	CDialog::OnCancel();
}

// CPlayerProfileDlg::CacheHeaderMetrics (0x456280) -- re-read the header strip and
// re-slice the two blend buttons out of it.
void CPlayerProfileDlg::CacheHeaderMetrics()
{
	int	dims[2] = { 0, 0 };

	m_headerLoaded = Launcher_HeaderLoaded();
	int*	pWH = (int*)Launcher_HeaderSize( dims );
	m_headerW = pWH[0];
	m_headerH = pWH[1];
	m_headerStride = Launcher_HeaderStride();

	if ( !m_headerLoaded )
		return;

	m_btnDone.FreeSkinBitmaps();
	m_btnDone.SetDIBData( CSize( m_headerW, m_headerH ), 19, m_headerLoaded );
	m_btnSetInfo.FreeSkinBitmaps();
	m_btnSetInfo.SetDIBData( CSize( m_headerW, m_headerH ), 68, m_headerLoaded );
}

// CPlayerProfileDlg::LoadPreviewDib (0x456310)
HGLOBAL CPlayerProfileDlg::LoadPreviewDib( int bModel, const char* pszName )
{
	if ( !pszName || !*pszName )
		return NULL;

	LPVOID	pFile = NULL;	// COM_LoadMallocFile buffer (model path)
	HANDLE	hFile = NULL;	// OpenFile handle (logo path)
	BYTE*	pPalDst;
	DWORD	dwSize;
	DWORD	dwRead;
	char	hdr[14];		// BITMAPFILEHEADER bytes
	const char*	pCopySrc = NULL;	// model: file bytes past the 14-byte header

	if ( bModel )
	{
		fileinfo_t	fi;
		pPalDst = g_palModelBase;
		dwSize = COM_OpenFile( pszName, &fi );
		if ( fi.handle == -1 || !dwSize )
			return NULL;
		COM_CloseFile( fi );
		if ( dwSize > 0x84D0 )
			return NULL;
		pFile = COM_LoadMallocFile( pszName );
		memcpy( hdr, pFile, 14 );
		pCopySrc = (const char*)pFile + 14;
	}
	else
	{
		OFSTRUCT	of;
		char		szPath[260];
		sprintf( szPath, "logos\\%s", pszName );
		pPalDst = g_palLogoBase;
		hFile = (HANDLE)OpenFile( szPath, &of, OF_READ );
		if ( hFile == (HANDLE)HFILE_ERROR )
			return NULL;
		dwSize = GetFileSize( hFile, 0 );
		if ( dwSize > 0x1448
			|| !ReadFile( hFile, hdr, 14, &dwRead, 0 ) || dwRead != 14 )
		{
			CloseHandle( hFile );
			return NULL;
		}
	}

	// 'BM' magic.
	if ( *(WORD*)hdr != 19778 )
	{
		if ( hFile )		CloseHandle( hFile );
		if ( pFile )		free( pFile );
		return NULL;
	}

	DWORD	dwBody = dwSize - 14;
	HGLOBAL	hDib = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, dwBody );
	if ( !hDib )
	{
		if ( hFile )		CloseHandle( hFile );
		if ( pFile )		free( pFile );
		return NULL;
	}

	BYTE*	pDib = (BYTE*)GlobalLock( hDib );
	if ( bModel )
	{
		memcpy( pDib, pCopySrc, dwBody );
		free( pFile );
	}
	else
	{
		if ( !ReadFile( hFile, pDib, dwBody, &dwRead, 0 ) || dwRead != dwBody )
		{
			GlobalUnlock( hDib );
			GlobalFree( hDib );
			CloseHandle( hFile );
			return NULL;
		}
		CloseHandle( hFile );
	}

	// Copy the BMP palette (RGBQUAD[256] just past the 40-byte info header) into
	// the base palette as BGR triples.
	const BYTE*	pQuad = pDib + 41;	// &palette[0].rgbGreen
	BYTE*		pOut  = pPalDst;
	for ( int i = 0; i < 256; i++ )
	{
		BYTE	g = pQuad[0];		// rgbGreen
		pQuad += 4;
		pOut[0] = g;
		pOut[1] = pQuad[-4];		// rgbBlue
		pOut[2] = pQuad[-5];		// rgbRed
		pOut += 3;
	}

	GlobalUnlock( hDib );

	if ( !bModel )
		RecolourLogoDib( m_colorCombo.CurrentSwatch() );

	return hDib;
}

// CPlayerProfileDlg::ScanModelList (0x456570) -- rescan models/player and move the
// "gordon" entry to the head of the list so it is the default.
void CPlayerProfileDlg::ScanModelList()
{
	if ( !COM_GetPlayerModelList( &g_pModelList ) )
		return;

	m_modelCount = 0;
	for ( mapinfo_t* p = g_pModelList; p; p = p->next )
		m_modelCount++;

	if ( !g_pModelList || strstr( g_pModelList->name, "gordon" ) )
		return;

	for ( mapinfo_t* pPrev = g_pModelList; pPrev; pPrev = pPrev->next )
	{
		mapinfo_t*	pFound = pPrev->next;
		if ( !pFound )
			return;
		if ( strstr( pFound->name, "gordon" ) )
		{
			pPrev->next  = pFound->next;
			pFound->next = g_pModelList;
			g_pModelList = pFound;
			return;
		}
	}
}

// CPlayerProfileDlg::ScanLogoList (0x456630) -- pack every logos\*.bmp name into
// one NUL-separated blob.
void CPlayerProfileDlg::ScanLogoList()
{
	WIN32_FIND_DATA	fd;
	int		nFiles = 0;
	int		nBytes = 0;

	HANDLE	hFind = FindFirstFile( "logos\\*.bmp", &fd );
	if ( hFind != INVALID_HANDLE_VALUE )
	{
		do
		{
			nFiles++;
			nBytes += strlen( fd.cFileName ) + 1;
		} while ( FindNextFile( hFind, &fd ) );
	}

	if ( m_pLogoNames )
	{
		delete m_pLogoNames;
		m_nLogoNamesLen = 0;
	}
	m_pLogoNames = NULL;

	if ( !nFiles )
	{
		m_logoCount  = 0;
		m_pLogoNames = new char[260];
		strcpy( m_pLogoNames, "" );
		return;
	}

	m_pLogoNames = new char[nBytes];
	memset( m_pLogoNames, 0, nBytes );
	m_nLogoNamesLen = nBytes;

	int	nUsed  = 0;
	int	nNames = 0;
	hFind = FindFirstFile( "logos\\*.bmp", &fd );
	if ( hFind != INVALID_HANDLE_VALUE )
	{
		do
		{
			int	len = strlen( fd.cFileName ) + 1;
			if ( nUsed + len - 1 >= m_nLogoNamesLen )
				break;
			strcpy( m_pLogoNames + nUsed, fd.cFileName );
			nUsed += len;
			nNames++;
		} while ( FindNextFile( hFind, &fd ) );
	}
	m_logoCount = nNames;
}

// CPlayerProfileDlg::LogoIndexByName (0x4567C0)
int CPlayerProfileDlg::LogoIndexByName()
{
	for ( int i = 0; i < m_logoCount; i++ )
	{
		if ( !strcmp( m_szLogo, LogoNameByIndex( i ) ) )
			return i;
	}
	return 0;
}

// CPlayerProfileDlg::LogoNameByIndex (0x456830) -- the index'th spray logo name
// out of the packed NUL-separated blob; defaults to "None".
const char* CPlayerProfileDlg::LogoNameByIndex( int idx )
{
	const char*	blob = m_pLogoNames;
	int			len  = m_nLogoNamesLen;
	const char*	name = "None";
	int			off = 0, n = 0;

	if ( !blob )
		return name;
	if ( len > 0 )
	{
		do {
			name = blob + off;
			if ( n == idx )
				break;
			++n;
			off += (int)strlen( blob + off ) + 1;
		} while ( off < len );
	}
	return name;
}

// CPlayerProfileDlg::ModelIndexByName (0x456890) -- match m_szModel against the
// scanned list and replace it with that entry's full path.
int CPlayerProfileDlg::ModelIndexByName()
{
	mapinfo_t*	p = g_pModelList;

	_strlwr( m_szModel );
	strcat( m_szModel, ".bmp" );

	for ( int i = 0; i < m_modelCount; i++ )
	{
		if ( strstr( p->name, m_szModel ) )
		{
			strcpy( m_szModel, p->name );
			return i;
		}
		p = p->next;
	}
	return 0;
}

// CPlayerProfileDlg::ModelNameByIndex (0x456950)
const char* CPlayerProfileDlg::ModelNameByIndex( int idx )
{
	int		count = m_modelCount;
	char*	node  = (char*)g_pModelList;
	int		i = 0;

	if ( count <= 0 )
		return 0;
	while ( i != idx )
	{
		node = *(char**)( node + 788 );
		if ( ++i >= count )
			return 0;
	}
	return node;
}

// CPlayerProfileDlg::RefreshLogoPreview (0x456980)
void CPlayerProfileDlg::RefreshLogoPreview()
{
	RecolourLogoDib( m_colorCombo.CurrentSwatch() );
	::InvalidateRect( m_hWnd, &s_rcLogoPreview, FALSE );
}

// CPlayerProfileDlg::ApplyModelPalette (0x4569B0)
void CPlayerProfileDlg::ApplyModelPalette()
{
	HGLOBAL	hDib = m_hModelDib;
	if ( !hDib )
		return;

	BYTE*	pDib = (BYTE*)GlobalLock( hDib );
	BOOL	bInfo = ( *(DWORD*)pDib == 40 );	// BITMAPINFOHEADER vs core
	const BYTE*	pSrc = g_palModelWork;
	BYTE*	pTri  = pDib + 13;	// RGBTRIPLE palette (core header)
	BYTE*	pQuad = pDib + 41;	// RGBQUAD palette (info header)

	for ( int i = 0; i < 256; i++ )
	{
		BYTE	b = pSrc[0];
		BYTE	g = pSrc[1];
		BYTE	r = pSrc[2];
		if ( bInfo )
		{
			pQuad[1] = b;		// rgbGreen slot gets B per the binary's swap
			pQuad[0] = g;
			pQuad[-1] = r;
		}
		else
		{
			pTri[1] = b;
			pTri[0] = g;
			pTri[-1] = r;
		}
		pSrc += 3;
		pQuad += 4;
		pTri += 3;
	}

	GlobalUnlock( m_hModelDib );
}

// CPlayerProfileDlg::RefreshModelPreview (0x456A40)
void CPlayerProfileDlg::RefreshModelPreview()
{
	memcpy( g_palModelWork, g_palModelBase, 0x300 );
	Palette_FillHSVBand( g_palModelWork, g_palModelBase,
		m_modelHue1, 192, 223 );
	Palette_FillHSVBand( g_palModelWork, g_palModelBase,
		m_modelHue0, 160, 191 );
	ApplyModelPalette();
}

// CPlayerProfileDlg::OnCommand (0x456AB0)
BOOL CPlayerProfileDlg::OnCommand( WPARAM wParam, LPARAM lParam )
{
	if ( HIWORD( wParam ) == CBN_SELCHANGE && LOWORD( wParam ) == 135 )
		RefreshLogoPreview();

	return CWnd::OnCommand( wParam, lParam );
}

// AveragePixels (0x456AF0) -- the shipping build point-samples: it returns the
// block's first pixel and never looks at the count or the quantiser state.
static BYTE AveragePixels( int /*count*/ )
{
	return s_pixdata[0];
}

// Logo_BuildMipTex (0x456B00) -- lay out a WAD3 miptex (4 mip levels + a 256
// entry palette whose last colour is the picked swatch) for the logo DIB.
// Returns its byte length, or 0 when the DIB is not a multiple of 16 on both axes.
static int Logo_BuildMipTex( HGLOBAL hDib, miptex_t* pMip, const char* pszName,
	COLORREF clr, int* pWidth, int* pHeight )
{
	LPBITMAPINFOHEADER	pDib = (LPBITMAPINFOHEADER)GlobalLock( hDib );
	int		w = pDib->biWidth;
	int		h = pDib->biHeight;

	*pWidth  = w;
	*pHeight = h;

	const BYTE*	pBits = (const BYTE*)DIB_FindBits( pDib );
	if ( ( w & 15 ) || ( h & 15 ) )
		return 0;

	pMip->width  = w;
	pMip->height = h;
	strcpy( pMip->name, pszName );
	pMip->offsets[0] = sizeof( miptex_t );

	// mip 0: the DIB rows run bottom-up, the miptex runs top-down
	BYTE*	pMip0 = (BYTE*)pMip + sizeof( miptex_t );
	BYTE*	pOut  = pMip0;
	const BYTE*	pSrc = pBits + w * ( h - 1 );
	for ( int row = 0; row < h; row++ )
	{
		for ( int col = 0; col < w; col++ )
			*pOut++ = *pSrc++;
		pSrc -= 2 * w;
	}

	for ( int i = 0; i < 256; i++ )
	{
		for ( int c = 0; c < 3; c++ )
			s_palFloat[i][c] = (float)( (double)g_palLogoBase[i * 3 + c] * ( 1.0 / 255.0 ) );
	}

	s_numColors = 256;
	for ( int e = 0; e < 256; e++ )
		s_colorUsed[e] = 1;
	s_avgReserved = 0;

	for ( int level = 1; level < 4; level++ )
	{
		int	step = 1 << level;

		s_avgR = 0;
		s_avgG = 0;
		s_avgB = 0;
		pMip->offsets[level] = pOut - (BYTE*)pMip;

		int	srcRow = 0;
		for ( int y = 0; y < h; y += step )
		{
			for ( int x = 0; x < w; x += step )
			{
				int	n = 0;
				const BYTE*	pBlock = pMip0 + srcRow + x;
				for ( int by = 0; by < step; by++, pBlock += w )
				{
					for ( int bx = 0; bx < step; bx++ )
						s_pixdata[n++] = pBlock[bx];
				}
				*pOut++ = AveragePixels( n );
			}
			srcRow += step * w;
		}
	}

	GlobalUnlock( hDib );

	*(WORD*)pOut = 256;
	pOut += 2;
	memcpy( pOut, g_palLogoBase, 765 );
	pOut[765] = GetRValue( clr );
	pOut[766] = GetGValue( clr );
	pOut[767] = GetBValue( clr );

	return ( pOut + 768 ) - (BYTE*)pMip;
}

// Logo_WriteDecalWad (0x456D70) -- rebuild logos\pldecal.wad from the tinted logo
// DIB and copy it into the active game dir.  pszLogo is unused (sic): the lump is
// always called "LOGO".
static void Logo_WriteDecalWad( const char* /*pszLogo*/, HGLOBAL hDib, COLORREF clr )
{
	char	szLump[260];

	sprintf( szLump, "LOGO" );
	if ( !szLump[0] || !hDib )
		return;

	miptex_t*	pMip = (miptex_t*)new char[0x2800];
	memset( pMip, 0xEA, 0x2800 );

	int		w = 0;
	int		h = 0;
	DWORD	dwLen = Logo_BuildMipTex( hDib, pMip, szLump, clr, &w, &h );

	if ( w != h )
	{
		delete pMip;
		Launcher_ShowMessageById( 0, IDS_LOGO_SIZEMISMATCH );	// 441
		return;
	}
	if ( w > 64 )
	{
		delete pMip;
		Launcher_ShowMessageById( 0, IDS_LOGO_OVERSIZED );		// 442
		return;
	}
	if ( w != 16 && w != 32 && w != 64 )
	{
		delete pMip;
		Launcher_ShowMessageById( 0, IDS_LOGO_POWEROF2 );		// 443
		return;
	}

	while ( dwLen & 3 )
		dwLen++;

	SetFileAttributes( "logos\\pldecal.wad", FILE_ATTRIBUTE_NORMAL );
	HANDLE	hFile = CreateFile( "logos\\pldecal.wad", GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

	wadinfo_t	wad;
	memcpy( wad.identification, "WAD3", 4 );
	wad.numlumps     = 1;
	wad.infotableofs = 0;

	DWORD	dwWritten;
	WriteFile( hFile, &wad, sizeof( wad ), &dwWritten, NULL );

	lumpinfo_t	lump;
	memset( &lump, 0, sizeof( lump ) );
	strcpy( lump.name, szLump );
	lump.filepos  = dwWritten;
	lump.disksize = dwLen;
	lump.size     = dwLen;
	lump.type     = 0x40;			// TYP_MIPTEX

	WriteFile( hFile, pMip, dwLen, &dwWritten, NULL );
	WriteFile( hFile, &lump, sizeof( lump ), &dwWritten, NULL );

	SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
	wad.infotableofs = dwLen + sizeof( wadinfo_t );
	WriteFile( hFile, &wad, sizeof( wad ), &dwWritten, NULL );
	CloseHandle( hFile );
	delete pMip;

	char	szDst[260];
	sprintf( szDst, "%s\\pldecal.wad", com_gamedir );
	CopyFile( "logos\\pldecal.wad", szDst, FALSE );
}

// CPlayerProfileDlg::OnModelPrev / OnModelNext (0x456FC0 / 0x457090)
void CPlayerProfileDlg::OnModelPrev()
{
	int	idx = m_modelIdx;
	if ( idx <= 0 )
		return;

	m_modelIdx = idx - 1;
	strcpy( m_szModel, ModelNameByIndex( idx - 1 ) );
	m_hModelDib = LoadPreviewDib( 1, m_szModel );
	RefreshModelPreview();

	HDC	hdc = ::GetDC( m_hWnd );
	CDC*	pDC = CDC::FromHandle( hdc );
	PaintModelPreview( pDC->m_hDC );
	::ReleaseDC( m_hWnd, pDC->m_hDC );

	char	szBase[260];
	COM_FileBase( m_szModel, szBase );
	const char*	fmt = Launcher_LoadString( 0x1F7 );
	m_lblModel.SetWindowText(
		Launcher_FormatString( fmt, szBase ) );
}

void CPlayerProfileDlg::OnModelNext()
{
	int	idx = m_modelIdx;
	if ( idx >= m_modelCount - 1 )
		return;

	m_modelIdx = idx + 1;
	strcpy( m_szModel, ModelNameByIndex( idx + 1 ) );
	m_hModelDib = LoadPreviewDib( 1, m_szModel );
	RefreshModelPreview();

	HDC	hdc = ::GetDC( m_hWnd );
	CDC*	pDC = CDC::FromHandle( hdc );
	PaintModelPreview( pDC->m_hDC );
	::ReleaseDC( m_hWnd, pDC->m_hDC );

	char	szBase[260];
	COM_FileBase( m_szModel, szBase );
	const char*	fmt = Launcher_LoadString( 0x1F7 );
	m_lblModel.SetWindowText(
		Launcher_FormatString( fmt, szBase ) );
}

// CPlayerProfileDlg::OnLogoPrev / OnLogoNext (0x457220 / 0x4574A0)
void CPlayerProfileDlg::OnLogoPrev()
{
	if ( Eng_ShouldReload() )
	{
		CPromptDlg	dlg( 2, this );
		dlg.SetMessage( Launcher_LoadString( 0x1D6 ) );
		if ( dlg.DoModal() != 1 )
			return;
		/* resumeOnSwitch = 0;  -- static in cd_win.c, skipped */
		if ( engineapi.Cbuf_AddText )
			engineapi.Cbuf_AddText( "disconnect\n" );
		Eng_Frame( 0 );
	}

	int	idx = m_logoIdx;
	if ( idx <= 0 )
		return;

	m_logoIdx = idx - 1;
	strcpy( m_szLogo, LogoNameByIndex( idx - 1 ) );
	m_hLogoDib = LoadPreviewDib( 0, m_szLogo );
	RefreshLogoPreview();
}

void CPlayerProfileDlg::OnLogoNext()
{
	if ( Eng_ShouldReload() )
	{
		CPromptDlg	dlg( 2, this );
		dlg.SetMessage( Launcher_LoadString( 0x1D6 ) );
		if ( dlg.DoModal() != 1 )
			return;
		/* resumeOnSwitch = 0;  -- static in cd_win.c, skipped */
		if ( engineapi.Cbuf_AddText )
			engineapi.Cbuf_AddText( "disconnect\n" );
		Eng_Frame( 0 );
	}

	int	idx = m_logoIdx;
	if ( idx >= m_logoCount - 1 )
		return;

	m_logoIdx = idx + 1;
	strcpy( m_szLogo, LogoNameByIndex( idx + 1 ) );
	m_hLogoDib = LoadPreviewDib( 0, m_szLogo );
	RefreshLogoPreview();
}

void Palette_FillHSVBand( BYTE* pDst, BYTE* pSrc, int iHue, int yTop, int yBot )
{
	double	flHue = (double)iHue * 1.4117647058823529;	// 0..170 column -> 0..240 deg (dbl_4B2CC0)

	if ( yTop > yBot )									// 0x457748
		return;

	BYTE*	pOut  = pDst + 3 * yTop;					// esi: dest pixel (byte0 = B)
	BYTE*	pIn   = pSrc + 3 * yTop + 1;				// edi: src pixel (points at G)
	int		nBias = (int)( pDst - pSrc );				// ebx: dest/src delta

	for ( int n = yBot - yTop + 1; n; n-- )
	{
		double	flR = (double)pIn[1];					// src R  ([edi+1])
		double	flG = (double)pIn[0];					// src G  ([edi])
		double	flB = (double)pIn[-1];					// src B  ([edi-1])

		// flMax = max(R,G,B), flMin = min(R,G,B)  (the fcomp ladder at 0x457799..)
		double	flMax = ( flB <= flR ) ? flR : flB;
		if ( flMax <= flG )		flMax = flG;
		double	flMin = ( flB >= flR ) ? flR : flB;
		if ( flMin >= flG )		flMin = flG;

		double	flV     = flMax * ( 1.0 / 255.0 );						// flt_4B2CBC
		// chroma floor = (1 - (V - min/255)/V) * V
		double	flChroma = ( 1.0 - ( flV - flMin * ( 1.0 / 255.0 ) ) / flV ) * flV;

		double	flResR, flResG, flResB;

		// 6-sector sweep; flV is the channel at full value, flChroma the floor.
		if ( flHue <= 120.0 )								// flt_4B2CB8
		{
			flResB = flChroma;
			if ( flHue >= 60.0 )							// flt_4ACF20
			{
				flResR = flV;
				flResG = ( flChroma - flV ) * ( 120.0 - flHue ) / flHue + flChroma;
			}
			else
			{
				flResG = flV;
				flResR = ( flChroma - flV ) * flHue / ( 120.0 - flHue ) + flChroma;
			}
		}
		else if ( flHue > 240.0 )							// flt_4B2CB4
		{
			flResR = flChroma;
			if ( flHue <= 300.0 )							// flt_4B2CB0
			{
				flResB = flV;
				flResG = ( flChroma - flV ) * ( flHue - 240.0 ) / ( 300.0 - flHue ) + flChroma;
			}
			else
			{
				flResG = flV;
				flResB = ( flChroma - flV ) * ( 360.0 - flHue ) / ( flHue - 300.0 ) + flChroma;
			}
		}
		else												// 120 < hue <= 240
		{
			flResG = flChroma;
			if ( flHue <= 180.0 )							// flt_4B2CAC
			{
				flResB = flV;
				flResR = ( flChroma - flV ) * ( flHue - 120.0 ) / ( 180.0 - flHue ) + flChroma;
			}
			else
			{
				flResR = flV;
				flResB = ( flChroma - flV ) * ( flHue - 180.0 ) / ( 240.0 - flHue ) + flChroma;
			}
		}

		pOut[2]        = (BYTE)( flResR * 255.0 );		// [esi+2]      = R
		pIn[nBias]     = (BYTE)( flResG * 255.0 );		// [ebx+edi]    = G (dest byte1)
		pOut[0]        = (BYTE)( flResB * 255.0 );		// [esi]        = B

		pIn  += 3;
		pOut += 3;
	}
}

// CPlayerProfileDlg::SliderScrolled (0x4579F0)
void CPlayerProfileDlg::SliderScrolled( int nSBCode, int nPos, CObject* pObj )
{
	CODSlider*	pSlider = (CODSlider*)pObj;
	if ( !pSlider )
		return;

	HWND	hWnd = pSlider->GetSafeHwnd();

	if ( hWnd == ( m_pModelColor0 ? m_pModelColor0->GetSafeHwnd() : NULL ) )
	{
		if ( nSBCode == SB_ENDSCROLL || nSBCode == SB_THUMBTRACK )
		{
			m_modelHue1 = nPos;
			RefreshModelPreview();

			CDC*	pDC = CDC::FromHandle( ::GetDC( m_hWnd ) );
			PaintModelPreview( pDC->m_hDC );
			::ReleaseDC( m_hWnd, pDC->m_hDC );
		}
		return;
	}

	if ( hWnd == ( m_pModelColor1 ? m_pModelColor1->GetSafeHwnd() : NULL ) )
	{
		if ( nSBCode == SB_ENDSCROLL || nSBCode == SB_THUMBTRACK )
		{
			m_modelHue0 = nPos;
			RefreshModelPreview();

			CDC*	pDC = CDC::FromHandle( ::GetDC( m_hWnd ) );
			PaintModelPreview( pDC->m_hDC );
			::ReleaseDC( m_hWnd, pDC->m_hDC );
		}
		return;
	}

	if ( hWnd == ( m_pVoiceXmit ? m_pVoiceXmit->GetSafeHwnd() : NULL ) )
	{
		if ( nSBCode == SB_ENDSCROLL )
			m_flVoiceXmit = (float)nPos;
		return;
	}

	if ( hWnd == ( m_pVoiceRecv ? m_pVoiceRecv->GetSafeHwnd() : NULL ) )
	{
		if ( nSBCode == SB_ENDSCROLL )
			m_flVoiceRecv = (float)nPos;
	}
}

// CPlayerProfileDlg::OnSliderScroll (0x457B80) -- wParam is
// MAKEWPARAM(SB_code, pos), lParam the sender.
LRESULT CPlayerProfileDlg::OnSliderScroll( WPARAM wParam, LPARAM lParam )
{
	SliderScrolled( LOWORD( wParam ), HIWORD( wParam ), (CObject*)lParam );
	return 1;
}

// CPlayerProfileDlg::OnHiModels (0x457BB0)
void CPlayerProfileDlg::OnHiModels()
{
	m_bHiModels = m_chkHiModels.m_bChecked;
}

// CPlayerProfileDlg::OnVoiceEnable (0x457BC0)
void CPlayerProfileDlg::OnVoiceEnable()
{
	m_bVoiceEnable = m_chkVoice.m_bChecked;
}

/*
==================
DrawShadowFrame

The drop-shadow bevel the overlay draws round both preview boxes.  The binary
open-codes the four strips at each site.
==================
*/
static void DrawShadowFrame( CDC* pDC, const RECT& box )
{
	CBrush	br( RGB( 56, 56, 56 ) );	// 0x383838
	RECT	r;

	r = box;	r.bottom = r.top + 3;					pDC->FillRect( &r, &br );	// top
	r = box;	r.right  = r.left + 3;	r.top += 3;	r.bottom += 25;	pDC->FillRect( &r, &br );	// left
	r = box;	r.left   = r.right - 3;	r.top += 3;	r.bottom += 25;	pDC->FillRect( &r, &br );	// right
	r = box;	r.right -= 3;	r.left += 3;	r.top = r.bottom - 3;	pDC->FillRect( &r, &br );	// bottom
}

// CPlayerProfileDlg::DrawDialogOverlay (0x457bd0, slot +216)
void CPlayerProfileDlg::DrawDialogOverlay( CDC* pDC, RECT* prcClient )
{
	// 1) outer page bevel -- 3 frames, inflating outward, focus-tinted.
	RECT	rc = *prcClient;

	for ( int i = 0; i < 3; i++ )
	{
		// The focus test sits inside the loop, as the binary has it.
		COLORREF	clrFrame = ( CWnd::FromHandle( ::GetFocus() ) == this )
			? RGB( 128, 128, 128 ) : RGB( 56, 56, 56 );
		CBrush		br( clrFrame );

		::InflateRect( &rc, 1, 1 );		// inflate before framing
		pDC->FrameRect( &rc, &br );
	}

	// 2) logo preview box: drop-shadow bevel + black interior + centred logo DIB.
	DrawShadowFrame( pDC, s_rcLogoPreview );
	{
		RECT	rcDst = s_rcLogoPreview;
		::InflateRect( &rcDst, -3, -3 );
		::PatBlt( pDC->GetSafeHdc(), rcDst.left, rcDst.top,
			rcDst.right - rcDst.left, rcDst.bottom - rcDst.top, BLACKNESS );

		if ( m_hLogoDib )
		{
			void*	pDib = GlobalLock( m_hLogoDib );
			if ( pDib )
			{
				int	w = DIB_Width( (LPBITMAPINFOHEADER)pDib );
				int	h = DIB_Height( (LPBITMAPINFOHEADER)pDib );
				GlobalUnlock( m_hLogoDib );

				RECT	dst;
				dst.left   = rcDst.left + ( rcDst.right  - rcDst.left - w ) / 2;
				dst.top    = rcDst.top  + ( rcDst.bottom - rcDst.top  - h ) / 2;
				dst.right  = dst.left + w;
				dst.bottom = dst.top  + h;
				RECT	src = { 0, 0, w, h };
				DIB_BlitDib( pDC->GetSafeHdc(), &dst, m_hLogoDib, &src );
			}
		}
	}

	// 3) model preview box (unless the mod hides it): drop-shadow bevel + model DIB.
	if ( g_pCurrentMod )
	{
		const char*	p = g_pCurrentMod->GetKey( "nomodels" );
		if ( p && *p && atoi( p ) )
			return;
	}
	DrawShadowFrame( pDC, s_rcModelPreview );
	PaintModelPreview( pDC->GetSafeHdc() );

#ifdef LAUNCHER_RE
	Launcher_DrawBuildMarker( pDC );		// this slot replaces CDlgBase's, which draws it
#endif
}

// CPlayerProfileDlg::OnSetInfo (0x458160) -- the "Advanced" button: parse
// user.scr and run its option page as a child of this one.
void CPlayerProfileDlg::OnSetInfo()
{
	m_pUserDesc = new CInfoDescription;

	if ( !m_pUserDesc->InitFromFile( "user.scr" ) )
	{
		delete m_pUserDesc;
		m_pUserDesc = NULL;
		m_btnSetInfo.SetHighlight( 1 );
		return;
	}

	m_pUserDesc->TransferCurrentValues( g_szConfigName );

	CSetInfoDlg	page( m_pUserDesc, NULL );		// 0x462530
	InitChildDialog( &page, &m_btnSetInfo );
	page.DoModal();
	RestoreAfterModal();

	delete m_pUserDesc;
	m_pUserDesc = NULL;
}

// CPlayerProfileDlg::OnActivateApp (0x406FE0)
void CPlayerProfileDlg::OnActivateApp( BOOL bActive, DWORD /*dwThreadID*/ )
{
	ActiveApp = bActive;
	Default();
}

// CPlayerProfileDlg::OnPaint (0x412860)
void CPlayerProfileDlg::OnPaint()
{
	PaintSkinnedDialog();
}

// CPlayerProfileDlg::OnEraseBkgnd (0x412870)
BOOL CPlayerProfileDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

// CPlayerProfileDlg::OnActivate (0x455E00) -- folded with CHLMainDlg::OnSize.
void CPlayerProfileDlg::OnActivate( UINT /*nState*/, CWnd* /*pWndOther*/, BOOL /*bMinimized*/ )
{
	Default();
}

