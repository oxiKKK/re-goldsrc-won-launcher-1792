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
// Purpose: declares the flat-buffer userinfo key/value API (Info_*).
//
// $NoKeywords: $
//=============================================================================

#ifndef INFO_H
#define INFO_H
#ifdef _WIN32
#pragma once
#endif

char* Info_ValueForKey( const char* s, const char* key );
void  Info_RemoveKey( char* s, const char* key );
void  Info_SetValueForStarKey( char* s, const char* key, const char* value, int maxsize );
void  Info_SetValueForKey( char* s, const char* key, const char* value, int maxsize );

#endif // INFO_H
