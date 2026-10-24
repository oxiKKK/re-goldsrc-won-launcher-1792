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
// Purpose: declares the MMIO RIFF/WAVE reader used by the DirectSound path.
//
// $NoKeywords: $
//=============================================================================

#ifndef WAVELIB_H
#define WAVELIB_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>
#include <mmsystem.h>

#define ER_MEM				0xE000
#define ER_CANNOTOPEN		0xE100
#define ER_NOTWAVEFILE		0xE101
#define ER_CANNOTREAD		0xE102
#define ER_CORRUPTWAVEFILE	0xE103

MMRESULT	WaveOpenFile( LPSTR pszFileName, HMMIO* phmmioIn, WAVEFORMATEX** ppwfxInfo, MMCKINFO* pckInRIFF );
MMRESULT	WaveStartDataRead( HMMIO* phmmioIn, MMCKINFO* pckIn, MMCKINFO* pckInRIFF );
MMRESULT	WaveReadFile( HMMIO hmmioIn, UINT cbRead, BYTE* pbDest, MMCKINFO* pckIn, UINT* cbActualRead );
MMRESULT	WaveLoadFile( LPSTR pszFileName, UINT* cbSize, DWORD* cSamples, WAVEFORMATEX** ppwfxInfo, BYTE** ppbData );

#endif // WAVELIB_H
