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
// Purpose: declares the game heap and platform gate, the window proc and
//          input, and the DirectDraw mode probe.
//
// $NoKeywords: $
//=============================================================================

#ifndef GAMEUI_H
#define GAMEUI_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>

int		AllocGameMem( void );						// the game heap + the platform gate
LRESULT CALLBACK MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
void	SleepUntilInput( int time );				// idle wait on tevent
void	AppActivate( int fActive, int minimize );
void	ClearAllStates( void );						// drop every held key/button
int		D_SurfaceCacheForRes( int width, int height );	// exported through ef
void	Vid_BuildModeList( void );
#ifdef LAUNCHER_FIXES
// Whether the mode dialog should offer this resolution for that renderer; the
// enumerated list is the union of every renderer's modes.
int		Vid_ModeAllowedForRenderer( int vidtype, int w, int h );
#endif
void	IN_ShowMouse( void );
void	IN_HideMouse( void );

int		Vid_D3DSupported( void );
int		Vid_OpenGLSupported( void );
int		Vid_TrySetMode( const char* pszDriver, int type, int mode, int w, int h, int bpp );

extern int		g_bWinNT;				// (4E1F0C) running on Windows NT
extern HANDLE	tevent;					// (4E1F10) idle-wait event (SleepUntilInput)
extern int		g_bBlockMouseEvents;	// (4E199C) swallow mouse input while a connect runs

#endif // GAMEUI_H
