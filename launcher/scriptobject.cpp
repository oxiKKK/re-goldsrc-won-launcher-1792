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
// Purpose: CScriptObject and CDescription, the settings.scr model.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

#define SCRIPT_VERSION	1.0f

// 0x4D1990 -- four 36-byte records.
static objtypedesc_t	objtypes[] =
{
	{ O_BOOL,   "BOOL" },
	{ O_NUMBER, "NUMBER" },
	{ O_LIST,   "LIST" },
	{ O_STRING, "STRING" },
};

static void UTIL_StripInvalidCharacters( CString* pStr );

/*
==================
CScriptListItem::CScriptListItem (0x45F410)
==================
*/
CScriptListItem::CScriptListItem( const CString& strItem, const CString& strValue )
{
	pNext = NULL;
	memset( szItemText, 0, sizeof( szItemText ) );
	memset( szValue, 0, sizeof( szValue ) );
	strcpy( szItemText, strItem );
	strcpy( szValue, strValue );
}

/*
==================
CScriptObject::CScriptObject (0x45F490)
==================
*/
CScriptObject::CScriptObject()
{
	type       = O_BOOL;
	pListItems = NULL;
	bSetInfo   = 0;
	pNext      = NULL;
}

/*
==================
CScriptObject::~CScriptObject (0x45F4D0)
==================
*/
CScriptObject::~CScriptObject()
{
	CScriptListItem*	p = pListItems;
	CScriptListItem*	pNextItem;

	while ( p )
	{
		pNextItem = p->pNext;
		delete p;
		p = pNextItem;
	}
	pListItems = NULL;
}

/*
==================
CScriptObject::AddItem (0x45F570)
==================
*/
void CScriptObject::AddItem( CScriptListItem* pItem )
{
	CScriptListItem*	p;

	if ( !pListItems )
	{
		pListItems = pItem;
		pItem->pNext = NULL;
		return;
	}

	p = pListItems;
	while ( p->pNext )
		p = p->pNext;
	p->pNext = pItem;
	pItem->pNext = NULL;
}

/*
==================
UTIL_StripInvalidCharacters (0x45F5B0)

Pulls the text out of a CString, strips the characters the settings.scr and
token writers cannot quote, and stores it back.
==================
*/
static void UTIL_StripInvalidCharacters( CString* pStr )
{
	char	szBuf[4096];

	strcpy( szBuf, *pStr );
	Sys_StripQuotesAndPercents( szBuf );
	*pStr = szBuf;
}

/*
==================
CScriptObject::WriteToScriptFile (0x45F610)

One full option block for settings.scr.  Each type prints its own prompt line
and its own default block, so the shapes differ per case.
==================
*/
void CScriptObject::WriteToScriptFile( FILE* fp )
{
	CScriptListItem*	pItem;

	UTIL_StripInvalidCharacters( &cvarname );
	fprintf( fp, "\t\"%s\"\r\n", (LPCSTR)cvarname );
	fprintf( fp, "\t{\r\n" );

	UTIL_StripInvalidCharacters( &prompt );

	switch ( type )
	{
	case O_BOOL:
		fprintf( fp, "\t\t\"%s\"\r\n", (LPCSTR)prompt );
		fprintf( fp, "\t\t{ BOOL }\r\n" );
		fprintf( fp, "\t\t{ \"%i\" }\r\n", (int)fcurValue != 0 );
		break;

	case O_NUMBER:
		fprintf( fp, "\t\t\"%s\"\r\n", (LPCSTR)prompt );
		fprintf( fp, "\t\t{ NUMBER %f %f }\r\n", fMin, fMax );
		fprintf( fp, "\t\t{ \"%f\" }\r\n", fcurValue );
		break;

	case O_LIST:
		fprintf( fp, "\t\t\"%s\"\r\n", (LPCSTR)prompt );
		fprintf( fp, "\t\t{\r\n\t\t\tLIST\r\n" );
		for ( pItem = pListItems; pItem; pItem = pItem->pNext )
		{
			Sys_StripQuotesAndPercents( pItem->szItemText );
			Sys_StripQuotesAndPercents( pItem->szValue );
			fprintf( fp, "\t\t\t\"%s\" \"%s\"\r\n", pItem->szItemText, pItem->szValue );
		}
		fprintf( fp, "\t\t}\r\n" );
		fprintf( fp, "\t\t{ \"%f\" }\r\n", fcurValue );
		break;

	case O_STRING:
		fprintf( fp, "\t\t\"%s\"\r\n", (LPCSTR)prompt );
		fprintf( fp, "\t\t{ STRING }\r\n" );
		UTIL_StripInvalidCharacters( &curValue );
		fprintf( fp, "\t\t{ \"%s\" }\r\n", (LPCSTR)curValue );
		break;

	default:
		break;
	}

	if ( bSetInfo )
		fprintf( fp, "\t\tSetInfo\r\n" );

	fprintf( fp, "\t}\r\n\r\n" );
}

/*
==================
CScriptObject::WriteToFile (0x45F7D0)

One "cvar" "value" line for game.cfg.  The NUMBER clamp is inclusive on both
ends, so a value sitting exactly on a bound is rewritten to it.
==================
*/
void CScriptObject::WriteToFile( FILE* fp )
{
	CScriptListItem*	pItem;
	float				v;
	int					idx;
	int					n;

	UTIL_StripInvalidCharacters( &cvarname );
	fprintf( fp, "\"%s\"\t\t", (LPCSTR)cvarname );

	switch ( type )
	{
	case O_BOOL:
		fprintf( fp, "\"%s\"\r\n", fcurValue != 0.0f ? "1" : "0" );
		break;

	case O_NUMBER:
		v = fcurValue;
		if ( fMin != -1.0f && v <= fMin )
			v = fMin;
		if ( fMax != -1.0f && v >= fMax )
			v = fMax;
		fprintf( fp, "\"%f\"\r\n", v );
		break;

	case O_LIST:
		pItem = pListItems;
		idx   = (int)fcurValue;
		n     = 0;
		if ( idx > 0 )
		{
			while ( pItem )
			{
				pItem = pItem->pNext;
				if ( ++n >= idx )
					break;
			}
		}
		if ( pItem )
		{
			Sys_StripQuotesAndPercents( pItem->szValue );
			fprintf( fp, "\"%s\"\r\n", pItem->szValue );
		}
		else
		{
			fprintf( fp, "\"0.0\"\r\n" );
		}
		break;

	case O_STRING:
		UTIL_StripInvalidCharacters( &curValue );
		fprintf( fp, "\"%s\"\r\n", (LPCSTR)curValue );
		break;

	default:
		break;
	}
}

/*
==================
CScriptObject::WriteToConfig (0x45F930)

Push this option's current value into the player token profile, tagged with
the SetInfo flag.
==================
*/
void CScriptObject::WriteToConfig()
{
	CScriptListItem*	pItem;
	char				szBuf[2048];
	float				v;
	int					idx;
	int					n;

	switch ( type )
	{
	case O_BOOL:
		sprintf( szBuf, "%s", fcurValue != 0.0f ? "1" : "0" );
		break;

	case O_NUMBER:
		v = fcurValue;
		if ( fMin != -1.0f && v <= fMin )
			v = fMin;
		if ( fMax != -1.0f && v >= fMax )
			v = fMax;
		sprintf( szBuf, "%f", v );
		break;

	case O_STRING:
		sprintf( szBuf, "%s", (LPCSTR)curValue );
		Sys_StripQuotesAndPercents( szBuf );
		break;

	case O_LIST:
		pItem = pListItems;
		idx   = (int)fcurValue;
		n     = 0;
		if ( idx > 0 )
		{
			while ( pItem )
			{
				pItem = pItem->pNext;
				if ( ++n >= idx )
					break;
			}
		}
		if ( pItem )
		{
			sprintf( szBuf, "%s", pItem->szValue );
			Sys_StripQuotesAndPercents( szBuf );
		}
		else
		{
			sprintf( szBuf, "0.0" );
		}
		break;

	default:
		break;
	}

	CFG_SetTokenProfile( g_szConfigName, cvarname, szBuf, bSetInfo );
}

/*
==================
CScriptObject::GetType (0x45FAA0)
==================
*/
objtype_t CScriptObject::GetType( char* pszType )
{
	int		i;
	int		nTypes = sizeof( objtypes ) / sizeof( objtypedesc_t );

	for ( i = 0; i < nTypes; i++ )
	{
		if ( !_stricmp( pszType, objtypes[i].szDescription ) )
			return objtypes[i].type;
	}

	return O_BADTYPE;
}

/*
==================
CScriptObject::ReadFromBuffer (0x45FAE0)

Parses <name> { "Prompt" { TYPE ... } { default } [SetInfo] } out of its own
CToken over *pBuffer, and on success hands the caller back the position it
stopped at.  Every step is guarded on a non-empty token, so a truncated file
fails rather than running off the end.
==================
*/
BOOL CScriptObject::ReadFromBuffer( char** pBuffer )
{
	CToken	tok( *pBuffer );

	tok.SetQuoteMode( TRUE );
	tok.SetCommentMode( TRUE );

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
		return FALSE;
	cvarname = tok.token;

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
		return FALSE;
	if ( strcmp( tok.token, "{" ) )
		goto EXPECTING_BRACE;

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
		return FALSE;
	prompt = tok.token;

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
		return FALSE;
	if ( strcmp( tok.token, "{" ) )
		goto EXPECTING_BRACE;

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
		return FALSE;

	type = GetType( tok.token );
	if ( type == O_BADTYPE )
	{
		Launcher_ErrorMessageBox( 0, "Type '%s' unknown", tok.token );
		return FALSE;
	}

	switch ( type )
	{
	case O_BOOL:
	case O_STRING:
		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			return FALSE;
		if ( strcmp( tok.token, "}" ) )
			goto EXPECTING_BRACE;
		break;

	case O_NUMBER:
		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			return FALSE;
		fMin = (float)atof( tok.token );

		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			return FALSE;
		fMax = (float)atof( tok.token );

		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			return FALSE;
		if ( strcmp( tok.token, "}" ) )
			goto EXPECTING_BRACE;
		break;

	case O_LIST:
		for ( ;; )
		{
			CString		strLabel;
			CString		strValue;

			tok.ParseNextToken();
			if ( !strlen( tok.token ) )
				return FALSE;
			if ( !strcmp( tok.token, "}" ) )
				break;

			strLabel = tok.token;
			tok.ParseNextToken();
			if ( !strlen( tok.token ) )
				return FALSE;
			strValue = tok.token;

			AddItem( new CScriptListItem( strLabel, strValue ) );
		}
		break;

	default:
		break;
	}

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
		return FALSE;
	if ( strcmp( tok.token, "{" ) )
		goto EXPECTING_BRACE;

	tok.ParseNextToken();
	defValue  = tok.token;
	fdefValue = (float)atof( tok.token );
	curValue  = defValue;
	fcurValue = (float)atof( curValue );

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
		return FALSE;
	if ( strcmp( tok.token, "}" ) )
		goto EXPECTING_BRACE;

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
		return FALSE;

	if ( !_strcmpi( tok.token, "SetInfo" ) )
	{
		bSetInfo = 1;
		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			return FALSE;
	}

	if ( strcmp( tok.token, "}" ) )
		goto EXPECTING_BRACE;

	*pBuffer = tok.GetData();
	return TRUE;

EXPECTING_BRACE:
	Launcher_ErrorMessageBox( 0, "Expecting '{', got '%s'", tok.token );
	return FALSE;
}

/*
==================
CDescription::CDescription (0x4601F0)
==================
*/
CDescription::CDescription()
{
	pObjList = NULL;
}

/*
==================
CDescription::~CDescription (0x460200)
==================
*/
CDescription::~CDescription()
{
	CScriptObject*	p = pObjList;
	CScriptObject*	pNextObj;

	// ~CScriptObject owns its own choice list -- freeing it here as well
	// double-frees every LIST node.
	while ( p )
	{
		pNextObj = p->pNext;
		delete p;
		p = pNextObj;
	}
	pObjList = NULL;

	free( m_pszHintText );
	free( m_pszDescriptionType );
}

/*
==================
CDescription::AddObject (0x460270)
==================
*/
void CDescription::AddObject( CScriptObject* pObj )
{
	CScriptObject*	p;

	if ( !pObjList )
	{
		pObjList = pObj;
		pObj->pNext = NULL;
		return;
	}

	p = pObjList;
	while ( p->pNext )
		p = p->pNext;
	p->pNext = pObj;
	pObj->pNext = NULL;
}

/*
==================
CDescription::ReadFromBuffer (0x4602B0)

VERSION <n> DESCRIPTION <name> { <object>... }.  The description name has to
match the one this object was built for, so a settings.scr written for another
page is rejected rather than half-parsed.
==================
*/
BOOL CDescription::ReadFromBuffer( char** pBuffer )
{
	CToken			tok( *pBuffer );
	CScriptObject*	pObj;
	char*			pData;

	tok.SetQuoteMode( TRUE );
	tok.SetCommentMode( TRUE );

	tok.ParseNextToken();
	if ( _stricmp( tok.token, "VERSION" ) )
	{
		Launcher_ErrorMessageBox( 0, "Expecting 'VERSION', got '%s'", tok.token );
		return FALSE;
	}

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
	{
		Launcher_ErrorMessageBox( 0, "Expecting version #" );
		return FALSE;
	}
	if ( (float)atof( tok.token ) != SCRIPT_VERSION )
	{
		Launcher_ErrorMessageBox( 0, "Version mismatch, expecting %f, got %f",
			SCRIPT_VERSION, (float)atof( tok.token ) );
		return FALSE;
	}

	tok.ParseNextToken();
	if ( _stricmp( tok.token, "DESCRIPTION" ) )
	{
		Launcher_ErrorMessageBox( 0, "Expecting 'DESCRIPTION', got '%s'", tok.token );
		return FALSE;
	}

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
	{
		Launcher_ErrorMessageBox( 0, "Expecting '%s'", m_pszDescriptionType );
		return FALSE;
	}
	if ( _stricmp( tok.token, m_pszDescriptionType ) )
	{
		Launcher_ErrorMessageBox( 0, "Expecting %s, got %s", m_pszDescriptionType, tok.token );
		return FALSE;
	}

	tok.ParseNextToken();
	if ( strcmp( tok.token, "{" ) )
	{
		Launcher_ErrorMessageBox( 0, "Expecting '{', got '%s'", tok.token );
		return FALSE;
	}

	for ( ;; )
	{
		pData = tok.GetData();

		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			return FALSE;					// truncated
		if ( !strcmp( tok.token, "}" ) )
		{
			*pBuffer = tok.GetData();
			return TRUE;
		}

		// Hand the whole block, name included, to the object itself.
		tok.SetData( pData );

		pObj = new CScriptObject;
		if ( !pObj )
		{
			Launcher_ErrorMessageBox( 0, "Couldn't create script object" );
			return FALSE;
		}

		if ( !pObj->ReadFromBuffer( &pData ) )
		{
			delete pObj;
			return FALSE;
		}
		tok.SetData( pData );

		AddObject( pObj );
	}
}

/*
==================
CDescription::InitFromFile (0x4605F0)

settings.scr lives in the gamedir, so it has to come through the search path,
not a bare fopen against the cwd.
==================
*/
BOOL CDescription::InitFromFile( const char* pszFile )
{
	FILE*	fp = NULL;
	char*	pBuf;
	char*	p;

	if ( COM_FindFile( pszFile, NULL, &fp ) == -1 )
		return FALSE;
	if ( fp )
		fclose( fp );

	pBuf = (char*)COM_LoadMallocFile( pszFile );
	if ( !pBuf )
		return FALSE;

	p = pBuf;
	ReadFromBuffer( &p );		// a parse error still counts as "found"
	free( pBuf );
	return TRUE;
}

/*
==================
CDescription::WriteToFile (0x460660)

game.cfg is exec'd as console commands, so it gets the comment banner only --
vtable slot 1.
==================
*/
int CDescription::WriteToFile( FILE* fp )
{
	CScriptObject*	p;

	WriteFileHeader( fp );
	for ( p = pObjList; p; p = p->pNext )
		p->WriteToFile( fp );
	return 0;
}

/*
==================
CDescription::WriteToConfig (0x460690)
==================
*/
void CDescription::WriteToConfig()
{
	CScriptObject*	p;

	for ( p = pObjList; p; p = p->pNext )
		p->WriteToConfig();
}

/*
==================
CDescription::WriteToScriptFile (0x4606B0)

Slot 0 opens VERSION / DESCRIPTION { -- without it the file this writes cannot
be parsed back.
==================
*/
int CDescription::WriteToScriptFile( FILE* fp )
{
	CScriptObject*	p;

	WriteScriptHeader( fp );
	for ( p = pObjList; p; p = p->pNext )
		p->WriteToScriptFile( fp );
	return fprintf( fp, "}\r\n" );
}

/*
==================
CDescription::TransferCurrentValues (0x4606F0)
==================
*/
void CDescription::TransferCurrentValues( const char* pszName )
{
	CScriptObject*	p;
	char			szValue[1024];

	for ( p = pObjList; p; p = p->pNext )
	{
		if ( CFG_ReadCvar( pszName, p->cvarname, szValue ) )
		{
			p->curValue   = szValue;
			p->fcurValue  = (float)atof( szValue );
			p->defValue   = szValue;
			p->fdefValue  = (float)atof( szValue );
		}
	}
}
