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
// Purpose: declares the mod descriptor key/value info table, the scanned mod
//          list, and CODModListCtrl's row drawing.
//
// $NoKeywords: $
//=============================================================================

#ifndef MOD_H
#define MOD_H
#ifdef _WIN32
#pragma once
#endif

// One key/value pair in a mod's info table.
typedef struct modkey_s
{
	char*			key;	// +0
	char*			value;	// +4
	struct modkey_s* next;	// +8
} modkey_t;

// A mod descriptor: a node in the mod list plus its info table.  Everything
// the table can do is a member, so a mod is always addressed through itself.
typedef struct mod_s
{
	mod_s*	Init();
	void	FreeKeys();
	int		GetKeyInt( const char* key );
	char*	GetKeyString( const char* key );
	char*	GetKey( const char* key );
	char*	SetKey( const char* key, const char* value );

	struct mod_s*	next;	// +0  next mod in the list
	modkey_t*		keys;	// +4  head of the key/value table
} mod_t;

mod_t*	ModList_FindByGamedir( mod_t** ppList, const char* name );
void	ModList_Clear( int keepInstalled );
mod_t*	Mod_ParseLiblist( char* gamedir, char* filename );
char*	ModList_Scan( void );
int		ModList_Compare( mod_t* a, mod_t* b, int field, int ascending );
int __stdcall ModList_CompareKeys( mod_t* a, mod_t* b, int flags );

extern mod_t*	g_pModList;		// head of the scanned mod list
extern mod_t*	g_pCurrentMod;	// the active gamedir's mod
extern mod_t*	g_pValveMod;	// the base "valve" mod

#endif // MOD_H
