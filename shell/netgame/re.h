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
// Purpose: declares the LOG / LOG_ENTER debug-logging macros gated on
//          LAUNCHER_RE.
//
// $NoKeywords: $
//=============================================================================

#pragma once

#if defined( LAUNCHER_RE ) && !defined( LAUNCHER_NO_LOG )
#define LAUNCHER_LOGGING 1
#else
#define LAUNCHER_LOGGING 0
#endif

#if LAUNCHER_LOGGING
#include <stdio.h>
#include <stdarg.h>

static __inline void Launcher_DebugLog( const char* func, const char* fmt, ... )
{
	char	msg[1024];
	char	line[1100];
	va_list	ap;

	va_start( ap, fmt );
	_vsnprintf( msg, sizeof( msg ) - 1, fmt, ap );
	va_end( ap );
	msg[sizeof( msg ) - 1] = 0;

	_snprintf( line, sizeof( line ) - 1, "[LAUNCHER] %s: %s\n", func, msg );
	line[sizeof( line ) - 1] = 0;

	OutputDebugStringA( line );
	fputs( line, stdout );
	fflush( stdout );
}

#if defined(_MSC_VER) && (_MSC_VER < 1300)
// VC6 has neither variadic macros nor __FUNCTION__; route LOG through a no-op.
static __inline void LOG(const char*, ...) {}
static __inline void LOG_ENTER() {}
#else
#define LOG( ... )		Launcher_DebugLog( __FUNCTION__, __VA_ARGS__ )
#define LOG_ENTER()		Launcher_DebugLog( __FUNCTION__, "enter" )
#endif

#else	// !LAUNCHER_LOGGING

#if defined(_MSC_VER) && (_MSC_VER < 1300)
static __inline void LOG(const char*, ...) {}
static __inline void LOG_ENTER() {}
#else
#define LOG( ... )		( (void)0 )
#define LOG_ENTER()		( (void)0 )
#endif

#endif	// LAUNCHER_LOGGING
