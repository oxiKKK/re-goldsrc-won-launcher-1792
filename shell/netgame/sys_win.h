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
// Purpose: declares launcher system helpers: registry reset, command-line
//          parm edits and string scrubbing.
//
// $NoKeywords: $
//=============================================================================

#ifndef SYS_WIN_H
#define SYS_WIN_H
#ifdef _WIN32
#pragma once
#endif

// Wipe HKCU\SOFTWARE\Valve\<AppName>.
void	Sys_ResetSettings( void );

void	Sys_StripCmdLineParm( const char* pszParm );
void	Sys_SetCmdLineParm( const char* pszParm, const char* pszValue );

// Lowercase-hex a byte buffer (the gore-lock password digest is stored as its hex).
const char*	Launcher_BinToHex( const unsigned char* pBytes, int nBytes );

// Strip '"' and '%' in place.
unsigned int	Sys_StripQuotesAndPercents( char* psz );

#endif // SYS_WIN_H
