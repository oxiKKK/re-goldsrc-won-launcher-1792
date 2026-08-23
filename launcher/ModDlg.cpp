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
// Purpose: the Custom Game / mod chooser page (CModDlg, IDD 234).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Entries at 0x4AFA20, base map 0x4B4398 = CDialog.
BEGIN_MESSAGE_MAP( CModDlg, CDialog )
	//{{AFX_MSG_MAP(CModDlg)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED( IDC_CUSTOMGAME_REFRESH_LIST, OnRefreshList )
	ON_BN_CLICKED( IDC_CUSTOMGAME_ACTIVATE, OnActivate )
	ON_BN_CLICKED( IDC_CUSTOMGAME_IINSTALL, OnInstall )
	ON_BN_CLICKED( IDC_CUSTOMGAME_DEACTIVATE, OnDeactivate )
	ON_BN_CLICKED( IDC_CUSTOMGAME_VIST_MOD_SITE, OnVisitModSite )
	ON_CONTROL( LBN_SELCHANGE, IDC_CUSTOMGAME_MODLIST, OnSelChangeList )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModDlg::CModDlg (0x42A840)

CModDlg::CModDlg( CWnd* pParent )
	: CDlgBase( IDD_CUSTOMGAME, pParent )
{
	m_pSelfWnd = this;		// gates the slide transition
	m_bReady   = 0;
	m_pList    = NULL;
	m_pMods    = NULL;
	InitMembers();
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::InitMembers (0x42A930)
//
// Slice the six buttons out of the loaded strip.  Every face is freed first,
// so a re-entry after a skin change re-slices rather than reusing the blend.

void CModDlg::InitMembers()
{
	int	wh[2];

	LoadHeaderBitmap( "head_custom", NULL );

	m_hHeaderDIB = Launcher_HeaderLoaded();
	Launcher_HeaderSize( wh );
	m_headerW      = wh[0];
	m_headerH      = wh[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_hHeaderDIB )
	{
		CSize	cell( m_headerW, m_headerH );

		m_btnDone.FreeSkinBitmaps();
		m_btnDone.SetDIBData( cell, BTNSTRIP_DONE, m_hHeaderDIB );
		m_btnActivate.FreeSkinBitmaps();
		m_btnActivate.SetDIBData( cell, BTNSTRIP_ACTIVATE, m_hHeaderDIB );
		m_btnInstall.FreeSkinBitmaps();
		m_btnInstall.SetDIBData( cell, BTNSTRIP_INSTALL, m_hHeaderDIB );
		m_btnVisitModSite.FreeSkinBitmaps();
		m_btnVisitModSite.SetDIBData( cell, BTNSTRIP_VISIT_MOD_SITE, m_hHeaderDIB );
		m_btnRefreshList.FreeSkinBitmaps();
		m_btnRefreshList.SetDIBData( cell, BTNSTRIP_REFRESH_LIST, m_hHeaderDIB );
		m_btnDeactivate.FreeSkinBitmaps();
		m_btnDeactivate.SetDIBData( cell, BTNSTRIP_DEACTIVATE, m_hHeaderDIB );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::~CModDlg (0x42AA50)

CModDlg::~CModDlg()
{
	FreeMods();
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::DoDataExchange (0x42AB00)

void CModDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_CUSTOMGAME_VIST_MOD_SITE, m_btnVisitModSite );
	DDX_Control( pDX, IDC_CUSTOMGAME_DEACTIVATE,    m_btnDeactivate );
	DDX_Control( pDX, IDC_CUSTOMGAME_ACTIVATE,      m_btnActivate );
	DDX_Control( pDX, IDC_CUSTOMGAME_REFRESH_LIST,  m_btnRefreshList );
	DDX_Control( pDX, IDC_CUSTOMGAME_IINSTALL,      m_btnInstall );
	DDX_Control( pDX, IDOK,                         m_btnDone );
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::OnInitDialog (0x42AB90)
//
// The eight-column mod list fills the right-hand panel; the six skinned
// buttons stack down the left, 32px apart, with a gap before Deactivate.

BOOL CModDlg::OnInitDialog()
{
	int			wh[2];
	RECT		rc;
	odcolumn_t	col;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	Launcher_HeaderSize( wh );

	int	w = wh[0] + 10 * Launcher_StringHeight( IDS_GERMAN, 0 );
	int	h = wh[1];

	m_pList = new CODModListCtrl();
	rc.left   = w - 15;
	rc.top    = 140;
	rc.right  = g_nLauncherDefW - 20;
	rc.bottom = g_nLauncherDefH - 35;
	m_pList->Create( 0, rc, this, IDC_CUSTOMGAME_MODLIST );
	m_pList->MoveWindow( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );

	m_pList->SetRowHeight( 30 );
	m_pList->SetSortKey( "Mod Sort Order" );

	Launcher_LoadStringInto( col.title, IDS_MODLIST_TYPE );
	col.width = 55;
	m_pList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_MODLIST_NAME );
	col.width = 120;
	m_pList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_MODLIST_VERSION );
	col.width = 50;
	m_pList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_MODLIST_SIZE );
	col.width = 50;
	m_pList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_MODLIST_RATING );
	col.width = 50;
	m_pList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_MODLIST_INSTALLED );
	col.width = 50;
	m_pList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_MODLIST_SERVERS );
	col.width = 50;
	m_pList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_MODLIST_PLAYERS );
	col.width = 50;
	m_pList->AddColumn( &col );

	m_pList->SetTransparent( 0 );
	m_pList->SetHeaderTransparent( 1 );
	m_pList->SetDrawFrame( 1 );
	m_pList->SetHighlight( RGB( 56, 56, 56 ) );
	m_pList->ShowWindow( SW_RESTORE );

	int	btnRight = w - 25;

	m_btnActivate.MoveWindow( 15, 140, btnRight - 15, h, TRUE );
	SetWindowTextSafe( &m_btnActivate, Launcher_LoadString( IDS_BTN_ACTIVATE ) );
	m_btnInstall.MoveWindow( 15, 172, btnRight - 15, h, TRUE );
	SetWindowTextSafe( &m_btnInstall, Launcher_LoadString( IDS_BTN_INSTALL ) );
	m_btnVisitModSite.MoveWindow( 15, 204, w - 35, h, TRUE );
	SetWindowTextSafe( &m_btnVisitModSite, Launcher_LoadString( IDS_BTN_VISIT ) );
	m_btnRefreshList.MoveWindow( 15, 236, btnRight - 15, h, TRUE );
	SetWindowTextSafe( &m_btnRefreshList, Launcher_LoadString( IDS_BTN_REFRESHMODS ) );
	m_btnDeactivate.MoveWindow( 15, 300, btnRight - 15, h, TRUE );
	SetWindowTextSafe( &m_btnDeactivate, Launcher_LoadString( IDS_BTN_DEACTIVATE ) );
	m_btnDone.MoveWindow( 15, 332, btnRight - 15, h, TRUE );
	SetWindowTextSafe( &m_btnDone, Launcher_LoadString( IDS_BTN_DONE ) );

	m_bReady = 1;
	PopulateList();
	UpdateButtons();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::FreeMods (0x42B050)

void CModDlg::FreeMods()
{
	mod_t*	m = m_pMods;

	while ( m )
	{
		mod_t*	next = m->next;
		m->FreeKeys();
		delete m;
		m = next;
	}
	m_pMods = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::PopulateList (0x42B0A0)
//
// Deep-copy the scanned mod list; the page edits its copy, never the global.

void CModDlg::PopulateList()
{
	for ( mod_t* src = g_pModList; src; src = src->next )
	{
		mod_t*	copy = ( new mod_t )->Init();

		for ( modkey_t* k = src->keys; k; k = k->next )
			copy->SetKey( k->key, k->value );

		copy->next = m_pMods;		// prepend
		m_pMods    = copy;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::RefreshList (0x42B140)
//
// Two queries: the custom-game masters supply the catalogue, then the
// Half-Life master fills in the live server and player counts.

void CModDlg::RefreshList()
{
	if ( !m_pList )
		return;

	m_pList->ResetContent();

	mod_t*		pFetched = NULL;
	CModReqDlg	dlgFetch( TRUE, &pFetched, NULL );		// custom-master mode

	dlgFetch.DoModal();

	FreeMods();
	PopulateList();

	for ( mod_t* m = pFetched; m; )
	{
		mod_t*	next     = m->next;
		BOOL	bPrepend = TRUE;

		// Already in the page's copy?  Merge its keys in and drop the fetched
		// node.
		mod_t*	existing = ModList_FindByGamedir( &m_pMods, m->GetKeyString( "gamedir" ) );
		if ( existing )
		{
			for ( modkey_t* k = m->keys; k; k = k->next )
				existing->SetKey( k->key, k->value );
			m->FreeKeys();
			delete m;
			m        = existing;
			bPrepend = FALSE;
		}

		// Flag against the locally installed copy.
		mod_t*	inst = ModList_FindByGamedir( &g_pModList, m->GetKeyString( "gamedir" ) );
		if ( inst )
		{
			m->SetKey( "installed", "1" );
			if ( inst->GetKeyInt( "version" ) < m->GetKeyInt( "version" ) )
				m->SetKey( "newversion", "1" );
		}

		if ( bPrepend )
		{
			m->next = m_pMods;
			m_pMods = m;
		}
		m = next;
	}

	// Reset the live counts before the Half-Life master query refills them.
	for ( mod_t* m = m_pMods; m; m = m->next )
	{
		m->SetKey( "servers", "0" );
		m->SetKey( "players", "0" );
	}

	CModReqDlg	dlgCounts( FALSE, &m_pMods, NULL );		// Half-Life master mode

	dlgCounts.DoModal();

	UpdateButtons();
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::UpdateButtons (0x42B350)
//
// Refill the list from the page's copy and re-sort it.

void CModDlg::UpdateButtons()
{
	if ( !m_pList )
		return;

	m_pList->ResetContent();

	for ( mod_t* m = m_pMods; m; m = m->next )
	{
		const char*	gamedir = m->GetKeyString( "gamedir" );
		if ( _stricmp( gamedir, "valve" ) )			// skip the base game
			m_pList->AddRow( m );			// the row record is the mod
	}

	m_pList->SortRows( (odrowcmp_t)ModList_CompareKeys, -1 );
	m_pList->SelectItem( 0, 1 );
	m_pList->RefitScrollbar();
	m_pList->UpdateScrollbar( 1 );
	UpdateButtonStates();
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::OnRefreshList (0x42B3F0)

void CModDlg::OnRefreshList()
{
	RefreshList();
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::SwitchToMod (0x42B400)
//
// Make pMod the active game, or NULL to fall back to the base game.

void CModDlg::SwitchToMod( mod_t* pMod )
{
	if ( g_pCurrentMod == pMod )
		return;

	GameInfo_t	gi;
	if ( engineapi.GetGameInfo( &gi, 0 ) && gi.state == ca_active && gi.signon )
	{
		CPromptDlg	dlg( 2, NULL );		// OK + Cancel

		dlg.SetMessage( Launcher_LoadString( IDS_NEWGAME_NEWPROMPT ) );
		if ( dlg.DoModal() != IDOK )
			return;

		if ( engineapi.Cbuf_AddText )
		{
			gBackground = 1;
			engineapi.Cbuf_AddText( "disconnect\n" );
			Eng_Frame( 1 );
		}
	}

	g_pCurrentMod = pMod;
	Launcher_SavePlayerInfo();

	if ( pMod )
	{
		const char*	gamedir = pMod->GetKeyString( "gamedir" );
		Sys_SetCmdLineParm( "-game", gamedir );
		COM_ResetGameDirectories();
		COM_AddGameDirectory( 0, COM_GetBaseDir(), pMod->GetKeyString( "gamedir" ) );
	}
	else
	{
		Sys_StripCmdLineParm( "-game" );
		COM_ResetGameDirectories();
	}

	Launcher_OnGameDirChanged();

	( (CHLMainDlg*)GetParent() )->RefreshDialogSkin();
	( (CHLMainDlg*)GetParent() )->LayoutMainMenu( 0, 1 );
	( (CHLMainDlg*)GetParent() )->InvalidateRect( NULL, TRUE );
	( (CHLMainDlg*)GetParent() )->UpdateWindow();

	InitMembers();
	InvalidateRect( NULL, TRUE );
	UpdateWindow();
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::OnActivate (0x42B710)

void CModDlg::OnActivate()
{
	int	sel = m_pList->GetCurSel();
	if ( sel == -1 )
		return;

	mod_t*	pMod = (mod_t*)m_pList->GetItemData( sel );
	if ( !pMod )
		return;

	// Must be installed locally to activate.
	mod_t*	pInst = pMod->GetKeyInt( "installed" )
		? ModList_FindByGamedir( &g_pModList, pMod->GetKeyString( "gamedir" ) )
		: NULL;
	if ( !pInst )
	{
		Launcher_ShowMessageByIdEx( NULL, IDS_MOD_NOTINSTALLED, pMod->GetKeyString( "game" ) );
		return;
	}

	// A newer version is available -- confirm before activating the old one.
	if ( pMod->GetKeyInt( "newversion" ) )
	{
		CPromptDlg	dlg( 2, NULL );		// OK + Cancel

		dlg.SetMessage( Launcher_LoadString( IDS_MOD_VERSION ), pMod->GetKey( "game" ) );
		if ( dlg.DoModal() != IDOK )
			return;
	}

	SwitchToMod( pInst );
	UpdateButtonStates();
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::OnInstall (0x42B9E0)
//
// Confirm, show the download agreement, run the transport the URL names, then
// fold the mod into the global list and tell a master it was installed.

void CModDlg::OnInstall()
{
	if ( !g_pModList )
		return;

	int	sel = m_pList->GetCurSel();
	if ( sel == -1 )
		return;

	mod_t*	pMod = (mod_t*)m_pList->GetItemData( sel );
	if ( !pMod )
		return;

	// Already installed and up to date?  Confirm a re-download first.
	if ( pMod->GetKeyInt( "installed" ) && !pMod->GetKeyInt( "newversion" ) )
	{
		CPromptDlg	dlg( 2, NULL );		// OK + Cancel

		dlg.SetMessage( Launcher_LoadString( IDS_MOD_REINSTALL ), pMod->GetKey( "game" ) );
		if ( dlg.DoModal() != IDOK )
			return;
	}

	// The one-time download agreement, shown until the user ticks it off.
	int	bPrompt = Launcher_GetProfileInt( "Settings", "Download Prompt", 1 );
	Launcher_WriteProfileInt( "Settings", "Download Prompt", bPrompt );
	if ( bPrompt )
	{
		char	szPath[MAX_PATH];

		sprintf( szPath, "download.txt" );

		char*	pszText = (char*)COM_LoadMallocFile( szPath );
		if ( !pszText )
			return;

		CPromptDlg	dlg( 0x80000002, NULL );		// OK + Cancel, with the title band

		dlg.SetMessage( pszText );
		free( pszText );
		dlg.SetCheckboxShown( 0 );
		dlg.SetPromptSize( 500, 450 );
		dlg.SetTextAlign( 0 );
		dlg.SetMessageFont( 11, FW_NORMAL );
		dlg.SetCheckboxText( Launcher_LoadString( IDS_WARN_CHECKPROMPT ) );
		dlg.SetTitle( Launcher_LoadString( IDS_WARN_TITLE ) );
		if ( dlg.DoModal() != IDOK )
			return;
		if ( dlg.IsCheckboxChecked() == 1 )
			Launcher_WriteProfileInt( "Settings", "Download Prompt", 0 );
	}

	// Crack url_dl -> pick the download transport.
	const char*	pszUrl = pMod->GetKeyString( "url_dl" );
	DWORD		dwService = AFX_INET_SERVICE_FTP;		// no "//" -> a bare FTP path

	if ( strstr( pszUrl, "//" ) )
	{
		CString			strServer, strObject;
		INTERNET_PORT	nPort = 0;

		if ( !AfxParseURL( pszUrl, dwService, strServer, strObject, nPort ) )
			return;
	}

	BOOL	bOk = FALSE;
	if ( dwService == AFX_INET_SERVICE_FTP )
	{
		CModDownloadDlg*	pDlg = new CModDownloadDlg( pMod, NULL );
		if ( !pDlg )
			return;
		if ( pDlg->DoModal() == IDOK )
			bOk = TRUE;
		delete pDlg;
	}
	else if ( dwService == AFX_INET_SERVICE_HTTP )
	{
		CModHttpDownloadDlg*	pDlg = new CModHttpDownloadDlg( pMod, NULL );
		if ( !pDlg )
			return;
		if ( pDlg->DoModal() == IDOK )
			bOk = TRUE;
		delete pDlg;
	}
	else
	{
		return;		// any other scheme: not handled, silently abort
	}

	if ( !bOk )
		return;

	// Post-download bookkeeping: mark installed, fold the keys into the global
	// mod list, creating a node if this gamedir is new.
	pMod->SetKey( "installed", "1" );
	pMod->SetKey( "newversion", "0" );

	mod_t*	pNew = ModList_FindByGamedir( &g_pModList, pMod->GetKeyString( "gamedir" ) );
	BOOL	bCreated = FALSE;

	if ( !pNew )
	{
		bCreated = TRUE;
		pNew = ( new mod_t )->Init();
	}
	if ( pNew )
	{
		for ( modkey_t* kv = pMod->keys; kv; kv = kv->next )
			pNew->SetKey( kv->key, kv->value );
		if ( bCreated )
		{
			pNew->next = g_pModList;
			g_pModList = pNew;
		}
	}

	// Tell the first custom-game master that answers; the count it keeps is
	// what feeds the list's "installs" ranking.
	{
		AFX_MANAGE_STATE( AfxGetModuleState() );

		gFavorites->BeginModList();
		while ( gFavorites->NextModList() )
		{
			CHLModSocket*	pSock = new CHLModSocket( NULL,
				gFavorites->GetModAddr(), gFavorites->GetModPort() );
			if ( !pSock )
				break;
			if ( pSock->StartList() )
			{
				pSock->SendInstallNotify( pMod->GetKeyInt( "*uniqueid" ) );
				delete pSock;
				break;
			}
			delete pSock;
		}

		::InvalidateRect( m_pList->m_hWnd, NULL, TRUE );
		::UpdateWindow( m_pList->m_hWnd );
		if ( pNew )
			SwitchToMod( pNew );
		UpdateButtonStates();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::OnDeactivate (0x42C230)

void CModDlg::OnDeactivate()
{
	if ( !g_pCurrentMod )
		return;

	if ( g_pCurrentMod == g_pValveMod )
	{
		g_pCurrentMod = NULL;
		return;
	}

	SwitchToMod( NULL );
	UpdateButtonStates();
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::OnVisitModSite (0x42C270)

void CModDlg::OnVisitModSite()
{
	if ( !g_pModList )
		return;

	int	sel = m_pList->GetCurSel();
	if ( sel == -1 )
		return;

	mod_t*	mod = (mod_t*)m_pList->GetItemData( sel );
	if ( !mod )
		return;

	const char*	url = mod->GetKey( "url_info" );
	if ( !url || !*url )
		return;

	if ( (INT_PTR)::ShellExecuteA( ::GetFocus(), "open", url, NULL, NULL, SW_SHOWNORMAL )
		<= (INT_PTR)HINSTANCE_ERROR )
		Launcher_ShowMessageByIdEx( NULL, IDS_URL_BROWSERFAIL, url );
	else
		ShowWindow( SW_MINIMIZE );
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::OnSelChangeList (0x42C2F0)

void CModDlg::OnSelChangeList()
{
	if ( m_bReady && g_pModList )
		UpdateButtonStates();
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::UpdateButtonStates (0x42C310)
//
// Dim each button against the selected mod: Deactivate needs a non-base game,
// Visit needs a usable url_info, Install a usable url_dl.

void CModDlg::UpdateButtonStates()
{
	// The !m_pList test is ours -- the binary only checks m_bReady, and a
	// selection change can reach here before OnInitDialog created the list.
	if ( !m_bReady || !m_pList )
		return;

	int	sel = m_pList->GetCurSel();

	m_btnDeactivate.SetHighlight( ( !g_pCurrentMod || g_pCurrentMod == g_pValveMod ) ? 1 : 0 );

	if ( sel == -1 )
	{
		m_btnVisitModSite.SetHighlight( 1 );
		m_btnActivate.SetHighlight( 1 );
		m_btnInstall.SetHighlight( 1 );
		return;
	}

	mod_t*	mod = (mod_t*)m_pList->GetItemData( sel );
	if ( mod )
	{
		const char*	info = mod->GetKey( "url_info" );
		m_btnVisitModSite.SetHighlight( ( info && *info && _stricmp( info, "none" ) ) ? 0 : 1 );

		const char*	dl = mod->GetKey( "url_dl" );
		if ( dl && *dl && _stricmp( dl, "none" ) )
		{
			m_btnInstall.SetHighlight( 0 );
			m_btnActivate.SetHighlight( 0 );
			return;
		}
	}
	else
	{
		m_btnVisitModSite.SetHighlight( 1 );
	}

	m_btnInstall.SetHighlight( 1 );
	m_btnActivate.SetHighlight( 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::OnPaint (0x412860)

void CModDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CModDlg::OnEraseBkgnd (0x412870)

BOOL CModDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}
