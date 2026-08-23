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
// Purpose: the Save Game dialog (CSaveDlg, IDD 203, "head_save").
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

#define SAVEGAME_FOURCC		1447121738						// 'VALV'
#define SAVEGAME_VERSION	113

static int	Sav_FindFreeSlot( void );

#define IDC_SAVE_LIST		106			// the owner-draw save-game list control

BEGIN_MESSAGE_MAP( CSaveDlg, CDialog )
	ON_MESSAGE( WM_DISPLAYCHANGE, &CSaveDlg::OnDisplayChange )
	ON_CONTROL( 2, IDC_SAVE_LIST, OnSave )						// list activate
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED( IDC_MAIN_LOAD_GAME, OnDelete )				// 1021 "Save" caption
	ON_CONTROL( 1, IDC_SAVE_LIST, UpdateButtonStates )			// list selchange
	ON_BN_CLICKED( IDC_MAIN_RETURN_TO_GAME, OnSave )			// 1019 "Delete" caption
	ON_WM_DRAWITEM()
	ON_WM_NCPAINT()
	ON_WM_ACTIVATEAPP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::CSaveDlg (0x45DD70)

CSaveDlg::CSaveDlg( CWnd* pParent )
	: CDlgBase( IDD_SAVE, pParent )
{
	int	dims[2];

	m_pSaves  = NULL;
	m_nSaves  = 0;
	m_clrText = RGB( 255, 255, 255 );	// 0xFFFFFF (+976)
	m_clrBk   = RGB( 63, 63, 63 );		// 0x3F3F3F (+972)
	m_bkBrush.CreateSolidBrush( m_clrBk );

	m_pSelfWnd = this;		// +204 -- the page points the slide at itself
	LoadHeaderBitmap( "head_save", 0 );
	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnSave.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_ROW_ODD, m_headerLoaded );
		m_btnBack.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_BACK, m_headerLoaded );
		m_btnDelete.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_LOAD_SAVE, m_headerLoaded );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::DoDataExchange (0x45DEE0)

void CSaveDlg::DoDataExchange( CDataExchange* pDX )
{
	CDialog::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_MAIN_LOAD_GAME, m_btnDelete );			// 1021 (+224)
	DDX_Control( pDX, IDCANCEL, m_btnBack );					// 2    (+464)
	DDX_Control( pDX, IDC_MAIN_RETURN_TO_GAME, m_btnSave );	// 1019 (+704)
}

/*
==================
Save_ParseGameHeader (0x45DF30)
==================
*/
BOOL Save_ParseGameHeader( FILE* fp, char* pszMapName, char* pszComment )
{
	int		nMagic, nVersion;

	fread( &nMagic, 4, 1, fp );
	if ( nMagic != SAVEGAME_FOURCC )
	{
		fclose( fp );
		return FALSE;
	}
	fread( &nVersion, 4, 1, fp );
	if ( nVersion != SAVEGAME_VERSION )
	{
		fclose( fp );
		return FALSE;
	}

	*pszMapName = 0;
	*pszComment = 0;

	int		cbStringData, nTokens, cbEntryData;
	fread( &cbStringData, 4, 1, fp );	// string-pool size
	fread( &nTokens,      4, 1, fp );	// token count
	fread( &cbEntryData,  4, 1, fp );	// entry-data size
	if ( (unsigned)nTokens > 0x2000000 || (unsigned)cbEntryData > 0x2000000 )
	{
		fclose( fp );
		return FALSE;
	}

	int		cbBlob = cbStringData + cbEntryData;
	char*	pBlob = new char[cbBlob];
	fread( pBlob, 1, cbBlob, fp );

	// Build the token-index -> string pointer table from the string pool.
	char**	ppTokens = NULL;
	char*	p = pBlob;
	if ( cbEntryData > 0 )
	{
		ppTokens = new char*[nTokens];
		for ( int i = 0; i < nTokens; i++ )
		{
			ppTokens[i] = ( *p != 0 ) ? p : NULL;
			if ( *p++ )
				p += strlen( p ) + 1;
		}
	}

	// First entry record must be "GameHeader".
	short	cbValue   = *(short*)p;
	short	iKeyToken = ( (short*)p )[1];
	char*	pRec      = (char*)( (short*)p + 1 );		// after cbValue
	if ( _strcmpi( ppTokens[iKeyToken], "GameHeader" ) )
	{
		delete[] pBlob;
		return FALSE;
	}

	int		nFields = *(char*)( pRec + 2 );
	char*	pField  = pRec + cbValue + 2;
	for ( ; nFields > 0; nFields-- )
	{
		short		cbFieldVal = *(short*)pField;
		const char*	pszKey     = ppTokens[ ( (short*)pField )[1] ];
		char*		pszVal     = pField + 4;

		if ( !_strcmpi( pszKey, "comment" ) )
			strncpy( pszComment, pszVal, cbFieldVal );
		else if ( !_strcmpi( pszKey, "mapName" ) )
			strncpy( pszMapName, pszVal, cbFieldVal );

		pField = pszVal + cbFieldVal;
	}

	if ( ppTokens )
		delete[] ppTokens;
	delete[] pBlob;
	return strlen( pszMapName ) && strlen( pszComment );
}

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::OnInitDialog (0x45E160)

BOOL CSaveDlg::OnInitDialog()
{
	RECT		rc;
	odcolumn_t	col;
	int			w, h, right;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	// The save-game list control.
	m_pList = (CODSaveGameListCtrl*)new CODSaveGameListCtrl();
	rc.left = 0; rc.top = 0; rc.right = 100; rc.bottom = 100;
	m_pList->Create( 0, rc, this, 106 );
	m_pList->SetHeaderTransparent( 1 );
	m_pList->SetTransparent( 0 );
	m_pList->SetDrawFrame( 1 );
	PopulateSaves();

	w = m_headerW;
	h = m_headerH;
	right = g_nLauncherDefW - 50;

	m_btnSave.MoveWindow( 50, 140, w, h, TRUE );
	SetWindowTextSafe( &m_btnSave, Launcher_LoadString( IDS_BTN_SAVE ) );
	m_pList->MoveWindow( w + 60, 140, right - ( w + 60 ), ( g_nLauncherDefH - 50 ) - 140, TRUE );

	// Columns: time, game, elapsed (third stretches to fill).
	Launcher_LoadStringInto( col.title, IDS_SAVE_TIMEHEADING );
	col.width = 100;
	m_pList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_SAVE_GAMEHEADING );
	col.width = 175;
	m_pList->AddColumn( &col );
	Launcher_LoadStringInto( col.title, IDS_SAVE_ELAPSEDHEADING );
	col.width = right - ( w + 60 ) - 275;
	m_pList->AddColumn( &col );

	m_btnDelete.MoveWindow( 50, 172, w, h, TRUE );
	SetWindowTextSafe( &m_btnDelete, Launcher_LoadString( IDS_BTN_DELETE ) );
	m_btnBack.MoveWindow( 50, 204, w, h, TRUE );
	SetWindowTextSafe( &m_btnBack, Launcher_LoadString( IDS_BTN_CANCEL ) );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::~CSaveDlg (0x45E3C0)
//
// m_pList is not freed here -- CODListCtrl::OnNcDestroy (0x44AD30) deletes it.

CSaveDlg::~CSaveDlg()
{
	if ( m_pSaves )
		delete[] m_pSaves;
	m_pSaves = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::OnSave (0x45E480)

void CSaveDlg::OnSave()
{
	int	sel = m_pList->GetCurSel();
	if ( sel == -1 )
	{
		Launcher_ShowMessageById( 0, IDS_SINGLE_NOSELECTION );
		return;
	}

	const char*	pszFile = m_pSaves[sel].filename;
	if ( !pszFile )
		return;

	// A save is only possible while a single-player game is connected.
	GameInfo_t	gi;
	if ( engineapi.GetGameInfo( &gi, 0 ) && ( gi.state != ca_active || !gi.signon ) )
	{
		CPromptDlg	dlg( 1 );		// single OK button
		dlg.SetMessage( Launcher_LoadString( IDS_SAVE_CANTSAVE ) );
		dlg.DoModal();
		return;
	}

	// Overwriting an existing slot (row 0 is the fresh slot) needs confirmation.
	if ( sel != 0 )
	{
		CPromptDlg	dlg( 2 );		// OK + Cancel
		dlg.SetMessage( Launcher_LoadString( IDS_SAVE_OVERWRITEPROMPT ) );
		if ( dlg.DoModal() != IDOK )
			return;
	}

	char	cmd[260];
	sprintf( cmd, "save %s\n", pszFile );
	AFXSetTopLevelFrame( 1 );
	Launcher_RunMapCommand( cmd );
	OnOK();						// CDialog::OnOK -> EndDialog( IDOK )  (vtable slot +196)

	CWinThread*	pThread = AfxGetThread();
	if ( pThread )
	{
		CWnd*	pMain = pThread->GetMainWnd();
		if ( pMain )
			pMain->ShowWindow( SW_HIDE );
	}
}

/*
==================
Save_GetSaveDir (0x45E840)
==================
*/
char* Save_GetSaveDir()
{
	static char	szDir[260];
	char		szGame[260];

	memset( szDir, 0, sizeof( szDir ) );
	sprintf( szGame, com_gamedir );
	sprintf( szDir, "%s/SAVE/", szGame );
	return szDir;
}

/*
==================
Save_ReadFileInfo (0x45E890)
==================
*/
BOOL Save_ReadFileInfo( LPCSTR lpFileName, const char* pszDisplayName, savegame_t* pRec )
{
	if ( !lpFileName || !pszDisplayName || !pRec )
		return FALSE;

	COM_FixSlashes( (char*)lpFileName );
	strcpy( pRec->filename, pszDisplayName );

	FILETIME	ftWrite = { 0, 0 };
	HANDLE		hFile = CreateFileA( lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
									OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		GetFileTime( hFile, NULL, NULL, &ftWrite );
		CloseHandle( hFile );
	}

	char	szMapName[256], szComment[256];
	FILE*	fp = fopen( lpFileName, "rb" );
	if ( !fp || !Save_ParseGameHeader( fp, szMapName, szComment ) )
		return FALSE;
	fclose( fp );

	strncpy( pRec->mapname, szMapName, 0x20 );
	pRec->mapname[31] = 0;
	strncpy( pRec->comment, szComment, 0x50 );
	pRec->comment[79] = 0;

	if ( strstr( lpFileName, "quick" ) )
		pRec->bQuicksave = 1;
	if ( strstr( lpFileName, "autosave" ) )
		pRec->bAutosave = 1;

	// split the trailing 5-char elapsed token off the comment
	char	szElapsed[64];
	int		len = strlen( pRec->comment ) + 1;
	sprintf( szElapsed, "??" );
	if ( len - 1 >= 6 )
	{
		strncpy( szElapsed, pRec->comment + ( len - 1 ) - 5, 5 );
		szElapsed[5] = 0;
		pRec->comment[len - 6] = 0;
		for ( int i = len - 7; i >= 1; i-- )		// trim trailing spaces
		{
			if ( pRec->comment[i] != ' ' )
				break;
			pRec->comment[i] = 0;
		}
	}
	strcpy( pRec->elapsed, szElapsed );

	pRec->fileTime = ftWrite;

	char	szDate[32];
	if ( ftWrite.dwLowDateTime && ftWrite.dwHighDateTime )
	{
		CTime	t( ftWrite, -1 );
		CString	str = t.Format( "%b %d, %I:%M %p" );
		if ( !str.IsEmpty() )
		{
			strncpy( szDate, str, 0x1F );
			szDate[31] = 0;
		}
		else
		{
			strcpy( szDate, "??" );
		}
	}
	else
	{
		strcpy( szDate, "??" );
	}
	strcpy( pRec->date, szDate );
	return TRUE;
}

// savefile.cpp -- .sav savegame header parsing.

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::PopulateSaves (0x45EBC0)

void CSaveDlg::PopulateSaves()
{
	// Clear the current rows + free the previous save array.
	m_pList->ResetContent();
	if ( m_pSaves )
	{
		delete[] m_pSaves;
		m_pSaves = NULL;
	}
	m_nSaves = 0;

	char	dir[280];
	sprintf( dir, "%s/SAVE/", com_gamedir );
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

	m_nSaves = count + 1;
	m_pSaves = new savegame_t[m_nSaves];
	memset( m_pSaves, 0, sizeof( savegame_t ) * m_nSaves );

	// Row 0: the new-save slot.
	sprintf( m_pSaves[0].filename, "Half-Life-%03i.sav", Sav_FindFreeSlot() );
	sprintf( m_pSaves[0].date, Launcher_LoadString( IDS_SAVE_NEWCAPTION ) );
	sprintf( m_pSaves[0].comment, Launcher_LoadString( IDS_SAVE_NEWGAMETXT ) );
	sprintf( m_pSaves[0].elapsed, Launcher_LoadString( IDS_SAVE_FILETIME ) );	// new-elapsed text

	// Second pass: parse each save into the array.
	int	rows = 1;
	hFind = FindFirstFileA( pattern, &fd );
	if ( hFind != INVALID_HANDLE_VALUE )
	{
		do
		{
			if ( _strnicmp( fd.cFileName, "HLSave", 6 ) )
			{
				char	full[560];
				sprintf( full, "%s%s", dir, fd.cFileName );
				if ( Launcher_ParseSaveFile( full, fd.cFileName, &m_pSaves[rows] ) )
					rows++;
				else
					memset( &m_pSaves[rows], 0, sizeof( savegame_t ) );

				if ( rows > m_nSaves )
				{
					Launcher_ShowMessageById( 0, IDS_SAVELOAD_NUMBEROFGAMESCHANGED );
					break;
				}
			}
		} while ( FindNextFileA( hFind, &fd ) );
		FindClose( hFind );
	}
	m_nSaves = rows;

	// Newest first (row 0 stays put).
	for ( int i = 1; i < m_nSaves; i++ )
		for ( int j = i + 1; j < m_nSaves; j++ )
			if ( CompareFileTime( &m_pSaves[i].fileTime, &m_pSaves[j].fileTime ) < 0 )
			{
				savegame_t	tmp = m_pSaves[i];
				m_pSaves[i] = m_pSaves[j];
				m_pSaves[j] = tmp;
			}

	// One 3-column row per record.
	for ( int n = 0; n < m_nSaves; n++ )
	{
		m_pList->AddRow( &m_pSaves[n] );
	}

	m_pList->SelectItem( 0, 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::UpdateButtonStates (0x45F010)

void CSaveDlg::UpdateButtonStates()
{
	GameInfo_t	gi;
	int			sel;

	if ( engineapi.GetGameInfo( &gi, 0 ) && ( gi.state != ca_active || !gi.signon ) )
	{
		m_btnSave.SetHighlight( 1 );
		return;
	}

	sel = m_pList->GetCurSel();
	if ( sel == -1 || !&m_pSaves[sel] )
	{
		m_btnSave.SetHighlight( 1 );
		return;
	}

	m_btnDelete.SetHighlight( sel == 0 );	// row 0 is the new-save slot
	m_btnSave.SetHighlight( 0 );
}

/*
==================
Sav_FindFreeSlot (0x45F0E0)
==================
*/
static int Sav_FindFreeSlot( void )
{
	char	path[300];
	FILE*	f;
	int		i = 0;

	for ( ;; )
	{
		sprintf( path, "%sHalf-Life-%03i.sav", Save_GetSaveDir(), i );
		f = fopen( path, "rb" );
		if ( !f )
			break;
		fclose( f );
		if ( ++i >= 1000 )
			return 0;
	}

	return i;
}

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::OnDelete (0x45F140)

void CSaveDlg::OnDelete()
{
	int	sel = m_pList->GetCurSel();
	if ( sel == -1 || sel == 0 )
		return;

	const char*	pszFile = m_pSaves[sel].filename;
	if ( !pszFile )
		return;

	CPromptDlg	dlg( 2 );			// OK + Cancel
	dlg.SetMessage( Launcher_LoadString( IDS_SAVE_DELETEPROMPT ) );
	if ( dlg.DoModal() != IDOK )
		return;

	char	path[260];
	sprintf( path, "%s%s", Save_GetSaveDir(), pszFile );
	_unlink( path );

	PopulateSaves();
	::UpdateWindow( m_pList->m_hWnd );
	m_pList->SelectItem( 0, 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::OnNcPaint (0x4269D0)

void CSaveDlg::OnNcPaint()
{
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::OnPaint (0x412860)

void CSaveDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::OnEraseBkgnd (0x412870)

BOOL CSaveDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::OnActivateApp (0x406FE0)

void CSaveDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CSaveDlg::OnDisplayChange (0x453D00)

LRESULT CSaveDlg::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}

/*
==================
Save_ClearRecord (0x45F3B0)
==================
*/
savegame_t* Save_ClearRecord( savegame_t* pRec )
{
	memset( pRec->filename, 0, sizeof( pRec->filename ) );	// +0    260
	memset( pRec->mapname,  0, sizeof( pRec->mapname ) );	// +260  32
	memset( pRec->comment,  0, sizeof( pRec->comment ) );	// +292  80
	memset( pRec->elapsed,  0, sizeof( pRec->elapsed ) );	// +372  32
	memset( pRec->date,     0, sizeof( pRec->date ) );		// +404  32
	pRec->bQuicksave = 0;									// +444
	pRec->bAutosave  = 0;									// +448
	return pRec;
}
