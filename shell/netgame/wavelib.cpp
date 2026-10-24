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
// Purpose: the MMIO RIFF/WAVE reader behind Snd_PlayMenuSound's DirectSound
//          path.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/*
==================
WaveOpenFile (0x46E490)

Opens the file, walks down to its 'fmt ' chunk and hands back a GlobalAlloc'd
WAVEFORMATEX.  On any failure the format block and the file are released and
*phmmioIn is cleared, so the caller only has to check the return.
==================
*/
MMRESULT WaveOpenFile( LPSTR pszFileName, HMMIO* phmmioIn, WAVEFORMATEX** ppwfxInfo, MMCKINFO* pckInRIFF )
{
	MMCKINFO		ckIn;
	PCMWAVEFORMAT	pcmWaveFormat;
	HMMIO			hmmioIn;
	MMRESULT		mmrError;
	WORD			cbExtraAlloc;

	*ppwfxInfo = NULL;

	hmmioIn = mmioOpen( pszFileName, NULL, MMIO_ALLOCBUF | MMIO_READ );
	if ( !hmmioIn )
	{
		mmrError = ER_CANNOTOPEN;
		goto ERROR_READING_WAVE;
	}

	mmrError = mmioDescend( hmmioIn, pckInRIFF, NULL, 0 );
	if ( mmrError )
		goto ERROR_READING_WAVE;

	if ( pckInRIFF->ckid != FOURCC_RIFF
	  || pckInRIFF->fccType != mmioFOURCC( 'W', 'A', 'V', 'E' ) )
	{
		mmrError = ER_NOTWAVEFILE;
		goto ERROR_READING_WAVE;
	}

	ckIn.ckid = mmioFOURCC( 'f', 'm', 't', ' ' );
	mmrError = mmioDescend( hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK );
	if ( mmrError )
		goto ERROR_READING_WAVE;

	if ( ckIn.cksize < sizeof( PCMWAVEFORMAT ) )
	{
		mmrError = ER_NOTWAVEFILE;
		goto ERROR_READING_WAVE;
	}

	if ( mmioRead( hmmioIn, (HPSTR)&pcmWaveFormat, sizeof( PCMWAVEFORMAT ) )
		!= sizeof( PCMWAVEFORMAT ) )
	{
		mmrError = ER_CANNOTREAD;
		goto ERROR_READING_WAVE;
	}

	// PCM carries no extra bytes and stores no cbSize; everything else does.
	if ( pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM )
	{
		cbExtraAlloc = 0;
	}
	else
	{
		if ( mmioRead( hmmioIn, (HPSTR)&cbExtraAlloc, sizeof( cbExtraAlloc ) )
			!= sizeof( cbExtraAlloc ) )
		{
			mmrError = ER_CANNOTREAD;
			goto ERROR_READING_WAVE;
		}
	}

	*ppwfxInfo = (WAVEFORMATEX*)GlobalAlloc( GMEM_FIXED, sizeof( WAVEFORMATEX ) + cbExtraAlloc );
	if ( !*ppwfxInfo )
	{
		mmrError = ER_MEM;
		goto ERROR_READING_WAVE;
	}

	memcpy( *ppwfxInfo, &pcmWaveFormat, sizeof( pcmWaveFormat ) );
	( *ppwfxInfo )->cbSize = cbExtraAlloc;

	if ( cbExtraAlloc )
	{
		if ( mmioRead( hmmioIn, (HPSTR)( *ppwfxInfo ) + sizeof( WAVEFORMATEX ), cbExtraAlloc )
			!= cbExtraAlloc )
		{
			mmrError = ER_NOTWAVEFILE;
			goto ERROR_READING_WAVE;
		}
	}

	mmrError = mmioAscend( hmmioIn, &ckIn, 0 );
	if ( mmrError )
		goto ERROR_READING_WAVE;

	*phmmioIn = hmmioIn;
	return 0;

ERROR_READING_WAVE:
	if ( *ppwfxInfo )
	{
		GlobalFree( *ppwfxInfo );
		*ppwfxInfo = NULL;
	}
	if ( hmmioIn )
		mmioClose( hmmioIn, 0 );
	*phmmioIn = NULL;
	return mmrError;
}

/*
==================
WaveStartDataRead (0x46E650)

Seeks back to the top of the RIFF body and descends into 'data'.
==================
*/
MMRESULT WaveStartDataRead( HMMIO* phmmioIn, MMCKINFO* pckIn, MMCKINFO* pckInRIFF )
{
	mmioSeek( *phmmioIn, pckInRIFF->dwDataOffset + sizeof( FOURCC ), SEEK_SET );
	pckIn->ckid = mmioFOURCC( 'd', 'a', 't', 'a' );
	return mmioDescend( *phmmioIn, pckIn, pckInRIFF, MMIO_FINDCHUNK );
}

/*
==================
WaveReadFile (0x46E690)

Copies out of the MMIO buffer a byte at a time, advancing it when it runs dry;
cbRead is clamped to what is left of the chunk.
==================
*/
MMRESULT WaveReadFile( HMMIO hmmioIn, UINT cbRead, BYTE* pbDest, MMCKINFO* pckIn, UINT* cbActualRead )
{
	MMIOINFO	mmii;
	MMRESULT	mmrError;
	UINT		cbDataIn;
	UINT		cT;

	mmrError = mmioGetInfo( hmmioIn, &mmii, 0 );
	if ( mmrError )
	{
		*cbActualRead = 0;
		return mmrError;
	}

	cbDataIn = cbRead;
	if ( cbDataIn > pckIn->cksize )
		cbDataIn = pckIn->cksize;
	pckIn->cksize -= cbDataIn;

	for ( cT = 0; cT < cbDataIn; cT++ )
	{
		if ( mmii.pchNext == mmii.pchEndRead )
		{
			mmrError = mmioAdvance( hmmioIn, &mmii, MMIO_READ );
			if ( mmrError )
			{
				*cbActualRead = 0;
				return mmrError;
			}
			if ( mmii.pchNext == mmii.pchEndRead )
			{
				*cbActualRead = 0;
				return ER_CORRUPTWAVEFILE;
			}
		}

		pbDest[cT] = *mmii.pchNext;
		mmii.pchNext++;
	}

	mmrError = mmioSetInfo( hmmioIn, &mmii, 0 );
	if ( mmrError )
	{
		*cbActualRead = 0;
		return mmrError;
	}

	*cbActualRead = cbDataIn;
	return mmrError;
}

/*
==================
WaveLoadFile (0x46E760)

Whole-file load: format block and PCM data both come back GlobalAlloc'd.
cSamples is taken but never written (sic).
==================
*/
MMRESULT WaveLoadFile( LPSTR pszFileName, UINT* cbSize, DWORD* cSamples, WAVEFORMATEX** ppwfxInfo, BYTE** ppbData )
{
	HMMIO		hmmioIn;
	MMCKINFO	ckInRIFF;
	MMCKINFO	ckIn;
	MMRESULT	mmrError;
	UINT		cbActualRead;

	*ppbData    = NULL;
	*ppwfxInfo  = NULL;
	*cbSize     = 0;

	mmrError = WaveOpenFile( pszFileName, &hmmioIn, ppwfxInfo, &ckInRIFF );
	if ( mmrError )
		goto ERROR_LOADING;

	mmrError = WaveStartDataRead( &hmmioIn, &ckIn, &ckInRIFF );
	if ( mmrError )
		goto ERROR_LOADING;

	*ppbData = (BYTE*)GlobalAlloc( GMEM_FIXED, ckIn.cksize );
	if ( !*ppbData )
	{
		mmrError = ER_MEM;
		goto ERROR_LOADING;
	}

	mmrError = WaveReadFile( hmmioIn, ckIn.cksize, *ppbData, &ckIn, &cbActualRead );
	if ( mmrError )
		goto ERROR_LOADING;

	*cbSize = cbActualRead;
	goto DONE;

ERROR_LOADING:
	if ( *ppbData )
	{
		GlobalFree( *ppbData );
		*ppbData = NULL;
	}
	if ( *ppwfxInfo )
	{
		GlobalFree( *ppwfxInfo );
		*ppwfxInfo = NULL;
	}

DONE:
	if ( hmmioIn )
		mmioClose( hmmioIn, 0 );
	return mmrError;
}
