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
// Purpose: declares the launcher colour scheme (Scheme_*).
//
// $NoKeywords: $
//=============================================================================

#ifndef SCHEME_H
#define SCHEME_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>

COLORREF	Scheme_GetColor( const char* pszName );
void		Scheme_Free( void );
void		Launcher_LoadScheme( void );

#endif // SCHEME_H
