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
// Purpose: the Load Game dialog (CLoadDlg, IDD 0x9F = 159).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::CLoadDlg (0x425850)

CLoadDlg::CLoadDlg( CWnd* pParent )
	: CDlgBase( IDD_LOADGAME, pParent )
{
	int	dims[2];

	m_pList   = NULL;
	m_pSaves  = NULL;					// +980 -- must be NULL'd; PopulateSaves frees it
	m_nSaves  = 0;						// +984
	m_clrText = RGB( 255, 255, 255 );	// 0xFFFFFF
	m_clrBk   = RGB( 63, 63, 63 );		// 0x3F3F3F
	m_bkBrush.CreateSolidBrush( m_clrBk );

	m_pSelfWnd = this;		// +204 -- the page points the slide at itself
	LoadHeaderBitmap( "head_load", 0 );
	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnLoad.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_ROW_EVEN, m_headerLoaded );
		m_btnBack.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_BACK, m_headerLoaded );
		m_btnDelete.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_LOAD_SAVE, m_headerLoaded );
	}
}

BEGIN_MESSAGE_MAP( CLoadDlg, CDialog )
	ON_MESSAGE( WM_DISPLAYCHANGE, &CLoadDlg::OnDisplayChange )
	ON_COMMAND( IDC_LOADGAME_LOAD_SAVED_GAME, OnLoad )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_COMMAND( IDC_LOADGAME_DELETE_GAME, OnDelete )
	ON_CONTROL( 1, IDC_LOADGAME_DELETE_GAME, UpdateButtonStates )
	ON_CONTROL( 2, IDC_LOADGAME_DELETE_GAME, OnLoad )
	ON_WM_DRAWITEM()
	ON_WM_NCPAINT()
	ON_WM_ACTIVATEAPP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::DoDataExchange (0x4259C0)

void CLoadDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_LOADGAME_DELETE_GAME, m_btnDelete );
	DDX_Control( pDX, IDCANCEL,    m_btnBack );
	DDX_Control( pDX, IDC_LOADGAME_LOAD_SAVED_GAME, m_btnLoad );
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::OnInitDialog (0x425A10)

BOOL CLoadDlg::OnInitDialog()
{
	RECT		rc;
	odcolumn_t	col;
	int			dims[2];
	int			w, h, right, dateW;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	// The save-game list control (shares CODListCtrl base, save-game vtable).
	m_pList = new CODSaveGameListCtrl();
	rc.left = 0; rc.top = 0; rc.right = 100; rc.bottom = 100;
	m_pList->Create( 0, rc, this, 1021 );
	m_pList->SetHeaderTransparent( 1 );
	m_pList->SetTransparent( 0 );
	m_pList->SetDrawFrame( 1 );
	PopulateSaves();

	Launcher_HeaderSize( dims );			// the shared button-cell size {w,h}
	w = dims[0];
	h = dims[1];
	right = g_nLauncherDefW - 50;

	m_btnLoad.MoveWindow( 50, 140, w, h, TRUE );
	SetWindowTextSafe( &m_btnLoad, Launcher_LoadString( IDS_BTN_LOAD ) );
	m_pList->MoveWindow( w + 60, 140, right - ( w + 60 ), ( g_nLauncherDefH - 50 ) - 140, TRUE );

	// Columns: name, date, map (third stretches to fill).
	Launcher_LoadStringInto( col.title, IDS_SAVELOAD_TIMECOL );
	col.width = 100;
	m_pList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_SAVELOAD_GAMECOL );
	dateW = Launcher_StringHeight( IDS_LOAD_OFFSET, 0 );
	col.width = 175 - dateW;
	m_pList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_SAVELOAD_ELAPSEDCOL );
	col.width = dateW + right - ( w + 60 ) - 275;
	m_pList->AddColumn( &col );

	m_btnDelete.MoveWindow( 50, 172, w, h, TRUE );
	SetWindowTextSafe( &m_btnDelete, Launcher_LoadString( IDS_BTN_DELETE ) );
	m_btnBack.MoveWindow( 50, 204, Launcher_StringHeight( IDS_LOAD_OFFSET, 1 ) + w, h, TRUE );
	SetWindowTextSafe( &m_btnBack, Launcher_LoadString( IDS_BTN_CANCEL ) );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::~CLoadDlg (0x425C80)

CLoadDlg::~CLoadDlg()
{
	if ( m_pSaves )
	{
		delete[] m_pSaves;
		m_pSaves = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::OnLoad (0x425D40)

void CLoadDlg::OnLoad()
{
	int	row = m_pList->GetCurSel();
	if ( row == -1 )
		return;

	savegame_t*	pRec = &m_pSaves[row];
	if ( !pRec )
		return;

	char		szCmd[256];
	GameInfo_t	gi;

	// Loading over a live session has to drop the connection first, and that
	// costs the player their game -- so it is confirmed.
	if ( engineapi.GetGameInfo( &gi, 0 )
	  && gi.state != ca_disconnected
	  && gi.state != ca_dedicated )
	{
		CPromptDlg	dlg( 2 );		// OK + Cancel
		dlg.SetMessage( Launcher_LoadString( IDS_LOAD_LOADPROMPT ) );
		if ( dlg.DoModal() != IDOK )
			return;

		sprintf( szCmd, "disconnect\nload %s\n", pRec->filename );
	}
	else
	{
		sprintf( szCmd, "load %s\n", pRec->filename );
	}

	AFXSetTopLevelFrame( 1 );
	Launcher_RunMapCommand( szCmd );
	OnOK();							// CDialog::OnOK -> EndDialog( IDOK )  (vtable slot +196)
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::WindowProc (0x426000)
//
// vftable +152; forwards to CWnd.

LRESULT CLoadDlg::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	return CWnd::WindowProc( message, wParam, lParam );
}

/*
==================
Launcher_ParseSaveFile (0x426020)
==================
*/
int Launcher_ParseSaveFile( char* lpFileName, const char* pszSaveName, savegame_t* rec )
{
	if ( !lpFileName || !pszSaveName || !rec )
		return 0;

	for ( char* q = lpFileName; *q; q++ )		// normalise slashes
		if ( *q == '/' )
			*q = '\\';

	strcpy( rec->filename, pszSaveName );

	FILETIME	ftWrite = { 0, 0 };
	HANDLE		h = CreateFileA( lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( h != INVALID_HANDLE_VALUE )
	{
		GetFileTime( h, NULL, NULL, &ftWrite );
		CloseHandle( h );
	}

	FILE*	f = fopen( lpFileName, "rb" );
	char	mapname[64] = { 0 };
	char	comment[128] = { 0 };
	if ( !f || !Save_ParseGameHeader( f, mapname, comment ) )
	{
		if ( f )
			fclose( f );
		return 0;
	}
	fclose( f );

	strncpy( rec->mapname, mapname, 0x20 );
	rec->mapname[31] = 0;
	strncpy( rec->comment, comment, 0x50 );
	rec->comment[79] = 0;

	if ( strstr( lpFileName, "quick" ) )
		rec->bQuicksave = 1;
	if ( strstr( lpFileName, "autosave" ) )
		rec->bAutosave = 1;

	// Split the trailing elapsed-time token (last 5 chars) off the comment.
	int	len = (int)strlen( rec->comment );
	strcpy( rec->elapsed, "??" );
	if ( len >= 6 )
	{
		strncpy( rec->elapsed, rec->comment + len - 5, 5 );
		rec->elapsed[5] = 0;
		rec->comment[len - 5] = 0;
		for ( int i = len - 6; i >= 1; i-- )	// strip trailing spaces
		{
			if ( rec->comment[i] != ' ' )
				break;
			rec->comment[i] = 0;
		}
	}

	rec->fileTime = ftWrite;
	if ( ftWrite.dwLowDateTime && ftWrite.dwHighDateTime )
	{
		CTime	t( ftWrite, -1 );
		CString	s = t.Format( "%b %d, %I:%M %p" );
		if ( !s.IsEmpty() )
			strncpy( rec->date, s, 0x1F );
		else
			strcpy( rec->date, "??" );
	}
	else
	{
		strcpy( rec->date, "??" );
	}
	rec->date[31] = 0;

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::PopulateSaves (0x426350)

void CLoadDlg::PopulateSaves()
{
	// Clear the current rows + free the previous save array.
	m_pList->ResetContent();
	if ( m_pSaves )
	{
		delete[] m_pSaves;
		m_pSaves = NULL;
	}
	m_nSaves = 0;

	char*	dir = Save_GetSaveDir();
	char	pattern[300];
	sprintf( pattern, "%s*.sav", dir );

	// First pass: count the eligible saves.
	WIN32_FIND_DATAA	fd;
	int					count = 0;
	HANDLE				hFind = FindFirstFileA( pattern, &fd );
	if ( hFind != INVALID_HANDLE_VALUE )
	{
		do
		{
			if ( _strnicmp( fd.cFileName, "HLSave", 6 ) )
				count++;
		} while ( FindNextFileA( hFind, &fd ) );
		FindClose( hFind );
	}

	if ( count > 0 )
	{
		m_pSaves = new savegame_t[count];
		memset( m_pSaves, 0, sizeof( savegame_t ) * count );

		// Second pass: parse each save into the array.
		int		n = 0;
		hFind = FindFirstFileA( pattern, &fd );
		if ( hFind != INVALID_HANDLE_VALUE )
		{
			do
			{
				if ( _strnicmp( fd.cFileName, "HLSave", 6 ) )
				{
					char	full[300];
					sprintf( full, "%s%s", dir, fd.cFileName );
					if ( Launcher_ParseSaveFile( full, fd.cFileName, &m_pSaves[n] ) )
						n++;
				}
			} while ( FindNextFileA( hFind, &fd ) );
			FindClose( hFind );
		}
		m_nSaves = n;

		// Sort newest-first by last-write time.
		for ( int i = 0; i < m_nSaves; i++ )
			for ( int j = i + 1; j < m_nSaves; j++ )
				if ( CompareFileTime( &m_pSaves[i].fileTime, &m_pSaves[j].fileTime ) < 0 )
				{
					savegame_t	tmp = m_pSaves[i];
					m_pSaves[i] = m_pSaves[j];
					m_pSaves[j] = tmp;
				}

		// Add a 3-column row per save (Time / Game / Elapsed); the text stays
		// alive in m_pSaves.
		for ( int i = 0; i < m_nSaves; i++ )
		{
			m_pList->AddRow( &m_pSaves[i] );
		}
	}

	m_pList->SelectItem( 0, 1 );

	// Enable Load/Delete only when there is at least one save.
	int	bNone = ( m_nSaves <= 0 );
	m_btnLoad.SetHighlight( bNone );
	m_btnDelete.SetHighlight( bNone );
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::UpdateButtonStates (0x426700)
//
// the list's selection-changed notify.

void CLoadDlg::UpdateButtonStates()
{
	int	row = m_pList->GetCurSel();
	if ( row == -1 )
	{
		m_btnLoad.SetHighlight( 1 );
		return;
	}

	savegame_t*	pRec = &m_pSaves[row];
	if ( pRec )
		m_btnLoad.SetHighlight( 0 );
	else
		m_btnLoad.SetHighlight( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::OnDelete (0x426760)

void CLoadDlg::OnDelete()
{
	int	row = m_pList->GetCurSel();
	if ( row == -1 )
		return;

	savegame_t*	pRec = &m_pSaves[row];
	if ( !pRec )
		return;

	CPromptDlg	dlg( 2 );			// OK + Cancel
	dlg.SetMessage( Launcher_LoadString( IDS_LOADSAVE_DELETEPROMPT ) );
	if ( dlg.DoModal() != IDOK )
		return;

	char	szPath[260];
	sprintf( szPath, "%s%s", Save_GetSaveDir(), pRec->filename );
	_unlink( szPath );

	PopulateSaves();
	::UpdateWindow( m_pList->GetSafeHwnd() );
	m_pList->SelectItem( 0, 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::OnNcPaint (0x4269D0)

void CLoadDlg::OnNcPaint()
{
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::OnPaint (0x412860)

void CLoadDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::OnEraseBkgnd (0x412870)

BOOL CLoadDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::OnActivateApp (0x406FE0)

void CLoadDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CLoadDlg::OnDisplayChange (0x453D00)

LRESULT CLoadDlg::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}

// Read one whitespace-separated token (skipping // comments).  Returns false at end.
/*
==================
Launcher_GetPlayerName (0x4269E0)

Read gfx/shell/kb_keys.lst into a key-name table, then report which keys the
bindings block has "save quick" and "load quick" on.
==================
*/
int Launcher_GetPlayerName( cfg_keybind_t* pBindings, char* pSaveKey, char* pLoadKey )
{
	char		keyName[256][32];
	char		name[64];
	char*		file;
	char*		p;
	int			count, err, i;
	CToken		tok( 0 );

	memset( keyName, 0, sizeof( keyName ) );
	strcpy( pSaveKey, "<Not assigned to key>" );
	strcpy( pLoadKey, "<Not assigned to key>" );

	file = (char*)COM_LoadMallocFile( "gfx/shell/kb_keys.lst" );
	if ( !file )
	{
		Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_EMPTY );
		PostQuitMessage( 0 );
		return 0;
	}

	tok.SetData( file );
	tok.SetQuoteMode( 1 );

	count = 0;
	err   = 0;
	for ( p = keyName[0]; ; p += 32 )
	{
		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			break;
		if ( count >= 256 )
		{
			Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_OVERFLOW );
			err = 1;
			break;
		}

		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			goto parseerr;
		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			goto parseerr;

		strcpy( name, tok.token );
		if ( _strnicmp( name, "<UNK", 4 ) )
			strcpy( p, name );

		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
		{
parseerr:
			Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_PARSEERROR );
			err = 1;
			break;
		}

		// A colour triple may follow the action; the values are read and dropped.
		if ( !_strnicmp( tok.token, "COLOR", 5 ) )
		{
			tok.ParseNextToken();	atoi( tok.token );
			tok.ParseNextToken();	atoi( tok.token );
			tok.ParseNextToken();	atoi( tok.token );
		}

		++count;
	}

	free( file );
	if ( err )
		return 0;

	for ( i = 0; i < 256; i++ )
	{
		if ( pBindings[i].m_pszBind && *pBindings[i].m_pszBind )
		{
			if ( strstr( pBindings[i].m_pszBind, "save quick" ) )
				strcpy( pSaveKey, keyName[i] );
			if ( strstr( pBindings[i].m_pszBind, "load quick" ) )
				strcpy( pLoadKey, keyName[i] );
		}
	}

	return 1;
}
