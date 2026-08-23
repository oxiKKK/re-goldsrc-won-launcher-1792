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
// Purpose: the WINMM mixer wrapper the player-profile page drives -- mic
//          boost, mic record volume and mic mute on the default mixer.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The most controls we will look at on any one source line.
#define MAX_MIXER_CONTROLS	128

/*
==================
CMixerControls::CMixerControls (0x46DDB0)
==================
*/
CMixerControls::CMixerControls()
{
	ResetState();
	Init();
}

/*
==================
CMixerControls::~CMixerControls (0x46DE50)
==================
*/
CMixerControls::~CMixerControls()
{
	Close();
}

/*
==================
CMixerControls::Init (0x46DEA0)
==================
*/
BOOL CMixerControls::Init()
{
	MIXERCAPSA			caps;
	MIXERLINEA			dstLine;
	MIXERLINEA			srcLine;
	MIXERLINECONTROLSA	mlc;
	MIXERCONTROLA		controls[MAX_MIXER_CONTROLS];
	MIXERCONTROLA*		pc;
	DWORD				dest, src, cControls, c;

	Close();
	mixerGetNumDevs();

	if ( mixerOpen( &m_hMixer, 0, 0, 0, 0 ) )
	{
		Close();
		return FALSE;
	}

	if ( mixerGetDevCapsA( (UINT_PTR)m_hMixer, &caps, sizeof( caps ) ) )
	{
		Close();
		return FALSE;
	}

	for ( dest = 0; dest < caps.cDestinations; dest++ )
	{
		dstLine.cbStruct = sizeof( dstLine );
		dstLine.dwDestination = dest;
		if ( mixerGetLineInfoA( (HMIXEROBJ)m_hMixer, &dstLine, MIXER_GETLINEINFOF_DESTINATION ) )
			continue;

		for ( src = 0; src < dstLine.cConnections; src++ )
		{
			srcLine.cbStruct = sizeof( srcLine );
			srcLine.dwDestination = dest;
			srcLine.dwSource = src;
			if ( mixerGetLineInfoA( (HMIXEROBJ)m_hMixer, &srcLine, MIXER_GETLINEINFOF_SOURCE ) )
				continue;

			cControls = srcLine.cControls;
			if ( cControls >= MAX_MIXER_CONTROLS )
				cControls = MAX_MIXER_CONTROLS;

			mlc.cbStruct = sizeof( mlc );
			mlc.dwLineID = srcLine.dwLineID;
			mlc.cControls = cControls;
			mlc.cbmxctrl = sizeof( MIXERCONTROLA );
			mlc.pamxctrl = controls;
			if ( mixerGetLineControlsA( (HMIXEROBJ)m_hMixer, &mlc, MIXER_GETLINECONTROLSF_ALL )
				 || !cControls )
				continue;

			for ( c = 0; c < cControls; c++ )
			{
				pc = &controls[c];

				// Every control the launcher wants hangs off a microphone
				// source line; the destination picks which one it is.
				if ( srcLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE )
				{
					if ( pc->dwControlType == MIXERCONTROL_CONTROLTYPE_ONOFF
						 && ( strstr( pc->szShortName, "Gain" )
							  || strstr( pc->szShortName, "Boost" )
							  || strstr( pc->szShortName, "+20d" ) ) )
					{
						m_controls[MIXER_CONTROL_MICBOOST].bFound = TRUE;
						m_controls[MIXER_CONTROL_MICBOOST].dwControlID = pc->dwControlID;
						m_controls[MIXER_CONTROL_MICBOOST].cMultipleItems = pc->cMultipleItems;
					}

					if ( dstLine.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_SPEAKERS )
					{
						if ( pc->dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE )
						{
							m_controls[MIXER_CONTROL_MICMUTE].bFound = TRUE;
							m_controls[MIXER_CONTROL_MICMUTE].dwControlID = pc->dwControlID;
							m_controls[MIXER_CONTROL_MICMUTE].cMultipleItems = pc->cMultipleItems;
						}
					}
					else if ( dstLine.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_WAVEIN
							  && pc->dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME )
					{
						m_controls[MIXER_CONTROL_MICVOLUME].bFound = TRUE;
						m_controls[MIXER_CONTROL_MICVOLUME].dwControlID = pc->dwControlID;
						m_controls[MIXER_CONTROL_MICVOLUME].cMultipleItems = pc->cMultipleItems;
					}
				}
			}
		}
	}

	return TRUE;
}

/*
==================
CMixerControls::Close (0x46E0E0)
==================
*/
void CMixerControls::Close()
{
	if ( m_hMixer )
	{
		mixerClose( m_hMixer );
		m_hMixer = NULL;
	}
	ResetState();
}

/*
==================
CMixerControls::GetValue (0x46E110)
==================
*/
BOOL CMixerControls::GetValue( int iControl, float* pflValue )
{
	DWORD	dwValue;
	BYTE	bValue;
	BOOL	ok;

	if ( iControl < 0 || iControl >= MIXER_CONTROL_COUNT || !m_controls[iControl].bFound )
		return FALSE;

	if ( iControl == MIXER_CONTROL_MICBOOST || iControl == MIXER_CONTROL_MICMUTE )
	{
		ok = GetBoolControlValue( m_controls[iControl].dwControlID,
				m_controls[iControl].cMultipleItems, &bValue );
		*pflValue = bValue;
		return ok;
	}

	if ( iControl == MIXER_CONTROL_MICVOLUME )
	{
		if ( !GetUnsignedControlValue( m_controls[iControl].dwControlID,
				m_controls[iControl].cMultipleItems, &dwValue ) )
			return FALSE;
		*pflValue = (float)dwValue * ( 1.0f / 65535.0f );
		return TRUE;
	}

	return FALSE;
}

/*
==================
CMixerControls::SetValue (0x46E1D0)
==================
*/
BOOL CMixerControls::SetValue( int iControl, float flValue )
{
	if ( iControl < 0 || iControl >= MIXER_CONTROL_COUNT || !m_controls[iControl].bFound )
		return FALSE;

	if ( iControl == MIXER_CONTROL_MICBOOST || iControl == MIXER_CONTROL_MICMUTE )
		return SetBoolControlValue( m_controls[iControl].dwControlID,
			m_controls[iControl].cMultipleItems, flValue != 0.0f );

	if ( iControl == MIXER_CONTROL_MICVOLUME )
		return SetUnsignedControlValue( m_controls[iControl].dwControlID,
			m_controls[iControl].cMultipleItems,
			(DWORD)( flValue * 65535.0f ) );

	return FALSE;
}

/*
==================
CMixerControls::Release (0x46E260)
==================
*/
void CMixerControls::Release()
{
	delete this;
}

/*
==================
CMixerControls::ResetState (0x46E270)
==================
*/
void CMixerControls::ResetState()
{
	m_hMixer = NULL;
	memset( m_controls, 0, sizeof( m_controls ) );
}

/*
==================
CMixerControls::GetBoolControlValue (0x46E290)
==================
*/
BOOL CMixerControls::GetBoolControlValue( DWORD dwControlID, DWORD cMultipleItems, BYTE* pbValue )
{
	MIXERCONTROLDETAILS	mcd;
	DWORD				dwValue;

	mcd.cbStruct = sizeof( mcd );
	mcd.dwControlID = dwControlID;
	mcd.cChannels = 1;
	mcd.cMultipleItems = cMultipleItems;
	mcd.cbDetails = sizeof( dwValue );
	mcd.paDetails = &dwValue;
	if ( mixerGetControlDetailsA( (HMIXEROBJ)m_hMixer, &mcd, 0 ) )
		return FALSE;

	*pbValue = ( dwValue != 0 );
	return TRUE;
}

/*
==================
CMixerControls::SetBoolControlValue (0x46E300)
==================
*/
BOOL CMixerControls::SetBoolControlValue( DWORD dwControlID, DWORD cMultipleItems, BYTE bValue )
{
	MIXERCONTROLDETAILS	mcd;
	DWORD				dwValue = bValue;

	mcd.cbStruct = sizeof( mcd );
	mcd.dwControlID = dwControlID;
	mcd.cChannels = 1;
	mcd.cMultipleItems = cMultipleItems;
	mcd.cbDetails = sizeof( dwValue );
	mcd.paDetails = &dwValue;
	return mixerSetControlDetails( (HMIXEROBJ)m_hMixer, &mcd, 0 ) == 0;
}

/*
==================
CMixerControls::GetUnsignedControlValue (0x46E360)
==================
*/
BOOL CMixerControls::GetUnsignedControlValue( DWORD dwControlID, DWORD cMultipleItems, DWORD* pdwValue )
{
	MIXERCONTROLDETAILS	mcd;
	DWORD				dwValue;

	mcd.cbStruct = sizeof( mcd );
	mcd.dwControlID = dwControlID;
	mcd.cChannels = 1;
	mcd.cMultipleItems = cMultipleItems;
	mcd.cbDetails = sizeof( dwValue );
	mcd.paDetails = &dwValue;
	if ( mixerGetControlDetailsA( (HMIXEROBJ)m_hMixer, &mcd, 0 ) )
		return FALSE;

	*pdwValue = dwValue;
	return TRUE;
}

/*
==================
CMixerControls::SetUnsignedControlValue (0x46E3D0)
==================
*/
BOOL CMixerControls::SetUnsignedControlValue( DWORD dwControlID, DWORD cMultipleItems, DWORD dwValue )
{
	MIXERCONTROLDETAILS	mcd;

	mcd.cbStruct = sizeof( mcd );
	mcd.dwControlID = dwControlID;
	mcd.cChannels = 1;
	mcd.cMultipleItems = cMultipleItems;
	mcd.cbDetails = sizeof( dwValue );
	mcd.paDetails = &dwValue;
	return mixerSetControlDetails( (HMIXEROBJ)m_hMixer, &mcd, 0 ) == 0;
}

/*
==================
CreateMixerControls (0x46E430)

The factory the player-config code uses.
==================
*/
IMixerControls* CreateMixerControls( void )
{
	return new CMixerControls;
}
