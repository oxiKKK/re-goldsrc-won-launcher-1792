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
// Purpose: WAV (RIFF) sound loading (QuakeWorld snd_mem.c lineage).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Declared in BSS-ascending order, which is the reverse of QuakeWorld's.
static unsigned char*	data_p;
static unsigned char*	iff_end;
static unsigned char*	last_chunk;
static unsigned char*	iff_data;
static int				iff_chunk_len;

/*
==================
GetLittleShort (0x4648A0)
==================
*/
static short GetLittleShort( void )
{
	short	val;

	val = *data_p;
	val = val + ( *( data_p + 1 ) << 8 );
	data_p += 2;
	return val;
}

/*
==================
GetLittleLong (0x4648C0)
==================
*/
static int GetLittleLong( void )
{
	int		val;

	val = *data_p;
	val = val + ( *( data_p + 1 ) << 8 );
	val = val + ( *( data_p + 2 ) << 16 );
	val = val + ( *( data_p + 3 ) << 24 );
	data_p += 4;
	return val;
}

/*
==================
FindNextChunk (0x464900)
==================
*/
static void FindNextChunk( const char* name )
{
	while ( 1 )
	{
		data_p = last_chunk;

		if ( data_p >= iff_end )
		{	// didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if ( iff_chunk_len < 0 )
		{
			data_p = NULL;
			return;
		}
		data_p -= 8;
		last_chunk = data_p + 8 + ( ( iff_chunk_len + 1 ) & ~1 );
		if ( !strncmp( (const char*)data_p, name, 4 ) )
			return;
	}
}

/*
==================
FindChunk (0x464970)
==================
*/
static void FindChunk( const char* name )
{
	last_chunk = iff_data;
	FindNextChunk( name );
}

/*
==================
GetWavinfo (0x464990)
==================
*/
wavinfo_t GetWavinfo( const char* name, unsigned char* wav, int wavlength )
{
	wavinfo_t	info;
	int			i;
	int			format;
	int			samples;

	memset( &info, 0, sizeof( info ) );

	if ( !wav )
		return info;

	iff_data = wav;
	iff_end = wav + wavlength;

// find "RIFF" chunk
	FindChunk( "RIFF" );
	if ( !( data_p && !strncmp( (const char*)data_p + 8, "WAVE", 4 ) ) )
		return info;			// missing RIFF/WAVE chunks

// get "fmt " chunk
	iff_data = data_p + 12;
	FindChunk( "fmt " );
	if ( !data_p )
		return info;			// missing fmt chunk
	data_p += 8;
	format = GetLittleShort();
	if ( format != 1 )
		return info;			// Microsoft PCM format only

	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4 + 2;
	info.width = GetLittleShort() / 8;

// get cue chunk
	FindChunk( "cue " );
	if ( data_p )
	{
		data_p += 32;
		info.loopstart = GetLittleLong();

	// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk( "LIST" );
		if ( data_p )
		{
			if ( !strncmp( (const char*)data_p + 28, "mark", 4 ) )
			{	// this is not a proper parse, but it works with cooledit...
				data_p += 24;
				i = GetLittleLong();	// samples in loop
				info.samples = info.loopstart + i;
			}
		}
	}
	else
		info.loopstart = -1;

// find data chunk
	FindChunk( "data" );
	if ( !data_p )
		return info;			// missing data chunk

	data_p += 4;
	samples = GetLittleLong() / info.width;

	if ( info.samples )
	{
		// data shorter than the cue loop is fatal, and unlike QuakeWorld there
		// is no message -- which is why `name` goes unused.
		if ( samples < info.samples )
			exit( -20 );
	}
	else
		info.samples = samples;

	info.dataofs = (int)( data_p - wav );

	return info;
}
