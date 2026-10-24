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
// Purpose: declares joystick input helpers (Joy_*).
//
// $NoKeywords: $
//=============================================================================

#ifndef JOYSTICK_H
#define JOYSTICK_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>

extern int	joy_avail;							// set by Joy_Detect

void	Joy_Detect( void );
DWORD*	Joy_RawValuePointer( int nAxis );
void	Joy_AdvancedUpdate( void );
char*	Joy_GetButtonName( void );
BOOL	Joy_ReadJoystick( void );
BOOL	Joy_GetPressedButton( char* pszName );

#endif // JOYSTICK_H
