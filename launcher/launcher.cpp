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
// Purpose: the launcher application itself: CNetGameApp / CLauncher boot,
//          CD-key, bitmap blit and the profile wrappers.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"
#ifdef _DEBUG
#endif

#define GFX_SHELL_DIR	"gfx/shell/"

// The two CD keys that reset a stored key.
#define CDKEY_RESETKEY1 "2123437429222"
#define CDKEY_RESETKEY2 "1911111111115"

exefuncs_t	ef;

int			g_nNumPings = 1;	// (4D1BF8)
char		gPendingMap[260];	// (4E3504)
int			gWSASockets = 1;
int			g_bRestartLauncher;

// Registry "Settings\Version" schema revision.  A higher stored value means the
// settings were written by a newer build, and Sys_ResetSettings has to run.
#define LAUNCHER_SETTINGS_VERSION	1

#if defined(LAUNCHER_RE) && defined(_DEBUG)
/*
==================
DestroyMFCSAsserts

Not original.
==================
*/
static int __cdecl DestroyMFCSAsserts( int reportType, char* message, int* retVal )
{
	if ( reportType == _CRT_ASSERT || reportType == _CRT_ERROR )
	{
		if ( message )
		{
			static char	s_seen[32][160];
			static int	s_nSeen = 0;
			int			i;

			for ( i = 0; i < s_nSeen; ++i )
				if ( strncmp( s_seen[i], message, sizeof( s_seen[0] ) - 1 ) == 0 )
					break;

			if ( i == s_nSeen )
			{
				if ( s_nSeen < 32 )
				{
					strncpy( s_seen[s_nSeen], message, sizeof( s_seen[0] ) - 1 );
					s_seen[s_nSeen][sizeof( s_seen[0] ) - 1] = 0;
					++s_nSeen;
				}
				OutputDebugStringA( message );
			}
		}
		if ( retVal )
			*retVal = 0;
		return TRUE;
	}
	return FALSE;
}
#endif

// engine / video mode
enginemode_t	g_EngineMode;
int				gA3dSupport;
int				windowed_mouse = 1;		// (4D06CC)
HINSTANCE		gLauncherHandle;		// (4E1F28)

// error / quit state, window chrome, titles
static char		g_szErrorMessage[256];	// (4E329C)
int				g_nQuitRequest;			// (4E392C)
int				g_nErrorState;			// (4E3930)
UINT			guMouseWheelMsg;		// (4E3904)
int				gTopLevelFrame = 0;	// (4E3928)
// The application object, stored by the constructor.  The free profile wrappers,
// Launcher_FormatAppName and Launcher_GetResourceModule all go through this
// rather than touching theApp directly.
class CNetGameApp*	g_pTheApp;			// (4E3918)
char			g_szAppName[256];		// (4E2FB8)
int				g_nLauncherDefW = 640;	// default launcher window width  (4CF8A0)
int				g_nLauncherDefH = 480;	// default launcher window height (4CF8A4)
char			g_szShortTitle[64];		// (4E2E94)
char			g_szPatchVersion[32];	// (4E34A0)

// menu state
int		g_nMenuState = 0;	// (4E2174)
int		g_nMenuShown = 0;	// (4E2150)

/////////////////////////////////////////////////////////////////////////////
// CNetGameApp

class CNetGameApp : public CWinApp
{
// Construction
public:
					CNetGameApp();

// Attributes
public:
	HMODULE			m_hResDll;			// +192
	int				m_nLauncherBPP;		// +196
	int				m_nLauncherWidth;	// +200
	int				m_nLauncherHeight;	// +204
	int 			m_iCPUMhz;			// +208
	CMutex*			m_pInstanceGuard;	// +212

// Operations
public:
	void			InitExeFuncs();
	int				CheckDisplayCaps();
	BOOL			LoadRes();
	void			SetEngineMode();

// Overrides
	//{{AFX_VIRTUAL(CNetGameApp)
public:
	virtual BOOL	InitInstance();
	virtual int		ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual 		~CNetGameApp();

	// Generated message map functions
	//{{AFX_MSG(CNetGameApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP( CNetGameApp, CWinApp )
	//{{AFX_MSG_MAP(CNetGameApp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CNetGameApp	theApp;


// menu button strip + shared background DIB
static HGLOBAL	g_hMainButtonsBmp;		// (4E395C)
HGLOBAL			g_hBackgroundDib = NULL;// (4E3960) shared background DIB (extern in launcher.h)
static int		g_bMainButtonsLoaded;	// (4E3964)
int				g_nMenuButtonWidth;		// (4E3950)
int				g_nMenuButtonHeight;	// (4E3954)
int				g_nMenuButtonCount;		// (4E3958)

static char		g_szStoredKey[16];		// (4E394C)

// connect / auth state
int		g_bAuthPromptActive;
int		g_bConnectEstablished;	// (4E1828)
int		g_bConnectInProgress;	// (4E194C)

unsigned char	gLauncherChecksum[16];	// (4E2D98)
crypt_api_t		crypt;					// (4E2E30)
crypt_parms_t	gCryptParms;			// (4E3140)
char			g_szGUID[65];			// (4E34C0)

CServerBrowser* g_pServerBrowser;

static BOOL	CDKey_IsValid( const char* key );

// Routines private to this module (not exported through launcher.h).
static int			HashEngineDlls( void );
static BOOL		GetExecutableName( char* lpFilename );
static char*		StripExtension( const char* src, char* dst );
static int			CheckExeChecksum( void );
static void		Launcher_CheckVersion( void );
static void		ErrorMessage( int nLevel, const char* pszErrorMessage );
static char*		Launcher_BuildCommandLine( void );
static int			WSA_Init( void );
static HINSTANCE	GetInstanceHandle( void );
static const char*	GetAppName( void );
static int			Launcher_ResetMainButtons( void );
static void		ChangeGameDirectory( const char* pszNewDirectory );

/*
==================
RunSierraUpdate (0x421850)
==================
*/
static int RunSierraUpdate( void )
{
	WinExec( "sierraup.exe", SW_SHOW );
	return 1;
}

/*
==================
Launcher_CheckVersion (0x421867)
==================
*/
static void Launcher_CheckVersion( void )
{
	int version = Launcher_GetProfileInt( "Settings", "Version", 0 );
	if ( version > LAUNCHER_SETTINGS_VERSION )
	{
		Launcher_ShowMessageById( 0, IDS_REGISTRY_UPDATE );
		Sys_ResetSettings();
	}
	Launcher_WriteProfileInt( "Settings", "Version", LAUNCHER_SETTINGS_VERSION );
}

/*
==================
Eng_ShouldReload (0x4218cc)
==================
*/
int Eng_ShouldReload( void )
{
	GameInfo_t	gi;

	if ( !engineapi.GetGameInfo(&gi, 0) )
		return 0;

	if ( gi.active && gi.maxclients > 1 )
		return 1;

	return gi.state != ca_disconnected && !gi.active;
}

/*
==================
Launcher_FindGameCD (0x421924)
==================
*/
static int Launcher_FindGameCD( void )
{
	char	szDrive[8];
	char	szPath[MAX_PATH];

	SetErrorMode( SEM_FAILCRITICALERRORS );

	HCURSOR	hWait  = LoadCursorA( NULL, IDC_WAIT );
	HCURSOR	hArrow = LoadCursorA( NULL, IDC_ARROW );
	SetCursor( hWait );

	strcpy( szDrive, "c:" );
	for ( szDrive[0] = 'c'; szDrive[0] <= 'z'; ++szDrive[0] )
	{
		if ( GetDriveTypeA( szDrive ) != DRIVE_CDROM )
			continue;

		sprintf( szPath, "%s\\valve.ico", szDrive );
		FILE*	fp = fopen( szPath, "r" );
		if ( fp )
		{
			fclose( fp );
			SetCursor( hArrow );
			return 1;
		}
	}
	SetCursor( hArrow );
	return 0;
}

/*
==================
CheckParm (0x421a28)
==================
*/
char* CheckParm( const char* psz, char** ppszValue )
{
	char*	pret;
	static char	sz[129];

	if ( !gpszCmdLine )
		return NULL;

	pret = strstr( gpszCmdLine, psz );
	if ( pret && ppszValue )
	{
		char*	p1 = pret;
		char*	pv;
		int		i;

		*ppszValue = NULL;

		while ( *p1 && *p1 != ' ' )
			++p1;

		pv = p1 + 1;
		for ( i = 0; i < 128 && *pv && *pv != ' '; ++i )
			sz[i] = *pv++;
		sz[i] = '\0';
		*ppszValue = sz;
	}
	return pret;
}

/*
==================
Launcher_BuildCommandLine (0x421B11)
==================
*/
static char* Launcher_BuildCommandLine( void )
{
	static char	expanded[8192];
	char		filename[260];
	char		msg[512];
	char*		raw;
	char*		src;
	char*		dst = expanded;

	raw = _strdup( GetCommandLineA() );

	for ( src = raw; *src; )
	{
		if ( *src == '@' )
		{
			char*	f = filename;
			FILE*	fp;

			++src;
			while ( *src && *src != ' ' )
				*f++ = *src++;
			*f = 0;
			if ( *src )
				++src;

			fp = fopen( filename, "r" );
			if ( fp )
			{
				int c;
				for ( c = fgetc( fp ); c != EOF; c = fgetc( fp ) )
				{
					if ( c == '\n' )
						c = ' ';
					*dst++ = (char)c;
				}
				*dst++ = ' ';
				fclose( fp );
			}
			else
			{
				sprintf( msg, "Parameter file '%s' not found, skipping...", filename );
				AfxMessageBox( msg );
			}
		}
		else
		{
			*dst++ = *src++;
		}
	}
	*dst = 0;

	free( raw );
	return _strdup( expanded );
}

/*
==================
IsValidCD (0x421D4B)
==================
*/
int IsValidCD( void )
{
	return g_bValidCD;
}

/*
==================
GetCDKey (0x421D55)
==================
*/
char* GetCDKey( char* pszCDKey, int* nLength, int* bDedicated )
{
	// This is the GUI launcher, never a dedicated server.
	if ( bDedicated )
		*bDedicated = 0;

	if ( nLength )
		*nLength = strlen( g_szGUID );

	return strcpy( pszCDKey, g_szGUID );
}

/*
==================
UTIL_CheckCDKey (0x421D92)
==================
*/
static int UTIL_CheckCDKey( void )
{
#ifdef LAUNCHER_RE
	// LAUNCHER_RE placeholder: skip CD-key validation and use a fixed GUID.
	strcpy( g_szGUID, "2335-40262-8334" );
	return 1;
#else
	CString	key = Launcher_GetProfileString( "Settings", "Key", "" );

	if ( !key.IsEmpty()
	  && ( !_strcmpi( key, CDKEY_RESETKEY1 )
	    || !_strcmpi( key, CDKEY_RESETKEY2 ) ) )
	{
		Launcher_WriteProfileString( "Settings", "Key", "" );
		key.Empty();
	}
	if ( !key.IsEmpty() )
	{
		if ( CDKey_IsValid( key ) )
		{
			Launcher_WriteProfileString( "Settings", "Key", key );
			strcpy( g_szGUID, key );
			return 1;
		}

		Launcher_WriteProfileString( "Settings", "Key", "" );
		Launcher_ShowMessageById( 0, IDS_CD_BADKEYTYPED );
	}
	for ( ;; )
	{
		CInputDlg	dlg;
		dlg.SetPrompt( Launcher_LoadString( IDS_CD_ENTERPROMPT ) );
		if ( dlg.DoModal() != IDOK )
			return 0;

		char	digits[65];
		int		n = 0;
		int		len = dlg.m_strInput.GetLength();
		const char*	s = dlg.m_strInput;
		for ( int i = 0; i < len && n < 64; ++i )
		{
			if ( s[i] >= '0' && s[i] <= '9' )
				digits[n++] = s[i];
		}
		digits[n] = 0;

		if ( n == 0 )
		{
			Launcher_ShowMessageById( 0, IDS_CD_NEEDCDKEY );
			return 0;
		}
		if ( !_strcmpi( digits, CDKEY_RESETKEY1 )
		  || !_strcmpi( digits, CDKEY_RESETKEY2 ) )
		{
			Launcher_WriteProfileString( "Settings", "Key", "" );
			Launcher_ShowMessageById( 0, IDS_CD_BADKEY );
			continue;
		}
		if ( CDKey_IsValid( digits ) )
		{
			Launcher_WriteProfileString( "Settings", "Key", digits );
			strcpy( g_szGUID, digits );
			return 1;
		}

		Launcher_WriteProfileString( "Settings", "Key", "" );
		Launcher_ShowMessageById( 0, IDS_CD_BADKEY );
	}
#endif
}

/*
==================
CheckCDKey (0x422248)
==================
*/
int CheckCDKey( void )
{
	return UTIL_CheckCDKey();
}

/*
==================
Launcher_HandleAuthFailure (0x422252)
==================
*/
void Launcher_HandleAuthFailure( void )
{
	void*	pAuth;

	Launcher_SetQuitFlag( 0 );

	pAuth = crypt.GetAuthObject();
	if ( !pAuth || !CryptApi_AuthHasError( pAuth ) )
		return;

	if ( engineapi.Cbuf_AddText )
		engineapi.Cbuf_AddText( "disconnect\n" );

	if ( g_bConnectInProgress )
	{
		if ( g_bAuthPromptActive )
			g_bAuthPromptActive = 0;
		g_bConnectEstablished = 0;
	}
	Eng_Frame( 1 );
	gDLLState = DLL_PAUSED;
	g_iForcedState = DLL_PAUSED;
	VID_HideEngineWindow();

	if ( Launcher_GetRestartFlag() )
		return;

	Launcher_ErrorMessageBox( 0, CryptApi_AuthErrorString( pAuth ) );

	if ( CryptApi_AuthErrorState( pAuth ) != -1 && CryptApi_AuthErrorCode( pAuth ) == -1501 )
	{
		if ( CryptApi_AuthErrorState( pAuth ) == 0 )
		{
			// Bad CD key: restore the stored key and re-prompt.
			Launcher_WriteProfileString( "Settings", "Key", g_szStoredKey );
			if ( !CheckCDKey() )
				PostQuitMessage( 0 );
		}
		else if ( CryptApi_AuthErrorState( pAuth ) == 1 )
		{
			// "Update required - restart to apply?"
			CPromptDlg	dlg( 2 );
			dlg.SetMessage( Launcher_LoadString( IDS_RUN_PATCH ) );
			if ( dlg.DoModal() == IDOK )
				Launcher_SetRestartFlag( 1 );
		}
	}
}

/*
==================
Console_Printf (0x422406)
==================
*/
void Console_Printf( char* fmt, ... )
{
#ifdef LAUNCHER_RE
	char	buf[1024];
	va_list	va;
	va_start( va, fmt );
	_vsnprintf( buf, sizeof( buf ) - 1, fmt ? fmt : "", va );
	buf[sizeof( buf ) - 1] = 0;
	va_end( va );
	LOG( "[engine] %s", buf );
#endif
}

/*
==================
ErrorMessage (0x42240B)
==================
*/
static void ErrorMessage( int nLevel, const char* pszErrorMessage )
{
	Launcher_SetErrorMessage( pszErrorMessage );
	Launcher_SetErrorState( nLevel + 1 );
	VID_HideEngineWindow();
	AfxMessageBox( pszErrorMessage, 0, 0 );
// no return -- the engine is gone, take the launcher down with it
	exit( 1 );
}

/*
==================
COM_Init (0x422454)
==================
*/
void COM_Init( void )
{
	byte	swaptest[2] = { 1, 0 };

// set the byte swapping variables in a portable manner
	bigendien = false;
	BigShort = ShortSwap;
	LittleShort = ShortNoSwap;
	BigLong = LongSwap;
	LittleLong = LongNoSwap;
	BigFloat = FloatSwap;
	LittleFloat = FloatNoSwap;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameApp::CNetGameApp (0x4224FB)

CNetGameApp::CNetGameApp()
{
	sprintf( g_szConfigName, "config.cfg" );
	sprintf( com_gamedir, "valve" );

	Eng_DeferMenuButton( 0 );
	Snd_ResetSlots();

	g_pTheApp         = this;
	m_hResDll         = NULL;
	m_nLauncherBPP    = 16;
	m_nLauncherWidth  = g_nLauncherDefW;
	m_nLauncherHeight = g_nLauncherDefH;

	g_pServerBrowser  = NULL;

	Launcher_ResetMainButtons();
	Sys_Init();
	COM_Init();
	VGui_Start();
}

/*
==================
CDKey_IsValid (0x4225DB)
==================
*/
static BOOL CDKey_IsValid( const char* key )
{
	if ( !key || !*key )
		return FALSE;

	if ( strlen( key ) == 13 )
		return CDKey_Checksum( key );

	return FALSE;
}

/*
==================
Launcher_CreateServerBrowser (0x42262B)
==================
*/
void Launcher_CreateServerBrowser( void )
{
	if (g_pServerBrowser)
		delete g_pServerBrowser;

	g_pServerBrowser = new CServerBrowser();
	g_pServerBrowser->SaveFavoriteServers();
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameApp::~CNetGameApp (0x4226DB)

CNetGameApp::~CNetGameApp()
{
	Launcher_FreeSplashBitmap();

	// Only g_pServerBrowser is nulled after its delete; gFavorites is not.
	if ( gFavorites )
		delete gFavorites;
	if ( g_pServerBrowser )
	{
		delete g_pServerBrowser;				// scalar deleting dtor ()
		g_pServerBrowser = NULL;
	}
	COM_Shutdown();

	Eng_PreLoad();
}

/*
==================
Launcher_GetGLDriver (0x4227E5)
==================
*/
char* Launcher_GetGLDriver( void )
{
	static char	szdriver[256];

	if ( !g_EngineMode.glDriver[0] )
		return NULL;

	if ( !_strcmpi( g_EngineMode.glDriver, "default" ) )
		return NULL;

	sprintf( szdriver, "%s", g_EngineMode.glDriver );
	return szdriver;
}

/*
==================
Launcher_InitCmds (0x42283C)
==================
*/
void Launcher_InitCmds( void )
{
#ifndef SWDS
	engineapi.Cmd_AddCommand( "cd", CD_f );
#endif
}

/*
==================
ChangeGameDirectory (0x422854)
==================
*/
static void ChangeGameDirectory( const char* pszNewDirectory )
{
	struct mod_s*	pMod;
	CWnd*			pWnd;

	pMod = ModList_FindByGamedir( &g_pModList, pszNewDirectory );
	if ( !pMod || pMod == g_pCurrentMod )
		return;

	Launcher_SavePlayerInfo();
	Sys_SetCmdLineParm( "-game", pszNewDirectory );
	COM_ResetGameDirectories();
	COM_AddGameDirectory( 0, COM_GetBaseDir(), pszNewDirectory );
	g_pCurrentMod = pMod;
	Launcher_OnGameDirChanged();

	pWnd = AfxGetMainWnd();
	if ( pWnd )
		( (CHLMainDlg *) pWnd )->RefreshDialogSkin( );

	Eng_SetCurrentMod();
}

/*
==================
HashEngineDlls (0x4228E9)
==================
*/
static int HashEngineDlls( void )
{
	char filename[260];

	strcpy( filename, COM_GetBaseDir() );
	_chdir( filename );

	if ( !GetModuleFileNameA( GetInstanceHandle(), filename, sizeof( filename ) ) )
		return 0;

	strcat( filename, ";sw.dll;hw.dll" );

	if ( crypt.MD5_File( gLauncherChecksum, filename ) )
		return 1;

	memset( gLauncherChecksum, 0, sizeof(gLauncherChecksum) );
	return 0;
}

/*
==================
AuthFailed (0x42298C)
==================
*/
void AuthFailed( void )
{
	Launcher_SetQuitFlag( 1 );
}

/*
==================
GetLocalizedString (0x42299B)
==================
*/
char* GetLocalizedString( unsigned int uID )
{
	return Launcher_LoadString( uID );
}

static void D_BeginDirectRect( int x, int y, byte* pbitmap, int width, int height )
{
}
static void D_EndDirectRect( int x, int y, int width, int height )
{
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameApp::InitExeFuncs (0x4229ac)

void CNetGameApp::InitExeFuncs( void )
{
	char*	pszValue;

	memset( &ef, 0, sizeof( ef ) );

	ef.AuthFailed			= AuthFailed;
	ef.ChangeGameDirectory	= ChangeGameDirectory;
	ef.Console_Printf		= Console_Printf;
	ef.GetCDKey				= GetCDKey;
	ef.IsValidCD			= IsValidCD;
	ef.InitCmds				= Launcher_InitCmds;
	ef.VID_LockBuffer		= VID_LockBuffer;
	ef.VID_UnlockBuffer		= VID_UnlockBuffer;
	ef.VID_Shutdown			= VID_Shutdown;
	ef.VID_Update			= VID_Update;
	ef.VID_ForceLockState	= VID_ForceLockState;
	ef.EF_VID_ForceUnlockedAndReturnState = VID_ForceUnlockedAndReturnState;
	ef.EF_VID_ForceLockState = VID_SetDefaultMode;
	ef.VID_GetExtModeDescription = VID_GetExtModeDescription;
	ef.VID_GetVID			= VID_GetVID;
	ef.D_BeginDirectRect	= D_BeginDirectRect;
	ef.D_EndDirectRect		= D_EndDirectRect;
	ef.AppActivate			= AppActivate;
	ef.CDAudio_Play			= CDAudio_Play;
	ef.CDAudio_Pause		= CDAudio_Pause;
	ef.CDAudio_Resume		= CDAudio_Resume;
	ef.CDAudio_Update		= CDAudio_Update;
	ef.ErrorMessage			= ErrorMessage;
	ef.D_SurfaceCacheForRes	= D_SurfaceCacheForRes;
	ef.GetLocalizedString	= GetLocalizedString;

	if ( CheckParm( "-mmx", NULL ) )
		ef.fMMX = 1;
	else if ( CheckParm( "-nommx", NULL ) )
		ef.fMMX = 0;
	else
		ef.fMMX = Sys_CheckMMXTechnology();

	m_iCPUMhz = GetProfileInt( "Settings", "CPUMHZ", 0 );
	ef.iCPUMhz = m_iCPUMhz;
	if ( CheckParm( "-cpumhz", &pszValue ) && pszValue )
		ef.iCPUMhz = atoi( pszValue ) * 1024;
	if ( !ef.iCPUMhz )
		ef.iCPUMhz = Sys_GetCPUSpeed();

	if ( CheckParm( "numpings", &pszValue ) && pszValue )
	{
		g_nNumPings = atoi( pszValue );
		if ( g_nNumPings < 1 )
			g_nNumPings = 1;
		if ( g_nNumPings > 32 )
			g_nNumPings = 32;
	}
	Launcher_WriteProfileInt( "Settings", "CPUMHZ", m_iCPUMhz );
	ef.InitCmds = Launcher_InitCmds;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameApp::LoadRes (0x422BF1)
//
// LAUNCHER_STATIC compiles shell/hl_res/hl_res.rc straight into hl.exe (see the
// root CMakeLists.txt), so the launcher's own module already has those
// resources and there is no HL_Res.dll to load.

BOOL CNetGameApp::LoadRes()
{
#ifdef LAUNCHER_STATIC
	m_hResDll = GetModuleHandle( NULL );
	return TRUE;
#else
	m_hResDll = LoadLibraryA( "HL_Res.dll" );
	if ( m_hResDll )
		return TRUE;

	AfxMessageBox( "Could not load HL_Res.dll", 0, 0 );
	return FALSE;
#endif
}

/*
==================
Launcher_DisableA3DSplash (0x422C33)
==================
*/
static BOOL Launcher_DisableA3DSplash( void )
{
	HKEY	hKey;
	DWORD	disp;
	DWORD	zero = 0;

	if ( RegCreateKeyExA( HKEY_LOCAL_MACHINE, "Software\\Aureal\\A3D", 0, NULL, 0,
			KEY_WRITE, NULL, &hKey, &disp ) )
		return FALSE;

	RegSetValueExA( hKey, "SplashScreen", 0, REG_DWORD, (BYTE*)&zero, sizeof( zero ) );
	RegCloseKey( hKey );
	return TRUE;
}

/*
==================
Launcher_SetMatroxOGLWrapper (0x422CA3)
==================
*/
static void Launcher_SetMatroxOGLWrapper( void )
{
	HKEY	hKey;

	if ( RegCreateKeyA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Matrox\\PowerDesk\\Current Settings", &hKey ) )
		return;

	RegSetValueExA( hKey, "OGL Wrapper", 0, REG_SZ, (const BYTE*)"1", 1 );
	RegFlushKey( hKey );
	RegCloseKey( hKey );
}

/*
==================
Launcher_ClearMatroxOGLWrapper (0x422D00)
==================
*/
static LSTATUS Launcher_ClearMatroxOGLWrapper( void )
{
	HKEY	hKey;
	LSTATUS	status;

	status = RegOpenKeyA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Matrox\\PowerDesk\\Current Settings", &hKey );
	if ( status )
		return status;

	RegDeleteValueA( hKey, "OGL Wrapper" );
	return RegCloseKey( hKey );
}

/*
==================
GetExecutableName (0x422D3E)
==================
*/
static BOOL GetExecutableName( char* lpFilename )
{
	return GetModuleFileNameA( GetModuleHandleA( 0 ), lpFilename, 256 ) != 0;
}

/*
==================
StripExtension (0x422D68)
==================
*/
static char* StripExtension( const char* src, char* dst )
{
	while ( *src && *src != '.' )
		*dst++ = *src++;
	*dst = 0;
	return dst;
}

/*
==================
CheckExeChecksum (0x422DA6)
==================
*/
static int CheckExeChecksum( void )
{
	char	szFileName[256];
	char	datfile[256];
	FILE*	fp;

	if ( !GetExecutableName( szFileName ) )
		return 0;

	StripExtension( szFileName, datfile );
	strcat( datfile, ".dat" );

	fp = fopen( datfile, "rb" );
	if ( fp && !CheckParm( "-newdat", 0 ) )
	{
		unsigned int	stored = 0;
		int				bOk = ( fread( &stored, 4, 1, fp ) == 1 );
		fclose( fp );

		if ( bOk && *(unsigned int*)gLauncherChecksum != stored )
		{
#ifndef LAUNCHER_RE
			AfxMessageBox( Launcher_LoadString( IDS_WON_MODIFIED ), MB_ICONWARNING, 0 );
#endif
			Launcher_FreeMainButtonsBitmap();
			return 0;
		}
	}
	else
	{
		unsigned int	current = *(unsigned int*)gLauncherChecksum;

		if ( fp )
			fclose( fp );

		fp = fopen( datfile, "wb" );
		if ( fp )
		{
			fwrite( &current, 4, 1, fp );
			fclose( fp );
		}
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameApp::CheckDisplayCaps (0x422F3B)

int CNetGameApp::CheckDisplayCaps( void )
{
	CDC	dc;

	if ( !dc.CreateCompatibleDC( NULL ) )
	{
		AfxMessageBox( Launcher_LoadString( IDS_MAIN_NOCAPS ) );
		return 0;
	}
	int	caps = ::GetDeviceCaps( dc.GetSafeHdc(), RASTERCAPS );
	if (   !( caps & RC_BITBLT ) 
		|| !( caps & RC_DI_BITMAP ) 
		|| !( caps & RC_DIBTODEV )
		|| !( caps & RC_STRETCHBLT ) 
		|| !( caps & RC_STRETCHDIB ) )
	{
		AfxMessageBox( Launcher_LoadString( IDS_MAIN_INSUFFICIENTCAPS ) );
	}
	dc.DeleteDC();
	return 1;
}

/*
==================
WSA_GetNumSockets (0x423070)
==================
*/
int WSA_GetNumSockets( void )
{
	return gWSASockets;
}

/*
==================
WSA_Init (0x42307A)
==================
*/
static int WSA_Init( void )
{
	WSADATA	wsaData;

	if ( !AfxSocketInit( &wsaData ) )
	{
		Launcher_ShowMessageById( 0, IDS_SERVER_MENU_CONNECT );
		return 0;
	}
	gWSASockets = wsaData.iMaxSockets - 8;
	if ( gWSASockets <= 4 )
	{
		Launcher_ShowMessageById( 0, IDS_MAIN_INSUFFICIENTSOCKETS );
		gWSASockets = 1;
	}
	return 1;
}

/*
==================
ReadSierraIdent (0x4230E3)
==================
*/
void ReadSierraIdent( char* pszTitle, char* pszVersion, const char* pszBaseDir )
{
	char	szPath[260];
	char	szSection[16384];
	int		len;
	char*	p;

	sprintf( szPath, "%s\\sierra.inf", pszBaseDir );
	memset( szSection, 0, sizeof( szSection ) );
	len = (int)GetPrivateProfileSectionA( "Ident", szSection, sizeof( szSection ), szPath );

	// version
	strcpy( pszVersion, VERSION_STRING );
	if ( len && len != sizeof( szSection ) - 2 )
	{
		for ( p = szSection; (int)( p - szSection - strlen( "PatchVersion=" ) ) <= len; ++p )
		{
			if ( !_strnicmp( p, "PatchVersion=", strlen( "PatchVersion=" ) ) )
			{
				strcpy( pszVersion, p + strlen( "PatchVersion=" ) );
				break;
			}
		}
	}
	// title
	strcpy( pszTitle, "HALFLIFE" );
	if ( len && len != sizeof( szSection ) - 2 )
	{
		for ( p = szSection; (int)( p - szSection - strlen( "ShortTitle=" ) ) <= len; ++p )
		{
			if ( !_strnicmp( p, "ShortTitle=", strlen( "ShortTitle=" ) ) )
			{
				strcpy( pszTitle, p + strlen( "ShortTitle=" ) );
				break;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameApp::InitInstance (0x423306)

BOOL CNetGameApp::InitInstance()
{
	char	szCommFile[260];
	char	szExeName[260];
	char	szGUID[260];
	char*	pszValue;
	int		buttonCell[2] = { 0, 0 };
	int		bChecksum;
	int		bDispCaps;
	int		bSockets;
	int		bCDKey;
	CWaitCursor	wait;

	LOG_ENTER();

#if defined( LAUNCHER_RE ) && defined( _DEBUG )
	_CrtSetReportHook( DestroyMFCSAsserts );
#endif

	gLauncherHandle = GetInstanceHandle();

	if ( !LoadRes() || !Launcher_DisableA3DSplash() )
	{
		LOG( "LoadRes/DisableA3DSplash failed -- abort" );
		return FALSE;
	}
	SetRegistryKey( "Valve" );
	LoadStdProfileSettings( 4 );

	SetFileAttributesA( "sierra.inf", FILE_ATTRIBUTE_NORMAL );
	SetFileAttributesA( "valve/config.cfg", FILE_ATTRIBUTE_NORMAL );

	Launcher_SetMatroxOGLWrapper();
	Launcher_SetRestartFlag( 0 );

#ifndef LAUNCHER_RE
	// Single-instance guard.  The binary tests the pointer and then calls through
	// vtable slot 3 -- CSyncObject::Lock( DWORD ) -- rather than reaching into
	// m_hObject and waiting on it directly.
	m_pInstanceGuard = new CMutex( FALSE, "HALFLIFELAUNCHER", NULL );
	if ( !m_pInstanceGuard || !m_pInstanceGuard->Lock( 0 ) )
	{
		LOG( "another launcher instance owns HALFLIFELAUNCHER -- abort" );
		return FALSE;
	}
#endif

	guMouseWheelMsg = RegisterWindowMessageA( "MSWHEEL_ROLLMSG" );

	do
	{
		if ( gpszCmdLine )
			free( gpszCmdLine );
		gpszCmdLine = Launcher_BuildCommandLine();
		LOG( "cmdline: %s", gpszCmdLine ? gpszCmdLine : "(null)" );

		COM_InitFilesystem();

		Launcher_LoadScheme();
		Launcher_LoadStrings();

		ReadSierraIdent( g_szShortTitle, g_szPatchVersion, COM_GetBaseDir() );

		Launcher_ComputeButtonCell( buttonCell );
		Launcher_SetButtonCell( buttonCell );
		Launcher_LoadMainButtonsBitmap();

		ModList_Clear( 0 );
		ModList_Scan();
		LOG( "mod list scanned" );

		srand( clock() );

		memset( g_szGUID, 0, sizeof( g_szGUID ) );

		if ( !Crypt_ReturnAPI( CRYPT_API_VERSION, &crypt ) )
		{
#ifndef LAUNCHER_RE
			break;
#else
			LOG( "[LAUNCHER_RE] Crypt_ReturnAPI failed -- continuing to the menu" );
#endif
		}
		if ( !HashEngineDlls() )
		{
#ifndef LAUNCHER_RE
			AfxMessageBox( Launcher_LoadString( IDS_MD5_HASHFAIL ), 0, 0 );
			return FALSE;
#else
			LOG( "[LAUNCHER_RE] HashEngineDlls failed -- continuing" );
#endif
		}
#ifndef LAUNCHER_RE
		if ( !CheckExeChecksum()
		  || !CheckDisplayCaps()
		  || !WSA_Init() )
		{
			break;
		}
#else
		// The rebuilt exe never matches the shipped checksum, and || would then
		// skip WSA_Init -- leaving AfxSocketInit uncalled, so every CAsyncSocket
		// (LAN broadcast, master query) fails to open.  Run all three.
		bChecksum = CheckExeChecksum();
		bDispCaps = CheckDisplayCaps();
		bSockets  = WSA_Init();
		if ( !bChecksum || !bDispCaps || !bSockets )
			LOG( "[LAUNCHER_RE] gate failed (checksum=%d caps=%d sockets=%d) -- continuing",
				bChecksum, bDispCaps, bSockets );
#endif
		// The binary calls CWinApp::Enable3dControlsStatic() here.  MFC 4.2 used it
		// to statically link CTL3D32 for pre-Win95 shells; it has been a no-op on
		// every OS this runs on, and modern MFC removed the member outright, so the
		// call cannot be reproduced literally.
#ifndef LAUNCHER_RE
		Enable3dControlsStatic();
#endif
		Launcher_CheckVersion();

		if ( !_strcmpi( Launcher_LoadString( IDS_LANGUAGE ), "GERMAN" ) )
			Launcher_WriteProfileString( "Settings", "User Token 3", "a34b68c0" );

		gFavorites = new CFavorites();
		gFavorites->Initialize();

		SetEngineMode();
		LOG( "engine mode set: windowed=%d vidtype=%d %dx%d", gEngineModeWindowed, gEngineVidType, g_EngineMode.width, g_EngineMode.height );
		Launcher_CreateServerBrowser();
		Eng_GameSetState( DLL_INACTIVE );
		InitExeFuncs();

		// Two consecutive calls in the binary; the first is immediately overwritten. (sic)
		SetDialogBkColor( RGB( 0, 0, 0 ), RGB( 255, 255, 255 ) );
		SetDialogBkColor( RGB( 63, 63, 63 ), RGB( 255, 255, 255 ) );

		g_uiScrollMsg = RegisterWindowMessageA( "HL_WM_SCROLL" );
		if ( !g_uiScrollMsg )
		{
			Launcher_ShowMessageById( 0, IDS_MAIN_REGISTERMSGFAIL );
			return FALSE;
		}
		bCDKey = CheckCDKey();
		LOG( "CheckCDKey() = %d", bCDKey );
		if ( bCDKey )
		{
			sprintf( szCommFile, "%s", "woncomm.lst" );
			if ( CheckParm( "-comm", &pszValue ) && pszValue )
				strcpy( szCommFile, pszValue );

			if ( !GetModuleFileNameA( GetInstanceHandle(), szExeName,
				sizeof( szExeName ) ) )
				break;

			GetCDKey( szGUID, NULL, NULL );

			gCryptParms.authType = CRYPT_AUTHTYPE_CLIENT;
			gCryptParms.pszBaseDir = com_gamedir;
			gCryptParms.pszGUID = szGUID;
			gCryptParms.pszServerFile = szCommFile;
			gCryptParms.pszExeName = szExeName;
			gCryptParms.pfnPrintf = NULL;
			gCryptParms.pfnAuthFailure = AuthFailed;
			gCryptParms.pfnGetLocalizedString = GetLocalizedString;
			crypt.Initialize( &gCryptParms );

			if ( AllocGameMem() )
			{
				LOG( "AllocGameMem ok (%d KB); constructing CHLMainDlg", (int)( giMemSize / 1024 ) );

				wait.Restore();

				// A stack object, not new/delete -- InitInstance's frame carries
				// the whole 7868-byte dialog.
				CHLMainDlg	mainDlg( NULL );

				m_pMainWnd = &mainDlg;
				LOG( "entering CHLMainDlg::DoModal" );
				mainDlg.DoModal();
				LOG( "DoModal returned; destructing dialog" );
			}
			else
			{
				LOG( "AllocGameMem FAILED -- no dialog" );
			}
			crypt.Shutdown();
		}
		Eng_Load( NULL, 0 );

		if ( gpMemBase )
			GlobalFree( gpMemBase );
		CloseHandle( tevent );

		CDAudio_Shutdown();
		Launcher_FreeMainButtonsBitmap();

	} while ( Launcher_GetRestartFlag() && !RunSierraUpdate() );

	// HL_Res.dll, loaded by LoadRes at the top of this function.  Under
	// LAUNCHER_STATIC, m_hResDll is the launcher's own module handle, not a
	// LoadLibrary reference, so there is nothing to free -- see LoadRes.
#ifndef LAUNCHER_STATIC
	if ( m_hResDll )
		FreeLibrary( m_hResDll );
#endif

	return FALSE;
}

/*
==================
Eng_VidTypeName (0x4239C3)
==================
*/
const char* Eng_VidTypeName( vidtype_t type )
{
	switch ( type )
	{
	case VT_None:		return "VT_None";
	case VT_Software:	return "VT_Software";
	case VT_OpenGL:		return "VT_OpenGL";
	case VT_Direct3D:	return "VT_Direct3D";
	default:			return "Unknown";
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameApp::SetEngineMode (0x423A12)

void CNetGameApp::SetEngineMode()
{
	char*	pszValue;
	BOOL	bWipe = ( CheckParm( "wipe", NULL ) != NULL );

	if ( bWipe )
	{
		m_nLauncherBPP = 16;
		m_nLauncherWidth = g_nLauncherDefW;
		m_nLauncherHeight = g_nLauncherDefH;
	}
	else
	{
		m_nLauncherBPP = GetProfileInt( "Settings", "LauncherBPP", 16 );
		m_nLauncherWidth = GetProfileInt( "Settings", "LauncherWidth", g_nLauncherDefW );
		m_nLauncherHeight = GetProfileInt( "Settings", "LauncherHeight", g_nLauncherDefH );
	}
	Launcher_WriteProfileInt( "Settings", "LauncherBPP", m_nLauncherBPP );
	Launcher_WriteProfileInt( "Settings", "LauncherWidth", m_nLauncherWidth );
	Launcher_WriteProfileInt( "Settings", "LauncherHeight", m_nLauncherHeight );

	CString	glDriver = Launcher_GetProfileString( "Settings", "EngineGLDriver", NULL );
	if ( CheckParm( "-gldrv", &pszValue ) && pszValue )
		glDriver = pszValue;
	if ( glDriver.IsEmpty() || bWipe )
	{
		HMODULE hGlide = LoadLibraryA( "glide2x" );
		if ( hGlide )
		{
			FreeLibrary( hGlide );
			glDriver = "3dfxgl.dll";
		}
		else
		{
			glDriver = "Default";
		}
		Launcher_WriteProfileString( "Settings", "EngineGLDriver", glDriver );
	}
	CString	d3dDevice = Launcher_GetProfileString( "Settings", "EngineD3DDevice", NULL );
	if ( d3dDevice.IsEmpty() || bWipe )
	{
		d3dDevice = "Default";
		Launcher_WriteProfileString( "Settings", "EngineD3DDevice", d3dDevice );
	}
	gEngineVidType = (vidtype_t)GetProfileInt( "Settings", "EngineType", VT_Software );
	if ( CheckParm( "-d3d", NULL ) )
		gEngineVidType = VT_Direct3D;
	if ( CheckParm( "-gl", NULL ) || CheckParm( "-opengl", NULL ) )
		gEngineVidType = VT_OpenGL;
	if ( CheckParm( "-soft", NULL ) || CheckParm( "-software", NULL ) )
		gEngineVidType = VT_Software;
	if ( gEngineVidType < VT_Software || gEngineVidType > VT_Direct3D || bWipe )
	{
		gEngineVidType = VT_Software;
		Launcher_WriteProfileInt( "Settings", "EngineType", VT_Software );
	}
	Vid_SetRendererFlags( gEngineVidType );
	WriteProfileInt( "Settings", "EngineType", gEngineVidType );

	int	w = GetProfileInt( "Settings", "EngineModeW", 400 );
	Launcher_WriteProfileInt( "Settings", "EngineModeW", w );
	int	h = GetProfileInt( "Settings", "EngineModeH", 300 );
	Launcher_WriteProfileInt( "Settings", "EngineModeH", h );
	int	bpp = GetProfileInt( "Settings", "EngineModeBPP", 16 );
	Launcher_WriteProfileInt( "Settings", "EngineModeBPP", bpp );
	int	captured = GetProfileInt( "Settings", "EngineModeCaptured", 1 );
	Launcher_WriteProfileInt( "Settings", "EngineModeCaptured", captured );

	int	a3d = GetProfileInt( "Settings", "A3D Support", gA3dSupport != 0 );
	Launcher_WriteProfileInt( "Settings", "A3D Support", a3d );
	gA3dSupport = ( a3d != 0 );

	gEngineModeWindowed = GetProfileInt( "Settings", "EngineModeWindowed", 0 );
	if ( CheckParm( "-windowed", NULL ) 
	  || CheckParm( "-win", NULL ) 
	  || CheckParm( "-sw", NULL ) 
	  || CheckParm( "-startwindowed", NULL ) )
	{
		Launcher_WriteProfileInt( "Settings", "EngineAllowWindowed", 1 );
		gEngineModeWindowed = 1;
	}
	if ( CheckParm( "-full", NULL ) 
	  || CheckParm( "-fullscreen", NULL ) )
		gEngineModeWindowed = 0;

	Launcher_WriteProfileInt( "Settings", "EngineModeWindowed", gEngineModeWindowed );

	windowed_mouse = !CheckParm( "-nowinmouse", NULL ) && ( !gEngineModeWindowed || captured );

	memset( &g_EngineMode, 0, sizeof( g_EngineMode ) );
	strcpy( g_EngineMode.glDriver, glDriver );
	strcpy( g_EngineMode.d3dDevice, d3dDevice );
	g_EngineMode.vidtype = gEngineVidType;
	strcpy( g_EngineMode.typeName, Eng_VidTypeName( gEngineVidType ) );
	g_EngineMode.width = w;
	g_EngineMode.height = h;
	g_EngineMode.bpp = bpp;
	g_EngineMode.windowed = gEngineModeWindowed;
	g_EngineMode.captured = captured;

#ifdef LAUNCHER_FIXES
	// Settle the fullscreen shell's display mode before anything is sized to it.
	Shell_ComputeMetrics();
#endif

	if ( CheckParm( "-lw", &pszValue ) && pszValue )
		g_nLauncherDefW = atoi( pszValue );
	if ( CheckParm( "-lh", &pszValue ) && pszValue )
		g_nLauncherDefH = atoi( pszValue );
}

/////////////////////////////////////////////////////////////////////////////
// CNetGameApp::ExitInstance (0x42405F)

int CNetGameApp::ExitInstance()
{
	if ( m_pInstanceGuard )
	{
		m_pInstanceGuard->Unlock();
		delete m_pInstanceGuard;
		m_pInstanceGuard = NULL;
	}
	ChangeDisplaySettingsA( NULL, 0 );

	if ( gpszCmdLine )
		free( gpszCmdLine );
	gpszCmdLine = NULL;

	ModList_Clear( 0 );
	Launcher_FreeStrings();				// drop the strings.lst overrides ()
	Scheme_Free();
	return 0;
}

/*
==================
GetAppName (0x42410F)
==================
*/
static const char* GetAppName( void )
{
	return "Half-Life";
}

/*
==================
AFXGetTopLevelFrame (0x42411F)
==================
*/
int AFXGetTopLevelFrame( void )
{
	return gTopLevelFrame;
}

/*
==================
AFXSetTopLevelFrame (0x424129)
==================
*/
void AFXSetTopLevelFrame( int bPending )
{
	gTopLevelFrame = bPending;
}

/*
==================
Launcher_RunMapCommand (0x424136)
==================
*/
void Launcher_RunMapCommand( const char* pszCmd )
{
	// A bare strcpy in the binary -- 22 bytes, tail-calling strcpy.  Callers pass
	// either a built command or "", never NULL.
	strcpy( gPendingMap, pszCmd );
}

/*
==================
Launcher_GetPendingMap (0x42414C)
==================
*/
const char* Launcher_GetPendingMap( void )
{
	return gPendingMap;
}

/*
==================
Launcher_FormatAppName (0x424156)
==================
*/
char* Launcher_FormatAppName( void )
{
	sprintf( g_szAppName, GetAppName() );
	return g_szAppName;
}

/*
==================
Launcher_GetResourceModule (0x424179)
==================
*/
HMODULE Launcher_GetResourceModule( void )
{
	return g_pTheApp->m_hResDll;
}

/*
==================
Launcher_IsQuitting (0x424189)
==================
*/
int Launcher_IsQuitting( void )
{
	return g_nQuitRequest;
}

/*
==================
Launcher_SetQuitFlag (0x424193)
==================
*/
void Launcher_SetQuitFlag( int value )
{
	g_nQuitRequest = value;
}

/*
==================
Launcher_GetErrorState (0x4241A0)
==================
*/
int Launcher_GetErrorState( void )
{
	return g_nErrorState;
}

/*
==================
Launcher_SetErrorState (0x4241AA)
==================
*/
void Launcher_SetErrorState( int value )
{
	g_nErrorState = value;
}

/*
==================
Launcher_SetErrorMessage (0x4241B7)
==================
*/
void Launcher_SetErrorMessage( const char* pszMessage )
{
	strcpy( g_szErrorMessage, pszMessage );
}

/*
==================
Launcher_GetErrorMessage (0x4241CD)
==================
*/
char* Launcher_GetErrorMessage( void )
{
	return g_szErrorMessage;
}

/*
==================
GetInstanceHandle (0x424360)
==================
*/
static HINSTANCE GetInstanceHandle( void )
{
	return AfxGetModuleState()->m_hCurrentInstanceHandle;
}

/*
==================
Launcher_MainButtonsLoaded (0x4244A0)
==================
*/
int Launcher_MainButtonsLoaded( void )
{
	return g_bMainButtonsLoaded;
}
/*
==================
Launcher_LoadSplashBitmap (0x4244b0)
==================
*/
void Launcher_LoadSplashBitmap( void )
{
	if ( g_hBackgroundDib )
	{
		GlobalFree( g_hBackgroundDib );
		g_hBackgroundDib = NULL;
	}
	char	szPath[260];
	sprintf( szPath, "%s%s.bmp", GFX_SHELL_DIR, "splash" );
	g_hBackgroundDib = DIB_LoadBitmapFile( szPath );
}
/*
==================
Launcher_FreeSplashBitmap (0x424510)
==================
*/
void Launcher_FreeSplashBitmap( void )
{
	if ( g_hBackgroundDib )
	{
		GlobalFree( g_hBackgroundDib );
		g_hBackgroundDib = NULL;
	}
}

/*
==================
Launcher_BlitBackground (0x424530)
==================
*/
void Launcher_BlitBackground( CDC* pDC, RECT* prcDst, RECT* prcSrc )
{
	if ( !g_hBackgroundDib )
		return;

	LPBITMAPINFOHEADER pDib = (LPBITMAPINFOHEADER)GlobalLock( g_hBackgroundDib );
	if ( pDib )
	{
		RECT	src;
		if ( prcSrc )
			src = *prcSrc;
		else
		{
			src.left = 0; src.top = 0;
			src.right = DIB_Width( pDib ); src.bottom = DIB_Height( pDib );
		}
		DIB_BlitDib( pDC->GetSafeHdc(), prcDst, g_hBackgroundDib, &src );
		GlobalUnlock( g_hBackgroundDib );
	}
}

/*
==================
Launcher_CopyParentBackground (0x4245C0)
==================
*/
void Launcher_CopyParentBackground( CDC* pDC, RECT* prcDst, RECT* prcSrc )
{
	if ( !g_hBackgroundDib )
		return;

	if ( GlobalLock( g_hBackgroundDib ) )
	{
		DIB_BlitDib( pDC->GetSafeHdc(), prcDst, g_hBackgroundDib, prcSrc );
		GlobalUnlock( g_hBackgroundDib );
	}
}

/*
==================
Launcher_LoadMainButtonsBitmap (0x424610)
==================
*/
void Launcher_LoadMainButtonsBitmap( void )
{
	char				szPath[260];
	LPBITMAPINFOHEADER	pDIB;
	int					height;

	if ( g_bMainButtonsLoaded )
		return;

	sprintf( szPath, "%s%s.bmp", GFX_SHELL_DIR, "btns_main" );

	g_hMainButtonsBmp = DIB_LoadBitmapFile( szPath );
	if ( !g_hMainButtonsBmp )
	{
		g_bMainButtonsLoaded = 0;
		return;
	}
	pDIB = (LPBITMAPINFOHEADER)GlobalLock( g_hMainButtonsBmp );
	if ( pDIB )
	{
		DIB_Width( pDIB );
		height = DIB_Height( pDIB );
		GlobalUnlock( g_hMainButtonsBmp );

		if ( g_nMenuButtonHeight > 0 )
		{
			g_nMenuButtonCount = height / g_nMenuButtonHeight / 3;
			if ( height % g_nMenuButtonHeight )
				Launcher_ShowMessageById( 0, IDS_BTN_STRANGESIZE );
		}
		g_bMainButtonsLoaded = 1;
	}
}

/*
==================
Launcher_FreeMainButtonsBitmap (0x4246E0)
==================
*/
void Launcher_FreeMainButtonsBitmap( void )
{
	if ( Launcher_MainButtonsLoaded() )
	{
		if ( g_hMainButtonsBmp )
			GlobalFree( g_hMainButtonsBmp );
		g_bMainButtonsLoaded = 0;
		g_hMainButtonsBmp = NULL;
	}
}

/*
==================
Launcher_ResetMainButtons (0x424710)
==================
*/
static int Launcher_ResetMainButtons( void )
{
	g_hMainButtonsBmp   = NULL;
	g_nMenuButtonCount  = 0;
	g_nMenuButtonWidth  = 0;
	g_nMenuButtonHeight = 0;
	return 0;
}

/*
==================
Launcher_HeaderLoaded (0x424730)
==================
*/
HGLOBAL Launcher_HeaderLoaded( void )
{
	return g_hMainButtonsBmp;
}
/*
==================
Launcher_HeaderStride (0x424740)
==================
*/
int Launcher_HeaderStride( void )
{
	return g_nMenuButtonCount;
}
/*
==================
Launcher_HeaderSize (0x424750)

Fills the caller's pair and hands the same pointer back, which is how
CVideoModeDlg reads it rather than through the out-buffer.
==================
*/
int* Launcher_HeaderSize( int* pWH )
{
	pWH[0] = g_nMenuButtonWidth;
	pWH[1] = g_nMenuButtonHeight;
	return pWH;
}
/*
==================
Launcher_SetButtonCell (0x424770)
==================
*/
void Launcher_SetButtonCell( int* pWH )
{
	g_nMenuButtonWidth  = pWH[0];
	g_nMenuButtonHeight = pWH[1];
}
/*
==================
Launcher_CompositeDib (0x424790)
==================
*/
void Launcher_CompositeDib( CDC* pDC, RECT* prcDst, HGLOBAL hDib, RECT* prcSrc )
{
	if ( !hDib )
		return;

	LPBITMAPINFOHEADER	pDib = (LPBITMAPINFOHEADER)GlobalLock( hDib );
	if ( !pDib )
		return;

	RECT	src;
	if ( prcSrc )
		src = *prcSrc;
	else
	{
		src.left = 0; src.top = 0;
		src.right = DIB_Width( pDib ); src.bottom = DIB_Height( pDib );
	}
	int		w = prcDst->right - prcDst->left;
	int		h = prcDst->bottom - prcDst->top;

	CDC		dibDC;
	if ( dibDC.CreateCompatibleDC( pDC ) )
	{
		CBitmap		dibBmp;
		dibBmp.CreateCompatibleBitmap( pDC, w, h );
		CBitmap*	pOldDib = dibDC.SelectObject( &dibBmp );

		RECT	r0 = { 0, 0, w, h };
		DIB_BlitDib( dibDC.GetSafeHdc(), &r0, hDib, &src );

		CDC		bgDC;
		if ( bgDC.CreateCompatibleDC( pDC ) )
		{
			CBitmap		bgBmp;
			bgBmp.CreateCompatibleBitmap( pDC, w, h );
			CBitmap*	pOldBg = bgDC.SelectObject( &bgBmp );

			bgDC.BitBlt( 0, 0, w, h, pDC, prcDst->left, prcDst->top, SRCCOPY );

			RECT	rb = { 0, 0, w, h };
			CODBlendBtn::BlendStates( &rb, &dibDC, &bgDC, &dibBmp, &bgBmp, 2 );

			pDC->BitBlt( prcDst->left, prcDst->top, w, h, &bgDC, 0, 0, SRCCOPY );
			bgDC.SelectObject( pOldBg );
		}
		dibDC.SelectObject( pOldDib );
	}
	GlobalUnlock( hDib );
}

/*
==================
Launcher_LoadHeaderBitmapFile (0x424AF0)
==================
*/
void* Launcher_LoadHeaderBitmapFile( const char* pszName, HGLOBAL* phDib, RECT* prcOut, RECT* prcSrc )
{
	char	szPath[260];

	sprintf( szPath, "%s%s.bmp", GFX_SHELL_DIR, pszName );
	*phDib = DIB_LoadBitmapFile( szPath );

	if ( prcSrc )
	{
		if ( *phDib )
			*prcOut = *prcSrc;
		return *phDib;
	}
	if ( *phDib )
	{
		LPBITMAPINFOHEADER	pDib = (LPBITMAPINFOHEADER)GlobalLock( *phDib );
		if ( pDib )
		{
			int	h = DIB_Height( pDib );
			int	w = DIB_Width( pDib );
			GlobalUnlock( *phDib );
			prcOut->left   = 45;
			prcOut->top    = 45;
			prcOut->right  = w + 45;
			prcOut->bottom = h + 45;
			return prcOut;
		}
	}
	return *phDib;
}

/*
==================
Launcher_SetRestartFlag (0x424BC0)
==================
*/
void Launcher_SetRestartFlag( int value )
{
	g_bRestartLauncher = value;
}

/*
==================
Launcher_GetRestartFlag (0x424BD0)
==================
*/
int Launcher_GetRestartFlag( void )
{
	return g_bRestartLauncher;
}

/*
==================
Launcher_GetProfileInt (0x424BE0)
==================
*/
int Launcher_GetProfileInt( const char* section, const char* key, int def )
{
	return g_pTheApp->GetProfileInt( section, key, def );
}

/*
==================
Launcher_GetProfileString (0x424C00)
==================
*/
char* Launcher_GetProfileString( const char* section, const char* key, const char* def )
{
	static char	sz[256];
	strcpy( sz, g_pTheApp->GetProfileString( section, key, def ) );
	return sz;
}
/*
==================
Launcher_WriteProfileInt (0x424CB0)
==================
*/
void Launcher_WriteProfileInt( const char* section, const char* key, int value )
{
	g_pTheApp->WriteProfileInt( section, key, value );
}

/*
==================
Launcher_WriteProfileString (0x424CD0)
==================
*/
BOOL Launcher_WriteProfileString( const char* section, const char* key, const char* value )
{
	return g_pTheApp->WriteProfileString( section, key, value );
}
