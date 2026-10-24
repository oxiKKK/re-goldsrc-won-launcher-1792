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
// Purpose: joystick input helpers (Joy_*).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

static char			joy_keyname[32];		// 0x4E2BE8  scratch for the key name
static JOYINFOEX	ji;						// 0x4E2C08  last polled position
static int			joy_numbuttons;			// 0x4E2C3C  number of buttons to scan
static DWORD		joy_flags;				// 0x4E2C40  accumulated joyGetPosEx flags
static int			joy_id;					// 0x4E2C44  joystick id (JOYSTICKID1..)
static DWORD		joy_oldpovstate;		// 0x4E2C48  last POV bitmask
static DWORD		joy_oldbuttonstate;		// 0x4E2C4C  last button bitmask

static DWORD*		pdwRawValue[6];			// 0x4E2C50  per-axis -> JOYINFOEX field
static DWORD		dwControlMap[6];		// 0x4E2C68  per-axis JOY_RELATIVE_AXIS bit
static DWORD		dwAxisMap[6];			// 0x4E2C80  per-axis function (AxisNada..)

int					joy_avail;				// 0x4E2C98  a usable stick was found
static int			joy_haspov;				// 0x4E2C9C  device reports a POV hat

static DWORD		joy_advaxis[6];			// 0x4E2CA0  -joyadv{x,y,z,r,u,v} packed values

// 0x4CF45C  the JOY_RETURN{X,Y,Z,R,U,V} flag for each axis ordinal.
static const DWORD	s_dwAxisFlags[6] =
{
	JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, JOY_RETURNR, JOY_RETURNU, JOY_RETURNV
};

// Axis-function ids + control-map bit.
enum { AxisNada = 0, AxisForward = 1, AxisLook = 2, AxisSide = 3, AxisTurn = 4 };
#define JOY_RELATIVE_AXIS	0x10

/*
==================
Joy_Detect (0x41B900)
==================
*/
void Joy_Detect( void )
{
	JOYCAPS		jc;
	int			numdevs;

	joy_avail = 0;

	if ( CheckParm( "-nojoy", NULL ) )
		return;

	// The binary takes &m_playerConfig without testing the browser pointer.
	if ( !g_pServerBrowser || g_pServerBrowser->m_playerConfig.joystick == 0.0f )
		return;

	numdevs = joyGetNumDevs();
	if ( !numdevs )
		return;

	for ( joy_id = 0; joy_id < numdevs; joy_id++ )
	{
		memset( &ji, 0, sizeof( ji ) );
		ji.dwSize  = sizeof( ji );
		ji.dwFlags = JOY_RETURNCENTERED;
		if ( joyGetPosEx( joy_id, &ji ) == JOYERR_NOERROR )
			break;
	}
	if ( joy_id >= numdevs )
		return;			// no stick answered

	memset( &jc, 0, sizeof( jc ) );
	if ( joyGetDevCapsA( joy_id, &jc, sizeof( jc ) ) == JOYERR_NOERROR )
	{
		joy_numbuttons     = jc.wNumButtons;
		joy_haspov         = jc.wCaps & JOYCAPS_HASPOV;
		joy_oldpovstate    = 0;
		joy_oldbuttonstate = 0;
		joy_avail          = 1;
	}
}

/*
==================
Joy_RawValuePointer (0x41BA20)
==================
*/
DWORD* Joy_RawValuePointer( int nAxis )
{
	switch ( nAxis )
	{
	case 1:		return &ji.dwYpos;
	case 2:		return &ji.dwZpos;
	case 3:		return &ji.dwRpos;
	case 4:		return &ji.dwUpos;
	case 5:		return &ji.dwVpos;
	default:	return &ji.dwXpos;
	}
}

/*
==================
Joy_AdvancedUpdate (0x41BA70)

-joy_advanced selects the default mapping, not the packed one (sic).
==================
*/
void Joy_AdvancedUpdate( void )
{
	char*	pszValue;
	int		i;

	if ( CheckParm( "-joyadvx", &pszValue ) && pszValue )
		joy_advaxis[0] = atoi( pszValue );
	if ( CheckParm( "-joyadvy", &pszValue ) && pszValue )
		joy_advaxis[1] = atoi( pszValue );
	if ( CheckParm( "-joyadvz", &pszValue ) && pszValue )
		joy_advaxis[2] = atoi( pszValue );
	if ( CheckParm( "-joyadvr", &pszValue ) && pszValue )
		joy_advaxis[3] = atoi( pszValue );
	if ( CheckParm( "-joyadvu", &pszValue ) && pszValue )
		joy_advaxis[4] = atoi( pszValue );
	if ( CheckParm( "-joyadvv", &pszValue ) && pszValue )
		joy_advaxis[5] = atoi( pszValue );

	for ( i = 0; i < 6; i++ )
	{
		dwAxisMap[i]    = 0;
		dwControlMap[i] = 0;
		pdwRawValue[i]  = Joy_RawValuePointer( i );
	}

	if ( CheckParm( "-joy_advanced", NULL ) )
	{
		dwAxisMap[0] = AxisTurn;			// JOY_AXIS_X
		dwAxisMap[1] = AxisForward;			// JOY_AXIS_Y
	}
	else
	{
		// Each -joyadv* value packs the axis function in the low nibble and the
		// absolute/relative bit in JOY_RELATIVE_AXIS.
		for ( i = 0; i < 6; i++ )
		{
			dwAxisMap[i]    = joy_advaxis[i] & 0x0F;
			dwControlMap[i] = joy_advaxis[i] & JOY_RELATIVE_AXIS;
		}
	}

	joy_flags = JOY_RETURNCENTERED | JOY_RETURNPOV | JOY_RETURNBUTTONS;
	for ( i = 0; i < 6; i++ )
	{
		if ( dwAxisMap[i] )
			joy_flags |= s_dwAxisFlags[i];
	}
}

/*
==================
Joy_GetButtonName (0x41BC90)

The POV hat reads back as four more buttons, AUX29 through AUX32.
==================
*/
char* Joy_GetButtonName( void )
{
	DWORD	povstate;
	DWORD	bit;
	int		i;

	if ( !joy_avail )
		return NULL;

	for ( i = 0; i < (int)joy_numbuttons; i++ )
	{
		bit = 1 << i;
		if ( ( ( ji.dwButtons & bit ) && !( joy_oldbuttonstate & bit ) )
		  || ( !( ji.dwButtons & bit ) && ( joy_oldbuttonstate & bit ) ) )
		{
			if ( i >= 4 )
				sprintf( joy_keyname, "AUX%i", i + 1 );
			else
				sprintf( joy_keyname, "JOY%i", i + 1 );
			return joy_keyname;
		}
	}
	joy_oldbuttonstate = ji.dwButtons;

	if ( joy_haspov )
	{
		povstate = 0;
		if ( ji.dwPOV != JOY_POVCENTERED )
		{
			switch ( ji.dwPOV )
			{
			case JOY_POVFORWARD:	povstate = 1;	break;
			case JOY_POVRIGHT:		povstate = 2;	break;
			case JOY_POVBACKWARD:	povstate = 4;	break;
			case JOY_POVLEFT:		povstate = 8;	break;
			}
		}

		for ( i = 0; i < 4; i++ )
		{
			bit = 1 << i;
			if ( ( ( povstate & bit ) && !( joy_oldpovstate & bit ) )
			  || ( !( povstate & bit ) && ( joy_oldpovstate & bit ) ) )
			{
				sprintf( joy_keyname, "AUX%i", i + 29 );
				return joy_keyname;
			}
		}
		joy_oldpovstate = povstate;
	}
	return NULL;
}

/*
==================
Joy_ReadJoystick (0x41BD90)
==================
*/
BOOL Joy_ReadJoystick( void )
{
	memset( &ji, 0, sizeof( ji ) );
	ji.dwSize  = sizeof( ji );
	ji.dwFlags = joy_flags;
	return joyGetPosEx( joy_id, &ji ) == JOYERR_NOERROR;
}

/*
==================
Joy_GetPressedButton (0x41BDD0)
==================
*/
BOOL Joy_GetPressedButton( char* pszName )
{
	char*	pszKey;

	*pszName = 0;
	if ( !Joy_ReadJoystick() )
		return FALSE;

	pszKey = Joy_GetButtonName();
	if ( !pszKey )
		return FALSE;

	strcpy( pszName, pszKey );
	return TRUE;
}
