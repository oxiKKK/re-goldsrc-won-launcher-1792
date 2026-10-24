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
// Purpose: the mod descriptor key/value info table, the scanned mod list, and
//          CODModListCtrl's row drawing.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"


#undef DrawText

// Sort fields for ModList_Compare.
#define SORT_TYPE		1
#define SORT_GAME		2
#define SORT_VERSION	3
#define SORT_SIZE		4
#define SORT_RATING		5
#define SORT_INSTALLED	6
#define SORT_SERVERS	7
#define SORT_PLAYERS	8

// The empty string GetKeyString hands back (0x4D04C0): a slot, not a literal,
// so a caller that writes through the result does not fault on .rdata.
static char*	s_pszEmptyString = "";

/*
==================
mod_t::Init (0x4293c0)
==================
*/
mod_t* mod_s::Init()
{
	mod_t*	mod = this;

	mod->keys = 0;

	mod->SetKey( "type", "" );
	mod->SetKey( "installed", "0" );
	mod->SetKey( "newversion", "0" );
	mod->SetKey( "svonly", "1" );
	mod->SetKey( "cldll", "0" );
	mod->SetKey( "gamedir", "" );
	mod->SetKey( "game", "" );
	mod->SetKey( "url_dl", "" );
	mod->SetKey( "url_info", "" );
	mod->SetKey( "date", "" );
	mod->SetKey( "startmap", "c0a0" );
	mod->SetKey( "trainmap", "t0a0" );
	mod->SetKey( "mpentity", "info_player_deathmatch" );
	mod->SetKey( "version", "0" );
	mod->SetKey( "size", "0" );
	mod->SetKey( "rating", "0" );
	mod->SetKey( "uniqueid", "0" );
	mod->SetKey( "requests", "0" );

	mod->next = 0;
	return mod;
}

/*
==================
mod_t::FreeKeys (0x429510)
==================
*/
void mod_s::FreeKeys()
{
	modkey_t*	kv = keys;

	while ( kv )
	{
		modkey_t* next = kv->next;
		free( kv->key );
		free( kv->value );
		free( kv );
		kv = next;
	}
	keys = 0;
}

/*
==================
ModList_FindByGamedir (0x429550)
==================
*/
mod_t* ModList_FindByGamedir( mod_t** ppList, const char* name )
{
	mod_t* mod;

	if ( !ppList )
		return 0;

	mod = *ppList;
	if ( !mod )
		return 0;

	while ( 1 )
	{
		char* gamedir = mod->GetKey( "gamedir" );
		if ( gamedir && !_stricmp( gamedir, name ) )
			break;

		mod = mod->next;
		if ( !mod )
			return 0;
	}
	return mod;
}

/*
==================
mod_t::GetKeyInt (0x4295a0)
==================
*/
int mod_s::GetKeyInt( const char* key )
{
	modkey_t*	kv = keys;

	if ( !kv )
		return 0;

	while ( _stricmp( kv->key, key ) )
	{
		kv = kv->next;
		if ( !kv )
			return 0;
	}
	return atoi( kv->value );
}

/*
==================
mod_t::GetKeyString (0x4295e0)

The empty string comes back through a slot, not a literal, so a caller that
writes through the result does not fault on .rdata.
==================
*/
char* mod_s::GetKeyString( const char* key )
{
	char*	value = GetKey( key );

	if ( !value )
		return s_pszEmptyString;
	return value;
}

/*
==================
mod_t::GetKey (0x429600)
==================
*/
char* mod_s::GetKey( const char* key )
{
	modkey_t*	kv = keys;

	if ( !kv )
		return 0;

	while ( _stricmp( kv->key, key ) )
	{
		kv = kv->next;
		if ( !kv )
			return 0;
	}
	return kv->value;
}

mod_t*	g_pModList;			// head of the list
mod_t*	g_pCurrentMod;		// the selected mod (-game)
mod_t*	g_pValveMod;		// the base "valve" mod

/*
==================
mod_t::SetKey (0x429640)
==================
*/
char* mod_s::SetKey( const char* key, const char* value )
{
	modkey_t*	kv = keys;
	modkey_t*	oldhead;

	if ( kv )
	{
		while ( _stricmp( kv->key, key ) )
		{
			kv = kv->next;
			if ( !kv )
				goto add;
		}
		free( kv->value );
		kv->value = _strdup( value );
		return kv->value;
	}

add:
	kv = (modkey_t*)malloc( sizeof( modkey_t ) );
	kv->key = 0;
	kv->value = 0;
	kv->next = 0;
	kv->key = _strdup( key );
	kv->value = _strdup( value );
	oldhead = keys;
	kv->next = oldhead;
	keys = kv;
	return (char*)oldhead;	// binary returns the previous list head
}

/*
==================
ModList_Clear (0x4296d0)
==================
*/
void ModList_Clear( int keepInstalled )
{
	mod_t*	mod = g_pModList;
	mod_t*	kept = 0;

	if ( !mod )
	{
		g_pModList = 0;
		return;
	}

	do
	{
		mod_t* next = mod->next;

		if ( keepInstalled && mod->GetKeyInt( "installed" ) )
		{
			mod->next = kept;
			kept = mod;
		}
		else
		{
			if ( mod == g_pCurrentMod )
				g_pCurrentMod = 0;
			if ( mod == g_pValveMod )
				g_pValveMod = 0;
			if ( mod )
			{
				mod->FreeKeys();
				delete mod;
			}
		}
		mod = next;
	}
	while ( mod );

	g_pModList = kept;
}

/*
==================
Mod_ParseLiblist (0x429760)
==================
*/
mod_t* Mod_ParseLiblist( char* gamedir, char* filename )
{
	FILE*		fp;
	long		size;
	char*		text;
	mod_t*		mod = 0;
	int			hasGamedll = 0;
	char		key[64];
	char		value[256];

	fp = fopen( filename, "rt" );
	if ( !fp )
		return 0;

	fseek( fp, 0, SEEK_END );
	size = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	text = new char[size + 1];
	if ( !text )
		return 0;

	memset( text, 0, size + 1 );
	fread( text, size, 1, fp );
	text[size] = 0;
	fclose( fp );

	CToken	tok( text );
	tok.SetQuoteMode( 1 );
	tok.SetCommentMode( 1 );

	mod = new mod_t;
	if ( mod )
		mod->Init();

	while ( 1 )
	{
		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			break;
		strcpy( key, tok.token );

		tok.ParseNextToken();
		strcpy( value, tok.token );

		if ( !_stricmp( key, "gamedll" ) )
		{
			hasGamedll = 1;
			mod->SetKey( "installed", "1" );
		}
		mod->SetKey( key, value );
	}

	delete[] text;

	if ( hasGamedll )
	{
		mod->SetKey( "gamedir", gamedir );
		return mod;
	}

	if ( mod )
	{
		mod->FreeKeys();
		delete mod;
	}
	return 0;
}

/*
==================
ModList_Scan (0x429980)
==================
*/
char* ModList_Scan( void )
{
	WIN32_FIND_DATAA	fd;
	WIN32_FIND_DATAA	fd2;
	char				pattern[260];
	char				path[260];
	char*				gameParam;
	char*				result;
	HANDLE				h, h2;

	sprintf( pattern, "*.*" );
	memset( &fd, 0, sizeof( fd ) );
	h = FindFirstFileA( pattern, &fd );
	if ( h != INVALID_HANDLE_VALUE )
	{
		do
		{
			if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				if ( _strnicmp( fd.cFileName, ".", 1 ) )
				{
					sprintf( path, "%s\\liblist.gam", fd.cFileName );
					memset( &fd2, 0, sizeof( fd2 ) );
					h2 = FindFirstFileA( path, &fd2 );
					if ( h2 != INVALID_HANDLE_VALUE )
					{
						mod_t* mod;
						_strupr( fd.cFileName );
						mod = Mod_ParseLiblist( fd.cFileName, path );
						if ( mod )
						{
							mod->next = g_pModList;
							g_pModList = mod;
							if ( !_stricmp( fd.cFileName, "VALVE" ) )
								g_pValveMod = mod;
						}
						FindClose( h2 );
					}
				}
			}
		}
		while ( FindNextFileA( h, &fd ) );
		FindClose( h );
	}

	result = CheckParm( "-game", &gameParam );
	if ( result )
	{
		result = gameParam;
		if ( gameParam && *gameParam )
		{
			mod_t* found = ModList_FindByGamedir( &g_pModList, gameParam );
			result = (char*)found;
			if ( found )
			{
				char* gd;
				g_pCurrentMod = found;
				gd = found->GetKey( "gamedir" );
				return (char*)sprintf( com_gamedir, gd );	// binary uses the value as the format
			}
		}
	}
	return result;
}

/*
==================
Launcher_SavePlayerInfo (0x429b10)
==================
*/
void Launcher_SavePlayerInfo( void )
{
	Launcher_SavePlayerInfoTo( "Player", &g_pServerBrowser->m_playerConfig );
}

/*
==================
Launcher_OnGameDirChanged (0x429b30)
==================
*/
void Launcher_OnGameDirChanged( void )
{
	Launcher_LoadScheme();
	Launcher_LoadStrings();
	Launcher_LoadSplashBitmap();
	Launcher_FreeMainButtonsBitmap();
	Launcher_LoadMainButtonsBitmap();
	Launcher_LoadPlayerInfo( "Player", &g_pServerBrowser->m_playerConfig );

	gCryptParms.pszBaseDir = com_gamedir;
#ifndef LAUNCHER_RE
	crypt.Initialize( &gCryptParms );
#endif
}

/*
==================
ModList_Compare (0x429b80)
==================
*/
int ModList_Compare( mod_t* a, mod_t* b, int field, int ascending )
{
	mod_t*	lhs;
	mod_t*	rhs;
	char*	va;
	char*	vb;
	int		numeric = 0;
	int		isInstalled = 0;

	if ( ascending <= 0 )
	{
		lhs = b;
		rhs = a;
	}
	else
	{
		lhs = a;
		rhs = b;
	}

	switch ( field )
	{
	case SORT_TYPE:
		va = lhs->GetKey( "type" );
		vb = rhs->GetKey( "type" );
		break;
	case SORT_GAME:
		va = lhs->GetKey( "game" );
		vb = rhs->GetKey( "game" );
		break;
	case SORT_VERSION:
		numeric = 1;
		va = lhs->GetKey( "version" );
		vb = rhs->GetKey( "version" );
		break;
	case SORT_SIZE:
		numeric = 1;
		va = lhs->GetKey( "size" );
		vb = rhs->GetKey( "size" );
		break;
	case SORT_RATING:
		numeric = 1;
		va = lhs->GetKey( "rating" );
		vb = rhs->GetKey( "rating" );
		break;
	case SORT_INSTALLED:
		numeric = 1;
		isInstalled = 1;
		va = lhs->GetKey( "installed" );
		vb = rhs->GetKey( "installed" );
		break;
	case SORT_SERVERS:
		numeric = 1;
		va = lhs->GetKey( "servers" );
		vb = rhs->GetKey( "servers" );
		break;
	case SORT_PLAYERS:
		numeric = 1;
		va = lhs->GetKey( "players" );
		vb = rhs->GetKey( "players" );
		break;
	default:
		return 0;
	}

	if ( !va )
		return -( vb != 0 );
	if ( !vb )
		return 1;

	if ( numeric )
	{
		if ( atof( vb ) > atof( va ) )
			return -1;
		return atof( vb ) < atof( va );
	}

	if ( !isInstalled )
		return _stricmp( va, vb );

	// "installed" matches tie-break on the available new version.
	if ( !_stricmp( va, vb ) )
	{
		char* na = lhs->GetKey( "newversion" );
		char* nb = rhs->GetKey( "newversion" );

		if ( !na )
			return -( nb != 0 );
		if ( !nb )
			return 1;
		if ( atof( nb ) > atof( na ) )
			return -1;
		return atof( nb ) < atof( na );
	}
	return _stricmp( va, vb );
}

/*
==================
ModList_CompareKeys (0x429df0)
==================
*/
int __stdcall ModList_CompareKeys( mod_t* a, mod_t* b, int )
{
	if ( !a || !b )
		return 0;

	for ( int i = 0; ; i++ )
	{
		int	key = KeyList_GetKey( i );
		if ( !key )
			break;

		int	ascending = key <= 0 ? -1 : 1;
		int	result = ModList_Compare( a, b, key * ascending, ascending );
		if ( result )
			return result;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CODModListCtrl::DrawRow (0x429E40)
//
// Two lines per row: the eight sortable columns across the top half, then the
// mod's info URL across the bottom.
void CODModListCtrl::DrawRow( CDC* pDC, int iRow )
{
	CRect	client;
	GetClientRect( &client );
	if ( m_bHasScrollbar )
		client.right -= 16;		// (sic) the scrollbar width is hard-coded

	int	vis = iRow - m_topRow;
	if ( vis < 0 || !m_rows )
		return;
	odrow_t*	pRow = m_rows[iRow];
	if ( !pRow || !pRow->record )
		return;

	mod_t*	pMod = (mod_t*)pRow->record;	// the row record is the mod itself
	int		bSel = ( pRow->flags & 1 );

	CRect	row;
	row.left = 0;
	row.top = vis * m_rowHeight;
	row.bottom = row.top + m_rowHeight - 1;
	row.right = client.right - client.left;

	pDC->SetBkColor( m_clrRowBg );
	if ( !m_bTransparent )
		pDC->FillRect( &row, bSel ? &m_brHighlight : &m_brBg );
	if ( bSel )
	{
		pDC->SetTextColor( m_clrSelText );
		if ( m_bTransparent )
		{
			// merge the highlight over the showing parent background
			CDC		mem;
			if ( mem.CreateCompatibleDC( pDC ) )
			{
				CBitmap	bmp;
				bmp.Attach( ::CreateCompatibleBitmap( pDC->GetSafeHdc(),
					row.Width(), row.Height() ) );
				CBitmap*	pOld = mem.SelectObject( &bmp );
				CRect	full( 0, 0, row.Width(), row.Height() );
				CBrush	hl( RGB( 80, 56, 24 ) );
				mem.FillRect( &full, &hl );
				pDC->BitBlt( row.left, row.top, row.Width(), row.Height(),
					&mem, 0, 0, SRCPAINT );
				mem.SelectObject( pOld );
			}
		}
	}
	else
	{
		pDC->SetTextColor( m_clrRowText );
	}

	pDC->SetBkMode( TRANSPARENT );
	CFont*	pOldFont = pDC->SelectObject( &m_headerFont );
	pDC->SetBkColor( m_clrRowBg );

	// top line: the eight columns
	char	buf[124];
	CRect	cell;
	int		x = 0;
	int		topLine = row.top;
	int		halfBot = row.top + m_rowHeight / 2 - 1;

	// The column bound is ours: the binary open-codes eight blocks and would
	// walk past a short m_cols.
	for ( int c = 0; c < 8 && c < m_nCols; c++ )
	{
		int	width = m_cols[c].width;
		const char*	val;

		switch ( c )
		{
		case 0:		// type
			val = pMod->GetKey( "type" );
			sprintf( buf, "%s", val ? val : "" );
			break;
		case 1:		// title
			val = pMod->GetKey( "game" );
			sprintf( buf, "%s", val ? val : "" );
			break;
		case 2:		// version
			val = pMod->GetKey( "version" );
			sprintf( buf, "%s", val ? val : "0" );
			break;
		case 3:		// size in MB
			// The mod "size" key is a raw byte count; show it in MiB.
			val = pMod->GetKey( "size" );
			sprintf( buf, "%.1fmb", atof( val ? val : "0" ) / ( 1024.0 * 1024.0 ) );
			break;
		case 4:		// rating
			val = pMod->GetKey( "rating" );
			sprintf( buf, "%.1f", atof( val ? val : "0.0" ) );
			break;
		case 5:		// install status, with its own colour
			val = pMod->GetKey( "installed" );
			if ( atoi( val ? val : "0" ) )
			{
				val = pMod->GetKey( "newversion" );
				if ( atoi( val ? val : "0" ) )
				{
					sprintf( buf, Launcher_LoadString( IDS_UPDATE ) );	// update available
					pDC->SetTextColor( RGB( 255, 63, 0 ) );	// orange "update" highlight
				}
				else
				{
					sprintf( buf, Launcher_LoadString( IDS_YES ) );	// installed
					pDC->SetTextColor( RGB( 112, 180, 20 ) );
				}
			}
			else
			{
				sprintf( buf, Launcher_LoadString( IDS_NO ) );		// not installed
				pDC->SetTextColor( RGB( 127, 127, 127 ) );
			}
			break;
		case 6:		// servers
			val = pMod->GetKey( "servers" );
			sprintf( buf, "%i", atoi( val ? val : "0" ) );
			break;
		case 7:		// players
			val = pMod->GetKey( "players" );
			sprintf( buf, "%i", atoi( val ? val : "0" ) );
			break;
		}

		cell.SetRect( x + 2, topLine, x + width, halfBot );
		const char*	fit = CODList_EllipsizeText( pDC, buf, width, 2 );
		pDC->DrawText( fit, -1, &cell, DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX );
		x += width;
	}

	// bottom line: the website
	pDC->SetTextColor( RGB( 200, 200, 200 ) );
	const char*	url = pMod->GetKey( "url_info" );
	sprintf( buf, "%s  %s", Launcher_LoadString( IDS_MOD_INFO ), url ? url : "" );
	cell.SetRect( 2, row.top + m_rowHeight / 2, row.right, row.bottom );
	const char*	fit = CODList_EllipsizeText( pDC, buf, row.Width(), 2 );
	pDC->DrawText( fit, -1, &cell, DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX );

	pDC->SelectObject( pOldFont );
}

/////////////////////////////////////////////////////////////////////////////
// CODModListCtrl::OnLButtonDown (0x42A7A0)
//
// A click in the header re-sorts; clicking the live column again flips it.
void CODModListCtrl::OnLButtonDown( UINT nFlags, CPoint point )
{
	RowFromPoint( &point );

	if ( m_bHeaderVisible )
	{
		int	col = ColumnFromPoint( &point );
		if ( col != -1 )
		{
			col++;						// keys are 1-based column numbers

			int	key = KeyList_GetKey( 0 );
			if ( !key )
				key = 1;

			if ( abs( key ) == col )
				key = -key;				// same column again -- flip the direction
			else
				key = col;

			KeyList_Set( key );
			SortRows( (odrowcmp_t)ModList_CompareKeys, -1 );
			return;
		}
	}

	CODListCtrl::OnLButtonDown( nFlags, point );
}

// Entries at 0x4AF9E8, base map 0x4B1B40 = CODListCtrl.
BEGIN_MESSAGE_MAP( CODModListCtrl, CODListCtrl )
	//{{AFX_MSG_MAP(CODModListCtrl)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
