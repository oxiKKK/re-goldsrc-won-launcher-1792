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
// Purpose: the launcher's video surface callbacks (exefuncs_t VID_*).
//
// $NoKeywords: $
//=============================================================================

#define CINTERFACE
#include <windows.h>
#include <string.h>
#include <ddraw.h>
#include <d3d.h>
#include "d3d_structs.h"

#include "vid.h"
#include "modes.h"
#include "launcher.h"
#include "engine.h"
#include "strings.h"
#include "cd_win.h"
#include "resource.h"
#include "viddef.h"
#include "gameui.h"

#ifndef LAUNCHER_FIXES
#define MAX_MODE_LIST			50
#endif	// LAUNCHER_FIXES defines it in viddef.h instead, so the mode probe can see it
#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)
#define DEVMODE_SIZE_WIN95		148

static int  VID_SetModeDisplaySettings( int mode );
static void VID_CenterWindow( HWND hWindow, int width, int height, int bNoCenter );

HDC				g_hdcSection;		// 0x4F93A8 DIB-section DC for the windowed blit
static HBITMAP	dibSection;			// 0x4F93A4 the windowed-software DIB section
static void*	dibBase;			// 0x4F93A0 its pixel base

struct viddef_s	vid;				// 0x4E6D60

HDC				dibdc;				// 0x4F93AC GDI DC while locked (if any)
int				DDActive;			// 0x4EA8E4 DirectDraw surface is up
int				lockcount;			// 0x4EA8E8
int				g_bLastMouseActive;	// 0x4E6E1C

int				in_mode_set;		// 0x4E6DF4
int				g_bEngineWindowUp;	// 0x4F93B0 true when engine window is showing

// Video-mode table
vmode_t			modelist[MAX_MODE_LIST]; // 0x4E6E30
int				nummodes = 0;		// 0x4EA8C8 number of enumerated modes
HWND			gLauncherWnd = NULL;	// 0x4EA8DC the launcher (dialog) window

// Engine window placement, mirrored to the engine via VID_UpdateWindowVars.
int				window_x;			// 0x4E6DBC
int				window_y;			// 0x4E6DB8
int				window_width;		// 0x4E6DB4
int				window_height;		// 0x4E6DB0
RECT			window_rect;		// 0x4E6DC8

// The surface the engine renders into, filled by surface creation.
int				DIBHeight;			// 0x4E6DC0
int				DIBWidth;			// 0x4E6DC4
RECT			WindowRect;			// 0x4E6DD8 current render rect
int				vid_stretched;		// 0x4E6DE8 render at half resolution
int				g_currentMode;		// 0x4E6E10 selected mode

// Selected vs running mode/renderer state owned here.
int				g_activeMode;		// 0x4E6E20 mode the engine window is running in
int				g_bVidGL;			// 0x4E6E00 renderer flags, see Vid_SetRendererFlags
int				g_bVidD3D;			// 0x4E6E04
int				g_bVidTypeChanged;	// 0x4E6DFC set by Eng_StartupEngine on renderer change
int				force_mode_set;		// 0x4E1ECC one-shot forced restart
int				g_bActiveModeWindowed;	// 0x4EA8D0 windowed state the engine is running with

static int		scr_disabled_for_loading;	// 0x4E6DEC reentrancy guard around the mode apply
int				force_minimized;	// 0x4E6DF0
static int		msg_suppress_1 = 0;	// 0x4E6DF8 suppress the mode-description print

// The "please wait" DIB header for the windowed software surface.
// BI_BITFIELDS puts three DWORD masks where the 256-entry palette would sit,
// so the tail is sized for the palette and only the first three are written.
static struct
{
	BITMAPINFOHEADER	header;
	DWORD				acolors[256];
} dibHeader;						// 0x4F8F78

D3DGLOBALS		d3dG;				// 0x4E1EA0 shared with the engine

/*
==================
VID_DestroyDIB (0x463740)
==================
*/
void VID_DestroyDIB( void )
{
	if ( g_hdcSection )
	{
		DeleteDC( g_hdcSection );
		g_hdcSection = NULL;
	}

	if ( dibSection )
	{
		DeleteObject( dibSection );
		dibSection = NULL;
		dibBase = NULL;
	}

	if ( dibdc )
	{
		ReleaseDC( mainwindow, dibdc );
		dibdc = NULL;
	}
}

/*
==================
VID_CreateWindowedSurface (0x4637A0)
==================
*/
static int VID_CreateWindowedSurface( int mode )
{
	HDC		dc;
	HFONT	font;
	HGDIOBJ	oldFont;
	MSG		msg;
	int		stretched;

	DDraw_Shutdown();

	stretched = modelist[mode].stretched;
	DIBWidth = modelist[mode].width;
	DIBHeight = modelist[mode].height;

	WindowRect.left = 0;
	WindowRect.top = 0;
	WindowRect.right = DIBWidth << stretched;
	WindowRect.bottom = DIBHeight << stretched;

	if ( !gEngineModeWindowed )
		VID_SetModeDisplaySettings( mode );

	AdjustWindowRectEx( &WindowRect, WindowStyle, FALSE, 0 );
	SetWindowPos( mainwindow, NULL, 0, 0, WindowRect.right - WindowRect.left,
		WindowRect.bottom - WindowRect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW );
	VID_CenterWindow( mainwindow, WindowRect.right - WindowRect.left,
		WindowRect.bottom - WindowRect.top, modelist[mode].nocenter );

	// Paint the "starting up" notice while the renderer comes up.
	dc = GetDC( mainwindow );
	PatBlt( dc, 0, 0, WindowRect.right, WindowRect.bottom, BLACKNESS );

	font = CreateFontA( -15, 0, 0, 0, FW_HEAVY, TRUE, FALSE, FALSE,
		ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
		VARIABLE_PITCH, "Arial" );

	SetTextColor( dc, RGB( 255, 180, 53 ) );
	SetBkMode( dc, TRANSPARENT );
	oldFont = SelectObject( dc, font );

	DrawTextA( dc, Launcher_LoadString( IDS_LOADING ), -1, &WindowRect,
		DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );

	SelectObject( dc, oldFont );
	DeleteObject( font );
	ReleaseDC( mainwindow, dc );
	UpdateWindow( mainwindow );

	// Let the paint/resize messages through before the engine takes over.
	while ( PeekMessageA( &msg, NULL, 0, 0, PM_REMOVE ) )
	{
		TranslateMessage( &msg );
		DispatchMessageA( &msg );
	}

	vid.height = vid.conheight = DIBHeight;
	vid.width = vid.conwidth = DIBWidth;
	vid.buffer = NULL;
	vid.conrowbytes = 0;
	vid.rowbytes = 0;
	vid.numpages = 1;
	vid.bits = modelist[mode].bpp;
	vid.maxwarpwidth = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.aspect = ( (float)vid.height / (float)vid.width ) * ( 320.0f / 240.0f );

	vid_stretched = modelist[mode].stretched;

	if ( gEngineModeWindowed && gEngineVidType == VT_Software )
	{
		memset( &dibHeader, 0, sizeof( dibHeader ) );

		if ( !dibdc )
		{
			dibdc = GetDC( mainwindow );
			if ( !dibdc )
				return 0;
		}

		dibHeader.header.biSize = sizeof( BITMAPINFOHEADER );
		dibHeader.header.biWidth = vid.width;
		dibHeader.header.biHeight = vid.height;
		dibHeader.header.biPlanes = 1;
		dibHeader.header.biBitCount = 16;
		dibHeader.header.biCompression = BI_BITFIELDS;
		dibHeader.header.biSizeImage = 0;
		dibHeader.header.biXPelsPerMeter = 0;
		dibHeader.header.biYPelsPerMeter = 0;
		dibHeader.header.biClrUsed = 0;
		dibHeader.header.biClrImportant = 0;

		// 565 masks.
		dibHeader.acolors[0] = 0xF800;
		dibHeader.acolors[1] = 0x07E0;
		dibHeader.acolors[2] = 0x001F;

		dibSection = CreateDIBSection( dibdc, (const BITMAPINFO*)&dibHeader,
			DIB_RGB_COLORS, &dibBase, NULL, 0 );
		if ( !dibSection )
		{
			VID_DestroyDIB();
			return 0;
		}

		vid.buffer = (pixel_t*)dibBase;
		vid.is15bit = 0;
		vid.rowbytes = 2 * vid.width;
		vid.conrowbytes = 2 * vid.width;
		memset( dibBase, 255, vid.height * vid.width );

		g_hdcSection = CreateCompatibleDC( dibdc );
		if ( !g_hdcSection )
		{
			VID_DestroyDIB();
			return 0;
		}

		SelectObject( g_hdcSection, dibSection );
		vid.direct = vid.buffer;
		vid.conbuffer = vid.buffer;
	}

#ifdef LAUNCHER_RE
	LOG( "VID_CreateWindowedSurface: dibRan=%d vidtype=%d windowed=%d buffer=%p %dx%d rowbytes=%d",
		( gEngineModeWindowed && gEngineVidType == VT_Software ), gEngineVidType,
		gEngineModeWindowed, (void*)vid.buffer, vid.width, vid.height, vid.rowbytes );
#endif

	return 1;
}

/*
==================
VID_CenterWindow (0x463C10)
==================
*/
static void VID_CenterWindow( HWND hWindow, int width, int height, int bNoCenter )
{
	int		x, y;

	if ( bNoCenter )
	{
		x = 0;
		y = 0;
	}
	else
	{
		x = ( GetSystemMetrics( SM_CXSCREEN ) - width ) / 2;
		y = ( GetSystemMetrics( SM_CYSCREEN ) - height ) / 2;
		if ( x < 0 )
			x = 0;
		if ( y < 0 )
			y = 0;
	}

	window_x = x;
	window_y = y;

	SetWindowPos( hWindow, NULL, x, y, 0, 0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW );
}

/*
==================
VID_SetMode (0x463C80)
==================
*/
HWND VID_SetMode( int mode )
{
	if ( mode == NO_MODE )
	{
		vid_modenum = NO_MODE;
		return NULL;
	}

	g_currentMode = mode;

	if ( !g_bVidTypeChanged && !VID_RestartNeeded() )
		return mainwindow;

	if ( gEngineVidType == VT_Direct3D || gEngineVidType == VT_OpenGL )
		return Eng_Load( "hw.dll", 1 ) == 0 ? mainwindow : NULL;
	else
		return Eng_Load( "sw.dll", 1 ) == 0 ? mainwindow : NULL;
}

/*
==================
VID_ApplyMode (0x463CF0)
==================
*/
HWND VID_ApplyMode( void )
{
	int		temp;
	int		bSurfaceCreated;

	vid_modenum = g_currentMode;
	g_bActiveModeWindowed = gEngineModeWindowed;

	temp = scr_disabled_for_loading;
	scr_disabled_for_loading = 1;
	in_mode_set = 1;

	engineapi.Snd_ReleaseBuffer();
	CDAudio_SwitchToLauncher();

	if ( !gBackground )
	{
		if ( gEngineVidType == VT_OpenGL || gEngineModeWindowed )
		{
			bSurfaceCreated = VID_CreateWindowedSurface( g_currentMode );
		}
		else
		{
			VID_DestroyDIB();
			bSurfaceCreated = DDraw_CreateSurfaces( g_currentMode, gEngineVidType != VT_Direct3D );
		}

		engineapi.IN_ActivateMouse();

		if ( !bSurfaceCreated )
			return NULL;
	}

	vid.vidtype = gEngineVidType;
	window_width = vid.width << vid_stretched;
	window_height = vid.height << vid_stretched;
	VID_UpdateWindowStatus();

	CDAudio_SwitchToEngine();
	engineapi.Snd_AcquireBuffer();

	scr_disabled_for_loading = temp;

// now we try to make sure we get the focus on the mode switch, because sometimes
// in some systems we don't.
	if ( !force_minimized && !gBackground )
		SetForegroundWindow( mainwindow );

	if ( !gBackground && ( vid.vidtype == VT_OpenGL || vid.vidtype == VT_Direct3D ) )
	{
		if ( gEngineVidType == VT_Direct3D && lpDD )
		{
			lpDD->lpVtbl->SetCooperativeLevel( lpDD, NULL, DDSCL_NORMAL );

			d3dG.lpDD4 = glpDD4;
			d3dG.lpD3D = (LPDIRECT3D3)glpD3D;
			d3dG.bFullscreen = TRUE;
			d3dG.bSecondary = TRUE;
			engineapi.QGL_D3DShared( &d3dG );
		}

		// A faulting driver fails the mode apply instead of taking the process
		// down with it.
		__try
		{
			if ( !engineapi.GL_SetMode( mainwindow, &g_maindc, &g_baseRC,
					vid.vidtype == VT_Direct3D, Launcher_GetGLDriver(), gpszCmdLine ) )
				return NULL;

			engineapi.GL_Init();
		}
		__except ( EXCEPTION_EXECUTE_HANDLER )
		{
			return NULL;
		}
	}

	if ( !force_minimized && !gBackground )
	{
		SetWindowPos( mainwindow, NULL, 0, 0, 0, 0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOCOPYBITS );
		SetForegroundWindow( mainwindow );
	}

// fix the leftover Alt from any Alt-Tab or the like that switched us away
	ClearAllStates();

	if ( !msg_suppress_1 )
		engineapi.Con_SafePrintf( "%s\n", VID_GetModeDescription( vid_modenum ) );

	in_mode_set = 0;
	vid.recalc_refdef = 1;

	if ( !gBackground )
		DDActive = 1;

	g_activeMode = g_currentMode;

	if ( gBackground )
	{
		// The background (menu) map runs behind the launcher shell: free the
		// cursor for it.
		ClipCursor( NULL );
		while ( ShowCursor( TRUE ) < 0 )
			;
	}

	return mainwindow;
}

/*
==================
VID_RestartNeeded (0x463FE0)
==================
*/
int VID_RestartNeeded( void )
{
	if ( force_mode_set )
	{
		force_mode_set = 0;
		return 1;
	}

	if ( g_currentMode != g_activeMode )
		return 1;

	if ( vid.vidtype != gEngineVidType )
		return 1;

	if ( gEngineModeWindowed != g_bActiveModeWindowed )
		return 1;

	// Fullscreen non-GL without a DirectDraw object: the surface is gone.
	if ( gEngineVidType != VT_OpenGL && !gEngineModeWindowed && !lpDD )
		return 1;

	// Windowed software rendering without its DIB section yet.
	if ( gEngineVidType == VT_Software && gEngineModeWindowed && !dibdc )
		return 1;

	return 0;
}

/*
==================
VID_ChangeDisplaySettings (0x464070)
==================
*/
int VID_ChangeDisplaySettings( int width, int height, int bpp )
{
	DEVMODEA	dm;

	memset( &dm, 0, sizeof( dm ) );

	// The Win95-era DEVMODE, before dmPanningWidth/Height; the binary hardcodes
	// it and a modern SDK's sizeof( DEVMODEA ) is 8 bytes larger.
	dm.dmSize = DEVMODE_SIZE_WIN95;
	dm.dmPelsWidth = width;
	dm.dmPelsHeight = height;
	dm.dmBitsPerPel = bpp;
	dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	return ChangeDisplaySettingsA( &dm, CDS_FULLSCREEN ) == DISP_CHANGE_SUCCESSFUL;
}

#ifdef LAUNCHER_FIXES
/*
==================
LAUNCHER_FIXES: fullscreen that fills the screen

Original behaviour: "fullscreen" (EngineModeWindowed=0) means the launcher pulls
the display down to LauncherWidth x LauncherHeight x 16 -- 640x480x16 -- and then
puts its frameless 640x480 dialog at (0,0).  At 640x480 the dialog *is* the
screen, so the corner is the whole picture, and the monitor does the enlarging.

Windows 8 dropped 16-bpp modes, so the switch fails -- silently, because the
original's "Couldn't change modes" complaint is one of the folded nullstubs --
and what is left is a 640x480 window in the corner of a 4K desktop.

The fix keeps the design and repairs the mode switch:

  * the depth comes from the desktop instead of being nailed to 16 bpp, and the
    mode is picked out of the driver's own list rather than assumed, so the
    shell gets the smallest real mode it fits in and the display scaler blows it
    up -- the whole viewport magnified, with no layout touched;

  * the exe is deliberately manifest-less, so the AppCompat DPI shim decides
    what GetSystemMetrics reports.  A mode is only accepted once it is set and
    the metrics come back no smaller than the skin, or the shim would stretch a
    640x480 window past the edges of a 640x480 mode;

  * if the driver offers nothing small enough -- laptop panels often start at
    1024x768 -- there is no mode to switch to, and the shell is centred on a
    black backdrop covering the desktop instead of sitting in its corner.
==================
*/

// The size the skin is drawn for; a mode smaller than this cannot hold it.
#define SHELL_MIN_W		640
#define SHELL_MIN_H		480

static int	s_shellModeW;			// the mode the shell runs in, 0 = none found
static int	s_shellModeH;
static int	s_savedW;				// the shell size to go back to
static int	s_savedH;

/*
==================
Shell_FullscreenActive
==================
*/
int Shell_FullscreenActive( void )
{
	return !gEngineModeWindowed;
}

/*
==================
Shell_BuildCandidates

The modes worth trying, smallest first: the driver's own list at the desktop's
depth, no smaller than the skin.  Ties in area go to the shape closest to the
desktop's, so the panel scales rather than letterboxes.

More than one is kept because a mode is only known to be usable once it has been
set: the exe is manifest-less, so the DPI shim decides what GetSystemMetrics
reports, and a mode whose virtual screen comes back smaller than the skin has to
be stepped over.  In practice the first candidate is the one that sticks --
Windows runs small modes at 100% -- so the display switches once.
==================
*/
#define SHELL_MAX_CANDIDATES	8

static int	s_candW[SHELL_MAX_CANDIDATES];
static int	s_candH[SHELL_MAX_CANDIDATES];
static int	s_nCandidates;

static void Shell_BuildCandidates( int wWanted, int hWanted )
{
	DEVMODEA	dm, cur;
	double		aspect;
	int			i, j;

	s_nCandidates = 0;

	memset( &cur, 0, sizeof( cur ) );
	cur.dmSize = sizeof( cur );
	if ( !EnumDisplaySettingsA( NULL, ENUM_CURRENT_SETTINGS, &cur ) )
		return;

	aspect = (double)cur.dmPelsWidth / (double)cur.dmPelsHeight;

	for ( i = 0; ; ++i )
	{
		int	w, h;

		memset( &dm, 0, sizeof( dm ) );
		dm.dmSize = sizeof( dm );
		if ( !EnumDisplaySettingsA( NULL, i, &dm ) )
			break;

		if ( dm.dmBitsPerPel != cur.dmBitsPerPel )
			continue;

		w = (int)dm.dmPelsWidth;
		h = (int)dm.dmPelsHeight;
		if ( w < wWanted || h < hWanted )
			continue;

		// One row per resolution, in order.
		for ( j = 0; j < s_nCandidates; ++j )
		{
			if ( s_candW[j] == w && s_candH[j] == h )
				break;

			if ( w * h < s_candW[j] * s_candH[j] )
				break;

			if ( w * h == s_candW[j] * s_candH[j] )
			{
				double	skew    = (double)w / (double)h - aspect;
				double	skewHad = (double)s_candW[j] / (double)s_candH[j] - aspect;

				if ( skew < 0.0 )    skew = -skew;
				if ( skewHad < 0.0 ) skewHad = -skewHad;
				if ( skew < skewHad )
					break;
			}
		}

		if ( j < s_nCandidates && s_candW[j] == w && s_candH[j] == h )
			continue;

		if ( s_nCandidates >= SHELL_MAX_CANDIDATES )
		{
			if ( j >= SHELL_MAX_CANDIDATES )
				continue;
		}
		else
		{
			s_nCandidates++;
		}

		// Open the slot and drop it in.
		{
			int	k;

			for ( k = s_nCandidates - 1; k > j; --k )
			{
				s_candW[k] = s_candW[k - 1];
				s_candH[k] = s_candH[k - 1];
			}
			s_candW[j] = w;
			s_candH[j] = h;
		}
	}
}

/*
==================
Shell_SetMode
==================
*/
static int Shell_SetMode( int w, int h )
{
	DEVMODEA	dm;

	memset( &dm, 0, sizeof( dm ) );
	dm.dmSize       = sizeof( dm );
	dm.dmPelsWidth  = w;
	dm.dmPelsHeight = h;
	dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

	return ChangeDisplaySettingsExA( NULL, &dm, NULL, CDS_FULLSCREEN, NULL )
			== DISP_CHANGE_SUCCESSFUL;
}

/*
==================
Shell_ComputeMetrics

Settle the shell's mode and its window size before the dialog is built.  With a
mode to switch to the window is the screen; without one it keeps the size the
skin was drawn for and Shell_Backdrop fills in around it.
==================
*/
void Shell_ComputeMetrics( void )
{
	char*	pszValue;
	int		w, h;

	s_shellModeW = s_shellModeH = 0;

	// The windowed size, to come back to.
	if ( !s_savedW )
	{
		s_savedW = g_nLauncherDefW;
		s_savedH = g_nLauncherDefH;
	}

	if ( !Shell_FullscreenActive() )
		return;

	// LauncherWidth/Height are read and written by SetEngineMode and used
	// nowhere in the original; they are the requested shell mode here.
	w = Launcher_GetProfileInt( "Settings", "LauncherWidth",  SHELL_MIN_W );
	h = Launcher_GetProfileInt( "Settings", "LauncherHeight", SHELL_MIN_H );

	if ( CheckParm( "-fsmode", &pszValue ) && pszValue )
	{
		int	fw = 0, fh = 0;

		if ( sscanf( pszValue, "%dx%d", &fw, &fh ) == 2 && fw > 0 && fh > 0 )
		{
			w = fw;
			h = fh;
		}
	}

	if ( w < SHELL_MIN_W )
		w = SHELL_MIN_W;
	if ( h < SHELL_MIN_H )
		h = SHELL_MIN_H;

	Shell_BuildCandidates( w, h );
	if ( !s_nCandidates )
	{
		LOG( "no mode at %dx%d or better -- backdrop fallback", w, h );
		return;
	}

	// Switch now, before the first window is sized: g_nLauncherDefW/H come out
	// of the metrics the new mode leaves behind.
	Shell_EnterFullscreen();
}

/*
==================
Shell_EnterFullscreen

Put the display into the shell's mode and take the window size from the metrics
afterwards, so the DPI shim's idea of the screen is the one the skin is laid out
in.  Called at startup and again every time the engine hands the display back.
==================
*/
int Shell_EnterFullscreen( void )
{
	int	i;

	if ( !Shell_FullscreenActive() )
		return 0;

	// A mode that has already proved itself is simply re-asserted; this is the
	// path taken every time the engine hands the display back.
	if ( s_shellModeW )
	{
		if ( !Shell_SetMode( s_shellModeW, s_shellModeH ) )
			return 0;

		g_nLauncherDefW = GetSystemMetrics( SM_CXSCREEN );
		g_nLauncherDefH = GetSystemMetrics( SM_CYSCREEN );
		return 1;
	}

	for ( i = 0; i < s_nCandidates; ++i )
	{
		if ( !Shell_SetMode( s_candW[i], s_candH[i] ) )
		{
			LOG( "mode %dx%d refused", s_candW[i], s_candH[i] );
			continue;
		}

		// What the shim reports is what the skin has to fit in.
		if ( GetSystemMetrics( SM_CXSCREEN ) < SHELL_MIN_W
		  || GetSystemMetrics( SM_CYSCREEN ) < SHELL_MIN_H )
		{
			LOG( "mode %dx%d virtualised to %dx%d -- too small",
				s_candW[i], s_candH[i],
				GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ) );
			continue;
		}

		s_shellModeW    = s_candW[i];
		s_shellModeH    = s_candH[i];
		g_nLauncherDefW = GetSystemMetrics( SM_CXSCREEN );
		g_nLauncherDefH = GetSystemMetrics( SM_CYSCREEN );

		LOG( "shell fullscreen mode %dx%d, shell %dx%d",
			s_shellModeW, s_shellModeH, g_nLauncherDefW, g_nLauncherDefH );
		return 1;
	}

	LOG( "no usable mode -- backdrop fallback" );
	ChangeDisplaySettingsExA( NULL, NULL, NULL, 0, NULL );
	return 0;
}

/*
==================
Shell_LeaveFullscreen

Hand the display back -- minimised, gone windowed, or on the way out -- and put
the shell back at the size it had before it owned the screen.  The mode it found
is kept, so coming back is a re-assert rather than another search.
==================
*/
void Shell_LeaveFullscreen( void )
{
	if ( !s_shellModeW )
		return;

	ChangeDisplaySettingsExA( NULL, NULL, NULL, 0, NULL );
	Shell_ShowBackdrop( 0 );

	if ( s_savedW )
	{
		g_nLauncherDefW = s_savedW;
		g_nLauncherDefH = s_savedH;
	}
}

/*
==================
Shell_HasFullscreenMode
==================
*/
int Shell_HasFullscreenMode( void )
{
	return s_shellModeW != 0;
}

#define SHELL_BACKDROP_CLASS	"HL1792ReShellBackdrop"

static HWND	s_hBackdrop;

/*
==================
Shell_ShowBackdrop

The fallback when the driver has no mode small enough: a black popup over the
whole desktop, sitting directly under the launcher, so a centred 640x480 shell
reads as fullscreen instead of as a window in the corner.
==================
*/
void Shell_ShowBackdrop( int bShow )
{
	if ( !bShow )
	{
		if ( s_hBackdrop )
			ShowWindow( s_hBackdrop, SW_HIDE );
		return;
	}

	if ( !s_hBackdrop )
	{
		static int	s_bClassRegistered;

		if ( !s_bClassRegistered )
		{
			WNDCLASSA	wc;

			memset( &wc, 0, sizeof( wc ) );
			wc.lpfnWndProc   = DefWindowProcA;
			wc.hInstance     = gLauncherHandle;
			wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
			wc.lpszClassName = SHELL_BACKDROP_CLASS;

			if ( !RegisterClassA( &wc ) )
				return;

			s_bClassRegistered = 1;
		}

		s_hBackdrop = CreateWindowExA( WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
			SHELL_BACKDROP_CLASS, SHELL_BACKDROP_CLASS, WS_POPUP,
			0, 0, 0, 0, NULL, NULL, gLauncherHandle, NULL );

		if ( !s_hBackdrop )
			return;
	}

	SetWindowPos( s_hBackdrop, gLauncherWnd, 0, 0,
		GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ),
		SWP_NOACTIVATE | SWP_SHOWWINDOW );
}
#endif	// LAUNCHER_FIXES

/*
==================
VID_SetModeDisplaySettings (0x4640D0)
==================
*/
static int VID_SetModeDisplaySettings( int mode )
{
	return VID_ChangeDisplaySettings( modelist[mode].width,
		modelist[mode].height, modelist[mode].bpp );
}

/*
==================
VID_SwitchToLauncher (0x464100)
==================
*/
void VID_SwitchToLauncher( void )
{
	engineapi.S_BlockSound();
	engineapi.S_ClearBuffer();

	if ( lpDD )
		DDraw_ReleaseSurfaces();

	if ( !gEngineModeWindowed )
	{
		ShowWindow( mainwindow, SW_HIDE );
		ShowWindow( gLauncherWnd, SW_MINIMIZE );

		if ( lpDD )
			lpDD->lpVtbl->RestoreDisplayMode( lpDD );
		else
			ChangeDisplaySettingsA( NULL, 0 );
	}

	ClipCursor( NULL );
	ReleaseCapture();
	engineapi.IN_DeactivateMouse();

	engineapi.SetMessagePumpDisableMode( gEngineModeWindowed == 0 );
}

/*
==================
VID_SwitchToEngine (0x464190)
==================
*/
void VID_SwitchToEngine( void )
{
	ShowWindow( mainwindow, SW_SHOWNORMAL );
	UpdateWindow( mainwindow );

	if ( lpDD )
		DDraw_Init( 1, 0 );

	if ( !gEngineModeWindowed )
	{
		if ( !lpDD || DDraw_CreateSurfaces( g_currentMode, g_bVidD3D == 0 ) )
			VID_SetModeDisplaySettings( g_currentMode );
		else
			Launcher_ShowMessageById( 0, IDS_DDRAW_RESTOREMODEFAIL );

		MoveWindow( mainwindow, 0, 0, modelist[g_activeMode].width,
			modelist[g_activeMode].height, FALSE );
	}

	VID_SetMode( g_currentMode );
	engineapi.VID_UpdateVID( &vid );
	// IN_HideMouse() -- folded nullstub

	if ( windowed_mouse )
	{
		ReleaseCapture();
		SetCapture( mainwindow );
		GetWindowRect( mainwindow, &window_rect );
		ClipCursor( &window_rect );
		engineapi.IN_ActivateMouse();
	}
	else
	{
		engineapi.IN_DeactivateMouse();
		ClipCursor( NULL );
		ReleaseCapture();
	}

	engineapi.S_UnblockSound();
	engineapi.Keyboard_ReturnToGame();
	engineapi.SetMessagePumpDisableMode( 0 );

	// Fullscreen without a DirectDraw object: fall back to plain display-mode
	// switches (800x600 first to force a change, then the desktop default).
	if ( !gEngineModeWindowed && !lpDD )
	{
		// LAUNCHER_FIXES: the two steps down to the shell's own mode are a
		// detour on the way to the engine's, and the 16-bpp request they use
		// cannot succeed any more anyway.
#ifndef LAUNCHER_FIXES
		VID_ChangeDisplaySettings( 800, 600, 16 );
		VID_ChangeDisplaySettings( g_nLauncherDefW, g_nLauncherDefH, 16 );
#endif
		VID_SetModeDisplaySettings( g_currentMode );
	}
}

/*
==================
VID_HideEngineWindow (0x464320)
==================
*/
void VID_HideEngineWindow( void )
{
	if ( engineapi.LauncherTakingFocus )
		engineapi.LauncherTakingFocus();

	if ( engineapi.StoreProfile )
		engineapi.StoreProfile();

	if ( !ActiveApp )
		return;

	engineapi.S_BlockSound();
	g_bEngineWindowUp = 0;

#ifdef LAUNCHER_FIXES
	if ( Shell_FullscreenActive() && !Shell_HasFullscreenMode() )
		Shell_ShowBackdrop( 1 );
#endif

	if ( !Launcher_MainButtonsLoaded() )
		Launcher_LoadMainButtonsBitmap();

#ifdef LAUNCHER_RE
	Launcher_UpdateShellAfterEngine();
#endif

	if ( gEngineVidType == VT_Direct3D && lpDD )
	{
		lpDD->lpVtbl->SetCooperativeLevel( lpDD,
			NULL, DDSCL_NORMAL );
	}

	ShowWindow( gLauncherWnd, SW_SHOW );
	ShowWindow( mainwindow, SW_HIDE );
	CDAudio_SwitchToLauncher();

	if ( !gEngineModeWindowed )
	{
		if ( lpDD )
		{
			DDraw_SetDisplayMode( -1 );
		}
#ifdef LAUNCHER_FIXES
		else if ( Shell_FullscreenActive() )
		{
			// Whatever mode the engine ran in, the shell wants its own back.
			Shell_EnterFullscreen();
		}
#endif
		else
		{
			if ( g_nLauncherDefW == 640 && g_nLauncherDefH == 480 &&
				!CheckParm( "-noextracds", NULL ) )
			{
				VID_ChangeDisplaySettings( 800, 600, 16 );
			}

			VID_ChangeDisplaySettings( g_nLauncherDefW, g_nLauncherDefH, 16 );
		}
	}

	ShowWindow( gLauncherWnd, SW_SHOW );
	SetForegroundWindow( gLauncherWnd );
	RedrawWindow( gLauncherWnd, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN );
	ClipCursor( NULL );

	if ( windowed_mouse && GetCapture() == mainwindow )
		ReleaseCapture();

	engineapi.IN_DeactivateMouse();
	SetFocus( gLauncherWnd );
	ClearAllStates();

	if ( engineapi.StoreProfile )
		Launcher_SaveFavoriteServers( g_pServerBrowser );
}

/*
==================
VID_ShowEngineWindow (0x4644E0)
==================
*/
void VID_ShowEngineWindow( int bShow )
{
	if ( g_bEngineWindowUp )
		return;

	g_bEngineWindowUp = 1;

#ifdef LAUNCHER_FIXES
	// The shell backdrop has no business over the game.
	Shell_ShowBackdrop( 0 );
#endif

	// IN_HideMouse() -- folded nullstub

	if ( !gEngineModeWindowed && bShow )
	{
		if ( lpDD )
			DDraw_SetDisplayMode( g_currentMode );
		else
			VID_SetModeDisplaySettings( g_currentMode );

		MoveWindow( mainwindow, 0, 0, modelist[g_activeMode].width,
			modelist[g_activeMode].height, FALSE );

		if ( vid.vidtype == VT_Direct3D )
		{
			lpDD->lpVtbl->SetCooperativeLevel( lpDD,
				gLauncherWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT );
		}
	}

	ShowWindow( mainwindow, SW_SHOW );
	ShowWindow( gLauncherWnd, SW_HIDE );
	SetForegroundWindow( mainwindow );
	SetFocus( mainwindow );

	if ( g_activeMode != g_currentMode )
		VID_SetMode( g_currentMode );

	if ( windowed_mouse )
	{
		if ( GetCapture() != mainwindow )
			ReleaseCapture();
		SetCapture( mainwindow );
		GetWindowRect( mainwindow, &window_rect );
		ClipCursor( &window_rect );
		engineapi.IN_ActivateMouse();
	}
	else
	{
		ClipCursor( NULL );
		ReleaseCapture();
		engineapi.IN_DeactivateMouse();
	}

	CDAudio_SwitchToEngine();
	engineapi.S_ClearBuffer();
	engineapi.S_UnblockSound();
	engineapi.Keyboard_ReturnToGame();

	if ( engineapi.EngineTakingFocus )
		engineapi.EngineTakingFocus();
}
