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
// Purpose: declares the state vid_win.cpp and modes.cpp share.
//
// $NoKeywords: $
//=============================================================================

#ifndef MODES_H
#define MODES_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>
#include "viddef.h"

// The windowed-software DIB section and its lock, shared with vid_win.cpp.
extern HDC		g_hdcSection;		// 0x4F93A8
extern HDC		dibdc;				// 0x4F93AC
extern int		lockcount;			// 0x4EA8E8
extern int		g_bLastMouseActive;	// 0x4E6E1C
extern int		window_width;		// 0x4E6DB4
extern int		window_height;		// 0x4E6DB0
extern RECT		window_rect;		// 0x4E6DC8
extern vmode_t	modelist[];			// 0x4E6E30
extern int		nummodes;			// 0x4EA8C8

vmode_t*		VID_GetModePtr( int mode );
void			VID_BlitSoftware( struct vrect_s* rects );
char*			VID_GetModeDescription( int mode );
#endif // MODES_H
