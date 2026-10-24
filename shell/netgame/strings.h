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
// Purpose: declares the localized string-table subsystem
//          (Launcher_LoadString*, ErrorMessageBox).
//
// $NoKeywords: $
//=============================================================================

#ifndef STRINGS_H
#define STRINGS_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>

void	Launcher_FreeStrings( void );
void	Launcher_LoadStrings( void );
int		Launcher_BuildResourcePath( UINT uID, char* out );
void	Launcher_ErrorMessageBox( int nStyle, const char* fmt, ... );
void	Launcher_ShowMessageById( int nStyle, UINT uID );
void	Launcher_ShowMessageByIdEx( int nStyle, UINT uID, ... );
char*	Launcher_LoadString( UINT uID );
int		Launcher_StringHeight( UINT uID, int iField );
int		Launcher_LoadStringInto( char* out, UINT uID, ... );
void	Launcher_ComputeButtonCell( int* pWH );
char*	Launcher_FormatString( const char* fmt, ... );

// The _mbs* runtime predates const-correctness and takes unsigned char*, and a
// CString will not cast straight to that -- MFC's operator LPCTSTR has to run
// first.  MBSTR does both steps so a comparison reads as a comparison.
#define MBSTR( str )	( (const unsigned char*)(LPCSTR)( str ) )


#endif // STRINGS_H
