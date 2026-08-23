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
// Purpose: launcher-side VGUI1 hosting.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"


using namespace vgui;

// vftable 0x4B3BC0 -- overrides only these two, both empty.
class LauncherApp : public App
{
public:
	// LauncherApp::LauncherApp (0x46A1E0)
	LauncherApp() : App( true )
	{
		setMinimumTickMillisInterval( 0 );
	}

	// LauncherApp::main (0x40C070)
	virtual void main( int /*argc*/, char* /*argv*/[] )
	{
	}

	// LauncherApp::platTick (0x40E460)
	virtual void platTick()
	{
	}
};

// 0x4FB368: the one-and-only launcher App.
static App*	g_pLauncherApp = NULL;

/*
==================
VGui_Start (0x4696A0)
==================
*/
void VGui_Start( void )
{
	if ( !g_pLauncherApp )
	{
		g_pLauncherApp = new LauncherApp();
		g_pLauncherApp->start();		// non-blocking (externalMain)
	}
}

/*
==================
VGui_Frame (0x469710)
==================
*/
void VGui_Frame( void )
{
	if ( g_pLauncherApp )
		g_pLauncherApp->externalTick();
}
