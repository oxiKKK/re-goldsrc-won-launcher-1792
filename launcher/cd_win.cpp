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
// Purpose: CD audio (background music) via MCI on a worker thread.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Worker command-queue capacity.
#define CD_MAX_QUEUED	10

typedef struct
{
	cdcmd_fn_t	pfn;		// +0
	int			param1;		// +4
	int			param2;		// +8
} cdcmd_t;					// 12 bytes

static int		cdValid;		// 0x4D97D0
static int		playing;		// 0x4D97D4
static int		wasPlaying;		// 0x4D97D8
static int		initialized;	// 0x4D97DC
static int		enabled;		// 0x4D97E0
static int		playLooping;	// 0x4D97E4
static float	cdvolume;		// 0x4D97B4 last bgmvolume seen (0 or 1)
static byte		remap[100];		// 0x4D9750
static byte		maxTrack;		// 0x4D974C
static byte		playTrack;		// 0x4D974D

static UINT		wDeviceID;		// 0x4D9748
static DWORD	mciWnd;			// 0x4D97CC (DWORD)mainwindow for the MCI dwCallback

static double	pauseTime;		// 0x4D97B8
static double	startTime;		// 0x4D97C0
static float	playTime;		// 0x4D97C8 current track length, seconds

int				resumeOnSwitch;	// 0x4D97E8

static CRITICAL_SECTION	cdCS;						// 0x4F9640
static HANDLE			hCDEvent;					// 0x4F9658
static HANDLE			hCDThread;					// 0x4F965C
static DWORD			cdThreadId;					// 0x4F963C
static cdcmd_t			cdQueue[CD_MAX_QUEUED];		// 0x4F95B0 pending commands
static int				cdQueueCount;				// 0x4F9638
static cdcmd_t			cdStaging[CD_MAX_QUEUED];	// 0x4F9528 snapshot while draining
static int				cdStagingCount;				// 0x4F9634

// Worker commands (queued, run on the CD thread).
static void	CDAudio_PlayCommand( int track, int looping );
static void	CDAudio_PauseCommand( void );
static void	CDAudio_ResumeCommand( void );
static void	CDAudio_UpdateCommand( void );
static int	CDAudio_GetAudioDiskInfoCommand( void );
static void	CDAudio_StopCommand( void );
static void	CDAudio_EjectCommand( void );
static void	CDAudio_CloseDoorCommand( void );
static void	CDAudio_SwitchToLauncherCommand( void );
static void	CDAudio_SwitchToEngineCommand( void );

static int	CDAudio_GetAudioDiskInfo( void );
int			CDAudio_OpenDevice( void );

/*
==================
CDAudio_ResetTiming (0x4036D0)
==================
*/
static void CDAudio_ResetTiming( void )
{
	playTime = 0.0f;
	startTime = 0.0;
	pauseTime = 0.0;
}

/*
==================
CDAudio_EjectCommand (0x403710)
==================
*/
static void CDAudio_EjectCommand( void )
{
	CDAudio_StopCommand();
	mciSendCommandA( wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, 0 );
	CDAudio_ResetTiming();
}

/*
==================
CDAudio_Eject (0x403740)
==================
*/
void CDAudio_Eject( void )
{
	CDAudio_QueueCommand( (cdcmd_fn_t)CDAudio_EjectCommand, 0, 0 );
}

/*
==================
CDAudio_CloseDoorCommand (0x403760)
==================
*/
static void CDAudio_CloseDoorCommand( void )
{
	mciSendCommandA( wDeviceID, MCI_SET, MCI_SET_DOOR_CLOSED, 0 );
	CDAudio_ResetTiming();
}

/*
==================
CDAudio_CloseDoor (0x403780)
==================
*/
void CDAudio_CloseDoor( void )
{
	CDAudio_QueueCommand( (cdcmd_fn_t)CDAudio_CloseDoorCommand, 0, 0 );
}

/*
==================
CDAudio_GetAudioDiskInfoCommand (0x4037A0)
==================
*/
static int CDAudio_GetAudioDiskInfoCommand( void )
{
	MCI_STATUS_PARMS	mciStatus;

	cdValid = 0;

	mciStatus.dwItem = MCI_STATUS_READY;
	if ( mciSendCommandA( wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD_PTR)&mciStatus ) )
		return -1;
	if ( !mciStatus.dwReturn )
		return -1;

	mciStatus.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	if ( mciSendCommandA( wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD_PTR)&mciStatus ) )
		return -1;
	if ( !mciStatus.dwReturn )
		return -1;

	maxTrack = mciStatus.dwReturn;
	cdValid = 1;
	return 0;
}

/*
==================
CDAudio_GetAudioDiskInfo (0x403850)
==================
*/
static int CDAudio_GetAudioDiskInfo( void )
{
	CDAudio_QueueCommand( (cdcmd_fn_t)CDAudio_GetAudioDiskInfoCommand, 0, 0 );
	return 0;
}

/*
==================
CDAudio_PlayCommand (0x403870)
==================
*/
static void CDAudio_PlayCommand( int track, int looping )
{
	MCI_STATUS_PARMS	mciStatus;
	MCI_PLAY_PARMS		mciPlay;
	DWORD				length;
	MCIERROR			err;
	int					cdTrack;
	char				szErr[256];

	if ( !enabled )
		return;

	if ( !cdValid )
	{
		CDAudio_GetAudioDiskInfo();
		if ( !cdValid )
			return;
	}

	cdTrack = remap[track];
	if ( cdTrack < 1 || cdTrack > maxTrack )
		return;

	// Audio track?
	mciStatus.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
	mciStatus.dwTrack = cdTrack;
	err = mciSendCommandA( wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD_PTR)&mciStatus );
	if ( err )
	{
		mciGetErrorStringA( err, szErr, sizeof( szErr ) );
		return;
	}
	if ( mciStatus.dwReturn != MCI_CDA_TRACK_AUDIO )
		return;

	// Track length.
	mciStatus.dwItem = MCI_STATUS_LENGTH;
	mciStatus.dwTrack = cdTrack;
	if ( mciSendCommandA( wDeviceID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD_PTR)&mciStatus ) )
		return;

	if ( playing )
	{
		if ( playTrack == cdTrack )
			return;		// already playing this track
		CDAudio_StopCommand();
	}

	length = mciStatus.dwReturn;
	mciPlay.dwCallback = mciWnd;
	mciPlay.dwFrom = MCI_MAKE_TMSF( cdTrack, 0, 0, 0 );
	mciPlay.dwTo = ( length << 8 ) | cdTrack;
	if ( !mciSendCommandA( wDeviceID, MCI_PLAY, MCI_FROM | MCI_TO | MCI_NOTIFY, (DWORD_PTR)&mciPlay ) )
	{
		CDAudio_ResetTiming();
		startTime = engineapi.Sys_FloatTime();
		playLooping = looping;
		playTrack = cdTrack;
		playing = 1;
		// Track length in seconds from the MSF result (minutes*60 + seconds).
		playTime = (float)MCI_MSF_MINUTE( length ) * 60.0f + (float)MCI_MSF_SECOND( length );
		if ( cdvolume == 0.0 )
			CDAudio_PauseCommand();
	}
}

/*
==================
CDAudio_Play (0x403A30)
==================
*/
void CDAudio_Play( int track, int looping )
{
	CDAudio_QueueCommand( (cdcmd_fn_t)CDAudio_PlayCommand, (byte)track, looping );
}

/*
==================
CDAudio_StopCommand (0x403A50)
==================
*/
static void CDAudio_StopCommand( void )
{
	if ( enabled && ( playing || wasPlaying ) )
	{
		playing = 0;
		wasPlaying = 0;
		mciSendCommandA( wDeviceID, MCI_STOP, 0, 0 );
		CDAudio_ResetTiming();
	}
}

/*
==================
CDAudio_Stop (0x403A90)
==================
*/
void CDAudio_Stop( void )
{
	CDAudio_QueueCommand( (cdcmd_fn_t)CDAudio_StopCommand, 0, 0 );
}

/*
==================
CDAudio_PauseCommand (0x403AB0)
==================
*/
static void CDAudio_PauseCommand( void )
{
	MCI_GENERIC_PARMS	mciParms;

	if ( !enabled )
		return;

	if ( playing )
	{
		mciParms.dwCallback = mciWnd;
		mciSendCommandA( wDeviceID, MCI_PAUSE, 0, (DWORD_PTR)&mciParms );

		wasPlaying = playing;
		playing = 0;
		pauseTime = engineapi.Sys_FloatTime();
	}
}

/*
==================
CDAudio_Pause (0x403B10)
==================
*/
void CDAudio_Pause( void )
{
	CDAudio_QueueCommand( (cdcmd_fn_t)CDAudio_PauseCommand, 0, 0 );
}

/*
==================
CDAudio_ResumeCommand (0x403B30)
==================
*/
static void CDAudio_ResumeCommand( void )
{
	MCI_PLAY_PARMS	mciPlay;

	if ( enabled && cdValid && wasPlaying )
	{
		mciPlay.dwCallback = (DWORD)mainwindow;
		mciPlay.dwFrom = MCI_MAKE_TMSF( playTrack, 0, 0, 0 );
		mciPlay.dwTo = MCI_MAKE_TMSF( playTrack + 1, 0, 0, 0 );

		// (sic) MCI_FROM is omitted -- resume plays from the current position.
		if ( mciSendCommandA( wDeviceID, MCI_PLAY, MCI_TO | MCI_NOTIFY, (DWORD_PTR)&mciPlay ) )
		{
			CDAudio_ResetTiming();
		}
		else
		{
			playing = 1;
			startTime += engineapi.Sys_FloatTime() - pauseTime;
			pauseTime = 0.0;
		}
	}
}

/*
==================
CDAudio_Resume (0x403BE0)
==================
*/
void CDAudio_Resume( void )
{
	CDAudio_QueueCommand( (cdcmd_fn_t)CDAudio_ResumeCommand, 0, 0 );
}

/*
==================
CDAudio_SwitchToLauncherCommand (0x403C00)
==================
*/
static void CDAudio_SwitchToLauncherCommand( void )
{
	if ( enabled && playing )
	{
		resumeOnSwitch = 1;
		CDAudio_PauseCommand();
	}
}

/*
==================
CDAudio_SwitchToLauncher (0x403C30)
==================
*/
void CDAudio_SwitchToLauncher( void )
{
	CDAudio_QueueCommand( (cdcmd_fn_t)CDAudio_SwitchToLauncherCommand, 0, 0 );
}

/*
==================
CDAudio_SwitchToEngineCommand (0x403C50)
==================
*/
static void CDAudio_SwitchToEngineCommand( void )
{
	if ( resumeOnSwitch )
	{
		resumeOnSwitch = 0;
		CDAudio_ResumeCommand();
	}
}

/*
==================
CDAudio_SwitchToEngine (0x403C70)
==================
*/
void CDAudio_SwitchToEngine( void )
{
	CDAudio_QueueCommand( (cdcmd_fn_t)CDAudio_SwitchToEngineCommand, 0, 0 );
}

/*
==================
CD_f (0x403C90)
==================
*/
void CD_f( void )
{
	char*	pszCommand;
	int		ret;
	int		n;

	if ( engineapi.Cmd_Argc() < 2 )
		return;

	pszCommand = engineapi.Cmd_Argv( 1 );

	if ( !_strcmpi( pszCommand, "on" ) )
	{
		enabled = 1;
		return;
	}

	if ( !_strcmpi( pszCommand, "off" ) )
	{
		if ( playing )
			CDAudio_Stop();
		enabled = 0;
		return;
	}

	if ( !_strcmpi( pszCommand, "reset" ) )
	{
		enabled = 1;
		if ( playing )
			CDAudio_Stop();
		for ( n = 0; n < 100; ++n )
			remap[n] = n;
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if ( !_strcmpi( pszCommand, "remap" ) )
	{
		ret = engineapi.Cmd_Argc() - 2;
		if ( ret <= 0 )
		{
			// (sic) nothing consumes the scan, so it has no effect.
			for ( n = 1; n < 100; ++n )
			{
				if ( remap[n] != n )
					break;
			}
			return;
		}

		for ( n = 1; n <= ret; ++n )
			remap[n] = atoi( engineapi.Cmd_Argv( n + 1 ) );
		return;
	}

	if ( !_strcmpi( pszCommand, "close" ) )
	{
		CDAudio_CloseDoor();
		return;
	}

	if ( !cdValid )
	{
		CDAudio_GetAudioDiskInfo();
		if ( !cdValid )
			return;
	}

	if ( !_strcmpi( pszCommand, "play" ) )
	{
		CDAudio_Play( atoi( engineapi.Cmd_Argv( 2 ) ), 0 );
		return;
	}

	if ( !_strcmpi( pszCommand, "loop" ) )
	{
		CDAudio_Play( atoi( engineapi.Cmd_Argv( 2 ) ), 1 );
		return;
	}

	if ( !_strcmpi( pszCommand, "stop" ) )
	{
		CDAudio_Stop();
		return;
	}

	if ( !_strcmpi( pszCommand, "pause" ) )
	{
		CDAudio_Pause();
		return;
	}

	if ( !_strcmpi( pszCommand, "resume" ) )
	{
		CDAudio_Resume();
		return;
	}

	if ( !_strcmpi( pszCommand, "eject" ) )
	{
		if ( playing )
			CDAudio_Stop();
		CDAudio_Eject();
		cdValid = 0;
		return;
	}

	if ( !_strcmpi( pszCommand, "info" ) )
	{
		engineapi.Con_Printf( "%u tracks\n", maxTrack );

		if ( playing )
		{
			engineapi.Con_Printf( "Currently %s track %u\n",
				playLooping ? "looping" : "playing",
				playTrack );
		}
		else if ( wasPlaying )
		{
			engineapi.Con_Printf( "Paused %s track %u\n",
				playLooping ? "looping" : "playing",
				playTrack );
		}

		engineapi.Con_Printf( "Volume is %f\n", cdvolume );
	}
}

/*
==================
CDAudio_MessageHandler (0x403F50)
==================
*/
LONG CDAudio_MessageHandler( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if ( lParam != wDeviceID )
		return 1;

	switch ( wParam )
	{
		case MCI_NOTIFY_SUCCESSFUL:
			if ( playing )
			{
				playing = 0;
				if ( playLooping )
					CDAudio_Play( playTrack, 1 );
			}
			break;

		case MCI_NOTIFY_ABORTED:
		case MCI_NOTIFY_SUPERSEDED:
			break;

		case MCI_NOTIFY_FAILURE:
			CDAudio_Stop();
			cdValid = 0;
			break;

		default:
			return 1;
	}

	return 0;
}

/*
==================
CDAudio_UpdateCommand (0x403FE0)

Stops the track once it has played its length plus a little slop; restarts it
when the play was looping.
==================
*/
static void CDAudio_UpdateCommand( void )
{
	if ( playing
	  && playTime != 0.0
	  && startTime != 0.0
	  && playTime + 2.0 <= engineapi.Sys_FloatTime() - startTime )
	{
		playing = 0;
		if ( playLooping )
			CDAudio_Play( playTrack, 1 );
	}
}

/*
==================
CDAudio_Update (0x404060)
==================
*/
void CDAudio_Update( void )
{
	if ( !enabled )
		return;

	if ( engineapi.Cvar_VariableInt( "bgmvolume" ) != cdvolume )
	{
		if ( cdvolume != 0.0 )
		{
			engineapi.Cvar_SetValue( "bgmvolume", 0.0 );
			cdvolume = 0.0;
			CDAudio_Pause();
			CDAudio_QueueCommand( (cdcmd_fn_t)CDAudio_UpdateCommand, 0, 0 );
			return;
		}

		engineapi.Cvar_SetValue( "bgmvolume", 1.0 );
		cdvolume = 1.0;
		CDAudio_Resume();
	}

	CDAudio_QueueCommand( (cdcmd_fn_t)CDAudio_UpdateCommand, 0, 0 );
}

/*
==================
CDAudio_OpenDevice (0x404110)
==================
*/
int CDAudio_OpenDevice( void )
{
	MCI_OPEN_PARMS	mciOpen;
	MCI_SET_PARMS	mciSet;
	int				i;

	if ( CheckParm( "-nocdaudio", NULL ) )
		return 0;

	CDAudio_ResetTiming();
	mciWnd = (DWORD)mainwindow;

	mciOpen.lpstrDeviceType = "cdaudio";
	if ( mciSendCommandA( 0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE, (DWORD_PTR)&mciOpen ) )
		return -1;

	wDeviceID = mciOpen.wDeviceID;

	mciSet.dwTimeFormat = MCI_FORMAT_TMSF;
	if ( mciSendCommandA( wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&mciSet ) )
	{
		mciSendCommandA( wDeviceID, MCI_CLOSE, 0, 0 );
		return -1;
	}

	for ( i = 0; i < 100; ++i )
		remap[i] = i;

	initialized = 1;
	enabled = 1;

	// (sic) CDAudio_GetAudioDiskInfo only queues, so cdValid is never reset here.
	if ( CDAudio_GetAudioDiskInfo() )
		cdValid = 0;

	return 0;
}

/*
==================
CDAudio_WorkerThread (0x4654D0)
==================
*/
static DWORD WINAPI CDAudio_WorkerThread( LPVOID param )
{
	int		i;

	CDAudio_OpenDevice();

	while ( 1 )
	{
		do
		{
			WaitForSingleObject( hCDEvent, INFINITE );
			EnterCriticalSection( &cdCS );
			cdStagingCount = cdQueueCount;
			memcpy( cdStaging, cdQueue, sizeof( cdcmd_t ) * cdQueueCount );
			cdQueueCount = 0;
			ResetEvent( hCDEvent );
			LeaveCriticalSection( &cdCS );
		} while ( cdStagingCount <= 0 );

		for ( i = 0; i < cdStagingCount; ++i )
		{
			if ( cdStaging[i].pfn )
				cdStaging[i].pfn( cdStaging[i].param1, cdStaging[i].param2 );
		}
	}
}

/*
==================
CDAudio_Shutdown (0x465570)
==================
*/
void CDAudio_Shutdown( void )
{
	if ( !hCDEvent )
		return;

	TerminateThread( hCDThread, 1 );
	CloseHandle( hCDEvent );
	DeleteCriticalSection( &cdCS );
}

/*
==================
CDAudio_Init (0x4655B0)
==================
*/
void CDAudio_Init( void )
{
	InitializeCriticalSection( &cdCS );
	hCDEvent = CreateEventA( NULL, TRUE, FALSE, NULL );
	hCDThread = CreateThread( NULL, 0, CDAudio_WorkerThread, NULL, 0, &cdThreadId );
}

/*
==================
CDAudio_QueueCommand (0x4655F0)
==================
*/
int CDAudio_QueueCommand( cdcmd_fn_t pfn, int param1, int param2 )
{
	int		added = 0;
	int		i;

	EnterCriticalSection( &cdCS );

	// Skip the append if the same command is already pending.
	for ( i = 0; i < cdQueueCount; ++i )
	{
		if ( cdQueue[i].pfn == pfn )
			break;
	}

	if ( i >= cdQueueCount && cdQueueCount < CD_MAX_QUEUED )
	{
		cdQueue[cdQueueCount].pfn = pfn;
		cdQueue[cdQueueCount].param1 = param1;
		cdQueue[cdQueueCount].param2 = param2;
		cdQueueCount++;
		added = 1;
	}

	LeaveCriticalSection( &cdCS );

	if ( added )
		SetEvent( hCDEvent );

	return added;
}
