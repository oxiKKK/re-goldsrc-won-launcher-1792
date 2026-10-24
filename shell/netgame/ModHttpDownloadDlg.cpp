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
// Purpose: the HTTP mod-download progress popup (CModHttpDownloadDlg).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Entries at 0x4AFE18, base map 0x4B4398 = CDialog.
BEGIN_MESSAGE_MAP( CModHttpDownloadDlg, CDialog )
	//{{AFX_MSG_MAP(CModHttpDownloadDlg)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
==================
ModHttp_NewFile (0x42ee60)

Zeroed queue node, one per URL.
==================
*/
static httpfile_t* ModHttp_NewFile( void )
{
	httpfile_t*	p = new httpfile_t;

	memset( p, 0, sizeof( httpfile_t ) );
	return p;
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::CModHttpDownloadDlg (0x42DF10)

CModHttpDownloadDlg::CModHttpDownloadDlg( mod_t* pMod, CWnd* pParent )
	: CDlgPopupBase( IDD_MODDOWNLOAD, pParent )
{
	int	wh[2];

	m_pPaintWnd    = this;
	m_pFileList    = NULL;
	m_pCurFile     = NULL;
	m_fpLocal      = NULL;
	m_pHttpFile    = NULL;
	m_pSession     = NULL;
	m_cbThisFile   = 0;
	m_cbReceived   = 0;
	m_cbExpected   = 0;
	m_bDownloadOK  = 0;
	m_bInitialized = 0;
	m_flStartTime  = (float)engineapi.Sys_FloatTime();
	m_pMod         = pMod;

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
// CModHttpDownloadDlg::~CModHttpDownloadDlg (0x42E070)
//
// The open object is closed but not deleted, and the local handle is left to
// the transfer step that opened it.

CModHttpDownloadDlg::~CModHttpDownloadDlg()
{
	httpfile_t*	p = m_pFileList;

	while ( p )
	{
		httpfile_t*	pNext = p->pNext;
		delete p;
		p = pNext;
	}
	m_pFileList = NULL;

	if ( m_pHttpFile )
	{
		m_pHttpFile->Close();
		m_pHttpFile = NULL;
	}

	if ( m_pSession )
	{
		delete m_pSession;
		m_pSession = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::DoDataExchange (0x42E110)

void CModHttpDownloadDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDOK,                              m_btnCancel );
	DDX_Control( pDX, IDC_MODDOWNLOAD_TIME,                   m_lblTime );
	DDX_Control( pDX, IDC_MODDOWNLOAD_TITLE, m_lblTitle );
	DDX_Control( pDX, IDC_MODDOWNLOAD_STATUS,                 m_lblStatus );
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::OnInitDialog (0x42E160)

BOOL CModHttpDownloadDlg::OnInitDialog()
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
	StartDownload();
	m_bInitialized = 1;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::NextFileOrFinish (0x42E410)
//
// WININET reports a bad URL by throwing, so the open is guarded and the
// exception's own text is what the user sees.

void CModHttpDownloadDlg::NextFileOrFinish()
{
	if ( m_pCurFile )
	{
		char	szUrl[260];

		strcpy( szUrl, m_pCurFile->szUrl );

		try
		{
			m_pHttpFile = (CInternetFile*)m_pSession->OpenURL( szUrl, 1,
				INTERNET_FLAG_TRANSFER_BINARY, 0, 0 );
		}
		catch ( CException* e )
		{
			char	szErr[2048];

			e->GetErrorMessage( szErr, sizeof( szErr ) - 1, 0 );

			CPromptDlg	dlg( 1, NULL );		// OK only

			dlg.SetMessageFont( 11, FW_NORMAL );
			dlg.SetTextAlign( 0 );
			dlg.SetPromptSize( 450, 350 );
			dlg.SetMessage( "%s", szErr );
			dlg.DoModal();
			e->Delete();
			m_pHttpFile = NULL;
		}

		if ( !m_pHttpFile )
		{
			m_bDownloadOK = 0;
			Launcher_ErrorMessageBox( 0, Launcher_LoadString( IDS_MOD_REMOTEOPENFAIL ), szUrl );
		}

		COM_FixSlashes( m_pCurFile->szDest );
		COM_CreatePath( m_pCurFile->szDest );
		m_fpLocal = fopen( m_pCurFile->szDest, "wb" );
		if ( !m_fpLocal )
		{
			m_bDownloadOK = 0;
			Launcher_ErrorMessageBox( 0, Launcher_LoadString( IDS_MOD_LOCALOPENFAIL ),
				m_pCurFile->szDest );
		}

		m_cbThisFile = 0;
	}
	else
	{
		char	szMsg[256];

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

		m_bDownloadOK = 0;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::ReadChunk (0x42E790)

void CModHttpDownloadDlg::ReadChunk()
{
	static float	s_flLastUpdate;

	if ( !m_pHttpFile || !m_fpLocal || !m_pCurFile )
		return;

	char	buf[1024];
	UINT	n = m_pHttpFile->Read( buf, sizeof( buf ) );

	if ( n )
		fwrite( buf, n, 1, m_fpLocal );

	m_cbThisFile += n;
	m_cbReceived += n;

	float	now = (float)engineapi.Sys_FloatTime();
	if ( now - s_flLastUpdate > 0.1f )
	{
		char	szUrl[260];

		s_flLastUpdate = now;
		strcpy( szUrl, m_pCurFile->szUrl );

		float	pct = 0.0f;
		if ( m_cbExpected )
		{
			pct = (float)m_cbReceived / (float)m_cbExpected * 100.0f;
			if ( pct < 0.0f )
				pct = 0.0f;
			if ( pct > 100.0f )
				pct = 100.0f;
		}

		const char*	pszUrl = m_pMod->GetKey( "url_dl" );
		if ( !pszUrl )
			pszUrl = "?";

		SetStatus( now - m_flStartTime, Launcher_LoadString( IDS_MOD_DLSTATUS ),
			m_pMod->GetKey( "game" ), pszUrl, szUrl, pct );
	}

	if ( n != sizeof( buf ) )
	{
		fclose( m_fpLocal );
		m_fpLocal = NULL;
		m_pHttpFile->Close();
		m_pHttpFile = NULL;
		m_pCurFile  = m_pCurFile->pNext;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::ServiceDownload (0x42E970)
//
// (sic) the failed branch closes the dialog and then falls through into the
// transfer step; with the queue already torn down, the step is a no-op.

void CModHttpDownloadDlg::ServiceDownload()
{
	if ( !m_bDownloadOK )
	{
		if ( !m_bInitialized )
			return;
		OnOK();
	}

	if ( m_pHttpFile )
		ReadChunk();
	else
		NextFileOrFinish();
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::SetStatus (0x42E9B0)
//
// Shared with CModDownloadDlg: identical bodies, folded to one copy.

void CModHttpDownloadDlg::SetStatus( float flElapsed, const char* pszFmt, ... )
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
// CModHttpDownloadDlg::RMLPreIdle (0x42EAB0)

int CModHttpDownloadDlg::RMLPreIdle()
{
	CDlgPopupBase::RMLPreIdle();
	OnEngineFrame();
	ServiceDownload();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::StartDownload (0x42EAD0)

void CModHttpDownloadDlg::StartDownload()
{
	char	szUrl[260];

	m_pSession = new CInternetSession( "Half-Life", 1, PRE_CONFIG_INTERNET_ACCESS, 0, 0, 0 );
	if ( !m_pSession )
	{
		OnCancel();
		return;
	}

	const char*	pszUrl = m_pMod->GetKey( "url_dl" );
	if ( !pszUrl || !*pszUrl )
	{
		OnCancel();
		return;
	}
	sprintf( szUrl, pszUrl );		// (sic) the URL is used as the format string

	SetStatus( (float)engineapi.Sys_FloatTime() - m_flStartTime,
		Launcher_LoadString( IDS_MOD_CONNECT ), pszUrl );

	if ( IsUrlReachable( pszUrl ) )
	{
		SetStatus( (float)engineapi.Sys_FloatTime() - m_flStartTime,
			Launcher_LoadString( IDS_MOD_GETTINGSIZE ) );
		m_cbExpected = BuildFileList( pszUrl );
		if ( m_cbExpected > 0 )
		{
			SetStatus( (float)engineapi.Sys_FloatTime() - m_flStartTime,
				Launcher_LoadString( IDS_MOD_DLSTATUSSHORT ),
				m_pMod->GetKey( "game" ), szUrl );
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

		sprintf( szMsg, Launcher_LoadString( IDS_MOD_NOLIBLIST ), m_pMod->GetKey( "game" ) );
		SetStatus( (float)engineapi.Sys_FloatTime() - m_flStartTime, szMsg );
		Sleep( 5000 );
	}

	if ( !m_bDownloadOK )
		OnCancel();
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::BuildFileList (0x42EE80)
//
// One node: an HTTP mod is a single archive, so the "list" never grows.

int CModHttpDownloadDlg::BuildFileList( const char* pszUrl )
{
	DWORD			dwService = 0;
	CString			strServer, strObject;
	INTERNET_PORT	nPort = 0;

	engineapi.Sys_FloatTime();

	if ( !AfxParseURL( pszUrl, dwService, strServer, strObject, nPort )
		|| dwService != AFX_INET_SERVICE_HTTP )
		return 0;

	httpfile_t*		pNode = ModHttp_NewFile();
	CInternetFile*	pFile = (CInternetFile*)m_pSession->OpenURL( pszUrl, 1,
		INTERNET_FLAG_TRANSFER_BINARY, 0, 0 );

	if ( pFile )
	{
		pNode->cbLength = (int)pFile->GetLength();
		pFile->Close();
	}

	char	szObject[260];

	strcpy( szObject, strObject );

	const char*	pszName = COM_SkipPath( szObject );

	strcpy( pNode->szUrl, pszUrl );
	sprintf( pNode->szDest, "%s/%s", m_pMod->GetKey( "gamedir" ), pszName );

	pNode->pNext = m_pFileList;
	m_pFileList  = pNode;
	return pNode->cbLength;
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::IsUrlReachable (0x42F110)

BOOL CModHttpDownloadDlg::IsUrlReachable( const char* pszUrl )
{
	DWORD			dwService = 0;
	CString			strServer, strObject;
	INTERNET_PORT	nPort = 0;

	if ( !AfxParseURL( pszUrl, dwService, strServer, strObject, nPort )
		|| dwService != AFX_INET_SERVICE_HTTP )
		return FALSE;

	CInternetFile*	pFile = (CInternetFile*)m_pSession->OpenURL( pszUrl, 1,
		INTERNET_FLAG_TRANSFER_BINARY, 0, 0 );
	if ( !pFile )
		return FALSE;

	BOOL	bOk = ( pFile->GetLength() != 0 );

	pFile->Close();
	return bOk;
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::ExtractArchive (0x42F320)
//
// A .zip is unpacked into the gamedir; a .exe is run there and waited on.
// The prompt names the source URL, not the file that was written.

void CModHttpDownloadDlg::ExtractArchive()
{
	if ( !m_bDownloadOK )
		return;
	m_bDownloadOK = 0;

	httpfile_t*	p = m_pFileList;
	if ( !p )
		return;

	char	szExt[_MAX_EXT];

	_splitpath( p->szUrl, NULL, NULL, NULL, szExt );

	int	bZip;
	if ( !_stricmp( szExt, ".exe" ) )
		bZip = 0;
	else if ( !_stricmp( szExt, ".zip" ) )
		bZip = 1;
	else
		return;

	char	szApp[260];
	char	szBuf[256];

	strcpy( szApp, p->szUrl );

	CPromptDlg	dlg( 2, NULL );		// OK + Cancel

	sprintf( szBuf, Launcher_LoadString( IDS_MOD_UNZIP ), szApp );
	dlg.SetMessage( szBuf );
	if ( dlg.DoModal() == IDOK )
	{
		if ( bZip )
		{
			char*	argv[4];

			argv[0] = _strdup( "unzip" );
			argv[1] = _strdup( "-d" );
			argv[2] = _strdup( m_pMod->GetKey( "gamedir" ) );
			argv[3] = _strdup( p->szDest );
			UzpMain( ARRAYSIZE( argv ), argv );

			free( argv[0] );
			free( argv[1] );
			free( argv[2] );
			free( argv[3] );
		}
		else
		{
			STARTUPINFO			si;
			PROCESS_INFORMATION	pi;

			memset( &si, 0, sizeof( si ) );
			si.cb = sizeof( si );
			memset( &pi, 0, sizeof( pi ) );

			if ( CreateProcessA( szApp, NULL, NULL, NULL, FALSE, 0, NULL,
					m_pMod->GetKeyString( "gamedir" ), &si, &pi ) )
			{
				DWORD	dwCode = STILL_ACTIVE;

				while ( GetExitCodeProcess( pi.hProcess, &dwCode ) && dwCode == STILL_ACTIVE )
					Sleep( 500 );
				CloseHandle( pi.hProcess );
			}
		}
		OnOK();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::OnEraseBkgnd (0x4112E0)

BOOL CModHttpDownloadDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	CDlgPopupBase::OnPaint();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg::OnPaint (0x4113F0)

void CModHttpDownloadDlg::OnPaint()
{
	CDlgPopupBase::OnPaint();
}
