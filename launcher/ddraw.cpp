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
// Purpose: DirectDraw setup and teardown for the software renderer.
//
// $NoKeywords: $
//=============================================================================

#define CINTERFACE
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <string.h>
#include "viddef.h"
#include "launcher.h"
#include "vid.h"
#include "gameui.h"
#include "engine.h"
#include "strings.h"
#include "d3d_structs.h"
#include "resource.h"

#define MAX_DD_DEVICES	16

// One record of the device list DDraw_EnumCallback fills: GUID, primary flag
// and driver description, 28 bytes apiece.
struct ddraw_device_s
{
	GUID	guid;			// device GUID (zeroed for the primary)
	int		bPrimary;		// NULL GUID -> primary display
	char	desc[8];		// driver description; completes the 28-byte record
};

// DirectDraw objects used by the software renderer.
LPDIRECTDRAW 			lpDD;			// 0x4E1A20
LPDIRECTDRAW4			glpDD4;			// 0x4E1A24
IUnknown*				glpD3D;			// 0x4E1A28
LPDIRECTDRAWSURFACE		lpPrimary;		// 0x4E1A2C  flip-chain front
LPDIRECTDRAWSURFACE		lpBackBuffer;	// 0x4E19A8  attached back buffer
LPDIRECTDRAWSURFACE		g_lpddsSystem;	// 0x4E1A30  locked draw surface

static struct ddraw_device_s	ddDevices[MAX_DD_DEVICES];	// 0x4E19B0
static int				nDDDevices;		// 0x4E1A34
static int				ddCreateFlags;	// 0x4E1A38

static const char*	DDraw_ErrorString( HRESULT hr );
static BOOL WINAPI	DDraw_EnumCallback( GUID* lpGUID, LPSTR lpDesc, LPSTR lpDrv, LPVOID lpCtx );

/*
==================
DDraw_QueryD3D (0x408310)
==================
*/
int DDraw_QueryD3D( void )
{
	if ( !lpDD )
		return 0;

	return lpDD->lpVtbl->QueryInterface( lpDD, IID_IDirect3D3, (LPVOID*)&glpD3D ) == DD_OK;
}

/*
==================
DDraw_IsModeAvailable (0x408330)
==================
*/
int DDraw_IsModeAvailable( void )
{
	int	found;

	nDDDevices = 0;
	DirectDrawEnumerateA( DDraw_EnumCallback, &found );
	if ( found )
		return 1;

	MessageBoxA( NULL, "Direct Draw Init Failed. No DD Devices\n", "ERROR", MB_OK );
	return 0;
}

/*
==================
DDraw_EnumCallback (0x408380)
==================
*/
static BOOL WINAPI DDraw_EnumCallback( GUID* lpGUID, LPSTR lpDesc, LPSTR lpDrv, LPVOID lpCtx )
{
	if ( lpGUID )
	{
		ddDevices[nDDDevices].guid     = *lpGUID;
		ddDevices[nDDDevices].bPrimary = 0;
	}
	else
	{
		ddDevices[nDDDevices].bPrimary = 1;
	}
	strcpy( ddDevices[nDDDevices].desc, lpDesc );
	++nDDDevices;
	*(int*)lpCtx = 1;
	return TRUE;
}

/*
==================
DDraw_Init (0x408420)
==================
*/
int DDraw_Init( int bExclusive, int flags )
{
	if ( !lpDD )
	{
		HRESULT hr = DirectDrawCreate( NULL, &lpDD, NULL );
		if ( hr != DD_OK )
		{
			Launcher_ShowMessageByIdEx( NULL, IDS_DDRAW_FAILEINIT, hr, DDraw_ErrorString( hr ) );
			return 0;
		}

		if ( g_bWinNT )
		{
			glpDD4 = NULL;
		}
		else
		{
			hr = lpDD->lpVtbl->QueryInterface( lpDD, IID_IDirectDraw4, (LPVOID*)&glpDD4 );
			if ( hr )
			{
				Launcher_ShowMessageByIdEx( NULL, IDS_DDRAW_DX4FAIL, hr, DDraw_ErrorString( hr ) );
				return 0;
			}
		}
	}

	lpDD->lpVtbl->SetCooperativeLevel( lpDD, NULL, DDSCL_NORMAL );
	// LAUNCHER_FIXES: the shell draws through GDI and no longer takes the display
	// exclusively -- the engine still claims it in DDraw_CreateSurfaces.
#ifndef LAUNCHER_FIXES
	if ( bExclusive && !gEngineModeWindowed )
		lpDD->lpVtbl->SetCooperativeLevel( lpDD, gLauncherWnd,
			DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT );
#endif

	ddCreateFlags = flags;
	return 1;
}

/*
==================
DDraw_ReleaseSurfaces (0x4084F0)
==================
*/
void DDraw_ReleaseSurfaces( void )
{
	if ( g_lpddsSystem )
	{
		g_lpddsSystem->lpVtbl->Unlock( g_lpddsSystem, NULL );
		g_lpddsSystem->lpVtbl->Release( g_lpddsSystem );
		g_lpddsSystem = NULL;
	}

	if ( lpPrimary )
	{
		lpPrimary->lpVtbl->DeleteAttachedSurface( lpPrimary, 0, lpBackBuffer );
		lpBackBuffer->lpVtbl->Release( lpBackBuffer );
		lpBackBuffer = NULL;
		lpPrimary->lpVtbl->Release( lpPrimary );
		lpPrimary = NULL;
	}
}

/*
==================
DDraw_Shutdown (0x408560)
==================
*/
void DDraw_Shutdown( void )
{
	if ( !lpDD )
		return;

	lpDD->lpVtbl->FlipToGDISurface( lpDD );

	if ( glpD3D )
	{
		glpD3D->lpVtbl->Release( glpD3D );
		glpD3D = NULL;
	}

	DDraw_ReleaseSurfaces();

	if ( glpDD4 )
	{
		glpDD4->lpVtbl->Release( glpDD4 );
		glpDD4 = NULL;
	}

	lpDD->lpVtbl->Release( lpDD );
	lpDD = NULL;
}

/*
==================
DDraw_SetDisplayMode (0x4085C0)
==================
*/
int DDraw_SetDisplayMode( int mode )
{
	HRESULT	hr;
	int		w, h, bits;
	int		result = 1;

	if ( !lpDD )
		return 0;

	if ( mode == -1 )
	{
#ifdef LAUNCHER_FIXES
		// The shell's mode is a real one now, picked out of the driver's list and
		// set through ChangeDisplaySettings -- not a 16-bpp request DirectDraw
		// has not been able to honour since Windows 8.
		if ( Shell_FullscreenActive() )
		{
			Shell_EnterFullscreen();
			MoveWindow( mainwindow, 0, 0, g_nLauncherDefW, g_nLauncherDefH, FALSE );
			lpDD->lpVtbl->FlipToGDISurface( lpDD );
			return 1;
		}
#endif
		MoveWindow( mainwindow, 0, 0, g_nLauncherDefW, g_nLauncherDefH, FALSE );
		hr = lpDD->lpVtbl->SetDisplayMode( lpDD, g_nLauncherDefW, g_nLauncherDefH, 16 );
		if ( hr )
		{
			DDraw_ErrorString( hr );
			// "Couldn't change modes" goes to a folded nullstub (0x40E460).
			result = 0;
		}
	}
	else
	{
		w = modelist[mode].width;
		h = modelist[mode].height;
		bits = modelist[mode].bpp;
		MoveWindow( mainwindow, 0, 0, w, h, FALSE );
		if ( lpDD->lpVtbl->SetDisplayMode( lpDD, w, h, bits ) )
		{
			// "Couldn't change modes" again here (sic).
			result = 0;
		}
	}

	lpDD->lpVtbl->FlipToGDISurface( lpDD );
	return result;
}

/*
==================
DDraw_CreateSurfaces (0x408690)
==================
*/
int DDraw_CreateSurfaces( int mode, int bExclusive )
{
	DDSURFACEDESC	ddsd;
	DDPIXELFORMAT	pf;
	DDBLTFX			bltfx;
	DDSCAPS			caps;
	int				w, h, halfres;

	if ( !lpDD )
		return 0;

	WindowRect.left = 0;
	WindowRect.top = 0;
	halfres = modelist[mode].stretched;
	w = modelist[mode].width;
	h = modelist[mode].height;
	WindowRect.right = w;
	WindowRect.bottom = h;

	DIBWidth = w;
	DIBHeight = h;
	if ( halfres )
	{
		DIBWidth = w >> 1;
		DIBHeight = h >> 1;
	}

	if ( lpPrimary )
		DDraw_ReleaseSurfaces();

	DDraw_SetDisplayMode( mode );

	if ( bExclusive )
	{
		lpDD->lpVtbl->SetCooperativeLevel( lpDD, gLauncherWnd,
			DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT );

		ddsd.dwSize = sizeof( ddsd );
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
		ddsd.dwBackBufferCount = 1;
		if ( lpDD->lpVtbl->CreateSurface( lpDD, &ddsd, &lpPrimary, NULL ) )
		{
			// "Couldn't change modes" goes to a folded nullstub (0x40E460).
			return 0;
		}

		// Clear the primary to black.
		bltfx.dwSize = sizeof( bltfx );
		bltfx.dwFillColor = 0;
		lpPrimary->lpVtbl->Blt( lpPrimary, NULL, NULL, NULL, DDBLT_COLORFILL, &bltfx );

		// The attached back buffer.
		caps.dwCaps = DDSCAPS_BACKBUFFER;
		vid.numpages = ( lpPrimary->lpVtbl->GetAttachedSurface( lpPrimary, &caps, &lpBackBuffer ) == 0 );

		// A system-memory surface the engine draws into.
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		ddsd.dwWidth = w;
		ddsd.dwHeight = h;
		if ( lpDD->lpVtbl->CreateSurface( lpDD, &ddsd, &g_lpddsSystem, NULL ) )
		{
			// "Couldn't create memory surface" goes to a folded nullstub.
			return 0;
		}
		if ( g_lpddsSystem->lpVtbl->Lock( g_lpddsSystem, NULL, &ddsd, DDLOCK_WAIT, NULL ) )
		{
			// "Couldn't lock memory surface" goes to a folded nullstub.
			return 0;
		}

		pf.dwSize = sizeof( pf );
		if ( g_lpddsSystem->lpVtbl->GetPixelFormat( g_lpddsSystem, &pf ) )
			return 0;

		vid.is15bit = ( pf.dwGBitMask == 0x3E0 );	// 555 green mask
		vid.direct = (pixel_t*)ddsd.lpSurface;
		vid.conbuffer = (pixel_t*)ddsd.lpSurface;
		vid.buffer = (pixel_t*)ddsd.lpSurface;
		vid.conrowbytes = ddsd.lPitch;
		vid.rowbytes = ddsd.lPitch;
		memset( ddsd.lpSurface, 0, 2 * w * h );
	}

	vid.conheight = DIBHeight;
	vid.height = DIBHeight;
	vid.conwidth = DIBWidth;
	vid.width = DIBWidth;
	g_currentMode = mode;
	vid_stretched = halfres;
	vid.maxwarpwidth  = WARP_WIDTH;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.bits = modelist[mode].bpp;
	vid.aspect = ( (float)vid.height / (float)vid.width ) * ( 320.0f / 240.0f );
	return 1;
}

/*
==================
DDraw_RestoreLostSurfaces (0x408990)
==================
*/
void DDraw_RestoreLostSurfaces( void )
{
	if ( lpDD
	  && ( lpBackBuffer->lpVtbl->Restore( lpBackBuffer )
		|| lpBackBuffer->lpVtbl->Restore( lpBackBuffer ) ) )
	{
		DDraw_CreateSurfaces( g_currentMode, 1 );
		engineapi.VID_UpdateVID( &vid );
	}
}

/*
==================
DDraw_ErrorString (0x4089E0)
==================
*/
static const char* DDraw_ErrorString( HRESULT hr )
{
	switch ( hr )
	{
	case DD_OK:                              return "No error";
	case DDERR_OUTOFMEMORY:                  return "Out Of Memory";
	case DDERR_INVALIDPARAMS:                return "Invalid Params";
	case DDERR_UNSUPPORTED:                  return "Unsupported Operation";
	case DDERR_GENERIC:                      return "Generic failure";
	case DDERR_ALREADYINITIALIZED:           return "Object already initialised";
	case DDERR_CANNOTATTACHSURFACE:          return "Cannot attack surface";
	case DDERR_CANNOTDETACHSURFACE:          return "Cannot detach surface";
	case DDERR_CURRENTLYNOTAVAIL:            return "Currently Not Available";
	case DDERR_EXCEPTION:                    return "Exception has occured";
	case DDERR_HEIGHTALIGN:                  return "Height Alignment";
	case DDERR_INCOMPATIBLEPRIMARY:          return "Incompatible Primary";
	case DDERR_INVALIDCAPS:                  return "Invalid Caps";
	case DDERR_INVALIDCLIPLIST:              return "Invlaid Clip List";
	case DDERR_INVALIDMODE:                  return "Invalid Mode";
	case DDERR_INVALIDOBJECT:                return "Invalid Object";
	case DDERR_INVALIDPIXELFORMAT:           return "Invalid Pixel Format";
	case DDERR_INVALIDRECT:                  return "Invalid Rectangle";
	case DDERR_LOCKEDSURFACES:               return "Locked Surfaces";
	case DDERR_NO3D:                         return "No 3d support present";
	case DDERR_NOALPHAHW:                    return "No Alpha Hardware";
	case DDERR_NOCLIPLIST:                   return "No Clip List";
	case DDERR_NOCOLORCONVHW:                return "No Colour Conversion Hardware";
	case DDERR_NOCOOPERATIVELEVELSET:        return "No Cooperative Level Set";
	case DDERR_NOCOLORKEY:                   return "Surface Has No Colour Key";
	case DDERR_NOCOLORKEYHW:                 return "No Hardware Colour Key";
	case DDERR_NODIRECTDRAWSUPPORT:          return "No Direct Draw Support";
	case DDERR_NOEXCLUSIVEMODE:              return "No Exclusive Mode";
	case DDERR_NOFLIPHW:                     return "No Flip Hardware";
	case DDERR_NOGDI:                        return "GDI is not present";
	case DDERR_NOMIRRORHW:                   return "No Mirror Hardware";
	case DDERR_NOTFOUND:                     return "Requested Item Was No Found";
	case DDERR_NOOVERLAYHW:                  return "No Overlay Hardware";
	case DDERR_NORASTEROPHW:                 return "No Raster Op Hardware";
	case DDERR_NOROTATIONHW:                 return "No Roatation Hardware";
	case DDERR_NOSTRETCHHW:                  return "No Stretch Hardware";
	case DDERR_NOT4BITCOLOR:                 return "Not 4 Bit Colour";
	case DDERR_NOT4BITCOLORINDEX:            return "Not 4 Bit Colour Index";
	case DDERR_NOT8BITCOLOR:                 return "Not 8 Bit Colour";
	case DDERR_NOTEXTUREHW:                  return "No Texture Hardware";
	case DDERR_NOVSYNCHW:                    return "No VSync Hardware";
	case DDERR_NOZBUFFERHW:                  return "No Z Buffer Hardware";
	case DDERR_NOZOVERLAYHW:                 return "No Z Overlay Hardware";
	case DDERR_OUTOFCAPS:                    return "Hardware Has Already Been Allocated";
	case DDERR_OUTOFVIDEOMEMORY:             return "Out Of Video Memory";
	case DDERR_OVERLAYCANTCLIP:              return "Hardware can't clip overlays";
	case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: return "Can only have colour key active at one time for overlays";
	case DDERR_PALETTEBUSY:                  return "Palette Locked by another thread";
	case DDERR_COLORKEYNOTSET:               return "Colour key is not set";
	case DDERR_SURFACEALREADYATTACHED:       return "Surface already attached";
	case DDERR_SURFACEALREADYDEPENDENT:      return "Surface Already Dependent";
	case DDERR_SURFACEBUSY:                  return "Surface Is Busy";
	case DDERR_SURFACEISOBSCURED:            return "Surface Is Obscured";
	case DDERR_SURFACELOST:                  return "Surface Lost";
	case DDERR_SURFACENOTATTACHED:           return "Surface Not Attached";
	case DDERR_TOOBIGHEIGHT:                 return "Height Too Big";
	case DDERR_TOOBIGSIZE:                   return "Size Too Big - Height and Width are individually OK";
	case DDERR_TOOBIGWIDTH:                  return "Width Too Big";
	case DDERR_UNSUPPORTEDFORMAT:            return "Unsupported Format";
	case DDERR_UNSUPPORTEDMASK:              return "Unsupported Mask Format";
	case DDERR_VERTICALBLANKINPROGRESS:      return "Vertical Blank In Progress";
	case DDERR_WASSTILLDRAWING:              return "Still Drawing";
	case DDERR_XALIGN:                       return "X Align";
	case DDERR_INVALIDDIRECTDRAWGUID:        return "Invalid Direct Draw GUID";
	case DDERR_DIRECTDRAWALREADYCREATED:     return "Direct Draw Already Created";
	case DDERR_NODIRECTDRAWHW:               return "No Direct Draw HardWare";
	case DDERR_PRIMARYSURFACEALREADYEXISTS:  return "Primary Surface Already Exists";
	case DDERR_NOEMULATION:                  return "No Emulation";
	case DDERR_REGIONTOOSMALL:               return "Region Too Small";
	case DDERR_CLIPPERISUSINGHWND:           return "Clipper is using a HWND";
	case DDERR_NOCLIPPERATTACHED:            return "No Clipper Attached";
	case DDERR_NOHWND:                       return "No HWND set";
	case DDERR_HWNDSUBCLASSED:               return "HWND Sub-Classed";
	case DDERR_HWNDALREADYSET:               return "HWND Already Set";
	case DDERR_NOPALETTEATTACHED:            return "No Palette Attrached";
	case DDERR_NOPALETTEHW:                  return "No Palette Hardware";
	case DDERR_BLTFASTCANTCLIP:              return "Blit Fast Can't Clip";
	case DDERR_NOBLTHW:                      return "No Blit Hardware";
	case DDERR_NODDROPSHW:                   return "No Direct Draw Raster OP Hardware";
	case DDERR_OVERLAYNOTVISIBLE:            return "Overlay Not Visible";
	case DDERR_NOOVERLAYDEST:                return "No Overlay Destination";
	case DDERR_INVALIDPOSITION:              return "Invlaid Position";
	case DDERR_NOTAOVERLAYSURFACE:           return "Not An Overlay Surface";
	case DDERR_EXCLUSIVEMODEALREADYSET:      return "Exclusive Mode Already Set";
	case DDERR_NOTFLIPPABLE:                 return "Not Flippable";
	case DDERR_CANTDUPLICATE:                return "Can't Duplicate";
	case DDERR_NOTLOCKED:                    return "Not Locked";
	case DDERR_CANTCREATEDC:                 return "Can't Create DC";
	case DDERR_NODC:                         return "No DC has ever Existed";
	case DDERR_WRONGMODE:                    return "Wrong Mode";
	case DDERR_IMPLICITLYCREATED:            return "Surface Cannot Be Restored";
	case DDERR_NOTPALETTIZED:                return "Not Palettized";
	case DDERR_UNSUPPORTEDMODE:              return "Unsupported Mode";

	default:                                 return "unk";
	}
}
