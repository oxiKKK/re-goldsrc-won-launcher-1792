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
// Purpose: declares CGameConfig player / game userinfo persistence.
//
// $NoKeywords: $
//=============================================================================

#ifndef GAMECONFIG_H
#define GAMECONFIG_H

#include <windows.h>

// One keybind-table slot (256-entry array).

typedef struct cfg_bind_t
{
	char*	m_pszKey;		// +0  strdup'd key name
	char*	m_pszCmd;		// +4  strdup'd bound command
} cfg_bind_t;

// One keybind record inside the player CGameClientConfig block.
typedef struct cfg_keybind_t
{
	char	m_szKey[32];	// +0   key/action name
	int		m_nState;		// +32  per-key state
	char*	m_pszBind;		// +36  strdup'd bound command (owned)
} cfg_keybind_t;

// Size of the whole player config block (256 keybind records + cvar fields).
#define CFG_BLOCK_SIZE		0x2A78
#define CFG_BIND_COUNT		256

// The player CGameClientConfig block in full.
typedef struct CGameClientConfig
{
	cfg_keybind_t	m_binds[CFG_BIND_COUNT];	// +0      256 * 40 = 10240 (0x2800)
	// cvar mirror (offset +10240) -- one float per game/input/audio cvar:
	float	m_pitch;			// +10240
	float	lookstrafe;			// +10244
	float	lookspring;			// +10248
	float	crosshair;			// +10252
	float	_windowed_mouse;	// +10256
	float	m_filter;			// +10260
	float	mlook;				// +10264  "+mlook" flag (derived from the +command list)
	float	jlook;				// +10268  "+jlook" flag (derived from the +command list)
	float	joystick;			// +10272
	float	sv_aim;				// +10276
	float	console;			// +10280
	float	cl_himodels;		// +10284
	float	d_spriteskip;		// +10288
	float	sensitivity;		// +10292
	float	viewsize;			// +10296
	float	brightness;			// +10300
	float	gamma;				// +10304
	float	bgmvolume;			// +10308
	float	suitvolume;			// +10312
	float	hisound;			// +10316
	float	volume;				// +10320
	float	s_a3d;				// +10324
	float	s_eax;				// +10328
	float	rate;				// +10332  (CGameConfig_ParseKey 0x4594C0)
	float	voice_scale;		// +10336
	float	voice_modenable;	// +10340
	char	model[260];			// +10344  player model name ("model" cvar; 0x459640)
	int		topcolor;			// +10604  ("topcolor")
	int		bottomcolor;		// +10608  ("bottomcolor")
	char	name[260];			// +10612  player name ("name" cvar)
} CGameClientConfig;			// 0x2A78 = 10872 bytes

// One "+command" node (e.g. "+mlook").
typedef struct cfg_cmd_t
{
	struct cfg_cmd_t*	m_pNext;	// +0
	char*				m_pszCmd;	// +4  strdup'd
} cfg_cmd_t;

// One cvar / setinfo node (malloc 0x10, 16 bytes).
typedef struct cfg_cvar_t
{
	struct cfg_cvar_t*	m_pNext;	// +0
	int					m_bSetInfo;	// +4   1 = "setinfo", 0 = plain cvar
	char*				m_pszName;	// +8   strdup'd
	char*				m_pszValue;	// +12  strdup'd
} cfg_cvar_t;

void		CFG_FreeBindTable( cfg_bind_t* pTable );					// 0x458BB0
int			CFG_KeyNameToIndex( const char* pszKey );					// 0x458BF0
void		CFG_FreeCmdList( cfg_cmd_t** ppHead );						// 0x458D40
void		CFG_UnlinkCmd( cfg_cmd_t** ppHead, cfg_cmd_t* pNode );		// 0x458D80
cfg_cmd_t*	CFG_FindCmd( cfg_cmd_t* pHead, const char* pszCmd );			// 0x458DD0
cfg_cmd_t*	CFG_AddCmd( cfg_cmd_t** ppHead, const char* pszCmd );		// 0x458E00

void		CFG_FreeBindings( cfg_keybind_t* pBlock );					// 0x4582B0
void		CFG_CopyConfig( cfg_keybind_t* pDst, const cfg_keybind_t* pSrc );	// 0x4582F0
int			CFG_FindKeyName( const char* pszName, const char* pszTable );	// 0x458390

// cvar / setinfo list primitives (canonical here; the triad below uses them too).
cfg_cvar_t*	CFG_FindCvar( cfg_cvar_t* pHead, const char* pszName );						// 0x458C80
cfg_cvar_t*	CFG_SetCvar( cfg_cvar_t** ppHead, const char* pszName, const char* pszValue );	// 0x458CB0
void		CFG_FreeCvarList( cfg_cvar_t** ppHead );									// 0x458C30

// config.cfg tokenizer / writer
char*		CFG_ParseToken( char** ppData );											// 0x458B10
void		CFG_ParseConfig( char* pData, cfg_bind_t* pBindTable, cfg_cvar_t** ppCvars, cfg_cmd_t** ppCmds );	// 0x458E50
int			CFG_WriteConfig( const char* pszName, cfg_bind_t* pBindTable, cfg_cvar_t* pCvars, cfg_cmd_t* pCmds );	// 0x458FE0

// config.cfg apply drivers (load via the engine VFS, merge the player config
// block, rewrite).
int			Keys_LoadNameTable( char* pDisplayRecs );									// 0x4583E0 (fills g_szBindNames)

// Player config block load/save/reset engine.
int			PlayerConfig_ApplyDefaults( char* pDisplayRecs );							// 0x458680  (loads kb_def.lst binds)
int			PlayerConfig_LoadDefaults( char* pBlock );									// 0x4588E0
int			Launcher_LoadPlayerInfo( const char* pszSection, void* pData );			// 0x458A60
int			Launcher_SavePlayerInfoTo( const char* pszSection, void* pData );			// 0x458AC0
int			PlayerConfig_Reset( const char* pszName, char* pBlock );						// 0x459150
int			PlayerConfig_LoadKeybinds( const char* pszName, char* pBlock );				// 0x459190
int			PlayerConfig_LoadCvars( const char* pszName, char* pBlock );				// 0x459650

char*		CFG_LoadFile( const char* pszName, void** ppOut );							// 0x45A2B0
void		CFG_SetTokenProfile( const char* pszFile, const char* pszName, const char* pszValue, int bSetInfo );	// 0x459DD0
void		CFG_ApplyKeybinds( const char* pszName, char* pCfgBlock );					// 0x459910
BOOL		CFG_ReadCvar( const char* pszName, const char* pszCvar, char* pszOut );		// 0x459EB0
void		CFG_ApplyCvars( const char* pszName, char* pCfgBlock );						// 0x459FA0

// The default-build config file name the load/save drivers consume (filled by the
// launcher before driving the PlayerConfig_* entry points).
extern char	g_szConfigName[256];	// 0x4E2EB4

class CServerBrowser;

// - the player/game cvar persistence triad -------------------------------
// pObj is the CGameClientConfig block; ppHead is the owner's cvar list head.
void		CGameConfig_ParseKey( cfg_cvar_t** ppHead, char* pObj, const char* pszKey );	// 0x459290
cfg_cvar_t*	CGameConfig_WriteKey( cfg_cvar_t** ppHead, char* pObj, const char* pszKey );	// 0x459A30
BOOL		CGameConfig_IsEqual( const CServerBrowser* pA, const CServerBrowser* pB );	// 0x4532E0

#endif // GAMECONFIG_H
