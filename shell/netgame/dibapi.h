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
// Purpose: declares shell artwork loading and the DIB helpers.
//
// $NoKeywords: $
//=============================================================================

#ifndef DIBAPI_H
#define DIBAPI_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>

#define DIB_HEADER_MARKER	( (WORD)( 'M' << 8 ) | 'B' )

HGLOBAL	WINAPI DIB_LoadBitmapFile( const char* pszName );
BOOL	WINAPI DIB_BlitDib( HDC hdc, RECT* prcDst, HGLOBAL hDib, RECT* prcSrc );
void*	WINAPI DIB_FindBits( LPBITMAPINFOHEADER pDIB );
DWORD	WINAPI DIB_Width( LPBITMAPINFOHEADER pDIB );
DWORD	WINAPI DIB_Height( LPBITMAPINFOHEADER pDIB );
WORD	WINAPI DIB_PaletteSize( LPBITMAPINFOHEADER pDIB );
WORD	WINAPI DIB_NumColors( LPBITMAPINFOHEADER pDIB );

#endif // DIBAPI_H
