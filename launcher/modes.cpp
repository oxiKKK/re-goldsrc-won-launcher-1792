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
// Purpose: the software-blit / buffer-lock half of the video module, plus
//          the mode-description lookups (Quake vid_win.c lineage).
//
// $NoKeywords: $
//=============================================================================

#define CINTERFACE
#include <windows.h>
#include <ddraw.h>

#include "modes.h"
#include "vid.h"
#include "viddef.h"
#include "launcher.h"
#include "engine.h"

#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)

int				vid_modenum = NO_MODE;	// 0x4D06C8
static int		g_iVidRestore;			// 0x4D06C4  pending surface-restore state

/*
==================
VID_GetModePtr (0x42DA20)
==================
*/
vmode_t* VID_GetModePtr( int mode )
{
	if ( mode < 0 || mode >= nummodes )
		return NULL;
	return &modelist[mode];
}

/*
==================
VID_UpdateWindowStatus (0x42DA50)
==================
*/
void VID_UpdateWindowStatus( void )
{
	window_rect.left = window_x;
	window_rect.top = window_y;
	window_rect.right = window_x + window_width;
	window_rect.bottom = window_y + window_height;

	if ( engineapi.VID_UpdateWindowVars )
		engineapi.VID_UpdateWindowVars( &window_rect,
			window_x + window_width / 2, window_y + window_height / 2 );
}

/*
==================
VID_BlitSoftware (0x42DAC0)

rects is taken to match the engine's signature and ignored -- the whole frame
is always presented.
==================
*/
void VID_BlitSoftware( struct vrect_s* rects )
{
	DDSURFACEDESC	ddsd;
	RECT			srcRect;
	int				y;
	int				srcY;

	srcRect.left   = 0;
	srcRect.top    = 0;
	srcRect.right  = vid.width;
	srcRect.bottom = vid.height;

	if ( !ActiveApp && !gEngineModeWindowed )
		return;

	if ( dibdc )
	{
		// Windowed: copy the DIB section to the window scanline-by-scanline,
		// flipped vertically (DIB is bottom-up).
		srcY = vid.height - 1;
		for ( y = 0; y < (int)vid.height; ++y, --srcY )
			BitBlt( dibdc, 0, y, vid.width, 1, g_hdcSection, 0, srcY, SRCCOPY );
		return;
	}

	// Fullscreen DirectDraw.
	g_lpddsSystem->lpVtbl->Unlock( g_lpddsSystem, NULL );

	if ( vid.numpages )
	{
		if ( lpBackBuffer->lpVtbl->BltFast( lpBackBuffer, 0, 0, g_lpddsSystem,
				&srcRect, DDBLTFAST_WAIT ) == DDERR_SURFACELOST )
		{
			DDraw_RestoreLostSurfaces();
			lpBackBuffer->lpVtbl->BltFast( lpBackBuffer, 0, 0, g_lpddsSystem,
				&srcRect, DDBLTFAST_WAIT );
		}

		if ( lpPrimary->lpVtbl->Flip( lpPrimary, NULL, DDFLIP_WAIT ) == DDERR_SURFACELOST )
		{
			DDraw_RestoreLostSurfaces();
			lpPrimary->lpVtbl->Flip( lpPrimary, NULL, DDFLIP_WAIT );
		}
	}
	else
	{
		if ( lpBackBuffer->lpVtbl->BltFast( lpBackBuffer, 0, 0, g_lpddsSystem,
				&srcRect, DDBLTFAST_WAIT ) == DDERR_SURFACELOST )
		{
			DDraw_RestoreLostSurfaces();
			lpBackBuffer->lpVtbl->BltFast( lpBackBuffer, 0, 0, g_lpddsSystem,
				&srcRect, DDBLTFAST_WAIT );
		}
	}

	memset( &ddsd, 0, sizeof( ddsd ) );
	ddsd.dwSize = sizeof( ddsd );
	g_lpddsSystem->lpVtbl->Lock( g_lpddsSystem, NULL, &ddsd, DDLOCK_WAIT, NULL );

	vid.direct = (pixel_t*)ddsd.lpSurface;
	vid.conbuffer = (pixel_t*)ddsd.lpSurface;
	vid.buffer = (pixel_t*)ddsd.lpSurface;
	vid.rowbytes = ddsd.lPitch;
}

/*
==================
VID_Update (0x42DC60)
==================
*/
void VID_Update( struct vrect_s* rects )
{
	if ( vid.vidtype == VT_OpenGL || vid.vidtype == VT_Direct3D )
	{
		engineapi.glSwapBuffers( g_maindc );

		if ( !g_iVidRestore && windowed_mouse != g_bLastMouseActive )
		{
			if ( windowed_mouse )
				engineapi.IN_ActivateMouse();
			else
				engineapi.IN_DeactivateMouse();
			g_bLastMouseActive = windowed_mouse;
		}
	}
	else
	{
		VID_BlitSoftware( rects );
	}
}

/*
==================
VID_Shutdown (0x42DC90)
==================
*/
void VID_Shutdown( void )
{
	if ( vid.vidtype == VT_OpenGL || vid.vidtype == VT_Direct3D )
		engineapi.GL_Shutdown( mainwindow, g_maindc, g_baseRC );

	if ( DDActive )
	{
		DDActive = 0;
		g_iVidRestore = 3;
	}
}

/*
==================
VID_LockBuffer (0x42DCE0)
==================
*/
void VID_LockBuffer( void )
{
	if ( vid.vidtype == VT_OpenGL || vid.vidtype == VT_Direct3D )
		return;
	if ( dibdc || !DDActive )
		return;
	if ( ++lockcount > 1 )
		return;

	vid.direct = vid.buffer;
	vid.conbuffer = vid.buffer;
}

/*
==================
VID_UnlockBuffer (0x42DD30)
==================
*/
void VID_UnlockBuffer( void )
{
	if ( vid.vidtype == VT_OpenGL || vid.vidtype == VT_Direct3D || dibdc )
		return;

	if ( !DDActive )
		return;

	--lockcount;
	if ( lockcount > 0 )
		return;
	if ( lockcount == 0 )
		return;

	Console_Printf( "Unbalanced unlock" );
}

/*
==================
VID_ForceUnlockedAndReturnState (0x42DD70)
==================
*/
int VID_ForceUnlockedAndReturnState( void )
{
	int	lk;

	if ( vid.vidtype == VT_OpenGL || vid.vidtype == VT_Direct3D || !lockcount || !DDActive )
		return 0;

	lk = lockcount;

	if ( dibdc )
	{
		lockcount = 0;
	}
	else
	{
		lockcount = 1;
		VID_UnlockBuffer();
	}

	return lk;
}

/*
==================
VID_ForceLockState (0x42DDD0)
==================
*/
void VID_ForceLockState( int lk )
{
	if ( vid.vidtype == VT_OpenGL || vid.vidtype == VT_Direct3D || !DDActive )
		return;

	if ( !dibdc && lk )
	{
		lockcount = 0;
		VID_LockBuffer();
	}

	lockcount = lk;
}

/*
==================
VID_GetVID (0x42DE20)
==================
*/
void VID_GetVID( struct viddef_s* pvid )
{
	if ( !pvid )
		return;

	memcpy( pvid, &vid, sizeof( vid ) );
}

/*
==================
VID_SetDefaultMode (0x42DE40)

Drops back to the windowed mode and lets the mouse go -- what the engine calls
before it puts a dialog up.
==================
*/
void VID_SetDefaultMode( void )
{
	if ( DDActive )
		VID_SetMode( 0 );

	engineapi.IN_DeactivateMouse();
}

/*
==================
VID_GetModeDescription (0x42DE60)
==================
*/
char* VID_GetModeDescription( int mode )
{
	vmode_t*	entry;

	if ( mode < 0 || mode >= nummodes )
		return NULL;

	entry = VID_GetModePtr( mode );
	if ( entry )
		return entry->modedesc;

	return NULL;
}

/*
==================
VID_GetExtModeDescription (0x42DEA0)
==================
*/
char* VID_GetExtModeDescription( int mode )
{
	static char	g_szModeDesc[256];		// 0x4E6890
	vmode_t*	pv;

	if ( mode < 0 || mode >= nummodes )
		return NULL;

	pv = VID_GetModePtr( mode );
	if ( !pv )
		return NULL;

	if ( modelist[mode].type == MS_FULLDIB )
		wsprintfA( g_szModeDesc, "%s fullscreen DIB", pv->modedesc );
	else
		wsprintfA( g_szModeDesc, "%s windowed", pv->modedesc );

	return g_szModeDesc;
}
