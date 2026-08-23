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
// Purpose: declares the exe-side video interface.
//
// $NoKeywords: $
//=============================================================================

#ifndef VID_H
#define VID_H
#ifdef _WIN32
#pragma once
#endif

#include "launcher.h"

#include "vmodes.h"

// The persisted engine video selection (g_EngineMode at 4E3160), mirrored to the
// "Settings" registry "Engine*" values.
typedef struct enginemode_s
{
	int		vidtype;			// 0x00  "EngineType"
	char	typeName[0x20];		// 0x04  "EngineTypeName" (e.g. "VT_OpenGL")
	int		mode;				// 0x24  "EngineMode" ordinal
	int		width;				// 0x28  "EngineModeW"
	int		height;				// 0x2C  "EngineModeH"
	int		bpp;				// 0x30  "EngineModeBPP"
	int		windowed;			// 0x34  "EngineModeWindowed"
	int		captured;			// 0x38  "EngineModeCaptured"
	char	glDriver[0x80];		// 0x3C  "EngineGLDriver"
	char	d3dDevice[0x80];	// 0xBC  "EngineD3DDevice"
} enginemode_t;					// 0x13C
extern enginemode_t	g_EngineMode;
const char*	Eng_VidTypeName( vidtype_t type );

extern int	g_nLauncherDefW;	// 0x4CF8A0 desktop/default width
extern int	g_nLauncherDefH;	// 0x4CF8A4 desktop/default height

#ifdef LAUNCHER_FIXES
// The fullscreen shell: a real display-mode switch, so the display scaler is
// what enlarges the 640x480 skin.  See the block comment in vid_win.cpp.
int		Shell_FullscreenActive( void );
void	Shell_ComputeMetrics( void );
int		Shell_EnterFullscreen( void );
void	Shell_LeaveFullscreen( void );
int		Shell_HasFullscreenMode( void );

// The black fill behind a shell that has no mode small enough to switch to.
void	Shell_ShowBackdrop( int bShow );
#else
#define Shell_FullscreenActive()	0
#define Shell_HasFullscreenMode()		0
#define Shell_ComputeMetrics()			( (void)0 )
#define Shell_EnterFullscreen()			0
#define Shell_LeaveFullscreen()			( (void)0 )
#define Shell_ShowBackdrop( bShow )		( (void)0 )
#endif

// VID_* -- exe-side video callbacks the engine binds into exefuncs_t.
void	VID_LockBuffer( void );
void	VID_UnlockBuffer( void );
void	VID_GetVID( struct viddef_s* pvid );
void	VID_Shutdown( void );
void	VID_Update( struct vrect_s* rects );
void	VID_ForceLockState( int lk );
int		VID_ForceUnlockedAndReturnState( void );
void	VID_SetDefaultMode( void );
char*	VID_GetExtModeDescription( int mode );

// Mode selection / engine-launcher window switch.
HWND	VID_SetMode( int mode );	// hw.dll/sw.dll via Eng_Load
HWND	VID_ApplyMode( void );
int		VID_RestartNeeded( void );
void	Vid_SetRendererFlags( vidtype_t type );
extern DWORD	WindowStyle;		// 0x4E2148 engine window style, set by
									//          Launcher_CreateEngineWindow
extern HWND		gLauncherWnd;		// 0x4EA8DC launcher dialog window
void			VID_DestroyDIB( void );
extern struct viddef_s	vid;				// 0x4E6D60 the shared video definition
extern int		in_mode_set;		// 0x4E6DF4 a mode switch is in progress
extern int		force_mode_set;		// 0x4E1ECC one-shot forced restart
extern int		force_minimized;	// 0x4E6DF0 suppressed during mode enum
extern int		window_x;			// 0x4E6DBC
extern int		window_y;			// 0x4E6DB8
extern int		DDActive;			// 0x4EA8E4 DirectDraw surface is up
extern int		vid_modenum;		// 0x4D06C8 current engine mode ordinal
extern int		g_bVidGL;			// 0x4E6E00 renderer flags (Vid_SetRendererFlags)
extern int		g_bVidD3D;			// 0x4E6E04
extern int		g_bVidTypeChanged;	// 0x4E6DFC set by Eng_StartupEngine on renderer change
extern int		g_bEngineWindowUp;	// 0x4F93B0 true when the engine window is showing

void			VID_UpdateWindowStatus( void );
int				VID_ChangeDisplaySettings( int w, int h, int bpp );

// Engine-window show / focus plumbing.
void	VID_ShowEngineWindow( int bShow );
// vid_win.cpp is built without the MFC headers, so it cannot name
// CServerBrowser; this shim forwards to the method for it.  Ours, not the
// binary's -- 0x464320 calls SaveFavoriteServers directly.
void	Launcher_SaveFavoriteServers( void* pBrowser );

void	VID_HideEngineWindow( void );	// back to the launcher shell
void	VID_SwitchToLauncher( void );	// focus lost while engine active
void	VID_SwitchToEngine( void );		// focus regained while engine active

void	DDraw_Shutdown( void );
int		DDraw_Init( int bExclusive, int flags );
void	DDraw_RestoreLostSurfaces( void );
void	DDraw_ReleaseSurfaces( void );
int		DDraw_CreateSurfaces( int mode, int bExclusive );
int		DDraw_SetDisplayMode( int mode );
int		DDraw_IsModeAvailable( void );
int		DDraw_QueryD3D( void );

extern int							ActiveApp;		// 0x4E1F00
extern struct IDirectDrawSurface*	lpPrimary;		// 0x4E1A2C
extern struct IDirectDrawSurface*	lpBackBuffer;	// 0x4E19A8
extern struct IDirectDrawSurface*	g_lpddsSystem;	// 0x4E1A30
extern struct IDirectDraw4*			glpDD4;			// 0x4E1A24
extern struct IUnknown*				glpD3D;			// 0x4E1A28 the queried IDirect3D3

extern int	g_currentMode;			// 0x4E6E10 selected mode
extern int	vid_stretched;			// 0x4E6DE8 render at half resolution
extern int	DIBWidth;				// 0x4E6DC4
extern int	DIBHeight;				// 0x4E6DC0
extern RECT	WindowRect;				// 0x4E6DD8 current render rect

class CServerBrowser;
extern CServerBrowser*	g_pServerBrowser;			// 0x4F4DF8

#endif // VID_H
