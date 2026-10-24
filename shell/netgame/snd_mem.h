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
// Purpose: declares the WAV (RIFF) sound loader (QuakeWorld snd_mem.c
//          lineage).
//
// $NoKeywords: $
//=============================================================================

#ifndef SND_MEM_H
#define SND_MEM_H
#ifdef _WIN32
#pragma once
#endif

// Parsed WAVE format / loop info.
typedef struct
{
	int		rate;		// samples/sec
	int		width;		// bytes per sample (bits / 8)
	int		channels;	// channel count
	int		loopstart;	// cue loop start (-1 if none)
	int		samples;	// total samples (or loop end)
	int		dataofs;	// byte offset of the data chunk from the file start
} wavinfo_t;

wavinfo_t	GetWavinfo( const char* name, unsigned char* wav, int wavlength );

#endif // SND_MEM_H
