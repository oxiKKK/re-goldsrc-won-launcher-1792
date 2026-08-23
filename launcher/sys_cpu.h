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
// Purpose: declares CPU feature and speed detection (CPUID, RDTSC, MMX).
//
// $NoKeywords: $
//=============================================================================

#ifndef SYS_CPU_H
#define SYS_CPU_H
#ifdef _WIN32
#pragma once
#endif

int		Sys_GetCPUSpeed( void );
int		Sys_CheckMMXTechnology( void );

#endif // SYS_CPU_H
