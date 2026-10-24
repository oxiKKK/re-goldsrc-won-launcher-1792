/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
****/

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
// Purpose: the engine host: load hw.dll / sw.dll, hand over the API tables and
//          drive it (Eng_*, Sys_Init).
//
// $NoKeywords: $
//=============================================================================

/*

===== engine.c ============================================================

  Loading, binding, running and unloading the engine DLL (+ the launcher's
  high-resolution timer and the periodic cheat-window scan).

  The engine (hw.dll / sw.dll) exports a single entry point -- Sys_EngineAPI,
  which fills our engine_api_t table.  In return the launcher hands the engine
  its own callback table (exefuncs_t) via engine_api_t::Game_Init.

*/

#include "precompiled.h"

/*
==================
nullapi stubs (0x40E440, 0x40E450, 0x40E460)

every slot of the default table points at one of three bodies: NullGetGameInfo
(0x40E440), the "return 0" stub (0x40E450) and the empty stub (0x40E460).  MSVC
folded all the same-shaped ones together, so one address stands for many names
here.
==================
*/
void NullCon_Printf( char*, ... )
{
}

void NullCon_SafePrintf( char*, ... )
{
}

void NullForceReloadProfile( void )
{
}

int NullGetGameInfo( struct GameInfo_s* pGI, char* )
{
	memset( pGI, 0, sizeof( GameInfo_t ) );
	return 0;
}

void NullGameSetBackground( int )
{
}

int NullGetPauseState( void )
{
	return 0;
}

void NullIN_ActivateMouse( void )
{
}

void NullIN_ClearStates( void )
{
}

void NullIN_DeactivateMouse( void )
{
}

void NullIN_MouseEvent( int )
{
}

void NullKeyboard_ReturnToGame( void )
{
}

void NullKey_ClearStates( void )
{
}

void NullKey_Event( int, int )
{
}

void NullS_BlockSound( void )
{
}

void NullS_ClearBuffer( void )
{
}

void NullS_GetDSPointer( struct IDirectSound**, struct IDirectSoundBuffer** )
{
}

void* NullS_GetWAVPointer( void )
{
	return NULL;
}

void NullS_UnblockSound( void )
{
}

void NullSetMessagePumpDisableMode( int )
{
}

void NullSetPauseState( int )
{
}

void NullSetStartupMode( int )
{
}

void NullSnd_AcquireBuffer( void )
{
}

void NullSnd_ReleaseBuffer( void )
{
}

void NullStoreProfile( void )
{
}

void NullVID_UpdateVID( struct viddef_s* )
{
}

// The default no-op table (0x4CEA28).
static const engine_api_t	nullapi =
{
	ENGINE_LAUNCHER_API_VERSION,	// version
	0,								// rendertype
	sizeof( engine_api_t ),			// size
	0,								// GetEngineState
	0,								// Cbuf_AddText
	0,								// Cbuf_InsertText
	0,								// Cmd_AddCommand
	0,								// Cmd_Argc
	0,								// Cmd_Args
	0,								// Cmd_Argv
	NullCon_Printf,					// Con_Printf
	NullCon_SafePrintf,				// Con_SafePrintf
	0,								// Cvar_Set
	0,								// Cvar_SetValue
	0,								// Cvar_VariableInt
	0,								// Cvar_VariableString
	0,								// Cvar_VariableValue
	NullForceReloadProfile,			// ForceReloadProfile
	NullGetGameInfo,				// GetGameInfo
	NullGameSetBackground,			// GameSetBackground
	0,								// GameSetState
	0,								// GameSetSubState
	NullGetPauseState,				// GetPauseState
	0,								// Host_Frame
	0,								// Host_GetHostInfo
	0,								// Host_Shutdown
	0,								// Game_Init
	NullIN_ActivateMouse,			// IN_ActivateMouse
	NullIN_ClearStates,				// IN_ClearStates
	NullIN_DeactivateMouse,			// IN_DeactivateMouse
	NullIN_MouseEvent,				// IN_MouseEvent
	NullKeyboard_ReturnToGame,		// Keyboard_ReturnToGame
	NullKey_ClearStates,			// Key_ClearStates
	NullKey_Event,					// Key_Event
	0,								// LoadGame
	NullS_BlockSound,				// S_BlockSound
	NullS_ClearBuffer,				// S_ClearBuffer
	NullS_GetDSPointer,				// S_GetDSPointer
	NullS_GetWAVPointer,			// S_GetWAVPointer
	NullS_UnblockSound,				// S_UnblockSound
	0,								// SaveGame
	0,								// SetAuth
	NullSetMessagePumpDisableMode,	// SetMessagePumpDisableMode
	NullSetPauseState,				// SetPauseState
	NullSetStartupMode,				// SetStartupMode
	0,								// SNDDMA_Shutdown
	NullSnd_AcquireBuffer,			// Snd_AcquireBuffer
	NullSnd_ReleaseBuffer,			// Snd_ReleaseBuffer
	NullStoreProfile,				// StoreProfile
	Sys_DoubleTime,					// Sys_FloatTime
	0,								// VID_UpdateWindowVars
	NullVID_UpdateVID,				// VID_UpdateVID
	0,								// VGui_CallEngineSurfaceProc
	0,								// EngineTakingFocus
	0,								// LauncherTakingFocus
	0,								// GL_Init
	0,								// GL_SetMode
	0,								// GL_Shutdown
	0,								// QGL_D3DShared
	0,								// glSwapBuffers
	0,								// DirectorProc
};

// The table the launcher calls through.
engine_api_t	engineapi = nullapi;

// Handle to the currently loaded engine DLL (hw.dll / sw.dll), NULL when none.
static HMODULE			ghMod;

// Engine run state.
int				gDLLState;
int				gDLLStateInfo;
int				gBackground;
int				gbConsoleMode;
int				gEngineModeWindowed;
int 			g_bResumeOnSwitch; 	// (4E6E1C) resume engine after a mode switch
vidtype_t		gEngineVidType;
double			g_flLastFrameTime;
int				g_bChangingVideoModes;
int				iWait;
int				g_bWaitingToRestore;	// (4E1EFC) restore pending after the iWait settle
int				g_iForcedState = -1;	// (4CF8A8) forced next state, -1 = none
int				g_bQuitting;

char*			gpszCmdLine;
unsigned char*	gpMemBase;
int				giMemSize;
HWND			mainwindow;
const char*		gszLastDLL;				// (4E1B40) name of the loaded engine dll
HDC				g_maindc;
HGLRC			g_baseRC;

// Quicksave-across-mode-switch state: 0 = no save, 1 = saved, -1 = failed to save
static int		g_iModeSwitchSaved;		// (4E1B3C)

static mod_s*			g_pEngineMod;			// (4E1EC4) mod the engine is running with
int				ActiveApp;				// (4E1F00) app has focus / is foreground
int				g_bRestartPending;		// (4F8C18) cleared once the engine is up
int				g_bValidCD;				// (4E1ED0) set once a valid key is confirmed

typedef int (*engine_api_func)( int version, int size, struct engine_api_s *api );

static int		lowshift;				// (4E1B4C) bits the perf counter is shifted down by
static double	pfreq;					// (4E1B50) 1.0 / (shifted) timer frequency
static double	curtime;				// (4E1ED8) accumulated seconds
static double	lastcurtime;			// (4E1EE0) last returned time (stuck-timer nudge)
static char		g_szConsoleCmdLine[512];	// (4E1B58) "-console" scratch for Game_Init

/*
==================
Sys_Init (0x40E470)
==================
*/
void Sys_Init( void )
{
	LARGE_INTEGER	PerformanceFreq;
	unsigned int	lowpart, highpart;

	if ( !QueryPerformanceFrequency( &PerformanceFreq ) )
	{
		Launcher_ErrorMessageBox( 0, "No hardware timer available" );
		exit( -1 );
	}

	lowpart  = (unsigned int)PerformanceFreq.LowPart;
	highpart = (unsigned int)PerformanceFreq.HighPart;

	lowshift = 0;
	while ( highpart || (double)lowpart > 2000000.0 )
	{
		++lowshift;
		lowpart >>= 1;
		lowpart  |= ( highpart & 1 ) << 31;
		highpart >>= 1;
	}

	pfreq = 1.0 / (double)lowpart;
}

/*
==================
Sys_DoubleTime (0x40E500)
==================
*/
double Sys_DoubleTime( void )
{
	static int			sametimecount;	// (4E1B44) consecutive identical timestamps
	static unsigned int	oldtime;		// (4E1B48) previous (shifted) counter sample
	static int			first = 1;		// (4CEA20) first call seeds oldtime
	LARGE_INTEGER		PerformanceCount;
	unsigned int		temp, t2;

	QueryPerformanceCounter( &PerformanceCount );

	temp = ( (unsigned int)PerformanceCount.LowPart  >> lowshift )
	     | ( (unsigned int)PerformanceCount.HighPart << ( 32 - lowshift ) );

	if ( first )
	{
		oldtime = temp;
		first   = 0;
		return curtime;
	}

	// Counter went backwards by a small amount: clamp without advancing time.
	if ( temp <= oldtime && oldtime - temp < 0x10000000 )
	{
		oldtime = temp;
		return curtime;
	}

	t2      = temp - oldtime;
	oldtime = temp;
	curtime += (double)t2 * pfreq;

	if ( curtime == lastcurtime )
	{
		if ( ++sametimecount > 100000 )
		{
			curtime += 1.0;
			sametimecount = 0;
		}
	}
	else
	{
		sametimecount = 0;
	}

	lastcurtime = curtime;
	return curtime;
}

/*
==================
Eng_SaveBeforeModeSwitch (0x40E5E0)
==================
*/
int Eng_SaveBeforeModeSwitch( void )
{
	int result = 0;

	g_iModeSwitchSaved = 0;

	if ( ghMod )
	{
		if ( engineapi.SaveGame )
		{
			result             = engineapi.SaveGame( "HLSave", "Mode Switch" );
			g_iModeSwitchSaved = result;
		}
	}
	return result;
}

/*
==================
Eng_OnLoaded (0x40E610)
==================
*/
int Eng_OnLoaded( void )
{
	if ( g_iModeSwitchSaved )
	{
		Eng_Frame( TRUE );
		return engineapi.LoadGame( "HLSave" );
	}
	return g_iModeSwitchSaved;
}

/*
==================
Eng_Shutdown (0x40E630)
==================
*/
HWND Eng_Shutdown( void )
{
	HWND	result;

	VID_Shutdown();
	Eng_Load( NULL, 0 );
	result = VID_SetMode( -1 );
	gDLLState = DLL_INACTIVE;
	gDLLStateInfo = 0;
	return result;
}

/*
==================
Eng_SetCurrentMod (0x40E660)
==================
*/
mod_s* Eng_SetCurrentMod( void )
{
	g_pEngineMod = g_pCurrentMod;
	return g_pCurrentMod;
}

/*
==================
Eng_NeedsReload (0x40E670)
==================
*/
int Eng_NeedsReload( void )
{
	mod_s*	prev = g_pEngineMod;
	int		result = 0;

	if ( g_pEngineMod )
	{
		if ( g_pEngineMod != g_pCurrentMod )
		{
			prev = g_pCurrentMod;
			result = 1;
			g_pEngineMod = g_pCurrentMod;
		}
		if ( prev )
			return result;
	}

	if ( g_pCurrentMod )
	{
		g_pEngineMod = g_pCurrentMod;
		if ( g_pCurrentMod != g_pValveMod )
			return 1;
	}
	return result;
}

/*
==================
Eng_ConfirmModeSwitch (0x40E6C0)
==================
*/
int Eng_ConfirmModeSwitch( int* pState )
{
	*pState = 1;
	return 1;
}

/*
==================
Eng_KillEngine (0x40E6D0)
==================
*/
HMODULE Eng_KillEngine( const char* pszNextDll, int* pbState, HMODULE* phModule )
{
	if ( !pszNextDll || Eng_ShouldReload() || gbConsoleMode )
		*pbState = 0;
	else
		*pbState = Eng_SaveBeforeModeSwitch();

	if ( Eng_ShouldReload() )
	{
		int bSave = gBackground;
		gBackground = 1;
		engineapi.Cbuf_AddText( "disconnect\n" );
		Eng_Frame( 1 );
		gBackground = bSave;
	}

	// Unload the engine DLL and restore the null interface table.
	if ( ghMod )
	{
		if ( engineapi.SNDDMA_Shutdown )
			engineapi.SNDDMA_Shutdown();

		if ( engineapi.Host_Shutdown )
			engineapi.Host_Shutdown();

		if ( !strcmp( gszLastDLL, "hw.dll" ) )
		{
			engineapi.GL_Shutdown( mainwindow, g_maindc, g_baseRC );
			Launcher_RestoreAfterEngine( gEngineModeWindowed );
		}

		FreeLibrary( ghMod );
		Eng_LoadStubs();
		*phModule = NULL;
		ghMod = NULL;
	}

	return ghMod;
}

/*
==================
Eng_StartupEngine (0x40E800)
==================
*/
int Eng_StartupEngine( int bModeSwitch, HMODULE* phModule, int* pnError )
{
	int		bWasChanging = g_bChangingVideoModes;
	HWND	hWindow;
	char*	pszCmdLine;
	char*	pszGG;

	g_bChangingVideoModes = 1;

	// Software fullscreen and D3D need DirectDraw; OpenGL releases it.
	if ( !gBackground )
	{
		if ( gEngineVidType == VT_Software )
		{
			if ( !lpDD && !gEngineModeWindowed )
				DDraw_Init( 1, 0 );
		}
		else if ( gEngineVidType == VT_OpenGL )
		{
			if ( lpDD )
				DDraw_Shutdown();
		}
		else if ( gEngineVidType == VT_Direct3D && !lpDD )
		{
			DDraw_Init( 1, 0 );
		}
	}

	// Apply the selected mode (quietly: no focus jolt during the bring-up).
	g_bVidTypeChanged = 1;
	force_minimized = 1;

	hWindow = VID_ApplyMode();

	g_bVidTypeChanged = 0;
	force_minimized = 0;

	if ( !IsWindow( hWindow ) )
	{
		switch ( gEngineVidType )
		{
		case VT_Software:	*pnError = IDS_VID_NOMODE; break;
		case VT_OpenGL:		*pnError = IDS_GL_NOMODE;  break;
		case VT_Direct3D:	*pnError = IDS_D3D_NOMODE; break;
		}
		goto failed;
	}

	pszCmdLine = gpszCmdLine ? gpszCmdLine : "empty";
	if ( gbConsoleMode )
	{
		sprintf( g_szConsoleCmdLine, "%s -console", pszCmdLine );
		pszCmdLine = g_szConsoleCmdLine;
	}

	if ( engineapi.GameSetBackground )
		engineapi.GameSetBackground( gBackground );

	// Unguarded in the binary, unlike GameSetBackground just above.
	engineapi.SetAuth( &crypt );

#ifdef LAUNCHER_RE
	// Diagnostic only: log the process CWD the engine will read as its basedir.
	{
		char	cwd[260] = { 0 };
		GetCurrentDirectoryA( sizeof( cwd ), cwd );
		LOG( "cwd before Game_Init = '%s'", cwd );
	}
#endif

	LOG( "pre-Game_Init: vidtype=%d windowed=%d bg=%d memBase=%p memSize=%d mainwindow=%p hWindow=%p cmd='%s' Game_Init=%p",
		gEngineVidType, gEngineModeWindowed, gBackground, gpMemBase, giMemSize,
		(void *)mainwindow, (void *)hWindow, pszCmdLine, (void *)engineapi.Game_Init );

	// The window is re-tested here, a second time after the check further up.
	if ( !IsWindow( hWindow )
	  || !engineapi.Game_Init( pszCmdLine, gpMemBase, giMemSize,
			&ef, &mainwindow, 0 ) )
	{
		*pnError = IDS_DLL_LOADFAIL;
		goto failed;
	}

	if ( CheckParm( "+connect", NULL ) )
		Sys_StripCmdLineParm( "+connect" );	// handled; do not reconnect on restarts

	engineapi.SetStartupMode( 1 );
	engineapi.ForceReloadProfile();

	if ( bModeSwitch )
	{
		Eng_OnLoaded();
	}
	else if ( engineapi.Cbuf_AddText )
	{
		if ( CheckParm( "-gamegauge", &pszGG ) )
		{
			if ( !pszGG )
				pszGG = "gg";

			engineapi.Cbuf_AddText( "gg " );
			engineapi.Cbuf_AddText( pszGG );
			engineapi.Cbuf_AddText( "\n" );
		}
		Eng_Frame( 1 );
	}

	g_bChangingVideoModes = bWasChanging;
	engineapi.SetStartupMode( 0 );

	if ( gBackground )
		ShowWindow( mainwindow, SW_HIDE );
	else
		VID_ShowEngineWindow( 0 );

	return 1;

failed:
	g_currentMode = 0;

	FreeLibrary( ghMod );
	Eng_LoadStubs();

	*phModule = NULL;
	ghMod = NULL;

	Eng_Load( NULL, ENG_NORMAL );
	gDLLState = DLL_INACTIVE;
	return 0;
}

/*
==================
Eng_LoadFunctions (0x40EA90)
==================
*/
static int Eng_LoadFunctions( void* handle )
{
	engine_api_func pfnEngineAPI;

	pfnEngineAPI = ( engine_api_func )GetProcAddress( (HMODULE)handle, "Sys_EngineAPI" );
	if ( !pfnEngineAPI )
		return 0;

	if ( !(*pfnEngineAPI)( ENGINE_LAUNCHER_API_VERSION, sizeof( engine_api_t ), &engineapi ) )
		return 0;

	// All is okay
	return 1;
}

/*
==================
Eng_LoadStubs (0x40EAF0)
==================
*/
void Eng_LoadStubs( void )
{
	engineapi = nullapi;
}

/*
==================
Eng_Load (0x40EB10)
==================
*/
int Eng_Load( const char* psz, int iSubState )
{
	int		state = 0;
	HMODULE	hMod = NULL;
	int		bConnect = 0;
	int		bVidRestart;
	int		bReload;
	int		result = 0;

	LOG( "dll=%s substate=%d", psz ? psz : "(NULL=unload/no-engine)", iSubState );

	bVidRestart = VID_RestartNeeded();
	bReload = Eng_NeedsReload();

	if ( psz && ghMod && !strcmp( psz, gszLastDLL ) && !bVidRestart && !bReload )
		return 0;	// nothing changed

	g_bValidCD = FALSE;

	if ( psz )
	{
		static int s_bFirst = 1;
		if ( s_bFirst )
		{
			s_bFirst = 0;
			if ( CheckParm( "+connect", NULL ) )
				bConnect = 1;
		}

		if ( !gbConsoleMode && !bConnect && !Eng_ConfirmModeSwitch( &g_bValidCD ) )
			return IDS_MD5_HASHFAIL;	// integrity / MD5 check failed
	}

	Eng_PreLoad();

	if ( ghMod )
		Eng_KillEngine( psz, &state, &hMod );

	if ( !psz )
	{
		hMod = 0;
		Eng_LoadStubs();
	}
	else if ( !ghMod )
	{
		hMod = LoadLibraryA( psz );
		if ( !hMod )
			return IDS_DLL_LOADFAIL;

		// Load function table from engine
		if ( !Eng_LoadFunctions( hMod ) )
		{
			FreeLibrary( hMod );
			Eng_LoadStubs();
			return IDS_DLL_LOADFAIL;
		}

		// Activate engine
		Eng_GameSetState( DLL_ACTIVE );
	}

	Eng_SetSubState( iSubState );

	gszLastDLL	= psz;
	ghMod		= hMod;

	// Eng_StartupEngine brings the engine up; on failure it writes an IDS error code
	// into `result` (and tears the engine back down), on success it leaves it 0.
	if ( hMod )
		Eng_StartupEngine( state, &hMod, &result );

	return result;
}

/*
==================
AntiCheat_DetectCheatWindows (0x40ECD0)
==================
*/
static BOOL CALLBACK AntiCheat_DetectCheatWindows( HWND gLauncherWnd, LPARAM lParam )
{
	char	szWindow[20];
	char	szA[8];
	char	szE[10];
	char	szB[5];
	char	szC[5];
	char	szD[5];
	HWND	hwnd1, hwnd2;

	// Obfuscated captions of period cheat tools' windows/controls; each decodes
	// (with its own key) to the plaintext noted.
	memcpy( szWindow, "(ycOhaB|40%{k8y,\\9#0", sizeof( szWindow ) );// "Scan Range (pixels)"
	memcpy( szA, "(kiw\te|1", sizeof( szA ) ); // "disable"
	strcpy( szE, "0luDpox6t" ); // "Autoshoot"
	strcpy( szB, ")wtp" ); // "stop"
	strcpy( szC, "(yeK" ); // "Team"
	strcpy( szD, "Jwt3" ); // "Stop"

	AntiCheat_DecodeString( szWindow, 20, 28 );
	AntiCheat_DecodeString( szE, 10, 5 );
	AntiCheat_DecodeString( szB, 5, 3 );
	AntiCheat_DecodeString( szC, 5, 31 );
	AntiCheat_DecodeString( szA, 8, 19 );
	AntiCheat_DecodeString( szD, 5, 96 );

	hwnd1 = FindWindowExA( gLauncherWnd, NULL, NULL, szWindow );
	hwnd2 = FindWindowExA( gLauncherWnd, NULL, NULL, szE );

	if ( hwnd1 || hwnd2 )
	{
		if ( FindWindowExA( gLauncherWnd, NULL, NULL, szB ) || FindWindowExA( gLauncherWnd, NULL, NULL, szD ) )
		{
			engineapi.Cbuf_AddText( "8hy" );
			return FALSE;
		}
		engineapi.Cbuf_AddText( "769d" );
	}
	else
	{
		engineapi.Cbuf_AddText( "769d" );

		if ( !FindWindowExA( gLauncherWnd, NULL, NULL, szC ) )
		{
			engineapi.Cbuf_AddText( "a07e" );
			AntiCheat_EncodeString( szWindow, 20, 28 );
			AntiCheat_EncodeString( szE, 10, 5 );
			AntiCheat_EncodeString( szB, 5, 3 );
			AntiCheat_EncodeString( szC, 5, 31 );
			AntiCheat_EncodeString( szA, 8, 19 );
			AntiCheat_EncodeString( szD, 5, 96 );
			return TRUE;
		}

		if ( FindWindowExA( gLauncherWnd, NULL, NULL, szA ) )
		{
			engineapi.Cbuf_AddText( "3f9a" );
			return FALSE;
		}

		engineapi.Cbuf_AddText( "a07e" );
	}

	return TRUE;
}

/*
==================
AntiCheat_ScanForCheatWindows (0x40ef80)
==================
*/
void AntiCheat_ScanForCheatWindows( double time )
{
	static double lastScan = 0;
	if ( time - lastScan >= 1.7742141 )
	{
		float fParam;

		lastScan = time;
		fParam = (float)time;
		EnumChildWindows( NULL, AntiCheat_DetectCheatWindows, (LPARAM)&fParam );
	}
}

/*
==================
Eng_Frame (0x40EFC0)
==================
*/
int Eng_Frame( int fForce )
{
	int		iState;
	double	flTime, dt;

	if ( gDLLState != DLL_ACTIVE && !fForce )
		return 0;

	if ( !ActiveApp && !gbConsoleMode && !gEngineModeWindowed )
		return 0;

	if ( engineapi.GameSetBackground )
		engineapi.GameSetBackground( gBackground );

	if ( gDLLState )
	{
		// Don't burn the CPU when minimized with no rendering surface.
		if ( gDLLState == DLL_PAUSED )
		{
			if ( !ActiveApp && !lpDD )
				SleepUntilInput( 50 );
		}
		else if ( !ActiveApp && !lpDD )
		{
			SleepUntilInput( 20 );
		}

		if ( engineapi.Sys_FloatTime )
			flTime = engineapi.Sys_FloatTime();
		else
			flTime = Sys_DoubleTime();

		dt = flTime - g_flLastFrameTime;
		if ( dt < 0.0 )
			dt = 0.02;

		gDLLStateInfo = 0;
		iState = engineapi.Host_Frame( (float)dt, gDLLState, &gDLLStateInfo );

#ifdef LAUNCHER_RE
		// LAUNCHER_RE bring-up trace (diagnostic only; no behaviour change).
		{
			static int	s_logFrames = 0;
			if ( s_logFrames < 16 )
			{
				s_logFrames++;
				LOG( "Host_Frame: in gDLLState=%d iState=%d info=%d (activeApp=%d windowed=%d lpDD=%p)",
					gDLLState, iState, gDLLStateInfo, ActiveApp, gEngineModeWindowed, lpDD );
			}
		}
#endif

		AntiCheat_ScanForCheatWindows( flTime );

		if ( g_iForcedState != -1 )
		{
			gDLLState = g_iForcedState;
			iState = g_iForcedState;
			g_iForcedState = -1;
		}

		// Out-of-band requests from the engine.
		switch ( gDLLStateInfo )
		{
		case DLL_INFO_RELAYOUT_1:	Eng_DeferRelayout1( 1 ); break;
		case DLL_INFO_RELAYOUT_2:	Eng_DeferRelayout2( 1 ); break;
		case DLL_INFO_OPEN_MANUAL:	Eng_DeferOpenManual( 1 ); break;
		case DLL_INFO_MENU_BUTTON:	Eng_DeferMenuButton( 1 ); break;
		case DLL_INFO_WORLDCRAFT:	// hand focus to Worldcraft
			{
				HWND hwndWC = FindWindowA( "VALVEWORLDCRAFT", NULL );
				if ( hwndWC )
				{
									AfxGetMainWnd()->ShowWindow( SW_MINIMIZE );
					SetForegroundWindow( hwndWC );
					SetFocus( hwndWC );
				}
			}
			break;
		default:
			break;
		}

		// Reconcile the state the engine wants with our own, honouring the
		// short "wait" used to settle a pause/restore.
		if ( iWait )
		{
			--iWait;

			if ( iState == DLL_PAUSED )
			{
				g_bWaitingToRestore = 1;
				Eng_GameSetState( DLL_ACTIVE );
				iState = DLL_ACTIVE;
			}
			if ( !iWait && g_bWaitingToRestore )
			{
				iState = DLL_PAUSED;
				gDLLState = DLL_ACTIVE;
				g_bWaitingToRestore = 0;
				Eng_GameSetState( iState );
				if ( iState == DLL_PAUSED && !g_bChangingVideoModes && !gBackground )
					VID_HideEngineWindow();
				g_flLastFrameTime = flTime;
				goto CheckClose;
			}
		}

		if ( iState == DLL_TRANS )
		{
			iState = DLL_ACTIVE;
			iWait = 5;
			Eng_GameSetState( DLL_ACTIVE );
		}

		if ( iState != gDLLState )
		{
			Eng_GameSetState( iState );
			if ( iState == DLL_PAUSED && !g_bChangingVideoModes && !gBackground )
				VID_HideEngineWindow();
		}

		g_flLastFrameTime = flTime;
	}

CheckClose:
	if ( gDLLState == DLL_CLOSE )
	{
		if ( !gBackground )
			VID_HideEngineWindow();

		// A plain close (no pending error) quits the app outright.
		if ( !Launcher_GetErrorState() )
		{
			g_bQuitting = 1;
			PostQuitMessage( 1 );
		}

		Eng_Shutdown();
	}

	// The WON auth layer flags failures through Launcher_SetQuitFlag.
	if ( Launcher_IsQuitting() )
		Launcher_HandleAuthFailure();

	return gDLLState;
}

/*
==================
Eng_SetSubState (0x40F270)
==================
*/
void Eng_SetSubState( int iSubState )
{
	if ( iSubState != ENG_NORMAL )
	{
		if ( engineapi.GameSetSubState )
			engineapi.GameSetSubState( iSubState );
	}
}

/*
==================
Eng_GameSetState (0x40F290)
==================
*/
void Eng_GameSetState( int iState )
{
	gDLLState = iState;

	if ( engineapi.GameSetState )
		engineapi.GameSetState( iState );
}

/*
==================
Eng_ConnectToServer (0x40F2B0)
==================
*/
void Eng_ConnectToServer( CServerInfo* pInfo )
{
	char	szCmd[512];

	if ( !pInfo )
		return;

	const char*	pszName = g_pServerBrowser->GetPlayerName();

	// 0x40F2CE reads +28 (m_strName) here, not +20 like the branch below (sic).
	if ( pInfo->m_bLan )
		sprintf( szCmd, "name \"%s\"\nconnect %s\n", pszName, (LPCSTR)pInfo->m_strName );
	else
		sprintf( szCmd, "name \"%s\"\nconnect %s:%i\n", pszName, (LPCSTR)pInfo->m_strAddress, pInfo->m_nPort );

	if ( engineapi.Cbuf_AddText )
		engineapi.Cbuf_AddText( szCmd );
}

