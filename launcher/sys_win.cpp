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
// Purpose: launcher system helpers: registry reset, command-line parm edits
//          and string scrubbing.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/*
==================
Sys_RegDeleteSubkeys (0x4689A0)
==================
*/
static LSTATUS Sys_RegDeleteSubkeys( HKEY hKey )
{
	char	name[256];
	DWORD	cchName = sizeof( name );
	LSTATUS	result;
	HKEY	hSub;

	memset( name, 0, sizeof( name ) );
	result = RegEnumKeyExA( hKey, 0, name, &cchName, NULL, NULL, NULL, NULL );
	if ( result == ERROR_SUCCESS )
	{
		do
		{
			result = RegOpenKeyExA( hKey, name, 0, KEY_ALL_ACCESS, &hSub );
			if ( result )
				break;
			Sys_RegDeleteSubkeys( hSub );
			RegCloseKey( hSub );
			RegDeleteKeyA( hKey, name );
			cchName = sizeof( name );
			result = RegEnumKeyExA( hKey, 0, name, &cchName, NULL, NULL, NULL, NULL );
		}
		while ( result != ERROR_NO_MORE_ITEMS );
	}
	return result;
}

/*
==================
Sys_ResetSettings (0x468A70)
==================
*/
void Sys_ResetSettings( void )
{
	HKEY	hKey;

	CString	strKey = CString( "SOFTWARE\\Valve\\" ) + "\\" + Launcher_FormatAppName();
	if ( RegOpenKeyExA( HKEY_CURRENT_USER, strKey, 0,
			KEY_ALL_ACCESS, &hKey ) == ERROR_SUCCESS )
	{
		Sys_RegDeleteSubkeys( hKey );
		RegCloseKey( hKey );
	}
	if ( RegOpenKeyExA( HKEY_CURRENT_USER, "SOFTWARE\\Valve\\", 0,
			KEY_ALL_ACCESS, &hKey ) == ERROR_SUCCESS )
	{
		RegDeleteKeyA( hKey, "Half-Life" );
		RegCloseKey( hKey );
	}
}

/*
==================
Sys_StripCmdLineParm (0x468BA0)
==================
*/
void Sys_StripCmdLineParm( const char* pszParm )
{
	char*	found;
	char*	end;
	int		total;
	int		tail;

	if ( !gpszCmdLine )
		return;

	while ( ( found = strstr( gpszCmdLine, pszParm ) ) != NULL )
	{
		total = strlen( gpszCmdLine ) + 1;
		end   = found + 1;		// skip the parm's own leading +/-
		while ( *end && *end != '-' && *end != '+' )
			++end;

		if ( *end )
		{
			// collapse [found, end): shift the tail down over the removed span
			tail = (int)( &gpszCmdLine[total - 1] - end );
			memmove( found, end, tail );
			found[tail] = 0;
		}
		else
		{
			memset( found, 0, end - found );	// run reached the end of the line
		}
	}

	// trailing-space trim (guarded against an emptied line)
	while ( *gpszCmdLine && gpszCmdLine[strlen( gpszCmdLine ) - 1] == ' ' )
		gpszCmdLine[strlen( gpszCmdLine ) - 1] = 0;
}

/*
==================
Sys_SetCmdLineParm (0x468C80)
==================
*/
void Sys_SetCmdLineParm( const char* pszParm, const char* pszValue )
{
	unsigned int	len;
	char*			pszNew;

	len = strlen( pszParm );
	if ( pszValue )
		len += strlen( pszValue ) + 1;
	len += 1;

	if ( gpszCmdLine )
	{
		Sys_StripCmdLineParm( pszParm );

		pszNew = (char*)malloc( len + strlen( gpszCmdLine ) + 2 );
		memset( pszNew, 0, len + strlen( gpszCmdLine ) + 2 );
		strcpy( pszNew, gpszCmdLine );
		strcat( pszNew, " " );
		strcat( pszNew, pszParm );
		if ( pszValue )
		{
			strcat( pszNew, " " );
			strcat( pszNew, pszValue );
		}

		free( gpszCmdLine );
		gpszCmdLine = pszNew;
	}
	else
	{
		gpszCmdLine = (char*)malloc( len );
		strcpy( gpszCmdLine, pszParm );
		if ( pszValue )
		{
			strcat( gpszCmdLine, " " );
			strcat( gpszCmdLine, pszValue );
		}
	}
}

/*
==================
Launcher_BinToHex (0x468E70)
==================
*/
const char* Launcher_BinToHex( const unsigned char* pBytes, int nBytes )
{
	static char	s_hex[128];		// 0x4F9A60
	char		hex[12];
	int			i;

	memset( s_hex, 0, sizeof( s_hex ) );
	if ( nBytes <= 0 )
		return s_hex;

	for ( i = 0; i < nBytes; i++ )
	{
		sprintf( hex, "%02x", pBytes[i] );
		strcat( s_hex, hex );
	}
	return s_hex;
}

/*
==================
Sys_StripQuotesAndPercents (0x468F00)
==================
*/
unsigned int Sys_StripQuotesAndPercents( char* psz )
{
	char			buf[4096];
	char*			out = buf;
	const char*		in = psz;
	char			c;
	unsigned int	len;

	buf[0] = 0;
	for ( c = *in; c; c = *++in )
	{
		if ( c != '"' && c != '%' )
			*out++ = c;
	}
	*out = 0;

	len = strlen( buf ) + 1;
	memcpy( psz, buf, len );
	return len;
}
