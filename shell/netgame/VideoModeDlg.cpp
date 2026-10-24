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
// Purpose: the Video Modes dialog (CVideoModeDlg, IDD 0xD0 = 208) and
//          CODVideoList.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

BEGIN_MESSAGE_MAP( CVideoModeDlg, CDialog )
	//{{AFX_MSG_MAP(CVideoModeDlg)
	ON_MESSAGE( WM_DISPLAYCHANGE, &CVideoModeDlg::OnDisplayChange )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_COMMAND( IDC_VIDMODE_ADVANCED, OnAdvanced )
	ON_COMMAND( IDC_VIDMODE_MOUSE,    OnMouseCheck )
	ON_COMMAND( IDC_VIDMODE_WINDOWED, OnWindowedCheck )
	ON_WM_ACTIVATEAPP()
	ON_CONTROL( LBN_SELCHANGE, IDC_VIDMODE_TABS,     OnSelectRenderer )
	ON_CONTROL( LBN_SELCHANGE, IDC_VIDMODE_MODELIST, OnSelectMode )
	ON_CONTROL( LBN_SELCHANGE, IDC_VIDMODE_GLCOMBO,  OnSelectGLDriver )
	ON_COMMAND( IDC_VIDMODE_CANCEL,       OnCancel )
	ON_COMMAND( IDC_VIDMODE_3D_INFO_SITE, On3DInfoSite )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
==================
Vid_SetRendererFlags (0x46B760)
==================
*/
void Vid_SetRendererFlags( vidtype_t type )
{
	switch ( type )
	{
	case VT_Software:
		g_bVidGL = 0;
		g_bVidD3D = 0;
		break;

	case VT_OpenGL:
		g_bVidGL = 1;
		g_bVidD3D = 0;
		break;

	case VT_Direct3D:
		g_bVidGL = 0;
		g_bVidD3D = 1;
		break;

	default:
		g_bVidGL = 0;
		nummodes = 0;
		g_bVidD3D = 0;
		break;
	}
}
/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::CVideoModeDlg (0x46B7D0)

CVideoModeDlg::CVideoModeDlg( CWnd* pParent )
	: CDlgBase( IDD_VIDMODE, pParent )
{
	int	dims[2];
	int	i;

	for ( i = 0; i < 128; i++ )
	{
		memset( m_glDrivers[i].label, 0, sizeof( m_glDrivers[i].label ) );
		memset( m_glDrivers[i].help, 0, sizeof( m_glDrivers[i].help ) );
	}
	for ( i = 0; i < 128; i++ )
	{
		memset( m_d3dDevices[i].label, 0, sizeof( m_d3dDevices[i].label ) );
		memset( m_d3dDevices[i].help, 0, sizeof( m_d3dDevices[i].help ) );
	}

	m_pSelfWnd = this;
	m_nGLDrivers = 0;
	m_nD3DDevices = 0;
	m_bOpenGLAvail = FALSE;
	m_bD3DAvail = FALSE;
	m_savedVidRestart = force_mode_set;

	m_pModeList = NULL;
	m_pRendererTabs = NULL;
	m_pGLDriverCombo = NULL;
	m_pD3DDeviceCombo = NULL;
	m_nVidType = VT_Software;

	LoadHeaderBitmap( "head_vidmodes", NULL );
	m_unk41476 = 0;
	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnOK.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_OK, m_headerLoaded );
		m_btnCancel.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_BACK, m_headerLoaded );
		m_btn3DInfoSite.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_3D_INFO_SITE, m_headerLoaded );
	}

	strcpy( m_szGLDriver, "Default" );
	strcpy( m_szD3DDevice, "Default" );
	m_b3DWarning = Launcher_GetProfileInt( "Settings", "3DWarning", 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::~CVideoModeDlg (0x46BA80)

CVideoModeDlg::~CVideoModeDlg()
{
	Launcher_WriteProfileInt( "Settings", "3DWarning", m_b3DWarning );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::DoDataExchange (0x46BB70)

void CVideoModeDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_VIDMODE_3D_INFO_SITE, m_btn3DInfoSite );
	DDX_Control( pDX, IDC_VIDMODE_HINT,     m_lblHint );
	DDX_Control( pDX, IDC_VIDMODE_WINDOWED, m_checkWindowed );
	DDX_Control( pDX, IDC_VIDMODE_MOUSE,    m_checkMouse );
	DDX_Control( pDX, IDC_VIDMODE_ADVANCED, m_lblAdvanced );
	DDX_Control( pDX, IDC_VIDMODE_CANCEL,   m_btnCancel );
	DDX_Control( pDX, IDOK,                 m_btnOK );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnSelectMode (0x46BC10)
//
// The mode list reports a new pick; mirror the chosen vmode_t's ordinal and
// geometry into the edit state.

void CVideoModeDlg::OnSelectMode()
{
	vmode_t*	pMode;
	int			sel, i;

	sel = m_pModeList->GetCurSel();
	if ( sel < 0 )
		return;

	pMode = (vmode_t*)m_pModeList->GetItemData( sel );
	if ( !pMode )
		return;

	for ( i = 0; i < nummodes; i++ )
		if ( pMode == &modelist[i] )
			break;
	if ( i >= nummodes )
		i = 0;

	m_nMode   = i;
	m_nWidth  = pMode->width;
	m_nHeight = pMode->height;
	m_nBPP    = pMode->bpp;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::AddModeColumns (0x46BC80)

void CVideoModeDlg::AddModeColumns( RECT* prcList )
{
	odcolumn_t	col;

	Launcher_LoadStringInto( col.title, IDS_VIDEO_MODECOL );
	col.width = 150;
	m_pModeList->AddColumn( &col );

	sprintf( col.title, "" );
	col.width = ( prcList->right - prcList->left ) - 150;
	m_pModeList->AddColumn( &col );

	m_pModeList->ShowWindow( SW_RESTORE );
	m_pModeList->SetDrawFrame( 1 );
	m_pModeList->SetTransparent( FALSE );
	m_pModeList->SetHeaderTransparent( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnInitDialog (0x46BD30)

BOOL CVideoModeDlg::OnInitDialog()
{
	RECT		rc;
	int			dims[2];
	int			w, h, right, listX, listW, listBottom;
	int			cancelPad, hintPad, infoPad;
	int			i, n;
	long		style;
	const char*	pszTab;
	const char*	pszRow;
	HMODULE		hGlide;

	CDialog::OnInitDialog();
	m_bWasWindowed = gEngineModeWindowed;
	m_nEngineVidType = gEngineVidType;
	Dlg_CenterWindow( this );

	// Edit against the current engine-mode descriptor (and refresh its
	// windowed/captured mirror from the live launcher state).
	m_nVidType = (vidtype_t)g_EngineMode.vidtype;
	m_nWidth   = g_EngineMode.width;
	m_nHeight  = g_EngineMode.height;
	m_nBPP     = g_EngineMode.bpp;
	g_EngineMode.windowed = gEngineModeWindowed;
	m_bWindowed = gEngineModeWindowed;
	g_EngineMode.captured = windowed_mouse;
	m_bWindowedMouse = windowed_mouse;

	rc.left = 0;
	rc.top = 0;
	rc.right = 100;
	rc.bottom = 100;

	m_pModeList = new CODVideoList();
	m_pModeList->Create( 0, rc, this, IDC_VIDMODE_MODELIST );

	// The renderer tab strip.
	m_pRendererTabs = new CODTabCtrl;
	m_pRendererTabs->Create( 0, &rc, this, IDC_VIDMODE_TABS );
	m_pRendererTabs->EnableStackedTabs( 0 );
	m_pRendererTabs->AddTab( "&Software" );
	if ( Vid_OpenGLSupported() )
	{
		m_bOpenGLAvail = TRUE;
		m_pRendererTabs->AddTab( "Open&GL" );
	}
	if ( Vid_D3DSupported() )
	{
		m_bD3DAvail = TRUE;
		m_pRendererTabs->AddTab( "&Direct3D" );
	}

	// The two driver drop lists.
	m_pGLDriverCombo = new CODDriverComboBox;
	m_pGLDriverCombo->Create( 0, &rc, this, IDC_VIDMODE_GLCOMBO );
	m_pGLDriverCombo->SetDropHeight( 75 );
	m_pD3DDeviceCombo = new CODDriverComboBox;
	m_pD3DDeviceCombo->Create( 0, &rc, this, IDC_VIDMODE_D3DCOMBO );
	m_pD3DDeviceCombo->SetDropHeight( 75 );

	// Layout: OK / Cancel down the left edge, tabs + mode list to the right.
	Launcher_HeaderSize( dims );
	w = dims[0];
	h = dims[1];
	right = g_nLauncherDefW - 50;
	listBottom = g_nLauncherDefH - h - 80;

	m_btnOK.MoveWindow( 50, 140, w, h, TRUE );
	SetWindowTextSafe( &m_btnOK, Launcher_LoadString( IDS_BTN_OK ) );

	listX = w + 90;
	listW = right - listX;
	m_pRendererTabs->MoveWindow( listX, 140, listW, h, TRUE );

	cancelPad = Launcher_StringHeight( IDS_VIDEOMODEDLG_OFFSET, 0 );
	m_btnCancel.MoveWindow( 50, 172, cancelPad + w, h, TRUE );
	SetWindowTextSafe( &m_btnCancel, Launcher_LoadString( IDS_BTN_CANCEL ) );

	rc.left = listX;
	rc.top = 172;
	rc.right = right;
	rc.bottom = listBottom;
	m_pModeList->MoveWindow( listX, 172, listW, listBottom - 172, TRUE );
	AddModeColumns( &rc );

	// The windowed / mouse check boxes ride under the mode list.
	rc.top = rc.bottom + 10;
	rc.right = 410;
	rc.bottom = rc.top + h;
	m_checkWindowed.MoveWindow( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );
	OffsetRect( &rc, 0, 32 );
	m_checkMouse.MoveWindow( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );

	// The "advanced" label and the two driver combos down the left column.
	m_lblAdvanced.MoveWindow( 56, 204, w, h, TRUE );
	rc.left = 56;
	rc.top = 231;
	rc.right = w + 56;
	rc.bottom = 301;
	m_pGLDriverCombo->MoveTo( &rc, 1 );
	m_pD3DDeviceCombo->MoveTo( &rc, 1 );

	// The help text (HELP_COLOR, 9pt) sits below, pushed to the z-bottom.
	hintPad = Launcher_StringHeight( IDS_GERMAN, 0 );
	m_lblHint.MoveWindow( 56, 331, w + 30 * hintPad, 148, TRUE );
	m_lblHint.SetTransparent( TRUE );
	m_lblHint.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_lblHint.SetFontSize( 9, FW_NORMAL );
	m_lblHint.SetWindowPos( &wndBottom, 0, 0, 0, 0,
		SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOSIZE );

	infoPad = Launcher_StringHeight( IDS_VIDEOMODEDLG_OFFSET, 1 );
	m_btn3DInfoSite.MoveWindow( infoPad + 480, 415, w, h, TRUE );

	// Select the tab matching the current renderer.
	n = m_pRendererTabs->GetTabCount();
	for ( i = 0; i < n; i++ )
	{
		pszTab = m_pRendererTabs->GetTabText( i );

		if ( !pszTab || !pszTab[0] )
			continue;
		if ( m_nVidType == VT_OpenGL && !_strcmpi( pszTab, "Open&GL" ) )
			break;
		if ( m_nVidType == VT_Direct3D && !_strcmpi( pszTab, "&Direct3D" ) )
			break;
		if ( m_nVidType != VT_OpenGL && m_nVidType != VT_Direct3D &&
			 !_strcmpi( pszTab, "&Software" ) )
			break;
	}
	m_pRendererTabs->SetCurSel( i < n ? i : 0, 1 );

	// Fill the mode list (matching the current w/h) and the driver tables.
	RebuildModeList( g_EngineMode.width, g_EngineMode.height );
	UpdateForRenderer( m_nVidType - 1 );

	m_nMode = g_EngineMode.mode;
	if ( m_nMode >= nummodes )
		m_nMode = 0;

	LoadGLDrivers();
	AddDriverRow( m_pD3DDeviceCombo, m_d3dDevices, &m_nD3DDevices, "Default", "Default" );

	// Current GL driver: the saved one, else 3dfxgl when a glide2x board is
	// present, else the system default.
	strcpy( m_szGLDriver, g_EngineMode.glDriver );
	if ( !m_szGLDriver[0] )
	{
		hGlide = LoadLibraryA( "glide2x" );
		if ( hGlide )
		{
			FreeLibrary( hGlide );
			strcpy( m_szGLDriver, "3dfxgl.dll" );
		}
		else
		{
			strcpy( m_szGLDriver, "Default" );
		}
	}

	strcpy( m_szD3DDevice, g_EngineMode.d3dDevice );
	if ( !m_szD3DDevice[0] )
		strcpy( m_szD3DDevice, "Default" );

	// Select the current GL driver by name (fall back to the first row).
	n = m_pGLDriverCombo->GetCount();
	for ( i = 0; i < n; i++ )
	{
		pszRow = m_pGLDriverCombo->GetString( i );

		if ( pszRow && !_strcmpi( m_szGLDriver, pszRow ) )
			break;
	}
	m_pGLDriverCombo->SetCurSel( i < n ? i : 0 );
	m_pD3DDeviceCombo->SetCurSel( 0 );

	// Software only?  Hide the renderer tabs altogether.
	if ( !m_bOpenGLAvail && !m_bD3DAvail )
		m_pRendererTabs->ShowWindow( SW_HIDE );

	// Seed the check boxes from the snapshot and repaint them owner-draw.
	m_checkWindowed.m_bChecked = m_bWindowed;
	::InvalidateRect( m_checkWindowed.GetSafeHwnd(), NULL, TRUE );
	m_checkMouse.m_bChecked = m_bWindowedMouse;
	::InvalidateRect( m_checkMouse.GetSafeHwnd(), NULL, TRUE );
	UpdateMouseCheck( m_bWindowed );

	style = m_checkWindowed.GetStyle() | BS_OWNERDRAW;
	SetWindowLongA( m_checkWindowed.GetSafeHwnd(), GWL_STYLE, style );
	style = m_checkMouse.GetStyle() | BS_OWNERDRAW;
	SetWindowLongA( m_checkMouse.GetSafeHwnd(), GWL_STYLE, style );
	SetWindowTextSafe( &m_checkWindowed, Launcher_LoadString( IDS_VIDMODE_WINDOWED ) );
	SetWindowTextSafe( &m_checkMouse, Launcher_LoadString( IDS_VIDMODE_MOUSE ) );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnSelectRenderer (0x46C5F0)

void CVideoModeDlg::OnSelectRenderer()
{
	int			iTab, iRenderer;
	const char*	pszTab;

	iTab = m_pRendererTabs->GetCurSel();
	if ( (UINT)iTab > 2 )
		return;

	pszTab = m_pRendererTabs->GetTabText( iTab );
	if ( !pszTab || !pszTab[0] )
		return;

	if ( !_strcmpi( pszTab, "Open&GL" ) )
	{
		if ( !m_b3DWarning )
		{
			CPromptDlg	dlg( 2 );		// OK + Cancel pair

			dlg.SetMessage( Launcher_LoadString( IDS_3D_WARNING ) );
			dlg.SetPromptSize( 320, 200 );
			dlg.SetTextAlign( DT_LEFT );
			if ( dlg.DoModal() != IDOK )
			{
				// Declined: snap back to Software.
				m_nVidType = VT_Software;
				m_pRendererTabs->SetCurSel( 0, 1 );
				return;
			}
		}
		m_b3DWarning = TRUE;
		iRenderer = 1;
		SetRenderer( VT_OpenGL );
	}
	else if ( !_strcmpi( pszTab, "&Direct3D" ) )
	{
		if ( !m_b3DWarning )
		{
			CPromptDlg	dlg( 2 );

			dlg.SetMessage( Launcher_LoadString( IDS_3D_WARNING ) );
			dlg.SetPromptSize( 320, 200 );
			dlg.SetTextAlign( DT_LEFT );
			if ( dlg.DoModal() != IDOK )
				return;
		}
		m_b3DWarning = TRUE;
		iRenderer = 2;
		SetRenderer( VT_Direct3D );
	}
	else
	{
		SetRenderer( VT_Software );
		iRenderer = 0;
	}

	force_mode_set = TRUE;
	UpdateForRenderer( iRenderer );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::SetRenderer (0x46CAA0)

void CVideoModeDlg::SetRenderer( vidtype_t type )
{
	m_nVidType = type;
	Vid_SetRendererFlags( type );
	RebuildModeList( 0, 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnOK (0x46CAD0)

void CVideoModeDlg::OnOK()
{
	CWinThread*	pThread;
	CWnd*		pMain;
	LONG		exStyle;
	int			bValid = FALSE;
	int			bWindowed;

	switch ( m_nVidType )
	{
	case VT_Software:
		bValid = Vid_TrySetMode( "software", VT_Software, m_nMode, m_nWidth, m_nHeight, m_nBPP );
		break;
	case VT_OpenGL:
		bValid = Vid_TrySetMode( m_szGLDriver, VT_OpenGL, m_nMode, m_nWidth, m_nHeight, m_nBPP );
		break;
	case VT_Direct3D:
		bValid = Vid_TrySetMode( m_szD3DDevice, VT_Direct3D, m_nMode, m_nWidth, m_nHeight, m_nBPP );
		break;
	}

	if ( !bValid )
	{
		CPromptDlg	dlg( 2 );		// OK + Cancel pair

		dlg.SetMessage( Launcher_LoadString( IDS_VIDEO_BADSETTINGS ) );
		if ( dlg.DoModal() != IDOK )
			return;					// keep editing
	}

	// Commit into the engine-mode descriptor.
	gEngineVidType = m_nVidType;
	Vid_SetRendererFlags( gEngineVidType );
	vid_modenum = m_nMode;
	g_EngineMode.mode = vid_modenum;
	g_EngineMode.vidtype = gEngineVidType;
	g_EngineMode.width = m_nWidth;
	g_EngineMode.height = m_nHeight;
	strcpy( g_EngineMode.typeName, Eng_VidTypeName( m_nVidType ) );
	strcpy( g_EngineMode.glDriver, m_szGLDriver );
	strcpy( g_EngineMode.d3dDevice, m_szD3DDevice );

	bWindowed = m_bWindowed != 0;
	g_EngineMode.windowed = bWindowed;
	g_EngineMode.captured = bWindowed ? ( m_bWindowedMouse != 0 ) : 1;
	windowed_mouse = g_EngineMode.captured;

	// Persist.
	Launcher_WriteProfileInt( "Settings", "EngineType", gEngineVidType );
	Launcher_WriteProfileInt( "Settings", "EngineMode", vid_modenum );
	Launcher_WriteProfileInt( "Settings", "EngineModeW", m_nWidth );
	Launcher_WriteProfileInt( "Settings", "EngineModeH", m_nHeight );
	Launcher_WriteProfileInt( "Settings", "EngineModeBPP", m_nBPP );
	Launcher_WriteProfileInt( "Settings", "EngineModeWindowed", bWindowed );
	Launcher_WriteProfileInt( "Settings", "EngineModeCaptured", m_bWindowedMouse != 0 );
	Launcher_WriteProfileString( "Settings", "EngineGLDriver", m_szGLDriver );
	Launcher_WriteProfileString( "Settings", "EngineD3DDevice", m_szD3DDevice );

	// Windowed state flipped: put the launcher display back.
	if ( bWindowed != m_bWasWindowed )
	{
		Launcher_RestoreAfterEngine( bWindowed );
		if ( gEngineModeWindowed )
			ChangeDisplaySettingsA( NULL, 0 );
		else if ( lpDD )
			DDraw_SetDisplayMode( -1 );
		else
			VID_ChangeDisplaySettings( g_nLauncherDefW, g_nLauncherDefH, 16 );
	}

	// The launcher frame floats topmost only while fullscreen.
	pThread = AfxGetThread();
	if ( pThread )
	{
		pMain = pThread->GetMainWnd();
		if ( pMain )
		{
			exStyle = GetWindowLongA( pMain->m_hWnd, GWL_EXSTYLE );
			if ( !bWindowed )
				exStyle |= WS_EX_TOPMOST;
			else
				exStyle &= ~WS_EX_TOPMOST;
			SetWindowLongA( pMain->m_hWnd, GWL_EXSTYLE, exStyle );
		}
	}

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::RebuildModeList (0x46CFC0)
//
// Refill the display-mode list from modelist with the modes this renderer can
// use, and re-select the one matching wCur x hCur (or the row that shares its
// width; failing both, the first row).

void CVideoModeDlg::RebuildModeList( int wCur, int hCur )
{
	vmode_t*	pMode;
	vmode_t*	pSel;
	const char*	pszType;
	char		szDesc[128];
	int			nWantType, nRow, nSel, i;

	Vid_SetRendererFlags( m_nVidType );

	if ( !wCur )
	{
		pSel = (vmode_t*)m_pModeList->GetItemData( m_pModeList->GetCurSel() );
		wCur = pSel ? pSel->width : 400;
	}

	m_pModeList->ResetContent();

	// OpenGL and the windowed path both list the windowed modes.
	nWantType = ( m_bWindowed || m_nVidType == VT_OpenGL ) ? MS_WINDOWED : MS_FULLSCREEN;

	nRow = 0;
	nSel = -1;

	for ( i = 0; i < nummodes; i++ )
	{
		pMode = &modelist[i];
		if ( pMode->type != nWantType )
			continue;
#ifdef LAUNCHER_FIXES
		if ( !Vid_ModeAllowedForRenderer( m_nVidType, pMode->width, pMode->height ) )
			continue;
#endif

		// (sic) the description is built and then dropped.
		sprintf( szDesc, "%d x %d %d hz", pMode->width, pMode->height, pMode->refresh );

		if ( pMode->width == wCur )
		{
			nSel = nRow;
			if ( pMode->height == hCur )
			{
				wCur = -1;
				hCur = -1;
			}
		}

		switch ( pMode->type )
		{
		case MS_WINDOWED:	pszType = "Windowed";			break;
		case MS_FULLSCREEN:	pszType = "Full Screen";		break;
		case MS_FULLDIB:	pszType = "Full Screen DIB";	break;
		default:			pszType = "Error";				break;
		}
		strcat( szDesc, pszType );

		m_pModeList->AddRow( pMode );
		nRow++;
	}

#ifdef LAUNCHER_FIXES
	// nSel counts the rows this renderer accepted, so it has to be bounded by
	// nRow, not by the unfiltered mode count.  Harmless while the filter drops
	// almost nothing, wrong once the list is enumerated from the driver.
	if ( nSel == -1 || nSel >= nRow )
#else
	if ( nSel == -1 || nSel >= nummodes )
#endif
		m_pModeList->SelectItem( 0, 1 );
	else
		m_pModeList->SelectItem( nSel, 1 );

	m_pModeList->RefitScrollbar();
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnCancel (0x46D1A0)
//
// Put the engine mode back as it was found.

void CVideoModeDlg::OnCancel()
{
	gEngineModeWindowed = m_bWasWindowed;
	gEngineVidType      = m_nEngineVidType;
	CDialog::OnCancel();
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::UpdateForRenderer (0x46D1C0)
//
// Show the driver combo + advanced text that belong to the picked renderer tab
// (0 software / 1 OpenGL / 2 Direct3D) and hide the others.

void CVideoModeDlg::UpdateForRenderer( int iTab )
{
	const char*	psz;
	int			nCount, nSel, i;

	if ( iTab == -1 )
		iTab = 0;
	else if ( (UINT)iTab > 2 )
		return;

	if ( iTab == 0 )
	{
		m_lblAdvanced.ShowWindow( SW_HIDE );
		m_lblHint.ShowWindow( SW_HIDE );
		m_pGLDriverCombo->ShowWindow( SW_HIDE );
		m_pD3DDeviceCombo->ShowWindow( SW_HIDE );
	}
	else if ( iTab == 1 )
	{
		m_pD3DDeviceCombo->ShowWindow( SW_HIDE );

		m_lblAdvanced.ShowWindow( SW_RESTORE );
		m_lblAdvanced.SetWindowText( Launcher_LoadString( IDS_VIDMODE_GLLISTHEADER ) );
		m_lblHint.ShowWindow( SW_RESTORE );
		m_lblHint.SetWindowText( Launcher_LoadString( IDS_VID_HINT ) );
		m_lblHint.SetWindowPos( &wndBottom, 0, 0, 0, 0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOCOPYBITS );

		// Re-select the configured driver by name.
		if ( m_szGLDriver[0] )
		{
			nCount = m_pGLDriverCombo->GetCount();
			nSel   = 0;
			if ( nCount > 0 )
			{
				for ( i = 0; i < nCount; i++ )
				{
					psz = m_pGLDriverCombo->GetString( i );
					if ( psz && !_strcmpi( m_szGLDriver, psz ) )
						break;
				}
				if ( i < nCount )
					nSel = i;
			}
			m_pGLDriverCombo->SetCurSel( nSel );
			m_pGLDriverCombo->ShowWindow( SW_RESTORE );
		}
	}
	else
	{
		m_pGLDriverCombo->ShowWindow( SW_HIDE );

		m_lblAdvanced.ShowWindow( SW_RESTORE );
		m_lblAdvanced.SetWindowText( Launcher_LoadString( IDS_VIDMODE_D3DHEADER ) );
		m_lblHint.ShowWindow( SW_HIDE );

		_strcmpi( m_szD3DDevice, "default" );		// (sic) result unused
		m_pD3DDeviceCombo->SetCurSel( 0 );
		m_pD3DDeviceCombo->ShowWindow( SW_RESTORE );
	}

	if ( Launcher_GetProfileInt( "Settings", "EngineAllowWindowed", 0 ) )
	{
		m_checkMouse.ShowWindow( SW_RESTORE );
		m_checkWindowed.ShowWindow( SW_RESTORE );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnSelectGLDriver (0x46D3C0)

void CVideoModeDlg::OnSelectGLDriver()
{
	const char*	psz;
	int			sel;

	sel = m_pGLDriverCombo->GetCurSel();
	if ( sel == -1 )
		return;

	psz = m_pGLDriverCombo->GetString( sel );
	if ( !psz )
		return;

	strcpy( m_szGLDriver, psz );
	force_mode_set = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::LoadGLDrivers (0x46D420)
//
// One "<dll> <description>" line per driver in gldrv\drvmap.txt; with no file
// the combo gets a single "Default" row.

void CVideoModeDlg::LoadGLDrivers()
{
	FILE*	fp;
	long	len;
	char*	pFile;
	char*	p;
	char*	pTok;
	int		n;
	char	szName[32];
	char	szDesc[128];

	fp = fopen( "gldrv\\drvmap.txt", "rb" );
	if ( !fp )
	{
		AddDriverRow( m_pGLDriverCombo, m_glDrivers, &m_nGLDrivers, "Default", "Default" );
		return;
	}

	fseek( fp, 0, SEEK_END );
	len = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	if ( len > 0 )
	{
		pFile = new char[len + 1];
		memset( pFile, 0, len + 1 );
		fread( pFile, len, 1, fp );

		p = pFile;
		do
		{
			pTok = p;
			while ( *p && *p != ' ' )
				++p;
			n = (int)( p - pTok );
			strncpy( szName, pTok, n );
			++p;
			szName[n] = 0;

			pTok = p;
			while ( *p && *p != '\r' && *p != '\n' )
				++p;
			n = (int)( p - pTok );
			strncpy( szDesc, pTok, n );
			szDesc[n] = 0;

			AddDriverRow( m_pGLDriverCombo, m_glDrivers, &m_nGLDrivers, szName, szDesc );

			while ( *p && ( *p == '\r' || *p == '\n' ) )
				++p;
		}
		while ( *p );

		delete[] pFile;
	}

	fclose( fp );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnMouseCheck (0x46D5A0)

void CVideoModeDlg::OnMouseCheck()
{
	m_bWindowedMouse = m_checkMouse.m_bChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnWindowedCheck (0x46D5B0)

void CVideoModeDlg::OnWindowedCheck()
{
	m_bWindowed = m_checkWindowed.m_bChecked;
	UpdateMouseCheck( m_bWindowed );
	RebuildModeList( 0, 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::UpdateMouseCheck (0x46D5E0)
//
// The windowed pair only exists when EngineAllowWindowed is set; the mouse box
// is greyed while fullscreen.

void CVideoModeDlg::UpdateMouseCheck( int bWindowed )
{
	if ( Launcher_GetProfileInt( "Settings", "EngineAllowWindowed", 0 ) )
	{
		m_checkWindowed.ShowWindow( SW_SHOW );
		m_checkWindowed.SetHighlight( 0 );
		m_checkMouse.ShowWindow( SW_SHOW );
		m_checkMouse.SetHighlight( bWindowed ? 0 : 1 );
	}
	else
	{
		m_checkWindowed.ShowWindow( SW_HIDE );
		m_checkWindowed.SetHighlight( 1 );
		m_checkMouse.ShowWindow( SW_HIDE );
		m_checkMouse.SetHighlight( 1 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::AddDriverRow (0x46D680)

void CVideoModeDlg::AddDriverRow( CODDriverComboBox* pCombo, vidmodedesc_t* pRows,
	int* pnRows, const char* pszLabel, const char* pszDesc )
{
	vidmodedesc_t*	pRow;
	int				iRow;

	if ( !pCombo )
		return;

	iRow = *pnRows;
	if ( iRow >= 128 )
		return;
	if ( !pszLabel || !*pszLabel || !pszDesc || !*pszDesc )
		return;

	pRow = &pRows[iRow];
	if ( !pRow )
		return;

	*pnRows = iRow + 1;
	strcpy( pRow->label, pszLabel );
	strcpy( pRow->help, pszDesc );
	pCombo->AddItem( pRow );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::On3DInfoSite (0x46D720)

void CVideoModeDlg::On3DInfoSite()
{
	char	szUrl[256];

	if ( !Launcher_LoadStringInto( szUrl, IDS_3DSITE_URL ) )
		return;

	if ( (INT_PTR)::ShellExecuteA( gLauncherWnd, "open", szUrl,
			NULL, NULL, SW_SHOWMAXIMIZED ) <= 32 )
		Launcher_ShowMessageByIdEx( 0, IDS_URL_BROWSERFAIL, szUrl );
}

/////////////////////////////////////////////////////////////////////////////
// CODVideoList::DrawRow (0x451A50)
//
// Each row record is the vmode_t itself, so the width/height come straight off
// the mode; the second column is deliberately blank.

void CODVideoList::DrawRow( CDC* pDC, int iRow )
{
	RECT		client, rc, cell;
	odrow_t*	pRow;
	vmode_t*	pMode;
	CFont*		pOldFont;
	char		buf[64];
	int			vis, bSel;

	GetClientRect( &client );
	if ( m_bHasScrollbar )
		client.right -= 16;

	vis = iRow - m_topRow;
	if ( vis < 0 )
		return;

	pRow = m_rows[iRow];
	pMode = (vmode_t*)pRow->record;
	if ( !pMode )
		return;

	sprintf( buf, "%i x %i", pMode->width, pMode->height );

	bSel = pRow->flags & 1;
	rc.left   = client.left;
	rc.right  = client.right;
	rc.top    = vis * m_rowHeight;
	rc.bottom = m_rowHeight + vis * m_rowHeight - 1;

	pDC->SetBkColor( m_clrRowBg );
	if ( !m_bTransparent )
		pDC->FillRect( &rc, bSel ? &m_brHighlight : &m_brBg );
	pDC->SetTextColor( bSel ? m_clrSelText : m_clrRowText );
	pDC->SetBkMode( TRANSPARENT );

	pOldFont = pDC->SelectObject( &m_headerFont );

	cell.left   = 2;
	cell.top    = vis * m_rowHeight;
	cell.right  = m_cols[0].width;
	cell.bottom = m_rowHeight + vis * m_rowHeight - 1;
	pDC->DrawText( CODList_EllipsizeText( pDC, buf, m_cols[0].width, 2 ), -1, &cell,
		DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER );

	sprintf( buf, "" );
	cell.left   = m_cols[0].width + 2;
	cell.top    = vis * m_rowHeight;
	cell.right  = m_cols[1].width + m_cols[0].width;
	cell.bottom = m_rowHeight + vis * m_rowHeight - 1;
	pDC->DrawText( CODList_EllipsizeText( pDC, buf, m_cols[1].width, 2 ), -1, &cell,
		DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER );

	pDC->SelectObject( pOldFont );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnActivateApp (0x406FE0)

void CVideoModeDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnAdvanced (0x40E460)
//
// Folded onto the shared empty stub.

void CVideoModeDlg::OnAdvanced()
{
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnPaint (0x412860)

void CVideoModeDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnEraseBkgnd (0x412870)

BOOL CVideoModeDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoModeDlg::OnDisplayChange (0x497E09)
//
// The binary binds the entry straight to MFC 4.2's own
// CWnd::OnDisplayChange( WPARAM, LPARAM ).  A modern MFC spells that
// handler void( UINT, int, int ), so it cannot be bound here; this
// forwarder reaches the same default handling.

LRESULT CVideoModeDlg::OnDisplayChange( WPARAM, LPARAM )
{
	return Default();
}
