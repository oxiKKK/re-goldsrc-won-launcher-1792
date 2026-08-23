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
// Purpose: the localized string-table subsystem (Launcher_LoadString*,
//          ErrorMessageBox).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The title !game expands to when no mod is current.  Not a literal: the
// trailing-space trim below writes through it.
static char	s_szDefaultGame[] = "Half-Life";		// 0x4CD9CC

static char	s_szFormatBuf[1024];	// 0x4F9F34  Launcher_FormatString
static char	s_szIntoBuf[1024];		// 0x4FA334  Launcher_LoadStringInto vsprintf scratch
static char	s_szLoadBuf[1024];		// 0x4FA734  Launcher_LoadString result
static char	s_szMsgBuf[1024];		// 0x4FAB34  Launcher_ShowMessageByIdEx vsprintf scratch
static char	s_szPromptBuf[1024];	// 0x4FAF34  Launcher_ErrorMessageBox
static int	s_bInLoadString;		// 0x4FB354  re-entrancy guard

// Per-id string override list (gfx/shell/strings.lst), head 0x4FB350.
struct strover_s { strover_s* next; UINT id; char* str; };
static strover_s*	s_pOverrides;	// empty until Launcher_LoadStrings runs

/*
==================
Launcher_FreeStrings (0x466700)
==================
*/
void Launcher_FreeStrings( void )
{
	strover_s*	next;

	while ( s_pOverrides )
	{
		next = s_pOverrides->next;
		free( s_pOverrides->str );
		free( s_pOverrides );
		s_pOverrides = next;
	}
}

/*
==================
Launcher_LoadStrings (0x466730)
==================
*/
void Launcher_LoadStrings( void )
{
	CToken		tok( NULL );
	char		path[260];
	char		value[1024];
	char*		file;
	strover_s*	o;
	int			id;

	Launcher_FreeStrings();

	strcpy( path, "gfx/shell/strings.lst" );
	file = (char*)COM_LoadMallocFile( path );
	if ( !file )
		return;

	tok.SetData( file );
	tok.SetQuoteMode( TRUE );
	tok.SetCommentMode( TRUE );

	for ( ;; )
	{
		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			break;
		id = atoi( tok.token );

		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			break;
		strcpy( value, tok.token );

		o = (strover_s*)malloc( sizeof( strover_s ) );
		o->str  = _strdup( value );
		o->id   = (UINT)id;
		o->next = s_pOverrides;
		s_pOverrides = o;
	}

	free( file );
}

/*
==================
Launcher_ExpandGameName (0x4668D0)

Substitutes the current mod's title for every "!game" in src.  The trim runs
through the returned key string itself, so the mod list keeps the trimmed form
from here on.
==================
*/
static int Launcher_ExpandGameName( char* out, const char* src )
{
	char		expanded[1024];
	char*		game = s_szDefaultGame;
	char*		g;
	char*		d;
	const char*	s;
	int			n;

	if ( g_pCurrentMod )
	{
		g = (char*)g_pCurrentMod->GetKeyString( "game" );
		if ( g && *g )
			game = g;
	}

	// Trim trailing spaces in place; index 0 is never zeroed.
	for ( n = (int)strlen( game ) - 1; n > 0 && game[n] == ' '; --n )
		game[n] = 0;

	d = expanded;
	s = src;
	while ( *s )
	{
		if ( *s == '!' && s[1] && s[2] && s[3] && s[4] && !_strnicmp( s, "!game", 5 ) )
		{
			for ( g = game; *g; )
				*d++ = *g++;
			s += 5;
			continue;
		}
		*d++ = *s++;
	}
	*d = 0;

	return sprintf( out, "%s", expanded );
}

/*
==================
Launcher_BuildResourcePath (0x4669E0)
==================
*/
int Launcher_BuildResourcePath( UINT uID, char* out )
{
	strover_s*	o;
	char		buf[1024];

	for ( o = s_pOverrides; o; o = o->next )
	{
		if ( o->id == uID )
			return Launcher_ExpandGameName( out, o->str );
	}

	::LoadStringA( Launcher_GetResourceModule(), uID, buf, sizeof( buf ) );
	return Launcher_ExpandGameName( out, buf );
}

/*
==================
Launcher_ErrorMessageBox (0x466A40)
==================
*/
void Launcher_ErrorMessageBox( int nStyle, const char* fmt, ... )
{
	va_list	va;

	va_start( va, fmt );
	vsprintf( s_szPromptBuf, fmt, va );
	va_end( va );

	CPromptDlg	dlg( nStyle, NULL );
	dlg.SetMessage( s_szPromptBuf );
	dlg.DoModal();
}

/*
==================
Launcher_ShowMessageById (0x466B90)
==================
*/
void Launcher_ShowMessageById( int nStyle, UINT uID )
{
	CString	str;
	char	text[1024];

	if ( Launcher_BuildResourcePath( uID, text ) > 0 )
	{
		Launcher_ErrorMessageBox( nStyle, text );
		return;
	}

	if ( str.LoadString( uID ) )
		Launcher_ErrorMessageBox( nStyle, str );
	else
		Launcher_ErrorMessageBox( 0, "Error Loading String: %i", uID );
}

/*
==================
Launcher_ShowMessageByIdEx (0x466C50)

The system message is formatted and freed without ever being shown -- only the
id reaches the box.
==================
*/
void Launcher_ShowMessageByIdEx( int nStyle, UINT uID, ... )
{
	CString	str;
	va_list	va;
	char	fmt[1024];
	LPSTR	pszSysMsg;
	DWORD	err;

	va_start( va, uID );

	if ( Launcher_BuildResourcePath( uID, fmt ) <= 0 )
	{
		if ( !str.LoadString( uID ) )
		{
			err = GetLastError();
			FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, err, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
				(LPSTR)&pszSysMsg, 0, NULL );
			Launcher_ErrorMessageBox( 0, "Error Loading String: %i", uID );
			LocalFree( pszSysMsg );
			va_end( va );
			return;
		}
		strcpy( fmt, str );
	}

	vsprintf( s_szMsgBuf, fmt, va );
	va_end( va );
	Launcher_ErrorMessageBox( nStyle, s_szMsgBuf );
}

/*
==================
Launcher_LoadString (0x466D80)
==================
*/
char* Launcher_LoadString( UINT uID )
{
	CString	str;

	if ( s_bInLoadString )
		return "";

	if ( Launcher_BuildResourcePath( uID, s_szLoadBuf ) <= 0 )
	{
		if ( str.LoadString( uID ) )
		{
			strcpy( s_szLoadBuf, str );
		}
		else
		{
			s_bInLoadString = 1;
			Launcher_ErrorMessageBox( 0, "Error Loading String: %i", uID );
			s_bInLoadString = 0;
			sprintf( s_szLoadBuf, "" );		// (sic) the empty string as a format
		}
	}
	return s_szLoadBuf;
}

/*
==================
Launcher_StringHeight (0x466E80)

Returns the iField'th space-separated number out of the string, which is how
the shell stores per-control metrics in one resource.
==================
*/
int Launcher_StringHeight( UINT uID, int iField )
{
	char		str[256];
	char		tok[128];
	const char*	p;
	char*		d;
	int			index = 0;

	strcpy( str, Launcher_LoadString( uID ) );
	if ( !strlen( str ) )
		return 0;

	p = str;
	for ( ;; )
	{
		d = tok;
		memset( tok, 0, sizeof( tok ) );
		while ( *p && *p != ' ' )
			*d++ = *p++;
		*d = 0;
		if ( !*p )
			break;
		++p;
		if ( ++index > iField )
			break;
	}

	return strlen( tok ) ? atoi( tok ) : 0;
}

/*
==================
Launcher_LoadStringInto (0x466F50)
==================
*/
int Launcher_LoadStringInto( char* out, UINT uID, ... )
{
	CString	str;
	va_list	va;
	char	fmt[1024];
	LPSTR	pszSysMsg;
	DWORD	err;
	int		len;

	va_start( va, uID );

	if ( Launcher_BuildResourcePath( uID, fmt ) <= 0 )
	{
		if ( !str.LoadString( uID ) )
		{
			err = GetLastError();
			FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, err, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
				(LPSTR)&pszSysMsg, 0, NULL );
			Launcher_ErrorMessageBox( 0, "Error Loading String: %i", uID );
			LocalFree( pszSysMsg );
			va_end( va );
			// The binary leaves out untouched here; callers read it regardless.
			if ( out )
				out[0] = 0;
			return 2;
		}
		strcpy( fmt, str );
	}

	len = vsprintf( s_szIntoBuf, fmt, va );
	va_end( va );
	strcpy( out, s_szIntoBuf );
	return len;
}

/*
==================
Launcher_ComputeButtonCell (0x4670C0)
==================
*/
void Launcher_ComputeButtonCell( int* pWH )
{
	char	buf[1024];
	int		w, h;

	pWH[0] = 156;	// default cell width
	pWH[1] = 26;	// default cell height

	if ( Launcher_BuildResourcePath( IDS_LAUNCHER_BUTTONSIZE, buf ) > 0
	  && sscanf( buf, "%i %i", &w, &h ) == 2
	  && w > 1 && w < 2048
	  && h > 1 && h < 2048 )
	{
		pWH[0] = w;
		pWH[1] = h;
	}
}

/*
==================
Launcher_FormatString (0x467190)
==================
*/
char* Launcher_FormatString( const char* fmt, ... )
{
	va_list	va;

	va_start( va, fmt );
	vsprintf( s_szFormatBuf, fmt, va );
	va_end( va );

	return s_szFormatBuf;
}
