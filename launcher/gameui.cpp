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
// Purpose: the game heap and platform gate, the window proc and input, and the
//          DirectDraw mode probe.
//
// $NoKeywords: $
//=============================================================================

// The DirectDraw mode enumeration drives DDraw through the C-style COM
// interfaces (lpVtbl + explicit this), matching the original C launcher.
#define CINTERFACE
#include "gameui.h"
#include "launcher.h"
#include "engine.h"
#include "vid.h"
#include "strings.h"
#include "cd_win.h"
#include "keydefs.h"
#include "resource.h"
#include "viddef.h"
#include <ddraw.h>
#include <mmsystem.h>
#include <string.h>
#include <stdlib.h>

// Mouse events suppressed (e.g. while a system dialog is up).
int				g_bBlockMouseEvents;	// (4E199C)

// Platform-validation flags + the target colour depth.  All but g_bWinNT are
// referenced only from this band.
static int	g_bEnoughMem;		// (4E1F04) >= 23 MB physical
static int	g_bWin9x;			// (4E1F08)
int			g_bWinNT;			// (4E1F0C)
static int	g_bD3DRequested;	// (4E1F18) written, never read anywhere in the binary
static int	g_bWinVer4Plus;		// (4E6E0C) written, never read
static int	g_nDesiredBpp = 16;	// (4E1F30) target colour depth (16 / -24bpp / -32bpp)

HANDLE	tevent;				// (4E1F10) idle-wait event

static int	g_bD3DAvailable;	// (4E1F2C) last Vid_D3DSupported result

// The DirectX capability level VID_GetPrimaryDepth reports: the DirectX version
// times 256.  DX6 (IDirectDrawSurface4) is what the launcher requires.
#define DXLEVEL_NONE	0
#define DXLEVEL_DX1		256
#define DXLEVEL_DX2		512
#define DXLEVEL_DX3		768
#define DXLEVEL_DX5		1280
#define DXLEVEL_DX6		1536

// id's own surface-cache figures (d_surf.c) and Quake's free-memory floor.
#define SURFCACHE_SIZE_AT_320X200	( 3 * 1024 * 1024 )
#define MINIMUM_MEMORY				0x800000	// 8 MB

static void	VID_GetPrimaryDepth( int* pDXLevel, int* pPlatform );

/*
==================
AllocGameMem (0x412B90)
==================
*/
int AllocGameMem( void )
{
	MEMORYSTATUS	memStatus;
	OSVERSIONINFOA	osvi;
	char*			p;

	LOG_ENTER();

	memStatus.dwLength = sizeof( memStatus );
	GlobalMemoryStatus( &memStatus );
	LOG( "physical RAM = %u KB", (unsigned)( memStatus.dwTotalPhys / 1024 ) );

	giMemSize = memStatus.dwTotalPhys;
	// Signed compare, as in the binary: it reads negative (and so trips this
	// "insufficient memory" bail) on machines with 2 GB or more of RAM.
	if ( (int)memStatus.dwTotalPhys < 0xF00000 )		// < 15 MB
	{
		Launcher_ShowMessageByIdEx( 0, IDS_MEM_INSUFFICIENT, (double)(int)memStatus.dwTotalPhys / 1024.0 );
		return 0;
	}

	g_bEnoughMem = (int)memStatus.dwTotalPhys >= 0x1700000;	// >= 23 MB

	// Use half of physical, clamped to [14 MB, 40 MB].
	giMemSize = memStatus.dwTotalPhys >> 1;
	if ( giMemSize >= 0xE00000 )
	{
		if ( giMemSize > 0x2800000 )
			giMemSize = 0x2800000;
	}
	else
	{
		giMemSize = 0xE00000;
	}

	if ( CheckParm( "-heapsize", &p ) && p )
		giMemSize = atoi( p ) * 1024;
	if ( CheckParm( "-minmemory", NULL ) )
		giMemSize = 0xE00000;

	gpMemBase = (unsigned char*)GlobalAlloc( 0, giMemSize );
	if ( !gpMemBase )
	{
		Launcher_ShowMessageByIdEx( 0, IDS_MEM_ALLOCFAIL, (double)giMemSize / 1024.0 );
		return 0;
	}

	tevent = CreateEvent( NULL, FALSE, FALSE, NULL );
	if ( !tevent )
	{
		Launcher_ShowMessageById( 0, IDS_EVENT_CREATEFAIL );
		return 0;
	}

	osvi.dwOSVersionInfoSize = sizeof( osvi );
	if ( !GetVersionExA( &osvi ) )
	{
		Launcher_ShowMessageById( 0, IDS_OSVER_FAIL );
		return 0;
	}
	g_bWinVer4Plus = osvi.dwMajorVersion >= 4;

	if ( osvi.dwPlatformId == VER_PLATFORM_WIN32s )
	{
		Launcher_ShowMessageById( 0, IDS_OSVER_OUTDATED );
		return 0;
	}

	if ( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		g_bWinNT = 1;
		if ( osvi.dwMajorVersion < 5 )		// NT4 requires Service Pack 3+
		{
			char* sp = strstr( osvi.szCSDVersion, "Service Pack " );
			if ( !sp || atoi( sp + strlen( "Service Pack " ) ) < 3 )
			{
				Launcher_ShowMessageById( 0, IDS_OSVER_NTSP3 );
				return 0;
			}
		}
	}
	else		// VER_PLATFORM_WIN32_WINDOWS
	{
		g_bWinNT = 0;
		g_bWin9x = 1;
	}

	// On Win9x 4.x the desktop must be 15/16-bit for the software DDraw path.
	if ( g_bWin9x && osvi.dwMajorVersion == 4 && osvi.dwMinorVersion < 10 )
	{
		HWND	hwndDesktop = GetDesktopWindow();
		HDC		dc = GetDC( hwndDesktop );
		int		depth = GetDeviceCaps( dc, BITSPIXEL );
		ReleaseDC( hwndDesktop, dc );
		if ( depth != 15 && depth != 16 )
		{
			Launcher_ShowMessageById( 0, IDS_OSVER_16BIT );
			return 0;
		}
	}

	int		nDXLevel;
	int		nPlatform;

	if ( g_bWinNT || ( VID_GetPrimaryDepth( &nDXLevel, &nPlatform ), nDXLevel == DXLEVEL_DX6 ) )
	{
		g_bD3DRequested = ( CheckParm( "-D3D", NULL ) != NULL );

		// Suppress the foreground/window-pos jolt while the mode probe brings
		// DirectDraw up and down during enumeration.
		force_minimized = 1;
		CDAudio_Init();
		Vid_BuildModeList();
		force_minimized = 0;
		return 1;
	}

	Launcher_ShowMessageById( 0, IDS_INIT_DX6REQUIRED );
	return 0;
}

// scan code -> engine key (4CEDA0): the classic Quake scantokey table with the
// GoldSrc K_* key codes (engine/keydefs.h).
static unsigned char	scantokey[128] =
{
//	0			1			2			3			4			5			6			7
//	8			9			A			B			C			D			E			F
	0,			27,			'1',		'2',		'3',		'4',		'5',		'6',
	'7',		'8',		'9',		'0',		'-',		'=',		K_BACKSPACE,9,			// 0
	'q',		'w',		'e',		'r',		't',		'y',		'u',		'i',
	'o',		'p',		'[',		']',		13,			K_CTRL,		'a',		's',		// 1
	'd',		'f',		'g',		'h',		'j',		'k',		'l',		';',
	'\'',		'`',		K_SHIFT,	'\\',		'z',		'x',		'c',		'v',		// 2
	'b',		'n',		'm',		',',		'.',		'/',		K_SHIFT,	'*',
	K_ALT,		' ',		K_CAPSLOCK,	K_F1,		K_F2,		K_F3,		K_F4,		K_F5,		// 3
	K_F6,		K_F7,		K_F8,		K_F9,		K_F10,		K_PAUSE,	0,			K_HOME,
	K_UPARROW,	K_PGUP,		K_KP_MINUS,	K_LEFTARROW,K_KP_5,		K_RIGHTARROW,K_KP_PLUS,	K_END,		// 4
	K_DOWNARROW,K_PGDN,		K_INS,		K_DEL,		0,			0,			0,			K_F11,
	K_F12,		0,			0,			0,			0,			0,			0,			0,			// 5
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			0,			// 6
	0,			0,			0,			0,			0,			0,			0,			0,
	0,			0,			0,			0,			0,			0,			0,			0			// 7
};

/*
==================
MapKey (0x412EA0)
==================
*/
static int MapKey( int key )
{
	int		result;
	int		scan = ( key >> 16 ) & 255;

	if ( scan > 127 )
		return 0;

	result = scantokey[scan];

	if ( key & ( 1 << 24 ) )		// extended key
	{
		switch ( result )
		{
		case 13:			return K_KP_ENTER;
		case '/':			return K_KP_SLASH;
		case K_CAPSLOCK:	return K_KP_PLUS;
		}
	}
	else
	{
		// Without the extended bit the navigation cluster is the keypad.
		switch ( result )
		{
		case K_HOME:		return K_KP_HOME;
		case K_UPARROW:		return K_KP_UPARROW;
		case K_PGUP:		return K_KP_PGUP;
		case K_LEFTARROW:	return K_KP_LEFTARROW;
		case K_RIGHTARROW:	return K_KP_RIGHTARROW;
		case K_END:			return K_KP_END;
		case K_DOWNARROW:	return K_KP_DOWNARROW;
		case K_PGDN:		return K_KP_PGDN;
		case K_INS:			return K_KP_INS;
		case K_DEL:			return K_KP_DEL;
		}
	}

	return result;
}

/*
==================
ClearAllStates (0x412F90)
==================
*/
void ClearAllStates( void )
{
	int		i;

	for ( i = 0; i < 256; ++i )
		engineapi.Key_Event( i, 0 );

	engineapi.Key_ClearStates();
	engineapi.IN_ClearStates();
}

/*
==================
MainWndProc (0x412FC0)
==================
*/
LRESULT CALLBACK MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	LRESULT	lRet = 0;

	if ( uMsg == MM_MCINOTIFY )
		return CDAudio_MessageHandler( hWnd, uMsg, wParam, lParam );

	if ( uMsg == guMouseWheelMsg )
	{
		// Registered MSWHEEL_ROLLMSG (legacy wheel drivers).
		if ( (int)wParam > 0 )
		{
			engineapi.Key_Event( K_MWHEELUP, 1 );
			engineapi.Key_Event( K_MWHEELUP, 0 );
		}
		else
		{
			engineapi.Key_Event( K_MWHEELDOWN, 1 );
			engineapi.Key_Event( K_MWHEELDOWN, 0 );
		}
		return DefWindowProcA( hWnd, uMsg, wParam, lParam );
	}

	// Give the engine's VGUI surface a look at every message.
	if ( engineapi.VGui_CallEngineSurfaceProc )
		engineapi.VGui_CallEngineSurfaceProc( hWnd, uMsg, wParam, lParam );

	switch ( uMsg )
	{
	case WM_CREATE:
		return 0;

	case WM_MOVE:
		window_x = LOWORD( lParam );
		window_y = HIWORD( lParam );
		VID_UpdateWindowStatus();
		return 0;

	case WM_SIZE:
		if ( wParam == SIZE_MINIMIZED )
		{
			// Park the minimized fullscreen window out of the way.
			MoveWindow( hWnd, 0, -20, 0, 20, FALSE );
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT	ps;

			BeginPaint( hWnd, &ps );
			EndPaint( hWnd, &ps );
		}
		return 0;

	case WM_CLOSE:
		if ( !in_mode_set && gDLLState != DLL_CLOSE )
			engineapi.Cbuf_AddText( "quit prompt\n" );
		return 0;

	case WM_ACTIVATEAPP:
		AppActivate( (int)wParam, 0 );
		return 0;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if ( in_mode_set )
			return 0;
		engineapi.Key_Event( MapKey( lParam ), 1 );
		return 0;

	case WM_SYSKEYUP:
		// Alt-Tab / Alt-Esc out of fullscreen: minimize first.
		if ( ( wParam == VK_TAB || wParam == VK_ESCAPE )
		  && ( lParam & ( 1 << 29 ) )			// context code: Alt held
		  && ( HIWORD( lParam ) & KF_UP ) )		// transition: key released
		{
			if ( !gEngineModeWindowed )
				ShowWindow( hWnd, SW_MINIMIZE );
		}
		// fall through to the key-up handling
	case WM_KEYUP:
		if ( in_mode_set )
			return 0;
		engineapi.Key_Event( MapKey( lParam ), 0 );
		return 0;

	case WM_SYSCHAR:
		return 0;

	case WM_SYSCOMMAND:
		if ( wParam == SC_SCREENSAVE || wParam == SC_CLOSE )
			return 0;
		if ( !in_mode_set )
		{
			engineapi.S_BlockSound();
			engineapi.S_ClearBuffer();
		}
		lRet = DefWindowProcA( hWnd, uMsg, wParam, lParam );
		if ( !in_mode_set )
			engineapi.S_UnblockSound();
		return lRet;

	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		{
			int		mstate;

			// In uncaptured-windowed mode the engine does not own the mouse.
			if ( gEngineModeWindowed && !windowed_mouse )
				return DefWindowProcA( hWnd, uMsg, wParam, lParam );

			if ( in_mode_set || g_bBlockMouseEvents )
				return 0;

			mstate = 0;
			if ( wParam & MK_LBUTTON )
				mstate |= 1;
			if ( wParam & MK_RBUTTON )
				mstate |= 2;
			if ( wParam & MK_MBUTTON )
				mstate |= 4;
			if ( wParam & MK_XBUTTON1 )
				mstate |= 8;
			if ( wParam & MK_XBUTTON2 )
				mstate |= 16;

			engineapi.IN_MouseEvent( mstate );
		}
		return 0;

	case WM_MOUSEWHEEL:
		if ( (short)HIWORD( wParam ) > 0 )
		{
			engineapi.Key_Event( K_MWHEELUP, 1 );
			engineapi.Key_Event( K_MWHEELUP, 0 );
		}
		else
		{
			engineapi.Key_Event( K_MWHEELDOWN, 1 );
			engineapi.Key_Event( K_MWHEELDOWN, 0 );
		}
		return 0;
	}

	return DefWindowProcA( hWnd, uMsg, wParam, lParam );
}

/*
==================
SleepUntilInput (0x4134F0)
==================
*/
void SleepUntilInput( int time )
{
	MsgWaitForMultipleObjects( 1, &tevent, FALSE, time, QS_KEY );
}

/*
==================
AppActivate (0x413510)
==================
*/
void AppActivate( int fActive, int minimize )
{
	if ( !in_mode_set && DDActive && !gBackground && g_bEngineWindowUp )
	{
		if ( fActive && ( !gEngineModeWindowed || windowed_mouse ) )
		{
			engineapi.IN_ActivateMouse();
			IN_HideMouse();
		}
		else
		{
			engineapi.IN_DeactivateMouse();
			IN_ShowMouse();
		}
	}

	if ( g_bChangingVideoModes != 1 )
	{
		ActiveApp = fActive;

		if ( fActive )
		{
			if ( gDLLState == DLL_ACTIVE )
				VID_SwitchToEngine();
			CDAudio_Resume();
		}
		else
		{
			if ( gDLLState == DLL_ACTIVE )
				VID_SwitchToLauncher();
			CDAudio_Pause();
		}
	}

	ClearAllStates();
}

/*
==================
VID_GetPrimaryDepth (0x4135B0)
==================
*/
static void VID_GetPrimaryDepth( int* pDXLevel, int* pPlatform )
{
	OSVERSIONINFOA			osvi;
	HINSTANCE				hInst;
	HRESULT					( WINAPI *pfnDirectDrawCreate )( GUID*, LPDIRECTDRAW*, IUnknown* );
	LPDIRECTDRAW			lpDDProbe = NULL;
	LPDIRECTDRAWSURFACE		lpSurf    = NULL;
	IUnknown*				lpQuery2  = NULL;
	IUnknown*				lpQuery3  = NULL;
	IUnknown*				lpQuery4  = NULL;
	DDSURFACEDESC			ddsd;

	osvi.dwOSVersionInfoSize = sizeof( osvi );
	if ( !GetVersionExA( &osvi ) )
	{
		*pDXLevel  = DXLEVEL_NONE;
		*pPlatform = 0;
		return;
	}

	if ( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		*pPlatform = VER_PLATFORM_WIN32_NT;

		if ( osvi.dwMajorVersion < 4 )
		{
			*pPlatform = 0;
			return;
		}

		if ( osvi.dwMajorVersion == 4 )
		{
			// NT 4: DirectInput is as far as the probe goes.
			*pDXLevel = DXLEVEL_DX2;

			hInst = LoadLibraryA( "DINPUT.DLL" );
			if ( !hInst )
			{
				OutputDebugStringA( "Couldn't LoadLibrary DInput\r\n" );
				return;
			}

			if ( GetProcAddress( hInst, "DirectInputCreateA" ) )
			{
				FreeLibrary( hInst );
				*pDXLevel = DXLEVEL_DX3;
			}
			else
			{
				FreeLibrary( hInst );
				OutputDebugStringA( "Couldn't GetProcAddress DInputCreate\r\n" );
			}
			return;
		}
	}
	else
	{
		*pPlatform = VER_PLATFORM_WIN32_WINDOWS;
	}

	hInst = LoadLibraryA( "DDRAW.DLL" );
	if ( !hInst )
	{
		*pDXLevel  = DXLEVEL_NONE;
		*pPlatform = 0;
		FreeLibrary( hInst );		// (sic) the binary frees the null handle
		return;
	}

	pfnDirectDrawCreate = (HRESULT ( WINAPI * )( GUID*, LPDIRECTDRAW*, IUnknown* ))
		GetProcAddress( hInst, "DirectDrawCreate" );
	if ( !pfnDirectDrawCreate )
	{
		*pDXLevel  = DXLEVEL_NONE;
		*pPlatform = 0;
		FreeLibrary( hInst );
		OutputDebugStringA( "Couldn't LoadLibrary DDraw\r\n" );
		return;
	}

	if ( FAILED( pfnDirectDrawCreate( NULL, &lpDDProbe, NULL ) ) )
	{
		*pDXLevel  = DXLEVEL_NONE;
		*pPlatform = 0;
		FreeLibrary( hInst );
		OutputDebugStringA( "Couldn't create DDraw\r\n" );
		return;
	}

	*pDXLevel = DXLEVEL_DX1;

	if ( FAILED( lpDDProbe->lpVtbl->QueryInterface( lpDDProbe, IID_IDirectDraw2, (LPVOID*)&lpQuery2 ) ) )
	{
		lpDDProbe->lpVtbl->Release( lpDDProbe );
		FreeLibrary( hInst );
		OutputDebugStringA( "Couldn't QI DDraw2\r\n" );
		return;
	}
	lpQuery2->lpVtbl->Release( lpQuery2 );
	*pDXLevel = DXLEVEL_DX3;

	memset( &ddsd, 0, sizeof( ddsd ) );
	ddsd.dwSize         = sizeof( ddsd );
	ddsd.dwFlags        = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	if ( FAILED( lpDDProbe->lpVtbl->SetCooperativeLevel( lpDDProbe, NULL, DDSCL_NORMAL ) ) )
	{
		lpDDProbe->lpVtbl->Release( lpDDProbe );
		FreeLibrary( hInst );
		*pDXLevel = DXLEVEL_NONE;
		OutputDebugStringA( "Couldn't Set coop level\r\n" );
		return;
	}

	if ( FAILED( lpDDProbe->lpVtbl->CreateSurface( lpDDProbe, &ddsd, &lpSurf, NULL ) ) )
	{
		lpDDProbe->lpVtbl->Release( lpDDProbe );
		FreeLibrary( hInst );
		*pDXLevel = DXLEVEL_NONE;
		OutputDebugStringA( "Couldn't CreateSurface\r\n" );
		return;
	}

	if ( SUCCEEDED( lpSurf->lpVtbl->QueryInterface( lpSurf, IID_IDirectDrawSurface3, (LPVOID*)&lpQuery3 ) ) )
	{
		lpQuery3->lpVtbl->Release( lpQuery3 );
		*pDXLevel = DXLEVEL_DX5;

		if ( SUCCEEDED( lpSurf->lpVtbl->QueryInterface( lpSurf, IID_IDirectDrawSurface4, (LPVOID*)&lpQuery4 ) ) )
		{
			lpQuery4->lpVtbl->Release( lpQuery4 );
			*pDXLevel = DXLEVEL_DX6;
			lpSurf->lpVtbl->Release( lpSurf );
		}
	}

	lpDDProbe->lpVtbl->Release( lpDDProbe );
	FreeLibrary( hInst );
}

/*
==================
Vid_D3DSupported (0x413900)
==================
*/
int Vid_D3DSupported( void )
{
	int		bCreatedDD = 0;

	if ( !g_bEnoughMem )
		return 0;

	if ( !lpDD )
	{
		bCreatedDD = 1;
		if ( DirectDrawCreate( NULL, &lpDD, NULL ) )
			return 0;
	}

	g_bD3DAvailable = DDraw_QueryD3D();

	if ( glpD3D )
	{
		glpD3D->lpVtbl->Release( glpD3D );
		glpD3D = NULL;
	}
	if ( bCreatedDD )
	{
		lpDD->lpVtbl->Release( lpDD );
		lpDD = NULL;
	}

	return g_bD3DAvailable;
}

/*
==================
Vid_OpenGLSupported (0x413980)
==================
*/
int Vid_OpenGLSupported( void )
{
	return g_bEnoughMem;
}

/*
==================
Vid_TrySetMode (0x413990)
==================
*/
int Vid_TrySetMode( const char* pszDriver, int type, int mode, int w, int h, int bpp )
{
	return 1;
}

/*
==================
D_SurfaceCacheForRes (0x4139A0)

surface-cache bytes needed for a w x h software frame.
==================
*/
int D_SurfaceCacheForRes( int width, int height )
{
	int		size, pix;
	char*	p;

	if ( CheckParm( "-surfcachesize", &p ) && p )
		return atoi( p ) * 1024;

	size = SURFCACHE_SIZE_AT_320X200;
	pix  = 2 * width * height;
	if ( pix > 64000 )
		size += ( pix - 64000 ) * 3;
	return size;
}

/*
==================
VID_CheckAdequateMem (0x4139F0)

does a w x h software frame plus its surface cache leave >= 8 MB free?
==================
*/
static int VID_CheckAdequateMem( int w, int h )
{
	return giMemSize - ( D_SurfaceCacheForRes( w, h ) + 4 * h * w ) >= MINIMUM_MEMORY;
}

#ifdef LAUNCHER_FIXES
/*
==================
LAUNCHER_FIXES: resolutions past the original 4:3 / 1280x960 ceiling

Original behaviour: Vid_BuildModeList seeded the windowed list from a fixed
seven-entry 4:3 probe table ending at 1280x960, and Vid_EnumModeCallback dropped
every DirectDraw mode that was not exactly 4:3 or was taller than 1024.  Since
CVideoModeDlg::RebuildModeList lists only MS_WINDOWED modes for OpenGL and for
the windowed path, that seven-entry table *was* the resolution list for the
renderer anyone actually uses.

With the fix the windowed list is enumerated from the display driver, the
DirectDraw aspect/height filter is dropped, and the 8 MB software-memory floor
is skipped for the hardware renderers it never applied to.  The original list is
still what the software rasteriser gets to choose from.
==================
*/

/*
==================
Vid_ModeAllowedForRenderer (LAUNCHER_FIXES)

Whether CVideoModeDlg should offer this resolution for that renderer.  Only the
software rasteriser is held to the frame-buffer-plus-surface-cache budget, and
only it keeps the original 4:3 shape, so switching to Software still offers
exactly the modes the retail launcher offered.
==================
*/
int Vid_ModeAllowedForRenderer( int vidtype, int w, int h )
{
	if ( vidtype != VT_Software )
		return 1;

	if ( h * 4 != w * 3 )
		return 0;

	return VID_CheckAdequateMem( w, h );
}

/*
==================
Vid_FindMode (LAUNCHER_FIXES)

The row already listing this type at this resolution, or NULL.  Enumeration
reports one mode per depth and refresh rate; the list carries one row per
resolution.
==================
*/
static vmode_t* Vid_FindMode( modestate_t type, int w, int h )
{
	for ( int i = 0; i < nummodes; ++i )
	{
		if ( modelist[i].type == type
		  && modelist[i].width == w && modelist[i].height == h )
			return &modelist[i];
	}
	return NULL;
}

/*
==================
Vid_EnumWindowedModes (LAUNCHER_FIXES)

Add the display driver's own mode list as windowed/DIB rows, one per
resolution, keeping the highest refresh rate each reports.  The original
seven-entry table has already been laid down, so it stays as a floor for
drivers that report an unhelpfully short list, and its rows are found here
rather than duplicated.
==================
*/
static void Vid_EnumWindowedModes( void )
{
	DEVMODE	dm;

	for ( int i = 0; nummodes < MAX_MODE_LIST; ++i )
	{
		memset( &dm, 0, sizeof( dm ) );
		dm.dmSize = sizeof( dm );
		if ( !EnumDisplaySettings( NULL, i, &dm ) )
			break;

		int	w = (int)dm.dmPelsWidth, h = (int)dm.dmPelsHeight;
		int	refresh = (int)dm.dmDisplayFrequency;

		if ( w < 400 || h < 300 )		// the original probe table's floor
			continue;

		vmode_t*	m = Vid_FindMode( MS_WINDOWED, w, h );
		if ( m )
		{
			if ( refresh > m->refresh )
				m->refresh = refresh;
			continue;
		}

		m = &modelist[nummodes];
		m->type       = MS_WINDOWED;
		m->width      = w;
		m->height     = h;
		sprintf( m->modedesc, "%d x %d", w, h );
		m->modenum    = nummodes;
		m->is15bit    = 0;
		m->stretched  = 0;
		m->dib        = 1;
		m->isHardware = 0;
		m->bpp        = g_nDesiredBpp;
		m->nocenter   = 0;
		m->refresh    = refresh;
		nummodes++;
	}
}
#endif	// LAUNCHER_FIXES

/*
==================
Vid_EnumModeCallback (0x413A20)

DirectDraw EnumDisplayModes callback -- accept 4:3 modes up to 1024
tall that match the desired depth and fit memory, adding them as hardware modes.
==================
*/
static HRESULT WINAPI Vid_EnumModeCallback( LPDDSURFACEDESC d, LPVOID ctx )
{
	int	h   = (int)d->dwHeight;
	int	w   = (int)d->dwWidth;
	int	bpp = (int)d->ddpfPixelFormat.dwRGBBitCount;

#ifdef LAUNCHER_FIXES
	// The aspect and height limits are what capped the fullscreen list at
	// 1280x960; VID_CheckAdequateMem below still keeps the software rasteriser
	// inside its frame-buffer budget.
	if ( nummodes >= MAX_MODE_LIST )
		return DDENUMRET_CANCEL;
	if ( Vid_FindMode( MS_FULLSCREEN, w, h ) )
		return DDENUMRET_OK;	// one row per resolution, not one per refresh rate
#else
	if ( (double)h / (double)w != 0.75 || h > 1024 )
		return DDENUMRET_OK;
#endif

	int	bppOk = ( g_nDesiredBpp == 16 ) ? ( bpp == 15 || bpp == 16 )
	                                      : ( bpp == g_nDesiredBpp );
	if ( bppOk && w >= 400 && VID_CheckAdequateMem( w, h ) )
	{
		vmode_t*	m = &modelist[nummodes];
		m->type       = MS_FULLSCREEN;
		m->width      = w;
		m->height     = h;
		sprintf( m->modedesc, "%d x %d", w, h );
		m->modenum    = nummodes;
		m->is15bit    = ( d->ddpfPixelFormat.dwGBitMask == 0x3E0 );	// RGB555 green mask
		m->stretched  = 0;
		m->dib        = 0;
		m->isHardware = 1;
		m->bpp        = bpp;
		m->nocenter   = 0;
		nummodes++;
	}
	return DDENUMRET_OK;
}

/*
==================
Vid_CompareModes (0x413B50)

by width, then height (ascending).
==================
*/
static int Vid_CompareModes( const void* pa, const void* pb )
{
	const vmode_t*	a = (const vmode_t*)pa;
	const vmode_t*	b = (const vmode_t*)pb;

	if ( a->width != b->width )
		return a->width < b->width ? -1 : 1;
	if ( a->height != b->height )
		return a->height < b->height ? -1 : 1;
	return 0;
}

/*
==================
Vid_BuildModeList (0x413B90)

software-DIB modes from a fixed 4:3 probe table, then the DirectDraw
hardware modes; sort and pick the current mode.
==================
*/
void Vid_BuildModeList( void )
{
	static const int	s_probe[7][2] =		// (0x4CEF70) 4:3 software resolutions
	{
		{ 400, 300 }, { 512, 384 }, { 640, 480 }, { 800, 600 },
		{ 1024, 768 }, { 1152, 864 }, { 1280, 960 }
	};

	if ( nummodes )		// already built
		return;

	g_nDesiredBpp = 16;
	if ( CheckParm( "-24bpp", NULL ) )
		g_nDesiredBpp = 24;
	else if ( CheckParm( "-32bpp", NULL ) )
		g_nDesiredBpp = 32;

	// Software-DIB modes that fit memory.
	for ( int i = 0; i < 7; ++i )
	{
		int	w = s_probe[i][0], h = s_probe[i][1];
#ifndef LAUNCHER_FIXES
		if ( !VID_CheckAdequateMem( w, h ) )
			continue;
#endif
		vmode_t*	m = &modelist[nummodes];
		m->type       = MS_WINDOWED;
		m->width      = w;
		m->height     = h;
		sprintf( m->modedesc, "%d x %d", w, h );
		m->modenum    = nummodes;
		// The binary does not write is15bit (+16) in this loop, only in
		// Vid_EnumModeCallback; modelist is in .bss, so it is already zero.
		m->stretched  = 0;
		m->dib        = 1;
		m->isHardware = 0;
		m->bpp        = g_nDesiredBpp;
		m->nocenter   = 0;
		nummodes++;
	}

#ifdef LAUNCHER_FIXES
	// Everything the display driver will actually set, so the windowed/OpenGL
	// list is not limited to the seven modes above.
	Vid_EnumWindowedModes();
#endif

	// DirectDraw hardware modes (NORMAL coop level at the menu -- see DDraw_Init).
	if ( !lpDD && !DDraw_Init( 1, 0 ) )
	{
		nummodes = 0;
		return;
	}
	// Not in the binary: DDraw_Init has just succeeded, so it calls through lpDD
	// unconditionally.
	if ( lpDD )
		lpDD->lpVtbl->EnumDisplayModes(
			lpDD, 0, NULL, NULL, Vid_EnumModeCallback );

	// Sort by width then height.
	qsort( modelist, nummodes, sizeof( vmode_t ), Vid_CompareModes );

	// Pick the current mode: the desired vidtype + resolution, falling back to the
	// first width match, else mode 0.
	int	wantType = ( gEngineModeWindowed || gEngineVidType == VT_OpenGL )
		? MS_WINDOWED : MS_FULLSCREEN;
	int	w = g_EngineMode.width, h = g_EngineMode.height;
	char*	p;
	if ( CheckParm( "-width",  &p ) && p ) w = atoi( p );
	if ( CheckParm( "-w",      &p ) && p ) w = atoi( p );
	if ( CheckParm( "-height", &p ) && p ) h = atoi( p );
	if ( CheckParm( "-h",      &p ) && p ) h = atoi( p );

	int	found = -1, i;
	for ( i = 0; i < nummodes; ++i )
	{
		if ( modelist[i].type == wantType && modelist[i].width == w )
		{
			found = i;
			if ( modelist[i].height == h && modelist[i].bpp == g_EngineMode.bpp )
				break;
		}
	}
	int	sel = ( i == nummodes ) ? ( found != -1 ? found : 0 ) : i;

	g_EngineMode.width  = w;
	g_EngineMode.height = h;
	g_EngineMode.bpp    = g_nDesiredBpp;
	g_currentMode       = sel;
	g_EngineMode.mode   = sel;

#ifdef LAUNCHER_FIXES
	LOG( "%d modes, selected %d = %dx%d", nummodes, sel, w, h );
	for ( i = 0; i < nummodes; ++i )
		LOG( "  mode %2d  %-11s %4dx%-4d %2d bpp %3d hz",
			i,
			modelist[i].type == MS_WINDOWED   ? "windowed"
			  : modelist[i].type == MS_FULLSCREEN ? "fullscreen" : "fulldib",
			modelist[i].width, modelist[i].height,
			modelist[i].bpp, modelist[i].refresh );
#endif
}

/*
==================
IN_ShowMouse / IN_HideMouse

cursor show/hide, paired with the engine's IN_Activate/DeactivateMouse from
AppActivate (the WinQuake in_win.c lineage).  Both fold onto NullStub, and MSVC
cross-jumped AppActivate's two identical calls into one, so the single call site
there stands for both.
==================
*/
void IN_ShowMouse( void )
{
}

void IN_HideMouse( void )
{
}
