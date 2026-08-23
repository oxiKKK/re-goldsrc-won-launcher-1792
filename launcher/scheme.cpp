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
// Purpose: the launcher colour scheme (Scheme_*).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// One named colour from colors.lst (malloc'd, 40 bytes, singly linked).
typedef struct scheme_s
{
	struct scheme_s*	next;		// +0
	char				name[32];	// +4
	COLORREF			color;		// +36
} scheme_t;

static scheme_t*	s_pScheme;		// 0x4FB360 -- empty until Launcher_LoadScheme runs

/*
==================
Scheme_GetDefaultColor (0x468F70)
==================
*/
static COLORREF Scheme_GetDefaultColor( const char* pszName )
{
	if ( !_strcmpi( pszName, "PROMPT_TEXT_COLOR" ) )
		return RGB( 255, 180, 24 );
	if ( !_strcmpi( pszName, "PROMPT_TITLE_COLOR" ) )
		return RGB( 255, 255, 255 );
	if ( !_strcmpi( pszName, "PROMPT_BG_COLOR" ) )
		return RGB( 56, 56, 56 );
	if ( !_strcmpi( pszName, "INPUT_TEXT_COLOR" ) )
		return RGB( 240, 180, 24 );
	if ( !_strcmpi( pszName, "INPUT_BG_COLOR" ) )
		return RGB( 56, 56, 56 );
	if ( !_strcmpi( pszName, "REFRESH_TITLE_COLOR" ) )
		return RGB( 240, 180, 24 );
	if ( !_strcmpi( pszName, "REFRESH_TEXT_COLOR" ) )
		return RGB( 255, 255, 255 );
	if ( !_strcmpi( pszName, "REFRESH_BG_COLOR" ) )
		return RGB( 56, 56, 56 );
	return RGB( 127, 127, 127 );
}

/*
==================
Scheme_GetColor (0x469050)
==================
*/
COLORREF Scheme_GetColor( const char* pszName )
{
	scheme_t*	s;

	if ( !s_pScheme )
		return RGB( 127, 127, 127 );

	for ( s = s_pScheme; s; s = s->next )
	{
		if ( !_strcmpi( pszName, s->name ) )
			return s->color;
	}
	return Scheme_GetDefaultColor( pszName );
}

/*
==================
Scheme_Free (0x4690A0)
==================
*/
void Scheme_Free( void )
{
	scheme_t*	s;
	scheme_t*	next;

	for ( s = s_pScheme; s; s = next )
	{
		next = s->next;
		free( s );
	}
	s_pScheme = NULL;
}

/*
==================
Launcher_LoadScheme (0x4690D0)
==================
*/
void Launcher_LoadScheme( void )
{
	CToken		tok( NULL );
	char		path[MAX_PATH];
	char		name[64];
	int			r, g, b;
	char*		file;
	scheme_t*	s;

	Scheme_Free();

	strcpy( path, "gfx/shell/colors.lst" );
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
		strcpy( name, tok.token );

		tok.ParseNextToken();
		r = atoi( tok.token );
		tok.ParseNextToken();
		g = atoi( tok.token );
		tok.ParseNextToken();
		b = atoi( tok.token );
		if ( !strlen( tok.token ) )
			break;			// truncated quadruple

		s = (scheme_t*)malloc( sizeof( scheme_t ) );
		memset( s, 0, sizeof( scheme_t ) );
		strcpy( s->name, name );
		s->color  = RGB( r, g, b );
		s->next   = s_pScheme;
		s_pScheme = s;
	}

	free( file );
}
