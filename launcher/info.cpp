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
// Purpose: the flat-buffer userinfo key/value API (Info_*).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/*
==================
Info_ValueForKey (0x41AED0)
==================
*/
char* Info_ValueForKey( const char* s, const char* key )
{
	char		pkey[512];
	static char	value[4][512];
	static int	valueindex;
	char*		o;

	valueindex = ( valueindex + 1 ) % 4;
	if ( *s == '\\' )
		s++;
	while ( 1 )
	{
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s )
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];
		while ( *s != '\\' && *s )
		{
			if ( !*s )
				return "";
			*o++ = *s++;
		}
		*o = 0;

		if ( !strcmp( key, pkey ) )
			return value[valueindex];

		if ( !*s )
			return "";
		s++;
	}
}

/*
==================
Info_RemoveKey (0x41AFC0)
==================
*/
void Info_RemoveKey( char* s, const char* key )
{
	char*	start;
	char	pkey[512];
	char	value[512];
	char*	o;

	if ( strstr( key, "\\" ) )
		return;

	while ( 1 )
	{
		start = s;
		if ( *s == '\\' )
			s++;
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while ( *s != '\\' && *s )
		{
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;

		if ( !strcmp( key, pkey ) )
		{
			strcpy( start, s );		// remove this part
			return;
		}

		if ( !*s )
			return;
	}
}

/*
==================
Info_SetValueForStarKey (0x41B0A0)
==================
*/
void Info_SetValueForStarKey( char* s, const char* key, const char* value, int maxsize )
{
	char		newpair[1024];
	const char*	v;
	int			c;

	if ( strstr( key, "\\" ) || strstr( value, "\\" ) )
		return;		// can't use keys or values with a '\'

	if ( strstr( key, "\"" ) || strstr( value, "\"" ) )
		return;		// can't use keys or values with a '"'

	if ( strlen( key ) > 127 || strlen( value ) > 127 )
		return;		// keys and values must be < 128 chars

	Info_RemoveKey( s, key );
	if ( !value || !strlen( value ) )
		return;

	sprintf( newpair, "\\%s\\%s", key, value );

	if ( (int)( strlen( newpair ) + strlen( s ) ) >= maxsize )
		return;		// info string length exceeded

	// only copy ascii values
	s += strlen( s );
	v = newpair;
	while ( *v )
	{
		c = (unsigned char)*v++;
		// client only allows high bits on name
		if ( stricmp( key, "name" ) != 0 )
		{
			c &= 127;
			if ( c < 32 || c > 127 )
				continue;
			// auto lowercase team
			if ( stricmp( key, "team" ) == 0 )
				c = tolower( c );
		}
		if ( c > 13 )		// strip CR and lower control chars
			*s++ = (char)c;
	}
	*s = 0;
}

/*
==================
Info_SetValueForKey (0x41B220)
==================
*/
void Info_SetValueForKey( char* s, const char* key, const char* value, int maxsize )
{
	if ( key[0] == '*' )
		return;		// '*' keys are server-reserved -- silently ignored

	Info_SetValueForStarKey( s, key, value, maxsize );
}
