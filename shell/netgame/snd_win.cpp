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
// Purpose: the single-slot WAVEHDR playback-buffer manager (snd_win.c
//          lineage).
//
// $NoKeywords: $
//=============================================================================

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <stdio.h>
#include "snd_win.h"
#include "snd_mem.h"
#include "wavelib.h"
#include "common.h"
#include "launcher.h"
#include "strings.h"
#include "resource.h"

#define SND_DEFAULT_BUFFER_BYTES	0x8000		// default PCM slot size (32 KiB)

// Every entry point takes an iSlot the binary uses to index these as if they
// were parallel arrays; Snd_AcquireSlot only ever yields 0, so there is exactly
// one slot and the parameter never selects anything else.
static LPWAVEHDR	lpWaveHdr;					// 0x4F950C  locked WAVEHDR
static HGLOBAL		hWaveHdr;					// 0x4F9510  WAVEHDR alloc handle
static void*		lpData;						// 0x4F9514  locked PCM data
static HGLOBAL		hData;						// 0x4F9518  PCM data alloc handle
static DWORD		gSndBufSize;				// 0x4F951C  PCM data size
static int			wav_init;					// 0x4F9520  buffers allocated
static int			s_unk4F9524;				// 0x4F9524  cleared, never read

/*
==================
Snd_AcquireSlot (0x464B50)

Yields the one slot when its header is idle, allocating the buffers first if
they are not up yet.  The retry counter only ever permits a single pass.
==================
*/
int Snd_AcquireSlot( void )
{
	int		nTries = 0;
	DWORD	dwFlags;

	while ( 1 )
	{
		if ( !wav_init )
			Snd_AllocBuffers( 0, SND_DEFAULT_BUFFER_BYTES );

		if ( wav_init )
		{
			if ( lpWaveHdr )
			{
				dwFlags = lpWaveHdr->dwFlags;
				if ( ( dwFlags & WHDR_PREPARED ) == 0
				  || ( ( dwFlags & WHDR_INQUEUE ) == 0 && ( dwFlags & WHDR_DONE ) != 0 ) )
					break;
			}
		}
		if ( ++nTries >= 1 )
			return -1;
	}

	s_unk4F9524 = 0;
	return 0;
}

/*
==================
Snd_GetBufferSize (0x464BB0)
==================
*/
int Snd_GetBufferSize( int iSlot )
{
	return gSndBufSize;
}

/*
==================
Snd_PlayMenuSound (0x464BC0)

Three routes, in the order the binary tries them: sndPlaySound when the engine
has no sound system up, waveOut when it hands back a wave device, and
DirectSound otherwise.  The waveOut route falls through into the DirectSound
one, so a clip can be written to both.
==================
*/
void Snd_PlayMenuSound( int id )
{
	char			name[64];
	char			rel[MAX_PATH];
	LPDIRECTSOUND	pDS    = NULL;
	LPDIRECTSOUNDBUFFER	pDSBuf = NULL;
	LPDIRECTSOUNDBUFFER	pBuf   = NULL;
	HWAVEOUT*		pWav   = NULL;
	FILE*			stream = NULL;
	WAVEFORMATEX*	pwfx   = NULL;
	BYTE*			pbData = NULL;
	LPWAVEHDR		hdr;
	DSBUFFERDESC	dsbd;
	wavinfo_t		info;
	void*			pLock;
	DWORD			cbLock;
	UINT			cbSize;
	DWORD			cSamples;
	int				filelen;
	int				slot;

	switch ( id )
	{
	case UISND_SELECT1: sprintf( name, "%s", "launch_select1" ); break;
	case UISND_SELECT2: sprintf( name, "%s", "launch_select2" ); break;
	case UISND_UPMENU:  sprintf( name, "%s", "launch_upmenu1" ); break;	// fly up
	case UISND_DNMENU:  sprintf( name, "%s", "launch_dnmenu1" ); break;	// fly down
	case UISND_GLOW:    sprintf( name, "%s", "launch_glow1"   ); break;	// hover
	case UISND_DENY2:   sprintf( name, "%s", "launch_deny2"   ); break;
	default:            sprintf( name, "%s", "launch_deny1"   ); break;	// UISND_DENY1
	}

	if ( !strlen( name ) )
	{
		Launcher_ShowMessageById( 0, IDS_SND_BADNAME );
		return;
	}

	// Is the engine's sound system up?  It hands us either a wave device or a
	// DirectSound object; with neither we take the sndPlaySound fallback.
	if ( engineapi.S_GetDSPointer && engineapi.S_GetWAVPointer )
	{
		engineapi.S_GetDSPointer( &pDS, &pDSBuf );
		pWav = (HWAVEOUT*)engineapi.S_GetWAVPointer();
	}

	if ( !pWav && !pDS )
	{
		sprintf( rel, "%s%s.wav", "media\\", name );
		sndPlaySoundA( COM_FindPath( rel ), SND_ASYNC );
		return;
	}

	// Engine sound is up: stream the clip out of the search paths.
	sprintf( rel, "%s%s.wav", "sound/Common/", name );

	filelen = COM_FindFile( rel, NULL, &stream );
	if ( filelen == -1 )
		filelen = 0;

	if ( pWav && filelen )
	{
		// waveOut path: reuse the single playback slot when its header is free.
		slot = Snd_AcquireSlot();
		if ( slot != -1 )
		{
			if ( !Snd_GetBufferSize( slot ) )
				Snd_AllocBuffers( slot, SND_DEFAULT_BUFFER_BYTES );
			if ( filelen > Snd_GetBufferSize( slot ) )
				Snd_SetBufferSize( slot, filelen );

			hdr = lpWaveHdr;
			if ( !hdr
			  || ( hdr->dwFlags & WHDR_PREPARED ) == 0
			  || ( ( hdr->dwFlags & WHDR_INQUEUE ) == 0 && ( hdr->dwFlags & WHDR_DONE ) != 0 ) )
			{
				memset( hdr, 0, sizeof( WAVEHDR ) );
				memset( lpData, 0, filelen );
				fread( lpData, filelen, 1, stream );
				fclose( stream );

				info = GetWavinfo( rel, (unsigned char*)lpData, filelen );
				hdr->dwBufferLength = info.samples;
				hdr->lpData         = (char*)lpData + info.dataofs;

				if ( waveOutPrepareHeader( *pWav, hdr, sizeof( WAVEHDR ) ) == 0 )
					waveOutWrite( *pWav, hdr, sizeof( WAVEHDR ) );
			}
		}
	}

	if ( !pDS || !filelen )
		return;

	// DirectSound path: the clip is re-read off disk through the MMIO reader
	// rather than out of the buffer the waveOut path just filled.
	fclose( stream );

	sprintf( rel, "%s%s.wav", "media\\", name );
	WaveLoadFile( COM_FindPath( rel ), &cbSize, &cSamples, &pwfx, &pbData );

	if ( pbData )
	{
		memset( &dsbd, 0, sizeof( dsbd ) );
		dsbd.dwSize        = sizeof( DSBUFFERDESC );
		dsbd.dwFlags       = DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME;
		dsbd.dwBufferBytes = cbSize;
		dsbd.lpwfxFormat   = pwfx;

		pDS->CreateSoundBuffer( &dsbd, &pBuf, NULL );
		if ( pBuf )
		{
			pBuf->Lock( 0, cbSize, &pLock, &cbLock, NULL, NULL, 0 );
			memcpy( pLock, pbData, cbSize );
			pBuf->Unlock( pLock, cbLock, NULL, 0 );
			pBuf->Play( 0, 0, 0 );
		}
	}

	if ( pwfx )
		GlobalFree( pwfx );
}

/*
==================
Snd_FreeBuffers (0x464FD0)

The unlock loops drain the lock count on the *locked pointer*, not the handle;
the GlobalFree that follows releases the handle.
==================
*/
int Snd_FreeBuffers( int iSlot )
{
	int	n;

	if ( wav_init )
	{
		n = 5;
		while ( GlobalUnlock( lpData ) )
		{
			if ( n-- < 0 )
				break;
			Sleep( 50 );
		}
		for ( n = 5; !GlobalUnlock( lpData ); n-- )
		{
			if ( n < 0 )
				break;
			Sleep( 50 );
		}
		GlobalFree( hData );

		n = 5;
		while ( GlobalUnlock( lpWaveHdr ) )
		{
			if ( n-- < 0 )
				break;
			Sleep( 50 );
		}
		for ( n = 5; !GlobalUnlock( lpWaveHdr ); n-- )
		{
			if ( n < 0 )
				break;
			Sleep( 50 );
		}
		GlobalFree( hWaveHdr );
	}

	wav_init    = 0;
	lpData      = NULL;
	hData       = NULL;
	lpWaveHdr   = NULL;
	hWaveHdr    = NULL;
	gSndBufSize = 0;
	return 0;
}

/*
==================
Snd_AllocBuffers (0x4650F0)
==================
*/
void Snd_AllocBuffers( int iSlot, int nBytes )
{
	gSndBufSize = nBytes;
	hData       = GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, nBytes );
	if ( !hData )
	{
		gSndBufSize = 0;
		wav_init    = 0;
		return;
	}

	lpData = GlobalLock( hData );
	if ( !lpData )
	{
		GlobalFree( hData );
		lpData      = NULL;
		hData       = NULL;
		gSndBufSize = 0;
		wav_init    = 0;
		return;
	}
	memset( lpData, 0, nBytes );

	hWaveHdr = GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, sizeof( WAVEHDR ) );
	if ( !hWaveHdr )
	{
		GlobalUnlock( hData );
		GlobalFree( hData );
		lpData      = NULL;
		hData       = NULL;
		gSndBufSize = 0;
		wav_init    = 0;
		return;
	}

	lpWaveHdr = (LPWAVEHDR)GlobalLock( hWaveHdr );
	if ( !lpWaveHdr )
	{
		GlobalUnlock( hData );
		GlobalFree( hData );
		GlobalFree( hWaveHdr );
		hWaveHdr    = NULL;
		lpData      = NULL;
		hData       = NULL;
		gSndBufSize = 0;
		wav_init    = 0;
		return;
	}

	memset( lpWaveHdr, 0, sizeof( WAVEHDR ) );
	wav_init = 1;
}

/*
==================
Eng_PreLoad (0x465240)
==================
*/
void Eng_PreLoad( void )
{
	int		size;

	if ( !wav_init )
		return;

	size = Snd_GetBufferSize( 0 );
	if ( size > 0 )
		Snd_FreeBuffers( 0 );
}

/*
==================
Snd_SetBufferSize (0x465260)
==================
*/
void Snd_SetBufferSize( int iSlot, int nBytes )
{
	if ( wav_init )
	{
		if ( ( lpWaveHdr->dwFlags & WHDR_DONE ) == 0 )
		{
			while ( 1 )								// (sic) original busy-waits for WHDR_DONE
				;
		}
		Snd_FreeBuffers( iSlot );
	}
	Snd_AllocBuffers( iSlot, nBytes );
}

/*
==================
Snd_ResetSlots (0x4652A0)
==================
*/
int Snd_ResetSlots( void )
{
	wav_init    = 0;
	gSndBufSize = 0;
	hData       = NULL;
	lpData      = NULL;
	hWaveHdr    = NULL;
	lpWaveHdr   = NULL;
	return 0;
}
