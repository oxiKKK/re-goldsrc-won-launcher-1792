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
// Purpose: declares IMixerControls and CMixerControls, the WINMM mixer
//          wrapper.
//
// $NoKeywords: $
//=============================================================================

#ifndef VOLUME_H
#define VOLUME_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>
#include <mmsystem.h>

class IMixerControls
{
public:
	// The three controls the launcher looks for on the default mixer.
	enum
	{
		MIXER_CONTROL_MICBOOST	= 0,	// mic +20dB boost switch
		MIXER_CONTROL_MICVOLUME	= 1,	// mic record (wave-in) volume
		MIXER_CONTROL_MICMUTE	= 2,	// mic mute on the speakers output
		MIXER_CONTROL_COUNT		= 3
	};

	// IMixerControls::~IMixerControls (0x46DE10)
	virtual ~IMixerControls() {}
	virtual void	Release() = 0;									// slot 1
	virtual BOOL	GetValue( int iControl, float* pflValue ) = 0;	// slot 2
	virtual BOOL	SetValue( int iControl, float flValue ) = 0;		// slot 3
};

// One discovered mixer control (CMixerControls keeps three, 12 bytes each).
typedef struct mixercontrol_s
{
	DWORD	dwControlID;	// +0  MIXERCONTROL.dwControlID
	DWORD	cMultipleItems;	// +4  MIXERCONTROL.cMultipleItems
	BYTE	bFound;			// +8  the line + control exist on this mixer
} mixercontrol_t;

class CMixerControls : public IMixerControls
{
public:
	CMixerControls();
	virtual ~CMixerControls();

	BOOL	Init();
	void	Close();

	virtual BOOL	GetValue( int iControl, float* pflValue );
	virtual BOOL	SetValue( int iControl, float flValue );
	virtual void	Release();

	void	ResetState();

	// mixerGet/SetControlDetails, one 4-byte value on one channel.
	BOOL	GetBoolControlValue( DWORD dwControlID, DWORD cMultipleItems, BYTE* pbValue );
	BOOL	SetBoolControlValue( DWORD dwControlID, DWORD cMultipleItems, BYTE bValue );
	BOOL	GetUnsignedControlValue( DWORD dwControlID, DWORD cMultipleItems, DWORD* pdwValue );
	BOOL	SetUnsignedControlValue( DWORD dwControlID, DWORD cMultipleItems, DWORD dwValue );

	HMIXER			m_hMixer;							// +4   mixerOpen(0) handle (NULL when closed)
	mixercontrol_t	m_controls[MIXER_CONTROL_COUNT];	// +8   boost / mic volume / mic mute records
};

IMixerControls*	CreateMixerControls( void );

#endif // VOLUME_H
