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
// Purpose: declares modestate_t and the 300-byte vmode_t, one entry of the
//          enumerated video-mode table.
//
// $NoKeywords: $
//=============================================================================

#ifndef VIDDEF_H
// The software renderer's warp buffer, in Quake's terms.
#define WARP_WIDTH		320
#define WARP_HEIGHT		200

#define VIDDEF_H
#ifdef _WIN32
#pragma once
#endif

typedef enum { MS_WINDOWED, MS_FULLSCREEN, MS_FULLDIB, MS_UNINIT } modestate_t;

#ifdef LAUNCHER_FIXES
// LAUNCHER_FIXES: modelist capacity.  The original define lives in vid_win.cpp
// alone and is 50, which is enough for a fixed seven-entry 4:3 probe table plus
// a handful of DirectDraw modes.  Enumerating the display driver's real mode
// list (Vid_BuildModeList) can easily exceed that, and neither insertion loop
// bounds-checks nummodes, so the table is exported here for the probe to check
// against.
#define MAX_MODE_LIST	256
#endif


typedef struct vmode_s
{
	modestate_t	type;			// +0
	int			width;			// +4
	int			height;			// +8
	int			modenum;		// +12  this mode's ordinal index
	int			is15bit;		// +16  (Quake: mode13) DDraw mode is 15-bit (555) colour
	int			stretched;		// +20  (Quake: stretched) render at half resolution
	int			dib;			// +24  software / DIB mode
	int			isHardware;		// +28  (Quake: fullscreen) DirectDraw hardware mode
	int			bpp;			// +32
	int			nocenter;		// +36  (Quake: halfscreen) place the window at 0,0
	int			refresh;		// +40  refresh rate in Hz (mode-list text)
	char		modedesc[256];	// +44  mode name (pads the entry to 300)
} vmode_t;

extern vmode_t			modelist[];	// 4E6E30
extern int				nummodes;	// 4EA8C8  number of enumerated modes

vmode_t*	VID_GetModePtr( int mode );		/* 0x42DA20 */

#endif // VIDDEF_H
