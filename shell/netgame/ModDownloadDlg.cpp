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
// Purpose: the FTP mod-download progress popup (CModDownloadDlg).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The FindFile flags the walk asks for: no cache, no auto-redirect, reload.
#define MODDL_FINDFLAGS		0xC4000000

// Entries at 0x4AFCD8, base map 0x4B4398 = CDialog.
BEGIN_MESSAGE_MAP( CModDownloadDlg, CDialog )
	//{{AFX_MSG_MAP(CModDownloadDlg)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
==================
ModDownload_NewFile (0x42d310)

Zeroed queue node.  The walk allocates one per remote file.
==================
*/
static dlfile_t* ModDownload_NewFile( void )
{
	dlfile_t*	p = new dlfile_t;

	memset( p, 0, sizeof( dlfile_t ) );
	return p;
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::CModDownloadDlg (0x42C430)

CModDownloadDlg::CModDownloadDlg( mod_t* pMod, CWnd* pParent )
	: CDlgPopupBase( IDD_MODDOWNLOAD, pParent )
{
	int	wh[2];

	m_pPaintWnd    = this;
	m_bInitialized = 0;
	m_pFileList    = NULL;
	m_bDownloadOK  = 0;
	m_pCurFile     = NULL;
	m_pFtpConn     = NULL;
	m_pSession     = NULL;
	m_cbReceived   = 0;
	m_cbThisFile   = 0;
	m_cbTotal      = 0;
	m_fpLocal      = NULL;
	m_pFtpFile     = NULL;
	m_flStartTime  = (float)engineapi.Sys_FloatTime();
	memset( m_szRemoteDir, 0, sizeof( m_szRemoteDir ) );
	m_pMod = pMod;

	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( wh );
	m_headerW      = wh[0];
	m_headerH      = wh[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
		m_btnCancel.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_BACK, m_headerLoaded );

	SetModalProgressPopup( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::~CModDownloadDlg (0x42C5A0)
//
// The transfer state is not unwound here: a download that is still running
// when the dialog closes has already been stopped by ServiceDownload.

CModDownloadDlg::~CModDownloadDlg()
{
	dlfile_t*	p = m_pFileList;

	while ( p )
	{
		dlfile_t*	pNext = p->pNext;
		delete p;
		p = pNext;
	}
	m_pFileList = NULL;

	if ( m_pFtpConn )
	{
		m_pFtpConn->Close();
		delete m_pFtpConn;
		m_pFtpConn = NULL;
	}

	if ( m_pSession )
	{
		delete m_pSession;
		m_pSession = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::DoDataExchange (0x42C690)

void CModDownloadDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDOK,                                m_btnCancel );
	DDX_Control( pDX, IDC_MODDOWNLOAD_TIME,                     m_lblTime );
	DDX_Control( pDX, IDC_MODDOWNLOAD_TITLE,   m_lblTitle );
	DDX_Control( pDX, IDC_MODDOWNLOAD_STATUS,                   m_lblStatus );
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::OnInitDialog (0x42C6F0)
//
// A fixed 300x200 popup, centred on the screen: caption band, status block,
// then the elapsed-time line and Cancel across the bottom.

BOOL CModDownloadDlg::OnInitDialog()
{
	int		wh[2];
	char	szCap[256];

	CDialog::OnInitDialog();

	MoveWindow( 0, 0, 300, 200, FALSE );

	RECT	rcWnd;
	GetWindowRect( &rcWnd );

	int	w = rcWnd.right - rcWnd.left;
	int	h = rcWnd.bottom - rcWnd.top;

#ifdef LAUNCHER_FIXES
	Dlg_CenterPopup( this, w, h );
#else
	MoveWindow( ( GetSystemMetrics( SM_CXSCREEN ) - w ) / 2,
				( GetSystemMetrics( SM_CYSCREEN ) - h ) / 2, w, h, TRUE );
#endif

	Launcher_HeaderSize( wh );

	int		hdrH = wh[1];
	RECT	rc;

	GetClientRect( &rc );

	m_lblTitle.MoveWindow( 10, 10, rc.right - rc.left - 20, 40, TRUE );
	m_lblTitle.SetTextColor( RGB( 240, 180, 56 ) );
	m_lblTitle.SetBgColor( RGB( 56, 56, 56 ) );
	m_lblTitle.SetFontSize( 16, FW_HEAVY );
	m_lblTitle.SetTransparent( FALSE );
	m_lblTitle.SetCentered( TRUE );
	sprintf( szCap, Launcher_LoadString( IDS_MOD_DOWNLOADING ), m_pMod->GetKey( "game" ) );
	m_lblTitle.SetWindowText( szCap );

	m_lblStatus.MoveWindow( 25, 60, rc.right - rc.left - 50,
							rc.bottom - rc.top - hdrH - 80, TRUE );
	m_lblStatus.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblStatus.SetBgColor( RGB( 56, 56, 56 ) );
	m_lblStatus.SetFontSize( 12, FW_NORMAL );
	m_lblStatus.SetTransparent( FALSE );

	int	bx = rc.right  - rc.left - 110;
	int	by = rc.bottom - rc.top  - hdrH - 10;

	m_btnCancel.MoveWindow( bx, by, ( rc.right - rc.left - 10 ) - bx,
							( rc.bottom - rc.top - 10 ) - by, TRUE );
	m_btnCancel.SetTransparent( FALSE );
	m_btnCancel.SetHasArrow( 0 );
	m_btnCancel.SetBkColor( RGB( 56, 56, 56 ) );
	m_btnCancel.SetTextColor( RGB( 255, 180, 0 ) );

	m_lblTime.MoveWindow( 10, by, bx - 20, ( rc.bottom - rc.top - 10 ) - by, TRUE );
	m_lblTime.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblTime.SetBgColor( RGB( 56, 56, 56 ) );
	m_lblTime.SetFontSize( 10, FW_HEAVY );
	m_lblTime.SetTransparent( FALSE );

	SetStatus( 0.0f, "" );

	ShowWindow( SW_RESTORE );
	UpdateWindow();

	m_flStartTime = (float)engineapi.Sys_FloatTime();
	Connect();
	m_bInitialized = 1;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::StartOrFinishFile (0x42C9A0)
//
// Open the next queued file, or -- with the queue drained -- report the total
// and hand over to the unpacker.

void CModDownloadDlg::StartOrFinishFile()
{
	if ( m_pCurFile )
	{
		char	szRemote[260];
		char	szLocal[260];

		if ( m_pCurFile->szSubDir[0] )
			sprintf( szRemote, "%s/%s", m_pCurFile->szSubDir, m_pCurFile->szName );
		else
			strcpy( szRemote, m_pCurFile->szName );

		// The queue holds paths relative to the server root; the local tree
		// starts at the mod's own directory.
		const char*	pszRel = szRemote;
		if ( m_szRemoteDir[0] && strlen( szRemote ) > strlen( m_szRemoteDir ) )
			pszRel = &szRemote[strlen( m_szRemoteDir ) + 1];

		sprintf( szLocal, "%s/%s", m_pMod->GetKey( "gamedir" ), pszRel );

		m_pFtpFile = m_pFtpConn->OpenFile( szRemote, GENERIC_READ, FTP_TRANSFER_TYPE_BINARY, 1 );
		if ( !m_pFtpFile )
		{
			m_bDownloadOK = 0;
			Launcher_ErrorMessageBox( 0, Launcher_LoadString( IDS_MOD_REMOTEOPENFAIL ), szRemote );
		}

		COM_FixSlashes( szLocal );
		COM_CreatePath( szLocal );
		m_fpLocal = fopen( szLocal, "wb" );
		if ( !m_fpLocal )
		{
			m_bDownloadOK = 0;
			Launcher_ErrorMessageBox( 0, Launcher_LoadString( IDS_MOD_LOCALOPENFAIL ), szLocal );
		}

		strcpy( m_pCurFile->szLocalPath, szLocal );
		m_cbThisFile = 0;
	}
	else
	{
		char	szMsg[256];

		m_bDownloadOK = 0;

		const char*	pszGame = m_pMod->GetKey( "game" );
		double		cb = m_cbReceived;

		if ( cb / 1048576.0 <= 0.9 )
			sprintf( szMsg, Launcher_LoadString( IDS_MOD_DLSIZEKB ), pszGame, cb / 1024.0 );
		else
			sprintf( szMsg, Launcher_LoadString( IDS_MOD_DLSIZEMB ), pszGame, cb / 1048576.0 );

		m_btnCancel.ShowWindow( SW_HIDE );
		SetStatus( (float)engineapi.Sys_FloatTime() - m_flStartTime, szMsg );
		Sleep( 1500 );

		if ( m_cbReceived > 0 )
			ExtractArchive();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::PumpChunk (0x42CC50)
//
// One kilobyte per pass.  A short read is the end of the file, so the local
// handle closes and the queue advances.

void CModDownloadDlg::PumpChunk()
{
	static float	s_flLastUpdate;

	if ( !m_pFtpFile || !m_fpLocal || !m_pCurFile )
		return;

	char	buf[1024];
	UINT	n = m_pFtpFile->Read( buf, sizeof( buf ) );

	if ( n )
		fwrite( buf, n, 1, m_fpLocal );

	m_cbThisFile += n;
	m_cbReceived += n;

	float	now = (float)engineapi.Sys_FloatTime();
	if ( now - s_flLastUpdate > 0.1f )
	{
		s_flLastUpdate = now;

		char	szRemote[260];
		if ( m_pCurFile->szSubDir[0] )
			sprintf( szRemote, "%s/%s", m_pCurFile->szSubDir, m_pCurFile->szName );
		else
			strcpy( szRemote, m_pCurFile->szName );

		float	pct = 0.0f;
		if ( m_cbTotal )
		{
			pct = (float)m_cbReceived / (float)m_cbTotal * 100.0f;
			if ( pct < 0.0f )
				pct = 0.0f;
			if ( pct > 100.0f )
				pct = 100.0f;
		}

		const char*	pszUrl = m_pMod->GetKey( "url_dl" );
		if ( !pszUrl )
			pszUrl = "?";

		SetStatus( now - m_flStartTime, Launcher_LoadString( IDS_MOD_DLSTATUS ),
			m_pMod->GetKey( "game" ), pszUrl, szRemote, pct );
	}

	if ( n != sizeof( buf ) )
	{
		fclose( m_fpLocal );
		m_fpLocal = NULL;
		m_pFtpFile->Close();
		delete m_pFtpFile;
		m_pFtpFile = NULL;
		m_pCurFile = m_pCurFile->pNext;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::ServiceDownload (0x42CE70)
//
// (sic) the failed branch closes the dialog and then falls through into the
// transfer step; with the queue already torn down, the step is a no-op.

void CModDownloadDlg::ServiceDownload()
{
	if ( !m_bDownloadOK )
	{
		if ( !m_bInitialized )
			return;
		OnOK();
	}

	if ( m_pFtpFile )
		PumpChunk();
	else
		StartOrFinishFile();
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::RMLPreIdle (0x42CEB0)

int CModDownloadDlg::RMLPreIdle()
{
	CDlgPopupBase::RMLPreIdle();
	OnEngineFrame();
	ServiceDownload();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::Connect (0x42CED0)
//
// Open the session, split the URL into host and remote directory, and check
// the directory really holds a mod before enumerating it.

BOOL CModDownloadDlg::Connect()
{
	char	szUrl[260];
	char	szHost[260];
	char	szFullUrl[260];

	m_pSession = new CInternetSession( "Half-Life", 1, PRE_CONFIG_INTERNET_ACCESS, 0, 0, 0 );
	if ( !m_pSession )
	{
		OnCancel();
		return FALSE;
	}

	const char*	pszUrl = m_pMod->GetKey( "url_dl" );
	if ( !pszUrl )
		pszUrl = "?";
	sprintf( szUrl, pszUrl );		// (sic) the URL is used as the format string
	strcpy( szFullUrl, szUrl );

	SetStatus( (float)engineapi.Sys_FloatTime() - m_flStartTime,
		Launcher_LoadString( IDS_MOD_CONNECT ), pszUrl );

	// Split host / path at the first slash; the split truncates szUrl, which is
	// why the whole URL was copied off first.
	char*	p = szUrl;
	char*	d = szHost;

	while ( *p && *p != '/' && *p != '\\' )
		*d++ = *p++;
	*d = 0;

	if ( *p == '/' || *p == '\\' )
	{
		*p = 0;
		strcpy( m_szRemoteDir, p + 1 );
	}
	else
	{
		strcpy( m_szRemoteDir, "" );
	}

	m_pFtpConn = m_pSession->GetFtpConnection( szHost, "anonymous",
		"anonymous@anonymous.com", 0, 0 );

	// A connection that never came up reports nothing: the message below is
	// only for a host that answered without a mod in the directory.
	if ( m_pFtpConn )
	{
		if ( HasLibList( m_pFtpConn, m_szRemoteDir ) )
		{
			SetStatus( (float)engineapi.Sys_FloatTime() - m_flStartTime,
				Launcher_LoadString( IDS_MOD_GETTINGSIZE ) );
			m_cbTotal = EnumDir( m_pFtpConn, m_szRemoteDir );
			if ( m_cbTotal > 0 )
			{
				SetStatus( (float)engineapi.Sys_FloatTime() - m_flStartTime,
					Launcher_LoadString( IDS_MOD_DLSTATUSSHORT ),
					m_pMod->GetKey( "game" ), szFullUrl );
				m_bDownloadOK = 1;
				m_pCurFile    = m_pFileList;
			}
			else
			{
				SetStatus( (float)engineapi.Sys_FloatTime() - m_flStartTime,
					Launcher_LoadString( IDS_MOD_NOFILES ) );
				Sleep( 1000 );
			}
		}
		else
		{
			char	szMsg[256];

			sprintf( szMsg, Launcher_LoadString( IDS_MOD_NOLIBLIST ), szFullUrl );
			SetStatus( (float)engineapi.Sys_FloatTime() - m_flStartTime, szMsg );
			Sleep( 5000 );
		}
	}

	if ( !m_bDownloadOK )
	{
		OnCancel();
		return FALSE;
	}
	return m_bDownloadOK;
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::EnumDir (0x42D330)
//
// Recursive walk: files go straight onto the queue, sub-directories are held
// on a local list and descended into once this level is done.

int CModDownloadDlg::EnumDir( CFtpConnection* pConn, const char* pszDir )
{
	char	szWild[260];
	char	szName[260];
	int		cbTotal = 0;
	dldir_t*	pSubDirs = NULL;

	engineapi.Sys_FloatTime();

	if ( pszDir )
	{
		char*	d = (char*)pszDir;
		size_t	n = strlen( d );

		if ( n && ( d[n - 1] == '/' || d[n - 1] == '\\' ) )
			d[n - 1] = 0;
		sprintf( szWild, "%s/*", d );
	}
	else
	{
		sprintf( szWild, "*" );
	}

	CFtpFileFind	finder( pConn, 1 );
	if ( finder.FindFile( szWild, MODDL_FINDFLAGS ) )
	{
		BOOL	bMore;
		do
		{
			bMore = finder.FindNextFile();
			strcpy( szName, finder.GetFileName() );
			if ( szName[0] != '.' )
			{
				if ( finder.IsDirectory() )
				{
					dldir_t*	pDir = new dldir_t;

					memset( pDir, 0, sizeof( dldir_t ) );
					strcpy( pDir->szName, szName );
					pDir->pNext = pSubDirs;
					pSubDirs    = pDir;
				}
				else
				{
					dlfile_t*	pFile = ModDownload_NewFile();

					pFile->cbLength = (int)finder.GetLength();
					strcpy( pFile->szName, szName );
					strcpy( pFile->szSubDir, ( pszDir && *pszDir ) ? pszDir : "" );
					pFile->pNext = m_pFileList;
					m_pFileList  = pFile;
					cbTotal += pFile->cbLength;
				}
			}
		}
		while ( bMore );
	}
	finder.Close();

	dldir_t*	pDir = pSubDirs;
	while ( pDir )
	{
		dldir_t*	pNext = pDir->pNext;

		if ( pszDir && *pszDir )
			sprintf( szName, "%s/%s", pszDir, pDir->szName );
		else
			strcpy( szName, pDir->szName );

		cbTotal += EnumDir( pConn, szName );
		delete pDir;
		pDir = pNext;
	}
	return cbTotal;
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::HasLibList (0x42D630)
//
// A directory without a liblist.gam is not a mod, whatever else is in it.

BOOL CModDownloadDlg::HasLibList( CFtpConnection* pConn, const char* pszDir )
{
	char	szPath[260];
	size_t	n = strlen( pszDir );

	if ( n && ( pszDir[n - 1] == '/' || pszDir[n - 1] == '\\' ) )
		sprintf( szPath, "%sliblist.gam", pszDir );
	else
		sprintf( szPath, "%s/liblist.gam", pszDir );

	CFtpFileFind	finder( pConn, 1 );
	BOOL			bFound = finder.FindFile( szPath, MODDL_FINDFLAGS );

	finder.Close();
	return bFound;
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::ExtractArchive (0x42D720)
//
// A mod that shipped as <gamedir>.zip is unpacked in place; anything else is
// already in the tree.

void CModDownloadDlg::ExtractArchive()
{
	if ( m_pFileList )
	{
		char	szZip[260];
		char	szPrompt[256];

		sprintf( szZip, "%s.zip", m_pMod->GetKey( "gamedir" ) );

		dlfile_t*	pZip = m_pFileList;
		while ( pZip && _stricmp( pZip->szName, szZip ) )
			pZip = pZip->pNext;
		if ( !pZip )
			return;

		CPromptDlg	dlg( 2, NULL );		// OK + Cancel

		sprintf( szPrompt, Launcher_LoadString( IDS_MOD_UNZIP ), szZip );
		dlg.SetMessage( szPrompt );
		if ( dlg.DoModal() == IDOK )
		{
			char*	argv[4];

			argv[0] = _strdup( "unzip" );
			argv[1] = _strdup( "-d" );
			argv[2] = _strdup( m_pMod->GetKey( "gamedir" ) );
			argv[3] = _strdup( pZip->szLocalPath );
			UzpMain( ARRAYSIZE( argv ), argv );

			free( argv[0] );
			free( argv[1] );
			free( argv[2] );
			free( argv[3] );
			OnOK();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::SetStatus (0x42E9B0)
//
// Shared with CModHttpDownloadDlg: identical bodies, folded to one copy.

void CModDownloadDlg::SetStatus( float flElapsed, const char* pszFmt, ... )
{
	char	szBuf[1024];
	va_list	va;

	va_start( va, pszFmt );
	vsprintf( szBuf, pszFmt, va );
	va_end( va );

	if ( m_lblStatus.GetSafeHwnd() )
	{
		m_lblStatus.SetWindowText( szBuf );
		::InvalidateRect( m_lblStatus.GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( m_lblStatus.GetSafeHwnd() );
	}

	if ( m_lblTime.GetSafeHwnd() )
	{
		if ( flElapsed == 0.0f )
			sprintf( szBuf, "" );
		else
			sprintf( szBuf, Launcher_LoadString( IDS_MOD_TIME ), flElapsed );
		m_lblTime.SetWindowText( szBuf );
		::InvalidateRect( m_lblTime.GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( m_lblTime.GetSafeHwnd() );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::OnEraseBkgnd (0x4112E0)

BOOL CModDownloadDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	CDlgPopupBase::OnPaint();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg::OnPaint (0x4113F0)

void CModDownloadDlg::OnPaint()
{
	CDlgPopupBase::OnPaint();
}
