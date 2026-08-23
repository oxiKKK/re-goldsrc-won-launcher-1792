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
// Purpose: CGameConfig player / game userinfo persistence.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The named CGameClientConfig layout must stay byte-exact to the binary's block.
typedef char _chk_cfg_size[ ( sizeof( CGameClientConfig ) == CFG_BLOCK_SIZE ) ? 1 : -1 ];

// Map a cvar key to its mirror float field in the player CGameClientConfig
// block.
static float* CfgFloatField( CGameClientConfig* pCfg, const char* pszKey )
{
	if ( !_strcmpi( pszKey, "lookstrafe" ) )			return &pCfg->lookstrafe;
	if ( !_strcmpi( pszKey, "lookspring" ) )		return &pCfg->lookspring;
	if ( !_strcmpi( pszKey, "m_pitch" ) )			return &pCfg->m_pitch;
	if ( !_strcmpi( pszKey, "crosshair" ) )			return &pCfg->crosshair;
	if ( !_strcmpi( pszKey, "_windowed_mouse" ) )	return &pCfg->_windowed_mouse;
	if ( !_strcmpi( pszKey, "m_filter" ) )			return &pCfg->m_filter;
	if ( !_strcmpi( pszKey, "joystick" ) )			return &pCfg->joystick;
	if ( !_strcmpi( pszKey, "sv_aim" ) )			return &pCfg->sv_aim;
	if ( !_strcmpi( pszKey, "console" ) )			return &pCfg->console;
	if ( !_strcmpi( pszKey, "cl_himodels" ) )		return &pCfg->cl_himodels;
	if ( !_strcmpi( pszKey, "d_spriteskip" ) )		return &pCfg->d_spriteskip;
	if ( !_strcmpi( pszKey, "sensitivity" ) )		return &pCfg->sensitivity;
	if ( !_strcmpi( pszKey, "viewsize" ) )			return &pCfg->viewsize;
	if ( !_strcmpi( pszKey, "brightness" ) )		return &pCfg->brightness;
	if ( !_strcmpi( pszKey, "gamma" ) )				return &pCfg->gamma;
	if ( !_strcmpi( pszKey, "bgmvolume" ) )			return &pCfg->bgmvolume;
	if ( !_strcmpi( pszKey, "suitvolume" ) )		return &pCfg->suitvolume;
	if ( !_strcmpi( pszKey, "hisound" ) )			return &pCfg->hisound;
	if ( !_strcmpi( pszKey, "volume" ) )			return &pCfg->volume;
	if ( !_strcmpi( pszKey, "voice_scale" ) )		return &pCfg->voice_scale;
	if ( !_strcmpi( pszKey, "voice_modenable" ) )	return &pCfg->voice_modenable;
	if ( !_strcmpi( pszKey, "s_a3d" ) )				return &pCfg->s_a3d;
	if ( !_strcmpi( pszKey, "s_eax" ) )				return &pCfg->s_eax;
	if ( !_strcmpi( pszKey, "rate" ) )				return &pCfg->rate;
	return NULL;
}

// CFG_FreeBindings (0x4582B0)
void CFG_FreeBindings( cfg_keybind_t* pBlock )
{
	if ( !pBlock )
		return;

	cfg_keybind_t*	p = pBlock;
	int				i = CFG_BIND_COUNT;
	do
	{
		if ( p->m_pszBind )
		{
			free( p->m_pszBind );
			p->m_pszBind = NULL;
		}
		memset( p, 0, sizeof( cfg_keybind_t ) );
		p++;
	}
	while ( --i );
}

// CFG_CopyConfig (0x4582F0)
void CFG_CopyConfig( cfg_keybind_t* pDst, const cfg_keybind_t* pSrc )
{
	CFG_FreeBindings( pDst );							// release dst's old bindings
	memcpy( pDst, pSrc, CFG_BLOCK_SIZE );				// copy everything (incl. cvars)
	memset( pDst, 0, CFG_BIND_COUNT * sizeof( cfg_keybind_t ) );	// clear the bind records

	for ( int i = 0; i < CFG_BIND_COUNT; i++ )
	{
		strcpy( pDst[i].m_szKey, pSrc[i].m_szKey );
		pDst[i].m_nState = pSrc[i].m_nState;
		if ( pSrc[i].m_pszBind )
			pDst[i].m_pszBind = _strdup( pSrc[i].m_pszBind );
	}
}

// 0x4F8818  the token scratch the config tokenizer parses into.
static char	g_szToken[256];
// "" (0x4D9740) is the shared "" sentinel; declared in strings.h, defined
// in strings.cpp.

// 0x4F6818 the keybind-name lookup table.
static char	g_szBindNames[256][32];

// CFG_FindKeyName (0x458390)
int CFG_FindKeyName( const char* pszName, const char* pszTable )
{
	if ( !pszName )
		return -1;

	int	idx = 0;
	while ( !pszTable || !*pszTable || _strcmpi( pszTable, pszName ) )
	{
		idx++;
		pszTable += 32;
		if ( idx >= 256 )
			return -1;
	}
	return idx;
}

// Keys_LoadNameTable (0x4583E0)
int Keys_LoadNameTable( char* pDisplayRecs )
{
	char		szName[256];

	memset( g_szBindNames, 0, sizeof( g_szBindNames ) );
	CToken	tok( NULL );

	char	szPath[64];
	strcpy( szPath, "gfx/shell/kb_keys.lst" );
	char*	pFile = (char*)COM_LoadMallocFile( szPath );
	if ( !pFile )
	{
		Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_EMPTY );			// kb_keys.lst missing
		PostQuitMessage( 0 );
		return 0;
	}

	tok.SetData( pFile );
	tok.SetQuoteMode( 1 );

	for ( char* pKey = g_szBindNames[0]; ; pKey += 32 )
	{
		tok.ParseNextToken();									// keyname
		if ( !strlen( tok.token ) )
			break;
		if ( pKey >= (char*)g_szBindNames + sizeof( g_szBindNames ) )	// table full (0x4F8818)
		{
			Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_OVERFLOW );
			break;
		}

		tok.ParseNextToken();									// display name
		if ( !strlen( tok.token ) )
		{
			Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_PARSEERROR );
			break;
		}
		strcpy( szName, tok.token );
		if ( _strnicmp( szName, "<UNK", 4 ) )		// skip "<UNKNOWN>" placeholders
		{
			strcpy( pKey, szName );
			strcpy( pDisplayRecs, szName );
		}

		tok.ParseNextToken();									// x
		if ( !strlen( tok.token ) || ( tok.ParseNextToken(), !strlen( tok.token ) ) )	// y
		{
			Launcher_ShowMessageById( 0, IDS_CONTROLS_KBKEYS_PARSEERROR );
			break;
		}

		if ( !_strnicmp( tok.token, "COLOR", 5 ) )	// optional COLOR r g b (parsed, unused in this build)
		{
			tok.ParseNextToken();	atoi( tok.token );
			tok.ParseNextToken();	atoi( tok.token );
			tok.ParseNextToken();	atoi( tok.token );
		}
		pDisplayRecs += 40;
	}

	free( pFile );
	return 1;
}

// The config file name the player-config engine loads/rewrites (config.cfg in
// the active gamedir) (0x4E2EB4)
char	g_szConfigName[256];	// 0x4E2EB4

// PlayerConfig_ApplyDefaults (0x458680)
int PlayerConfig_ApplyDefaults( char* pDisplayRecs )
{
	char		szKey[256];
	char		szCmd[256];

	CToken	tok( NULL );
	Keys_LoadNameTable( pDisplayRecs );

	char	szPath[64];
	strcpy( szPath, "gfx/shell/kb_def.lst" );
	char*	pFile = (char*)COM_LoadMallocFile( szPath );
	if ( !pFile )
	{
		Launcher_ShowMessageByIdEx( 0, IDS_PROFILE_DEFAULTMISSING, szPath );		// kb_def.lst missing
		return 0;
	}

	tok.SetData( pFile );
	tok.SetQuoteMode( 1 );

	while ( TRUE )
	{
		tok.ParseNextToken();									// keyname
		if ( !strlen( tok.token ) )
			break;
		strcpy( szKey, tok.token );

		tok.ParseNextToken();									// command
		if ( !strlen( tok.token ) )
			break;
		strcpy( szCmd, tok.token );

		int	idx = CFG_FindKeyName( szKey, g_szBindNames[0] );
		if ( idx != -1 )
		{
			int		n   = strlen( szCmd ) + 1;
			char*	pCmd = (char*)malloc( n );
			cfg_keybind_t*	pRec = (cfg_keybind_t*)( pDisplayRecs + 40 * idx );
			pRec->m_pszBind = pCmd;
			if ( !pCmd )
			{
				Launcher_ShowMessageById( 0, IDS_BINDINGS_ALLOCFAIL );
				return 0;
			}
			memset( pCmd, 0, n );
			strcpy( pRec->m_pszBind, szCmd );
			pRec->m_nState = n - 1;
		}
	}

	free( pFile );
	return 1;
}

// PlayerConfig_LoadDefaults (0x4588E0)
int PlayerConfig_LoadDefaults( char* pBlock )
{
	CGameClientConfig*	pCfg = (CGameClientConfig*)pBlock;

	Keys_LoadNameTable( pBlock );

	pCfg->m_pitch			= 0.022f;
	pCfg->lookstrafe		= 0.0f;
	pCfg->lookspring		= 0.0f;
	pCfg->crosshair			= 1.0f;
	pCfg->_windowed_mouse	= 0.0f;
	pCfg->m_filter			= 0.0f;
	pCfg->mlook				= 1.0f;
	pCfg->jlook				= 1.0f;
	pCfg->joystick			= 0.0f;
	pCfg->sv_aim			= 1.0f;
	pCfg->console			= 0.0f;
	if ( CheckParm( "-console", NULL ) || CheckParm( "-toconsole", NULL ) || CheckParm( "-dev", NULL ) )
		pCfg->console		= 1.0f;
	pCfg->brightness		= 1.0f;
	pCfg->bgmvolume			= 1.0f;
	pCfg->hisound			= 1.0f;
	pCfg->voice_modenable	= 1.0f;
	pCfg->cl_himodels		= 0.0f;
	pCfg->d_spriteskip		= 0.0f;
	pCfg->sensitivity		= 3.0f;
	pCfg->viewsize			= 120.0f;
	pCfg->gamma				= 2.5f;
	pCfg->suitvolume		= 0.25f;
	pCfg->volume			= 0.8f;
	pCfg->s_a3d				= 0.0f;
	pCfg->s_eax				= 0.0f;
	pCfg->rate				= 2500.0f;
	pCfg->voice_scale		= 0.75f;

	strcpy( pCfg->model, "gordon" );
	pCfg->topcolor    = 30;
	pCfg->bottomcolor = 6;
	strcpy( pCfg->name, "Player" );
	return 1;
}

// Launcher_LoadPlayerInfo (0x458A60)
int Launcher_LoadPlayerInfo( const char* pszSection, void* pData )
{
	char*	pBlock = (char*)pData;

	if ( !pszSection )
		return 0;
	if ( !pBlock )
		return 0;

	CFG_FreeBindings( (cfg_keybind_t*)pBlock );
	memset( pBlock, 0, CFG_BLOCK_SIZE );

	if ( !PlayerConfig_LoadDefaults( pBlock ) )
		return 0;

	PlayerConfig_LoadKeybinds( g_szConfigName, pBlock );
	PlayerConfig_LoadCvars( g_szConfigName, pBlock );
	return 1;
}

// Launcher_SavePlayerInfoTo (0x458AC0)
int Launcher_SavePlayerInfoTo( const char* pszSection, void* pData )
{
	char*	pBlock = (char*)pData;

	if ( !pszSection || !pBlock )
		return 0;

	g_bRestartPending = 1;
	Keys_LoadNameTable( pBlock );
	CFG_ApplyKeybinds( g_szConfigName, pBlock );
	CFG_ApplyCvars( g_szConfigName, pBlock );
	return 1;
}

// CFG_ParseToken (0x458B10)
char* CFG_ParseToken( char** ppData )
{
	int		n = 0;
	char*	p = *ppData;

	g_szToken[0] = 0;
	if ( !p )
	{
		*ppData = NULL;
		return (char*)"";
	}

	// skip whitespace + // line comments
	while ( TRUE )
	{
		char	c = *p;
		while ( c && c <= ' ' )			// skip whitespace
			c = *++p;
		if ( !c )
		{
			*ppData = NULL;
			return (char*)"";
		}
		if ( c == '/' && p[1] == '/' )	// // comment -> skip to newline
		{
			while ( *p && *p != '\n' )
				p++;
			continue;
		}
		break;
	}

	// pull the token
	if ( *p == '\"' )					// quoted
	{
		p++;
		while ( *p && *p != '\"' )
			g_szToken[n++] = *p++;
		if ( *p == '\"' )
			p++;
	}
	else								// bare word
	{
		while ( *p > ' ' )
			g_szToken[n++] = *p++;
	}
	g_szToken[n] = 0;
	*ppData = p;
	return g_szToken;
}

// CFG_FreeBindTable (0x458BB0)
void CFG_FreeBindTable( cfg_bind_t* pTable )
{
	int	i = 256;
	do
	{
		if ( pTable->m_pszKey )
			free( pTable->m_pszKey );
		if ( pTable->m_pszCmd )
			free( pTable->m_pszCmd );
		pTable->m_pszKey = NULL;
		pTable->m_pszCmd = NULL;
		pTable++;
	}
	while ( --i );
}

// CFG_KeyNameToIndex (0x458BF0)
int CFG_KeyNameToIndex( const char* pszKey )
{
	int	idx = 0;
	char*	p = g_szBindNames[0];
	while ( _strcmpi( p, pszKey ) )
	{
		p += 32;
		idx++;
		if ( p >= (char*)g_szBindNames + sizeof( g_szBindNames ) )	// past the last name (0x4F8818)
			return 0;
	}
	return idx;
}

// CFG_FreeCvarList (0x458C30)
void CFG_FreeCvarList( cfg_cvar_t** ppHead )
{
	cfg_cvar_t*	pNext;
	for ( cfg_cvar_t* p = *ppHead; p; p = pNext )
	{
		pNext = p->m_pNext;
		if ( p->m_pszName )
			free( p->m_pszName );
		if ( p->m_pszValue )
			free( p->m_pszValue );
		free( p );
	}
	*ppHead = NULL;
}

// CFG_FindCvar (0x458C80)
cfg_cvar_t* CFG_FindCvar( cfg_cvar_t* pHead, const char* pszName )
{
	cfg_cvar_t*	p = pHead;
	if ( !p )
		return NULL;

	while ( _strcmpi( p->m_pszName, pszName ) )
	{
		p = p->m_pNext;
		if ( !p )
			return NULL;
	}
	return p;
}

// CFG_SetCvar (0x458CB0)
cfg_cvar_t* CFG_SetCvar( cfg_cvar_t** ppHead, const char* pszName, const char* pszValue )
{
	cfg_cvar_t*	p = CFG_FindCvar( *ppHead, pszName );
	if ( p )
	{
		if ( _strcmpi( pszValue, p->m_pszValue ) )
		{
			free( p->m_pszValue );
			p->m_pszValue = _strdup( pszValue );
		}
		return p;
	}

	p = (cfg_cvar_t*)malloc( sizeof( cfg_cvar_t ) );
	p->m_pNext    = NULL;
	p->m_bSetInfo = 0;
	p->m_pszName  = NULL;
	p->m_pszValue = NULL;
	p->m_pszName  = _strdup( pszName );		// +8
	p->m_pszValue = _strdup( pszValue );	// +12
	p->m_pNext    = *ppHead;				// link at head
	*ppHead       = p;
	return p;
}

// CFG_FreeCmdList (0x458D40)
void CFG_FreeCmdList( cfg_cmd_t** ppHead )
{
	cfg_cmd_t*	pNext;
	for ( cfg_cmd_t* p = *ppHead; p; p = pNext )
	{
		pNext = p->m_pNext;
		if ( p->m_pszCmd )
			free( p->m_pszCmd );
		free( p );
	}
	*ppHead = NULL;
}

// CFG_UnlinkCmd (0x458D80)
void CFG_UnlinkCmd( cfg_cmd_t** ppHead, cfg_cmd_t* pNode )
{
	cfg_cmd_t**	pp = ppHead;
	while ( *pp )
	{
		if ( *pp == pNode )
		{
			*pp = pNode->m_pNext;
			if ( pNode->m_pszCmd )
				free( pNode->m_pszCmd );
			free( pNode );
			return;
		}
		pp = &( *pp )->m_pNext;
	}
}

// CFG_FindCmd (0x458DD0)
cfg_cmd_t* CFG_FindCmd( cfg_cmd_t* pHead, const char* pszCmd )
{
	for ( cfg_cmd_t* p = pHead; p; p = p->m_pNext )
	{
		if ( !_strcmpi( p->m_pszCmd, pszCmd ) )
			return p;
	}
	return NULL;
}

// CFG_AddCmd (0x458E00)
cfg_cmd_t* CFG_AddCmd( cfg_cmd_t** ppHead, const char* pszCmd )
{
	cfg_cmd_t*	p = CFG_FindCmd( *ppHead, pszCmd );
	if ( !p )
	{
		p = (cfg_cmd_t*)malloc( sizeof( cfg_cmd_t ) );
		p->m_pNext  = NULL;
		p->m_pszCmd = NULL;
		p->m_pszCmd = _strdup( pszCmd );
		p->m_pNext  = *ppHead;
		*ppHead     = p;
	}
	return p;
}

// CFG_ParseConfig (0x458E50)
void CFG_ParseConfig( char* pData, cfg_bind_t* pBindTable, cfg_cvar_t** ppCvars, cfg_cmd_t** ppCmds )
{
	while ( CFG_ParseToken( &pData ) )
	{
		if ( !strlen( g_szToken ) )
			break;

		if ( !_strcmpi( g_szToken, "unbindall" ) )
			continue;

		if ( !_strnicmp( g_szToken, "bind", 4 ) )
		{
			CFG_ParseToken( &pData );						// key
			int	idx = CFG_KeyNameToIndex( g_szToken );
			if ( pBindTable[idx].m_pszKey )
				free( pBindTable[idx].m_pszKey );
			pBindTable[idx].m_pszKey = _strdup( g_szToken );

			CFG_ParseToken( &pData );						// command
			if ( pBindTable[idx].m_pszCmd )
				free( pBindTable[idx].m_pszCmd );
			pBindTable[idx].m_pszCmd = _strdup( g_szToken );
		}
		else if ( g_szToken[0] == '+' )
		{
			cfg_cmd_t*	pCmd = (cfg_cmd_t*)malloc( sizeof( cfg_cmd_t ) );
			pCmd->m_pNext  = NULL;
			pCmd->m_pszCmd = NULL;
			pCmd->m_pszCmd = _strdup( g_szToken );
			pCmd->m_pNext  = *ppCmds;
			*ppCmds        = pCmd;
		}
		else
		{
			cfg_cvar_t*	pCvar = (cfg_cvar_t*)malloc( sizeof( cfg_cvar_t ) );
			pCvar->m_pNext    = NULL;
			pCvar->m_bSetInfo = 0;
			pCvar->m_pszName  = NULL;
			pCvar->m_pszValue = NULL;

			int	bSetInfo = 0;
			if ( !_strcmpi( g_szToken, "setinfo" ) )
			{
				bSetInfo = 1;
				CFG_ParseToken( &pData );					// the real name follows
			}
			pCvar->m_pszName = _strdup( g_szToken );
			CFG_ParseToken( &pData );						// value
			pCvar->m_pszValue = _strdup( g_szToken );
			pCvar->m_bSetInfo = bSetInfo;
			pCvar->m_pNext    = *ppCvars;
			*ppCvars          = pCvar;
		}
	}
}

// CFG_WriteConfig (0x458FE0)
int CFG_WriteConfig( const char* pszName, cfg_bind_t* pBindTable, cfg_cvar_t* pCvars, cfg_cmd_t* pCmds )
{
	char	szPath[260];
	sprintf( szPath, "%s/%s", com_gamedir, pszName );

	FILE*	fp = fopen( szPath, "w" );
	if ( !fp )
		return 0;

	fprintf( fp, "unbindall\n" );

	int	i = 256;
	cfg_bind_t*	pb = pBindTable;
	do
	{
		if ( pb->m_pszKey && pb->m_pszCmd && pb->m_pszCmd[0] )
		{
			if ( strlen( pb->m_pszKey ) == 1 && pb->m_pszKey[0] >= 'A' && pb->m_pszKey[0] <= 'Z' )
			{
				char	szKey[8];
				strcpy( szKey, pb->m_pszKey );
				_strlwr( szKey );
				fprintf( fp, "bind \"%s\" \"%s\"\n", szKey, pb->m_pszCmd );
			}
			else
			{
				fprintf( fp, "bind \"%s\" \"%s\"\n", pb->m_pszKey, pb->m_pszCmd );
			}
		}
		pb++;
	}
	while ( --i );

	for ( cfg_cvar_t* pv = pCvars; pv; pv = pv->m_pNext )
	{
		if ( pv->m_pszValue[0] )
		{
			if ( pv->m_bSetInfo )
				fprintf( fp, "setinfo %s \"%s\"\n", pv->m_pszName, pv->m_pszValue );
			else
				fprintf( fp, "%s \"%s\"\n", pv->m_pszName, pv->m_pszValue );
		}
	}

	for ( cfg_cmd_t* pc = pCmds; pc; pc = pc->m_pNext )
		fprintf( fp, "%s\n", pc->m_pszCmd );

	return fclose( fp );
}

// PlayerConfig_Reset (0x459150)
int PlayerConfig_Reset( const char* pszName, char* pBlock )
{
	CFG_FreeBindings( (cfg_keybind_t*)pBlock );
	memset( pBlock, 0, CFG_BLOCK_SIZE );
	PlayerConfig_LoadDefaults( pBlock );
	PlayerConfig_ApplyDefaults( pBlock );
	return Launcher_SavePlayerInfoTo( "Player", pBlock );
}

// PlayerConfig_LoadKeybinds (0x459190)
int PlayerConfig_LoadKeybinds( const char* pszName, char* pBlock )
{
	cfg_bind_t	bindTable[256];
	cfg_cvar_t*	pCvars = NULL;
	cfg_cmd_t*	pCmds  = NULL;
	void*		pFile  = NULL;

	CFG_LoadFile( pszName, &pFile );
	if ( !pFile )
		return PlayerConfig_Reset( pszName, pBlock );

	memset( bindTable, 0, sizeof( bindTable ) );
	CFG_ParseConfig( (char*)pFile, bindTable, &pCvars, &pCmds );

	cfg_keybind_t*	pRecs = (cfg_keybind_t*)pBlock;
	for ( int i = 0; i < 256; i++ )
	{
		if ( pRecs[i].m_pszBind )
			free( pRecs[i].m_pszBind );
		pRecs[i].m_pszBind = NULL;
		if ( bindTable[i].m_pszCmd )
			pRecs[i].m_pszBind = _strdup( bindTable[i].m_pszCmd );
	}

	CFG_FreeBindTable( bindTable );
	CFG_FreeCvarList( &pCvars );
	CFG_FreeCmdList( &pCmds );
	free( pFile );			// return value unused
	return 0;
}

// CGameConfig_ParseKey (0x459290)
void CGameConfig_ParseKey( cfg_cvar_t** ppHead, char* pObj, const char* pszKey )
{
	cfg_cvar_t*	pNode = CFG_FindCvar( *ppHead, pszKey );
	if ( !pNode )
		return;

	CGameClientConfig*	pCfg = (CGameClientConfig*)pObj;

	float*	pflField = CfgFloatField( pCfg, pszKey );
	if ( pflField )
	{
		*pflField = (float)atof( pNode->m_pszValue );			// 0x45956A
		if ( !_strcmpi( pszKey, "rate" ) && atof( pNode->m_pszValue ) == 0.0 )	// 0x459599
			*pflField = 2500.0f;
		return;
	}

	if ( !_strcmpi( pszKey, "topcolor" ) )
	{
		pCfg->topcolor = atoi( pNode->m_pszValue );				// 0x4595EC
		return;
	}
	if ( !_strcmpi( pszKey, "bottomcolor" ) )
	{
		pCfg->bottomcolor = atoi( pNode->m_pszValue );
		return;
	}

	if ( !_strcmpi( pszKey, "model" ) )
		strcpy( pCfg->model, pNode->m_pszValue );					// 0x459640
	else if ( !_strcmpi( pszKey, "name" ) )
		strcpy( pCfg->name, pNode->m_pszValue );
}

// PlayerConfig_LoadCvars (0x459650)
int PlayerConfig_LoadCvars( const char* pszName, char* pBlock )
{
	cfg_bind_t	bindTable[256];
	cfg_cvar_t*	pCvars = NULL;
	cfg_cmd_t*	pCmds  = NULL;
	void*		pFile  = NULL;

	CFG_LoadFile( pszName, &pFile );
	if ( !pFile )
		return PlayerConfig_Reset( pszName, pBlock );

	memset( bindTable, 0, sizeof( bindTable ) );
	CFG_ParseConfig( (char*)pFile, bindTable, &pCvars, &pCmds );

	CGameConfig_ParseKey( &pCvars, pBlock, "lookstrafe" );
	CGameConfig_ParseKey( &pCvars, pBlock, "m_pitch" );
	CGameConfig_ParseKey( &pCvars, pBlock, "lookspring" );
	CGameConfig_ParseKey( &pCvars, pBlock, "crosshair" );
	CGameConfig_ParseKey( &pCvars, pBlock, "_windowed_mouse" );
	CGameConfig_ParseKey( &pCvars, pBlock, "m_filter" );
	CGameConfig_ParseKey( &pCvars, pBlock, "joystick" );
	CGameConfig_ParseKey( &pCvars, pBlock, "sv_aim" );
	CGameConfig_ParseKey( &pCvars, pBlock, "cl_himodels" );
	CGameConfig_ParseKey( &pCvars, pBlock, "d_spriteskip" );
	CGameConfig_ParseKey( &pCvars, pBlock, "sensitivity" );
	CGameConfig_ParseKey( &pCvars, pBlock, "viewsize" );
	CGameConfig_ParseKey( &pCvars, pBlock, "brightness" );
	CGameConfig_ParseKey( &pCvars, pBlock, "gamma" );
	CGameConfig_ParseKey( &pCvars, pBlock, "bgmvolume" );
	CGameConfig_ParseKey( &pCvars, pBlock, "suitvolume" );
	CGameConfig_ParseKey( &pCvars, pBlock, "hisound" );
	CGameConfig_ParseKey( &pCvars, pBlock, "volume" );
	CGameConfig_ParseKey( &pCvars, pBlock, "voice_scale" );
	CGameConfig_ParseKey( &pCvars, pBlock, "voice_modenable" );
	CGameConfig_ParseKey( &pCvars, pBlock, "s_a3d" );
	CGameConfig_ParseKey( &pCvars, pBlock, "s_eax" );
	CGameConfig_ParseKey( &pCvars, pBlock, "rate" );
	CGameConfig_ParseKey( &pCvars, pBlock, "model" );
	CGameConfig_ParseKey( &pCvars, pBlock, "topcolor" );
	CGameConfig_ParseKey( &pCvars, pBlock, "bottomcolor" );
	CGameConfig_ParseKey( &pCvars, pBlock, "name" );

	( (CGameClientConfig*)pBlock )->mlook = CFG_FindCmd( pCmds, "+mlook" ) ? 1.0f : 0.0f;
	( (CGameClientConfig*)pBlock )->jlook = CFG_FindCmd( pCmds, "+jlook" ) ? 1.0f : 0.0f;

	CFG_FreeBindTable( bindTable );
	CFG_FreeCvarList( &pCvars );
	CFG_FreeCmdList( &pCmds );
	free( pFile );			// return value unused
	return 0;
}

// CFG_ApplyKeybinds (0x459910)
void CFG_ApplyKeybinds( const char* pszName, char* pCfgBlock )
{
	cfg_bind_t	bindTable[256];
	cfg_cvar_t*	pCvars = NULL;
	cfg_cmd_t*	pCmds  = NULL;
	void*		pFile  = NULL;

	memset( bindTable, 0, sizeof( bindTable ) );
	CFG_LoadFile( pszName, &pFile );
	if ( pFile )
		CFG_ParseConfig( (char*)pFile, bindTable, &pCvars, &pCmds );

	cfg_keybind_t*	pRecs = (cfg_keybind_t*)pCfgBlock;
	for ( int i = 0; i < 256; i++ )
	{
		if ( bindTable[i].m_pszCmd )
			free( bindTable[i].m_pszCmd );
		bindTable[i].m_pszCmd = NULL;

		if ( pRecs[i].m_pszBind )
		{
			if ( !bindTable[i].m_pszKey )
				bindTable[i].m_pszKey = _strdup( pRecs[i].m_szKey );
			bindTable[i].m_pszCmd = _strdup( pRecs[i].m_pszBind );
		}
	}

	CFG_WriteConfig( pszName, bindTable, pCvars, pCmds );
	CFG_FreeBindTable( bindTable );
	CFG_FreeCvarList( &pCvars );
	CFG_FreeCmdList( &pCmds );
	if ( pFile )
		free( pFile );
}

// The whitelisted cvars copied from the player config block on apply (the same
// set as Profile.cpp's field-mapper covers).
static const char* s_rgszApplyCvars[] =
{
	"lookstrafe", "m_pitch", "lookspring", "crosshair", "_windowed_mouse",
	"m_filter", "joystick", "sv_aim", "console", "cl_himodels", "d_spriteskip",
	"sensitivity", "viewsize", "brightness", "gamma", "bgmvolume", "suitvolume",
	"hisound", "volume", "voice_scale", "voice_modenable", "s_a3d", "s_eax",
	"rate", "model", "topcolor", "bottomcolor", "name"
};

// CGameConfig_WriteKey (0x459A30)
cfg_cvar_t* CGameConfig_WriteKey( cfg_cvar_t** ppHead, char* pObj, const char* pszKey )
{
	char	szBuf[32];

	CGameClientConfig*	pCfg = (CGameClientConfig*)pObj;

	float*	pflField = CfgFloatField( pCfg, pszKey );
	if ( pflField )
	{
		sprintf( szBuf, "%f", *pflField );							// 0x459CFA
		return CFG_SetCvar( ppHead, pszKey, szBuf );
	}

	if ( !_strcmpi( pszKey, "topcolor" ) )
	{
		sprintf( szBuf, "%i", pCfg->topcolor );						// 0x459D5B
		return CFG_SetCvar( ppHead, pszKey, szBuf );
	}
	if ( !_strcmpi( pszKey, "bottomcolor" ) )
	{
		sprintf( szBuf, "%i", pCfg->bottomcolor );
		return CFG_SetCvar( ppHead, pszKey, szBuf );
	}

	if ( !_strcmpi( pszKey, "model" ) )
		return CFG_SetCvar( ppHead, pszKey, pCfg->model );
	if ( !_strcmpi( pszKey, "name" ) )
		return CFG_SetCvar( ppHead, pszKey, pCfg->name );

	// The binary falls off the end here and returns whatever the last compare
	// left behind; no caller looks at it.
	return NULL;
}

// CFG_SetTokenProfile (0x459DD0)
void CFG_SetTokenProfile( const char* pszFile, const char* pszName, const char* pszValue, int bSetInfo )
{
	cfg_bind_t	bindTable[CFG_BIND_COUNT];
	cfg_cvar_t*	pCvars = NULL;
	cfg_cmd_t*	pCmds  = NULL;
	void*		pData  = NULL;
	cfg_cvar_t*	pCvar;

	CFG_LoadFile( pszFile, &pData );
	memset( bindTable, 0, sizeof( bindTable ) );
	if ( pData )
		CFG_ParseConfig( (char*)pData, bindTable, &pCvars, &pCmds );

	pCvar = CFG_SetCvar( &pCvars, pszName, pszValue );
	if ( pCvar )
		pCvar->m_bSetInfo = bSetInfo;	// cvar +4

	CFG_WriteConfig( pszFile, bindTable, pCvars, pCmds );
	CFG_FreeBindTable( bindTable );
	CFG_FreeCvarList( &pCvars );
	CFG_FreeCmdList( &pCmds );

	if ( pData )
		free( pData );
}

// CFG_ReadCvar (0x459EB0)
BOOL CFG_ReadCvar( const char* pszName, const char* pszCvar, char* pszOut )
{
	cfg_bind_t	bindTable[256];
	cfg_cvar_t*	pCvars = NULL;
	cfg_cmd_t*	pCmds  = NULL;
	void*		pFile  = NULL;
	BOOL		bFound = FALSE;

	memset( bindTable, 0, sizeof( bindTable ) );
	CFG_LoadFile( pszName, &pFile );
	if ( pFile )
		CFG_ParseConfig( (char*)pFile, bindTable, &pCvars, &pCmds );

	cfg_cvar_t*	pCvar = CFG_FindCvar( pCvars, pszCvar );
	if ( pCvar )
	{
		strcpy( pszOut, pCvar->m_pszValue );
		bFound = TRUE;
	}

	CFG_FreeBindTable( bindTable );
	CFG_FreeCvarList( &pCvars );
	CFG_FreeCmdList( &pCmds );
	if ( pFile )
		free( pFile );
	return bFound;
}

// CFG_ApplyCvars (0x459FA0)
void CFG_ApplyCvars( const char* pszName, char* pCfgBlock )
{
	cfg_bind_t	bindTable[256];
	cfg_cvar_t*	pCvars = NULL;
	cfg_cmd_t*	pCmds  = NULL;
	void*		pFile  = NULL;

	memset( bindTable, 0, sizeof( bindTable ) );
	CFG_LoadFile( pszName, &pFile );
	if ( pFile )
		CFG_ParseConfig( (char*)pFile, bindTable, &pCvars, &pCmds );

	for ( int i = 0; i < ARRAYSIZE( s_rgszApplyCvars ); i++ )
		CGameConfig_WriteKey( &pCvars, pCfgBlock, s_rgszApplyCvars[i] );

	// +mlook
	if ( ( (CGameClientConfig*)pCfgBlock )->mlook == 0.0f )
	{
		cfg_cmd_t*	p = CFG_FindCmd( pCmds, "+mlook" );
		if ( p )
			CFG_UnlinkCmd( &pCmds, p );
	}
	else
		CFG_AddCmd( &pCmds, "+mlook" );

	// +jlook
	if ( ( (CGameClientConfig*)pCfgBlock )->jlook == 0.0f )
	{
		cfg_cmd_t*	p = CFG_FindCmd( pCmds, "+jlook" );
		if ( p )
			CFG_UnlinkCmd( &pCmds, p );
	}
	else
		CFG_AddCmd( &pCmds, "+jlook" );

	CFG_WriteConfig( pszName, bindTable, pCvars, pCmds );
	CFG_FreeBindTable( bindTable );
	CFG_FreeCvarList( &pCvars );
	CFG_FreeCmdList( &pCmds );
	if ( pFile )
		free( pFile );
}

// CFG_LoadFile (0x45A2B0)
char* CFG_LoadFile( const char* pszName, void** ppOut )
{
	char*	p = (char*)COM_LoadMallocFile( pszName );
	*ppOut = p;
	return p;
}


/*
==================
CGameConfig_IsEqual (0x4532e0)

Sits with the browser document's bodies rather than the rest of this file, so
it goes after them all.  (sic) it returns TRUE when the two configs *differ*.
==================
*/
BOOL CGameConfig_IsEqual( const CServerBrowser* pA, const CServerBrowser* pB )
{
	const CGameClientConfig&	a = pA->m_playerConfig;
	const CGameClientConfig&	b = pB->m_playerConfig;

	if ( _strcmpi( pA->m_szLogoName, pB->m_szLogoName ) )		return TRUE;
	if ( _strcmpi( pA->m_szLogoColor, pB->m_szLogoColor ) )	return TRUE;

	if ( a.m_pitch			!= b.m_pitch )			return TRUE;
	if ( a.lookstrafe		!= b.lookstrafe )		return TRUE;
	if ( a.lookspring		!= b.lookspring )		return TRUE;
	if ( a.crosshair		!= b.crosshair )		return TRUE;
	if ( a._windowed_mouse	!= b._windowed_mouse )	return TRUE;
	if ( a.m_filter			!= b.m_filter )			return TRUE;
	if ( a.mlook			!= b.mlook )			return TRUE;
	if ( a.jlook			!= b.jlook )			return TRUE;
	if ( a.joystick			!= b.joystick )			return TRUE;
	if ( a.sv_aim			!= b.sv_aim )			return TRUE;
	if ( a.console			!= b.console )			return TRUE;
	if ( a.cl_himodels		!= b.cl_himodels )		return TRUE;
	if ( a.d_spriteskip		!= b.d_spriteskip )		return TRUE;
	if ( a.sensitivity		!= b.sensitivity )		return TRUE;
	if ( a.viewsize			!= b.viewsize )			return TRUE;
	if ( a.brightness		!= b.brightness )		return TRUE;
	if ( a.gamma			!= b.gamma )			return TRUE;
	if ( a.bgmvolume		!= b.bgmvolume )		return TRUE;
	if ( a.suitvolume		!= b.suitvolume )		return TRUE;
	if ( a.hisound			!= b.hisound )			return TRUE;
	if ( a.volume			!= b.volume )			return TRUE;
	if ( a.s_a3d			!= b.s_a3d )			return TRUE;
	if ( a.s_eax			!= b.s_eax )			return TRUE;
	if ( a.rate				!= b.rate )				return TRUE;
	if ( a.topcolor			!= b.topcolor )			return TRUE;
	if ( a.bottomcolor		!= b.bottomcolor )		return TRUE;
	if ( a.voice_scale		!= b.voice_scale )		return TRUE;
	if ( a.voice_modenable	!= b.voice_modenable )	return TRUE;
	if ( _strcmpi( a.model, b.model ) )				return TRUE;
	if ( strcmp ( a.name,  b.name  ) )				return TRUE;

	for ( int i = 0; i < CFG_BIND_COUNT; i++ )
	{
		const char*	pszA = a.m_binds[i].m_pszBind;
		const char*	pszB = b.m_binds[i].m_pszBind;
		if ( ( pszA == NULL ) != ( pszB == NULL ) )		// one NULL, other not
			return TRUE;
		if ( pszA && pszB && strcmp( pszA, pszB ) )
			return TRUE;
	}

	return FALSE;
}
