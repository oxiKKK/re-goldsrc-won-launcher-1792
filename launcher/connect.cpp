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
// Purpose: drive a candidate server list to a live connection -- wait out the
//          master query, ping the candidates, issue the engine "connect".
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

enum
{
	CONNECT_STAGE_QUERY   = 1,	// waiting on the master/browser query
	CONNECT_STAGE_PING    = 2,	// pinging the candidates
	CONNECT_STAGE_CONNECT = 3,	// "connect" issued, watching cls.state
};

enum
{
	CONNECT_OUTCOME_NONE   = 0,
	CONNECT_OUTCOME_JOINED = 1,
	CONNECT_OUTCOME_FAILED = 2,
};

#define CONNECT_QUERY_TIMEOUT	10.0f	// stage-1 cap
#define CONNECT_PING_GRACE		7.5		// keeps the ping stage alive
#define CONNECT_MSG_INTERVAL	0.5		// status-cvar throttle

// The connect state block: one contiguous run of file statics at 0x4E17F0-0x4E1998.
static CServerInfo*	g_pQueryCursor;			// 0x4E17F8  cursor over the candidates
static CServerInfo*	g_pConnectBest;			// 0x4E17FC  best candidate (CollectJoinable)
static CServerInfo*	g_pConnectCurrent;		// 0x4E1800  candidate being tried
static int		g_nConnectOutcome;			// 0x4E1818
static int		g_bConnectDirect;			// 0x4E181C  a specific server was requested
static int		g_bGameInfoValid;			// 0x4E1820  last GetGameInfo result
static int		g_bConnectHandshaking;		// 0x4E1824  ca_connected seen; timeout restarted
static int		g_bConnectSucceeded;		// 0x4E1828  what Connect_Run returns
static int		g_bConnectIssued;			// 0x4E182C  "connect" issued for this candidate
static float	g_flConnectElapsed;			// 0x4E1930
static float	g_flConnectTimeout;			// 0x4E1934
static int		g_nPingTotal;				// 0x4E1938
static int		g_nPingLastRemaining;		// 0x4E193C  last "N servers left" value
static int		g_nPingRemaining;			// 0x4E1940
static int		g_nJoinableCount;			// 0x4E1944
static CNetGameDlg*	g_pConnectSheet;		// 0x4E1950  the sheet driving this connect
static double	g_flPingElapsed;			// 0x4E1958
static double	g_flConnectMsgStamp;		// 0x4E1960
static double	g_flConnectUnk1968;			// 0x4E1968  stamped, never read back
static double	g_flConnectNow;				// 0x4E1970
static double	g_flConnectStageStart;		// 0x4E1978
static double	g_flPingNow;				// 0x4E1990
static int		g_nConnectStage;			// 0x4E1998
static float	g_flPingPassStart;			// 0x4E3914  ping-pass start (outside the run)
static int		g_nConnectLastState;		// 0x4CD968  previous cls.state, for edges
static int		g_bAutoConnectBusy;			// 0x4EA8F0  a sheet-less auto-connect is running

// Per-phase counters Connect_BeginPingStage resets; only the socket/info/connect
// three are read back by the ping service pass.
static int		g_nPingSockets;				// 0x4E1804  open query sockets
static int		g_nPingInfoSent;			// 0x4E1810  info retransmits
static int		g_nPingConnects;			// 0x4E1814  connect retransmits
static int		g_nPingUnk17F0;				// 0x4E17F0
static int		g_nPingUnk17F4;				// 0x4E17F4
static int		g_nPingUnk1808;				// 0x4E1808
static int		g_nPingUnk180C;				// 0x4E180C
static int		g_nPingUnk1948;				// 0x4E1948
static double	g_flPingStamp0;				// 0x4E1980
static double	g_flPingStamp1;				// 0x4E1988

// The engine's view of the connection, refreshed through engineapi.GetGameInfo.
static GameInfo_t	g_connectGameInfo;		// 0x4E1D60

// (sic) a local of Connect_ServiceConnecting in the binary, written on one
// pass and read on the next off a stack slot the fixed call chain keeps alive.
static char		g_szConnectStatus[256];

static void	Connect_BeginPingStage( void );
static int	Connect_ServicePinging( void );
static void	Connect_BeginConnectStage( void );
static int	Connect_ServiceConnecting( void );

/*
==================
Connect_GetTimeout (0x4055D0)
==================
*/
static double Connect_GetTimeout( void )
{
	char*	pszValue = 0;

	if ( !CheckParm( "-timeout", &pszValue ) || !pszValue )
		return 120.0;

	double	flTimeout = atoi( pszValue );
	return ( flTimeout < 20.0 ) ? 20.0 : flTimeout;
}

/*
==================
Connect_CheckSignon (0x405630)
==================
*/
static int Connect_CheckSignon( void )
{
	GameInfo_t	gi;

	if ( !g_bConnectInProgress
	  || !engineapi.GetGameInfo( &gi, 0 )
	  || gi.state != ca_active
	  || !gi.signon )
		return 0;

	g_bConnectSucceeded = 1;
	return 1;
}

/*
==================
Connect_ServiceStage (0x405690)

advance the three-stage machine one step.
==================
*/
static int Connect_ServiceStage( void )
{
	if ( g_nConnectStage == CONNECT_STAGE_QUERY )
	{
		if ( !g_pConnectSheet->HasPendingQuery() )
		{
			engineapi.Cvar_Set( "scr_connectmsg", Launcher_LoadString( IDS_SERVERS_LISTREC ) );
			g_nConnectStage = CONNECT_STAGE_PING;
			Connect_BeginPingStage();
			return 1;
		}

		// still querying: mirror the sheet's status line and hold the clock
		engineapi.Cvar_Set( "scr_connectmsg", g_pConnectSheet->m_szStatus );
		g_flConnectStageStart = engineapi.Sys_FloatTime();
		return 1;
	}

	if ( g_nConnectStage == CONNECT_STAGE_PING )
	{
		if ( !Connect_ServicePinging() )
		{
			engineapi.Cvar_Set( "scr_connectmsg", Launcher_LoadString( IDS_SERVERS_CONNECTING ) );
			g_nConnectStage = CONNECT_STAGE_CONNECT;
			Connect_BeginConnectStage();
		}
		return 1;
	}

	if ( g_nConnectStage != CONNECT_STAGE_CONNECT || Connect_ServiceConnecting() )
		return 1;

	if ( !g_bConnectSucceeded && engineapi.Cbuf_AddText )
		engineapi.Cbuf_AddText( "disconnect\n" );

	return 0;
}

/*
==================
Connect_Pump (0x405780)

one frame of the connect loop.  Returns 0 while running,
nonzero once it is finished (the value is the connect result).
==================
*/
static int Connect_Pump( void )
{
	if ( g_pConnectSheet )
		g_pConnectSheet->Pump();

	if ( !Connect_ServiceStage() )
		return 1;

	if ( engineapi.Sys_FloatTime() - g_flConnectStageStart >= g_flConnectTimeout
	  && g_nConnectStage == CONNECT_STAGE_QUERY )
	{
		// The master never answered.  With no list to fall back on there is nothing
		// left to try, so bail out of the whole attempt.
		if ( !g_pConnectSheet->m_pServerListHead )
			return -1;

		engineapi.Cvar_Set( "scr_connectmsg", Launcher_LoadString( IDS_QUICK_USINGPREVIOUSLIST ) );
		g_nConnectStage = CONNECT_STAGE_PING;
		Connect_BeginPingStage();
	}

	Eng_Frame( 0 );

	if ( gDLLState == DLL_PAUSED )
		return 0;

	if ( !g_bConnectInProgress )
		return g_bConnectSucceeded;

	if ( !Connect_CheckSignon() )
		return g_bConnectSucceeded;

	return 1;
}

/*
==================
Connect_Run (0x405830)

seed the state block and loop until the machine settles.
==================
*/
int Connect_Run( CNetGameDlg* pSheet, CServerInfo* pInfo )
{
	g_flConnectTimeout    = CONNECT_QUERY_TIMEOUT;
	g_pConnectSheet       = NULL;
	g_bConnectHandshaking = 0;
	g_bGameInfoValid      = 0;
	g_bConnectDirect      = ( pInfo != NULL );
	g_nConnectStage       = 0;
	g_bConnectInProgress     = 0;
	g_bConnectSucceeded   = 0;
	g_nConnectOutcome     = CONNECT_OUTCOME_NONE;
	g_flConnectStageStart = engineapi.Sys_FloatTime();

	int	bOwnSheet = 0;

	engineapi.Cvar_Set( "scr_connectmsg", "Starting Network..." );
	g_bBlockMouseEvents = 1;
	Eng_Frame( 0 );

	if ( pInfo )
	{
		g_pQueryCursor    = pInfo;
		g_pConnectBest    = pInfo;
		g_pConnectCurrent = pInfo;
		g_pConnectSheet   = pSheet;
		g_nConnectStage   = CONNECT_STAGE_CONNECT;
		Connect_BeginConnectStage();
	}
	else
	{
		// No specific server: stand up a private sheet and run a master query.
		bOwnSheet = 1;
		g_bAutoConnectBusy = 1;

		g_pConnectSheet = new CNetGameDlg( NULL, 0 );
		if ( !g_pConnectSheet )
			goto bail;

		netfilter_t	filter;
		g_pConnectSheet->BuildFilter( &filter );
		g_pConnectSheet->QueryMaster( &filter );
		g_nConnectStage = CONNECT_STAGE_QUERY;
	}

	g_bGameInfoValid = engineapi.GetGameInfo( &g_connectGameInfo, 0 );
	if ( !g_bGameInfoValid )
	{
		if ( bOwnSheet && g_pConnectSheet )
			delete g_pConnectSheet;
bail:
		g_bBlockMouseEvents = 0;
		g_bAutoConnectBusy  = 0;
		engineapi.Cvar_Set( "scr_connectmsg", "" );
		return 0;
	}

	// Drop any live session before taking over the connection.
	if ( g_connectGameInfo.state != ca_disconnected )
	{
		engineapi.Cbuf_AddText( "disconnect\n" );
		gBackground = 1;
		Eng_Frame( 0 );
	}

	while ( !Connect_Pump() )
		;

	engineapi.Cvar_Set( "scr_connectmsg", "" );
	g_bBlockMouseEvents = 0;

	if ( bOwnSheet && g_pConnectSheet )
		delete g_pConnectSheet;

	g_bAutoConnectBusy = 0;
	return g_bConnectSucceeded;
}

/*
==================
Connect_BeginPingStage (0x405A30)
==================
*/
static void Connect_BeginPingStage( void )
{
	int	nCount;

	g_nPingUnk1948  = 0;
	g_pQueryCursor = NULL;
	g_nPingLastRemaining = 0;

	if ( g_bConnectDirect )
	{
		nCount = 1;
	}
	else
	{
		engineapi.Cvar_Set( "scr_connectmsg", Launcher_LoadString( IDS_SERVERS_REQUESTINFO ) );
		ServerBrowser_RequestPings( g_pConnectSheet->m_pServerListHead );
		nCount = ServerBrowser_RefreshActiveTimes( g_pConnectSheet->m_pServerListHead );
	}

	g_nJoinableCount = nCount;
	g_nPingTotal     = nCount;
	g_nPingConnects  = 0;
	g_nPingInfoSent  = 0;
	g_nPingUnk180C    = 0;
	g_nPingUnk1808    = 0;

	g_flPingStamp1 = engineapi.Sys_FloatTime();
	g_flPingStamp0 = g_flPingStamp1;

	g_nPingSockets = 0;
	g_nPingUnk17F4  = 0;
	g_nPingUnk17F0  = 0;

	g_flPingPassStart = (float)engineapi.Sys_FloatTime();
}

/*
==================
Connect_ServicePinging (0x405AF0)

one pass of the per-record query machine.
Returns nonzero while the stage should keep running.
==================
*/
static int Connect_ServicePinging( void )
{
	g_nPingRemaining = 0;

	if ( g_bConnectDirect )
	{
		// a specific server was asked for: the cursor already points at it
	}
	else
	{
		g_pQueryCursor = g_pConnectSheet->m_pServerListHead;
	}

	for ( CServerInfo* p = g_pQueryCursor; p; p = g_pQueryCursor )
	{
		if ( !g_bConnectDirect )
		{
			// LAN records answer their own broadcast; filtered ones are not candidates.
			if ( p->m_bLan )
				goto next;
			if ( p->GetFiltered() )
				goto next;
		}

		g_flPingNow = engineapi.Sys_FloatTime();
		p = g_pQueryCursor;

		if ( p->m_nStatus == SVQ_IDLE || p->m_nStatus == SVQ_DEAD )
		{
			// Finished or written off: give the socket back.
			if ( p->m_pSocket )
			{
				--g_nPingSockets;
				p->CloseSocket();
			}
			goto next;
		}

		++g_nPingRemaining;
		g_flPingElapsed = g_flPingNow - p->m_dSendTime;

		if ( g_flPingElapsed < g_pConnectSheet->m_dTimeout )
		{
			if ( p->m_nStatus == SVQ_QUEUED )
			{
				// Hold the record queued while every socket slot is busy.
				if ( g_nPingSockets >= g_pConnectSheet->m_nMaxSockets )
				{
					p->m_dSendTime = engineapi.Sys_FloatTime();
					goto next;
				}
				if ( !p->OpenConnection() )
					goto next;
				++g_nPingSockets;
				p->ResetRetry();
				p = g_pQueryCursor;
			}

			if ( p->m_nStatus == SVQ_SOCKET_OPEN )
			{
				if ( !p->Connect() )
					goto next;
				p->m_dSendTime = engineapi.Sys_FloatTime();
				p->ResetRetry();
				p = g_pQueryCursor;
			}

			if ( p->m_nStatus == SVQ_CONNECT_RETRY )
				goto next;

			if ( p->m_nStatus == SVQ_CONNECTED )
			{
				p->SendInfoRequest();
				p->m_dSendTime = engineapi.Sys_FloatTime();
				p = g_pQueryCursor;
			}

			if ( p->m_nStatus == SVQ_INFO_SENT )
				goto next;

			p->m_nStatus = SVQ_DEAD;
			goto next;
		}

		// The state timed out: retransmit until the retry budget runs out.
		{
			int	nRetry = p->m_nRetry;
			int	nState = p->m_nStatus;

			p->m_nRetry = nRetry + 1;

			if ( nRetry >= g_pConnectSheet->m_nRetries )
			{
				if ( nState < SVQ_INFO_DONE )
				{
					p->m_nStatus     = SVQ_IDLE;
					p->m_bNoResponse = 1;
				}
				else
				{
					p->m_nStatus = SVQ_DEAD;
				}
				goto next;
			}

			if ( nState == SVQ_CONNECT_RETRY )
			{
				p->Connect();
				p->m_dSendTime = engineapi.Sys_FloatTime();
				++g_nPingConnects;
				goto next;
			}

			p = g_pQueryCursor;
			if ( nState == SVQ_INFO_SENT )
			{
				p->SendInfoRequest();
				p->m_dSendTime = engineapi.Sys_FloatTime();
				++g_nPingInfoSent;
			}
		}

next:
		g_pQueryCursor = g_pQueryCursor->m_pNext;
		if ( !g_pQueryCursor )
			break;
	}

	if ( g_nPingLastRemaining != g_nPingRemaining )
	{
		char	szMsg[256];
		sprintf( szMsg, "%i servers left...\n", g_nPingRemaining );
		engineapi.Cvar_Set( "scr_connectmsg1", szMsg );
	}
	g_nPingLastRemaining = g_nPingRemaining;

	if ( !g_nPingRemaining )
		return 0;

	// Keep going while a socket slot is still owed, or inside the grace window.
	return g_nPingRemaining >= g_pConnectSheet->m_nMaxSockets
		|| engineapi.Sys_FloatTime() - g_flPingPassStart <= CONNECT_PING_GRACE;
}

/*
==================
Connect_BeginConnectStage (0x405DB0)
==================
*/
static void Connect_BeginConnectStage( void )
{
	char*	pszGame = NULL;
	char*	pEnd;
	double	flTimeout;

	g_bConnectSucceeded = 0;
	g_bConnectIssued    = 0;

	flTimeout = Connect_GetTimeout();

	g_bConnectHandshaking = 0;
	g_flConnectTimeout    = (float)flTimeout;

	if ( g_bConnectDirect )
		return;

	g_pConnectCurrent = NULL;
	g_pConnectBest    = NULL;
	ServerBrowser_CollectJoinable( g_pConnectSheet, &g_pConnectCurrent, &g_pConnectBest,
								   &g_nJoinableCount, g_bGameInfoValid );

	if ( g_pConnectBest && g_nJoinableCount > 0 )
	{
		ServerBrowser_SortList( g_nJoinableCount, g_pConnectCurrent );
		return;
	}

	// Nothing joinable: report it against the current mod's display name.  The
	// binary has no local to copy into and trims GetKeyString's buffer in
	// place; the literal fallback is left alone, because writing into it would
	// fault on a read-only .rdata.
	if ( g_pCurrentMod )
		pszGame = g_pCurrentMod->GetKeyString( "game" );

	if ( pszGame && *pszGame )
	{
		for ( pEnd = pszGame + strlen( pszGame ) - 1; pEnd > pszGame && *pEnd == ' '; --pEnd )
			*pEnd = 0;
	}
	else
	{
		pszGame = (char*)"Half-Life";
	}

	sprintf( g_connectGameInfo.szStatus, Launcher_LoadString( IDS_QUICK_NOSERVERS ), pszGame );
}

/*
==================
Connect_ServiceConnecting (0x405EA0)

stage 3: walk the joinable chain, issue the
engine connect, and follow cls.state.  Returns nonzero while the stage is running.
==================
*/
static int Connect_ServiceConnecting( void )
{
	if ( !g_pConnectCurrent || g_bConnectSucceeded || gDLLState != DLL_ACTIVE )
		return 0;

	g_bGameInfoValid = engineapi.GetGameInfo( &g_connectGameInfo, 0 );

	if ( !g_bConnectIssued )
	{
		if ( g_bConnectSucceeded )
			return 1;

		Eng_ConnectToServer( g_pConnectCurrent );
		g_bConnectInProgress    = 1;
		g_nConnectLastState  = ca_disconnected;
		g_nConnectOutcome    = CONNECT_OUTCOME_NONE;
		g_flConnectTimeout   = (float)Connect_GetTimeout();

		g_flConnectStageStart = engineapi.Sys_FloatTime();
		g_flConnectNow        = g_flConnectStageStart;
		g_flConnectUnk1968    = g_flConnectStageStart;
		g_flConnectMsgStamp   = g_flConnectStageStart;

		sprintf( g_szConnectStatus, "" );
		g_bGameInfoValid = engineapi.GetGameInfo( &g_connectGameInfo, 0 );
		g_bConnectIssued = 1;
		return 1;
	}

	if ( g_bConnectSucceeded )
		return 1;

	g_flConnectNow     = engineapi.Sys_FloatTime();
	g_flConnectElapsed = (float)( g_flConnectNow - g_flConnectStageStart );

	if ( !g_bConnectHandshaking && g_flConnectElapsed > g_flConnectTimeout )
	{
		// Out of time on this candidate: move to the next one.
		g_bConnectIssued = 0;
		if ( !g_pConnectCurrent )
			return 0;
		if ( g_bConnectDirect )
			g_pConnectCurrent = NULL;
		else
			g_pConnectCurrent = g_pConnectCurrent->m_pJoinNext;
		return 1;
	}

	if ( g_flConnectNow - g_flConnectMsgStamp > CONNECT_MSG_INTERVAL )
	{
		g_flConnectMsgStamp = g_flConnectNow;

		if ( g_bGameInfoValid )
		{
			if ( strlen( g_connectGameInfo.szStatus ) )
				sprintf( g_szConnectStatus, g_connectGameInfo.szStatus );
			g_bConnectHandshaking = 0;

			int	nState = g_connectGameInfo.state;
			if ( nState != g_nConnectLastState )
			{
				if ( nState != ca_disconnected )
					g_flConnectStageStart = engineapi.Sys_FloatTime();
				g_nConnectLastState = nState;
			}

			switch ( nState )
			{
			case ca_active:
				g_nConnectOutcome = CONNECT_OUTCOME_JOINED;
				break;
			case ca_disconnected:
				g_nConnectOutcome = CONNECT_OUTCOME_FAILED;
				break;
			case ca_connected:
				// handshaking: restart the clock and skip the countdown line
				g_flConnectStageStart = engineapi.Sys_FloatTime();
				g_bConnectHandshaking = 1;
				break;
			}
		}

		char	szMsg[256];
		if ( g_bConnectHandshaking )
		{
			sprintf( szMsg, Launcher_LoadString( IDS_STATUS_CONNECTING ), (LPCSTR)g_pConnectCurrent->m_strName );
			engineapi.Cvar_Set( "scr_connectmsg", szMsg );
			engineapi.Cvar_Set( "scr_connectmsg1", g_szConnectStatus );
		}
		else
		{
			sprintf( szMsg, Launcher_LoadString( IDS_STATUS_CONNECTING ), (LPCSTR)g_pConnectCurrent->m_strName );
			engineapi.Cvar_Set( "scr_connectmsg", szMsg );

			// IDS_SECONDS_LEFT is "%i seconds left..."; the binary truncates with
			// __ftol and pushes an int, not a double.
			sprintf( szMsg, Launcher_LoadString( IDS_SECONDS_LEFT ),
					 (int)( g_flConnectTimeout - g_flConnectElapsed ) );
			engineapi.Cvar_Set( "scr_connectmsg1", szMsg );
			engineapi.Cvar_Set( "scr_connectmsg2", g_szConnectStatus );
		}

		if ( g_nConnectOutcome == CONNECT_OUTCOME_FAILED )
			Sleep( 750 );
	}

	if ( !g_nConnectOutcome )
		return 1;

	g_bConnectIssued = 0;

	if ( g_nConnectOutcome == CONNECT_OUTCOME_JOINED )
	{
		g_bConnectSucceeded = 1;
		char	szMsg[256];
		sprintf( szMsg, Launcher_LoadString( IDS_STATUS_CONNECTIONESTABLISHED ), (LPCSTR)g_pConnectCurrent->m_strName );
		engineapi.Cvar_Set( "scr_connectmsg", szMsg );
		return 0;
	}

	if ( !g_pConnectCurrent )
		return 0;
	if ( g_bConnectDirect )
	{
		g_pConnectCurrent = NULL;
		return 1;
	}
	g_pConnectCurrent = g_pConnectCurrent->m_pJoinNext;
	return 1;
}

/*
==================
Launcher_ConnectAndLaunch (0x406270)

start the engine and hand it the connect.
==================
*/
int Launcher_ConnectAndLaunch( CNetGameDlg* pSheet, CServerInfo* pInfo )
{
	char	szPassword[512];
	int		bHavePassword = 0;

	if ( !pInfo )
		goto launch;

	// A protocol the launcher does not speak: say so and give up before starting.
	if ( pInfo->m_nProtocol && pInfo->m_nProtocol != g_nDefaultProtocol )
	{
		char	szMsg[2048];
		sprintf( szMsg, Launcher_LoadString( IDS_CONNECT_PROTOCOLBAD ) );

		CPromptDlg	prompt( 1, NULL );
		prompt.SetMessage( szMsg );
		prompt.DoModal();
		return 0;
	}

	if ( !pInfo->m_bPassword )
		goto launch;

	{
		// Already have a password on the command line or in the cvar?  Use it.
		char*	pszExisting = NULL;
		if ( engineapi.Cvar_VariableString )
		{
			pszExisting = engineapi.Cvar_VariableString( "password" );
		}
		else
		{
			CheckParm( "+password", &pszExisting );
			if ( !pszExisting )
				CheckParm( "-password", &pszExisting );
		}

		if ( pszExisting && *pszExisting && _strcmpi( pszExisting, "none" ) )
			goto launch;
	}

	{
		CInputDlg	input( 0 );
		input.SetPrompt( Launcher_LoadString( IDS_MULTI_NEEDPASSWORD ) );

		int	nLen = 0;
		if ( input.DoModal() != IDOK
		  || ( nLen = input.m_strInput.GetLength() ) == 0
		  || nLen > 128 )
			return 0;

		bHavePassword = 1;
		sprintf( szPassword, "password \"%s\"\n", (LPCSTR)input.m_strInput );
	}

launch:
	if ( !Launcher_StartEngine( 0 ) )
		return 0;

	if ( bHavePassword )
		engineapi.Cbuf_AddText( szPassword );

	Eng_Frame( 1 );
	Eng_SetSubState( ENG_ESCAPEEXITS );

	g_bChangingVideoModes = 1;
	int	bJoined = Connect_Run( pSheet, pInfo );
	g_bChangingVideoModes = 0;

	engineapi.Cvar_Set( "scr_connectmsg",  "0" );
	engineapi.Cvar_Set( "scr_connectmsg1", "0" );
	engineapi.Cvar_Set( "scr_connectmsg2", "0" );

	// Only a live session with a signon counts as joined.
	GameInfo_t	gi;
	if ( bJoined && engineapi.GetGameInfo( &gi, 0 ) )
		bJoined = ( gi.state == ca_active && gi.signon );

	if ( engineapi.GameSetSubState )
		engineapi.GameSetSubState( ENG_RESET );

	if ( !bJoined || gDLLState == DLL_PAUSED )
	{
		bJoined     = 0;
		gBackground = 0;
		Eng_GameSetState( DLL_PAUSED );
	}
	else if ( engineapi.Cbuf_AddText )
	{
		engineapi.Cbuf_AddText( "hideconsole\n" );
		Rate_ApplyFromConfig();
	}

	return bJoined;
}

/*
==================
Launcher_HandleConnectFailure (0x406650)

decide whether the failed attempt is
worth retrying.  Returns nonzero after queueing a password for another go.
==================
*/
int Launcher_HandleConnectFailure( void )
{
	GameInfo_t	gi;

	if ( !engineapi.GetGameInfo( &gi, 0 ) || !strlen( g_connectGameInfo.szStatus ) )
		return 0;

	if ( !strstr( g_connectGameInfo.szStatus, "BADPASSWORD" ) )
	{
		// Not a password problem.  Suppress the box when the auth layer already
		// reported an error of its own.
		void*	pAuth = crypt.GetAuthObject ? crypt.GetAuthObject() : NULL;
		if ( pAuth && CryptApi_AuthHasError( pAuth ) )
			return 0;

		char	szMsg[2048];
		char*	pszDetail = strstr( g_connectGameInfo.szStatus, ":" );
		if ( pszDetail )
		{
			sprintf( szMsg, Launcher_LoadString( IDS_CONNECT_FAILURE ), pszDetail + 1 );
		}
		else
		{
			strcpy( g_connectGameInfo.szStatus, "Could not complete connection attempt." );
			sprintf( szMsg, Launcher_LoadString( IDS_CONNECT_FAILURE ), g_connectGameInfo.szStatus );
		}

		CPromptDlg	prompt( 1, NULL );
		prompt.SetTextAlign( DT_LEFT );
		prompt.SetMessage( szMsg );
		prompt.DoModal();
		return 0;
	}

	CInputDlg	input( 0 );
	input.SetPrompt( Launcher_LoadString( IDS_MULTI_NEEDPASSWORD ) );

	if ( input.DoModal() != IDOK )
		return 0;

	int	nLen = input.m_strInput.GetLength();
	if ( nLen == 0 || nLen > 128 || !engineapi.Cbuf_AddText )
		return 0;

	char	szCmd[512];
	sprintf( szCmd, "password \"%s\"\n", (LPCSTR)input.m_strInput );
	engineapi.Cbuf_AddText( szCmd );
	return 1;
}

