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
// Purpose: declares CD audio (background music) via MCI on a worker thread.
//
// $NoKeywords: $
//=============================================================================

#ifndef CD_WIN_H
#define CD_WIN_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>

// A queued CD command: a worker function plus its two int args.
typedef void ( *cdcmd_fn_t )( int param1, int param2 );

void	CDAudio_Init( void );
void	CDAudio_Shutdown( void );
void	CDAudio_Play( int track, int looping );
void	CDAudio_Pause( void );
void	CDAudio_Resume( void );
void	CDAudio_Update( void );
void	CDAudio_Stop( void );
void	CDAudio_Eject( void );
void	CDAudio_CloseDoor( void );
void	CDAudio_SwitchToLauncher( void );
void	CDAudio_SwitchToEngine( void );
void	CD_f( void );
LONG	CDAudio_MessageHandler( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
int		CDAudio_OpenDevice( void );
int		CDAudio_QueueCommand( cdcmd_fn_t pfn, int param1, int param2 );

// Set when a mode switch paused CD audio; cleared once it is resumed.
extern int	resumeOnSwitch;

#endif // CD_WIN_H
