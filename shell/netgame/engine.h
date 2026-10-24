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
// Purpose: declares the engine host interface (Eng_*, Sys_Init) and the engine
//          state the launcher reads.
//
// $NoKeywords: $
//=============================================================================

#ifndef ENGINE_H
#define ENGINE_H
#ifdef _WIN32
#pragma once
#endif


#include "launcher.h"

#include "dll_state.h"

// Client connection state, mirroring the engine's cactive_t
typedef enum
{
	ca_dedicated = 0,
	ca_disconnected,
	ca_connecting,
	ca_connected,
	ca_uninitialized,
	ca_active
} cactive_t;

// Host_Frame stateInfo out-of-band request codes (gDLLStateInfo).
#define DLL_INFO_NONE			0
#define DLL_INFO_RELAYOUT_1		1
#define DLL_INFO_RELAYOUT_2		2
#define DLL_INFO_OPEN_MANUAL	3
#define DLL_INFO_MENU_BUTTON	4
#define DLL_INFO_WORLDCRAFT		5

extern int			gDLLState;
extern int			gDLLStateInfo;
extern int			gBackground;
extern int			ActiveApp;
extern int			gbConsoleMode;
extern int			gEngineModeWindowed;
extern vidtype_t	gEngineVidType;

extern double		g_flLastFrameTime;
extern int			g_bChangingVideoModes;
extern int			iWait;
extern int			g_bWaitingToRestore;
extern int			g_iForcedState;

extern const char*	gszLastDLL;
extern HDC			g_maindc;
extern HGLRC		g_baseRC;
extern int			g_bQuitting;
extern int			g_bResumeOnSwitch;
extern int			g_bRestartPending;
extern int			g_bValidCD;

// Engine-owned struct filled by engineapi.GetGameInfo
typedef struct GameInfo_s
{
	int				build_number;	// +0  engine build_number
	int				multiplayer;	// +4  svs.maxclients != 1
	int				maxclients;		// +8
	char			levelname[32];	// +12  sv.name
	int				active;			// +44  sv.active (live server)
	unsigned char	ip[4];			// +48  remote server ip (when connected)
	unsigned short	port;			// +52  remote server port
	char			szStatus[256];	// +54  human-readable connection status
	cactive_t		state;			// +312  cls.state (ca_active == 5)
	int				signon;			// +316  cls.signon != 0
} GameInfo_t;						// 0x140 = 320 bytes

static_assert( sizeof( GameInfo_t ) == 0x140, "GameInfo_t size mismatch" );

void	Sys_Init( void );
double	Sys_DoubleTime( void );

void	Eng_GameSetState( int iState );
int		Launcher_StartEngine( int bBackground );
void	Eng_SetSubState( int iSubState );
int		Eng_SaveBeforeModeSwitch( void );
int		Eng_StartupEngine( int bModeSwitch, HMODULE* phModule, int* pnError );
HWND	Eng_Shutdown( void );
int		Eng_OnLoaded( void );
int		Eng_NeedsReload( void );
int		Eng_ConfirmModeSwitch( int* pState );

void	Eng_PreLoad( void );
int		Eng_ShouldReload( void );
HWND	Launcher_RestoreAfterEngine( int windowed );

class CServerInfo;
void	Eng_ConnectToServer( CServerInfo* pInfo );


void	AntiCheat_ScanForCheatWindows( double time );

void	Eng_DeferRelayout1( int a );
void	Eng_DeferRelayout2( int a );
void	Eng_DeferOpenManual( int a );
void	Eng_DeferMenuButton( int a );

#endif // ENGINE_H
