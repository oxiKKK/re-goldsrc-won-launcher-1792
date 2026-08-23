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
// Purpose: declares the single-slot WAVEHDR playback-buffer manager (snd_win.c
//          lineage).
//
// $NoKeywords: $
//=============================================================================

#ifndef SND_WIN_H
#define SND_WIN_H
#ifdef _WIN32
#pragma once
#endif

// Single-slot waveOut playback buffers (menu beeps).
void	Snd_AllocBuffers( int iSlot, int nBytes );
int		Snd_FreeBuffers( int iSlot );
int		Snd_AcquireSlot( void );
int		Snd_GetBufferSize( int iSlot );
void	Snd_SetBufferSize( int iSlot, int nBytes );
int		Snd_ResetSlots( void );

void	Snd_PlayMenuSound( int id );

// Snd_PlayMenuSound ids -> media/<name>.wav
#define	UISND_SELECT1	0	// launch_select1
#define	UISND_SELECT2	1	// launch_select2
#define	UISND_UPMENU	2	// launch_upmenu1 (menu fly up)
#define	UISND_DNMENU	3	// launch_dnmenu1 (menu fly down)
#define	UISND_GLOW		4	// launch_glow1 (hover)
#define	UISND_DENY1		5	// launch_deny1
#define	UISND_DENY2		6	// launch_deny2

#endif // SND_WIN_H
