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
// Purpose: declares the launcher-wide globals, the CNetGameApp / CLauncher
//          interface and the shared helpers.
//
// $NoKeywords: $
//=============================================================================

#ifndef LAUNCHER_H
#define LAUNCHER_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>
#include <stdlib.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE( a )	( sizeof( a ) / sizeof( (a)[0] ) )
#endif

#include "re.h"
#include "strings.h"

typedef unsigned char	byte;
typedef int				qboolean;

#define	TRUE	1
#define	FALSE	0

#include "engine_launcher_api.h"
#include "exefuncs.h"
#include "LauncherServers.h"
#include "vmodes.h"

typedef VidTypes	vidtype_t;

class CWnd;
class CDC;
class CServerBrowser;
struct mod_s;
struct crypt_parms_s;
struct IDirectDraw;


extern	engine_api_t	engineapi;
extern	exefuncs_t		ef;

int			Eng_Load (const char *pszDll, int iSubState);
HMODULE		Eng_KillEngine (const char *pszNextDll, int *pbState, HMODULE *phModule);
int			Eng_Frame (int iState);
void		Eng_LoadStubs (void);
struct mod_s *Eng_SetCurrentMod (void);

// shared globals
extern	char			*gpszCmdLine;
extern	unsigned char	*gpMemBase;
extern	int				giMemSize;
extern	HWND			mainwindow;
extern	struct IDirectDraw	*lpDD;
extern	UINT			guMouseWheelMsg;
extern	int				g_nNumPings;
extern	int				windowed_mouse;

extern	HINSTANCE		gLauncherHandle;
extern	char			g_szPatchVersion[];
extern	int				g_nMenuShown, g_nMenuState;
extern	int				gTopLevelFrame;

extern	CServerBrowser		*g_pServerBrowser;
extern	struct crypt_parms_s	gCryptParms;
extern	char			g_szGUID[];
extern	char			com_gamedir[];
extern	HGLOBAL			g_hBackgroundDib;
extern	int			g_bConnectInProgress;	// (4E194C) a connect attempt is in flight

char		*CheckParm (const char *psz, char **ppszValue);

void		Launcher_WriteProfileInt (const char *section, const char *key, int value);
int			Launcher_GetProfileInt (const char *section, const char *key, int def);
int			Launcher_WriteProfileString (const char *section, const char *key, const char *value);
char		*Launcher_GetProfileString (const char *section, const char *key, const char *def);

char		*GetLocalizedString (unsigned int uID);
char		*Launcher_FormatAppName (void);
HMODULE		Launcher_GetResourceModule (void);
char		*COM_GetBaseDir (void);
BOOL		CDKey_Checksum (const char *key);

// quit / error / auth
void		Launcher_SetQuitFlag (int value);
int			Launcher_IsQuitting (void);
int			Launcher_GetErrorState (void);
void		Launcher_SetErrorState (int value);
void		Launcher_SetErrorMessage (const char *pszMessage);
char*		Launcher_GetErrorMessage (void);
void		Console_Printf (char *fmt, ...);
void		Launcher_HandleAuthFailure (void);
#ifdef LAUNCHER_RE
void		Launcher_UpdateShellAfterEngine (void);
#endif

void		Launcher_SetRestartFlag (int value);
void		AuthFailed (void);
char		*GetCDKey (char *pszCDKey, int *nLength, int *bDedicated);
int			IsValidCD (void);
int			CheckCDKey (void);
int			Launcher_GetRestartFlag (void);
void		Launcher_InitCmds (void);
int			WSA_GetNumSockets (void);

int			Launcher_MainButtonsLoaded (void);
void		Launcher_LoadMainButtonsBitmap (void);
void		Launcher_FreeMainButtonsBitmap (void);
void		Launcher_SetButtonCell (int *pWH);

HGLOBAL		Launcher_HeaderLoaded (void);
int			*Launcher_HeaderSize (int *pWH);	// returns its own argument
int			Launcher_HeaderStride (void);
void		*Launcher_LoadHeaderBitmapFile (const char *pszName, HGLOBAL *phDib, RECT *prcOut, RECT *prcSrc);

void		Launcher_LoadSplashBitmap (void);
void		Launcher_FreeSplashBitmap (void);

void		Launcher_BlitBackground (CDC *pDC, RECT *prcDst, RECT *prcSrc);
void		Launcher_CopyParentBackground (CDC *pDC, RECT *prcDst, RECT *prcSrc);
void		Launcher_CompositeDib (CDC *pDC, RECT *prcDst, HGLOBAL hDib, RECT *prcSrc);



void		Launcher_SavePlayerInfo (void);
void		Launcher_OnGameDirChanged (void);
int			Launcher_LoadPlayerInfo (const char *pszSection, void *pBuf);
int			Launcher_SavePlayerInfoTo (const char *pszSection, void *pBuf);

char		*Launcher_GetGLDriver (void);

void		Launcher_CreateServerBrowser (void);

void		Launcher_RunMapCommand (const char *pszCmd);
const char	*Launcher_GetPendingMap (void);

int			AFXGetTopLevelFrame (void);
void		AFXSetTopLevelFrame (int bPending);

void		NullStub (void);
void		NullStub (int flag);

void		VGui_Start (void);
void		VGui_Frame (void);

#endif // LAUNCHER_H
