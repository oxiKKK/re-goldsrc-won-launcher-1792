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
// Purpose: CModInfoSocket, the mod-info query socket.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/////////////////////////////////////////////////////////////////////////////
// CModInfoSocket::CModInfoSocket (0x42F7B0)

CModInfoSocket::CModInfoSocket( mod_t* pModList )
{
	m_pMsg         = new CMessageBuffer( 0x2000 );
	m_nState       = MODINFO_QUERYING;
	m_pModList     = pModList;
	m_nRetriesLeft = MODINFO_RETRIES;
	m_flTimeout    = MODINFO_TIMEOUT;
	m_flLastSend   = 0.0f;
}

/////////////////////////////////////////////////////////////////////////////
// CModInfoSocket::~CModInfoSocket (0x42F870)

CModInfoSocket::~CModInfoSocket()
{
	if ( m_pMsg )
	{
		m_pMsg->SZ_Clear();
		delete m_pMsg;
		m_pMsg = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModInfoSocket::OnReceive (0x42F8D0)
//
// CAsyncSocket slot 7.

void CModInfoSocket::OnReceive( int nErrorCode )
{
	m_nState = MODINFO_ERROR;	// assume failure until valid bytes arrive

	if ( nErrorCode )
	{
		CAsyncSocket::OnReceive( nErrorCode );
		m_nBytesRead = 0;
		return;
	}

	m_pMsg->SZ_Clear();
	m_nBytesRead = Receive( m_pMsg->GetData(), m_pMsg->GetMaxSize(), 0 );
	if ( m_nBytesRead <= 0 )
		return;

	m_pMsg->SetCurSize( m_nBytesRead );
	m_pMsg->MSG_BeginReading();

	if ( m_pMsg->MSG_ReadLong() != -1 )		// connectionless header
	{
		m_nBytesRead = 0;
		return;
	}

	if ( m_pMsg->MSG_ReadByte() == 'y' )	// mod-info reply opcode
	{
		ParseModList();
		CAsyncSocket::OnReceive( 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModInfoSocket::Pump (0x42F980)

BOOL CModInfoSocket::Pump()
{
	float	flNow = (float)engineapi.Sys_FloatTime();

	if ( m_nState )
		return FALSE;

	if ( flNow - m_flLastSend < m_flTimeout )
		return TRUE;

	if ( m_nRetriesLeft-- > 0 )
	{
		AsyncSelect( FD_READ );
		Send( m_pMsg->GetData(), m_pMsg->GetCurSize(), 0 );
		m_flLastSend = (float)engineapi.Sys_FloatTime();
		m_flTimeout += m_flTimeout;		// double the backoff
		return TRUE;
	}

	m_nState = MODINFO_DONE;			// retries exhausted
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CModInfoSocket::SendRequest (0x42FA00)

void CModInfoSocket::SendRequest( const char* pszToken )
{
	char	szRequest[128];

	sprintf( szRequest, "x\r\n%s\r\n", pszToken );

	m_pMsg->SZ_Clear();
	m_pMsg->MSG_WriteString( szRequest );

	AsyncSelect( FD_READ );
	Send( m_pMsg->GetData(), m_pMsg->GetCurSize(), 0 );

	m_flLastSend   = (float)engineapi.Sys_FloatTime();
	m_nState       = MODINFO_QUERYING;
	m_nRetriesLeft = MODINFO_RETRIES;
	m_flTimeout    = MODINFO_TIMEOUT;
}

/////////////////////////////////////////////////////////////////////////////
// CModInfoSocket::StartList (0x42FA90)

void CModInfoSocket::StartList()
{
	m_nState       = MODINFO_QUERYING;
	m_nRetriesLeft = MODINFO_RETRIES;
	m_flTimeout    = MODINFO_TIMEOUT;
	SendRequest( "start-of-list" );
}

/////////////////////////////////////////////////////////////////////////////
// CModInfoSocket::ParseModList (0x42FAB0)
//
// An empty token only ends the page when the whole message has been consumed;
// otherwise the walk keeps going.

void CModInfoSocket::ParseModList()
{
	mod_t*		pMod;
	const char*	pszTok = NULL;
	char		szGamedir[256];
	char		szServers[128];
	char		szPlayers[128];
	int			nRecords = 0;
	int			nServers;
	int			nPlayers;
	int			nOldServers;
	int			nOldPlayers;

	m_pMsg->MSG_ReadByte();			// spacer / opcode position
	szGamedir[0] = 0;

	for ( ;; )
	{
		pszTok = m_pMsg->MSG_ReadString();		// gamedir
		if ( !pszTok || !*pszTok )
		{
			if ( m_pMsg->GetReadCount() >= m_pMsg->GetCurSize() )
				break;
			continue;
		}
		if ( !_strcmpi( pszTok, "end-of-list" ) || !_strcmpi( pszTok, "more-in-list" ) )
			break;

		strcpy( szGamedir, pszTok );

		strcpy( szServers, m_pMsg->MSG_ReadString() );
		strcpy( szPlayers, m_pMsg->MSG_ReadString() );
		nServers = atoi( szServers );
		nPlayers = atoi( szPlayers );
		++nRecords;

		if ( szServers[0] && szPlayers[0] && m_pModList )
		{
			pMod = ModList_FindByGamedir( &m_pModList, szGamedir );
			if ( pMod )
			{
				nOldServers = atoi( pMod->GetKeyString( "servers" ) );
				nOldPlayers = atoi( pMod->GetKeyString( "players" ) );
				sprintf( szServers, "%i", nServers + nOldServers );
				sprintf( szPlayers, "%i", nPlayers + nOldPlayers );
				pMod->SetKey( "servers", szServers );
				pMod->SetKey( "players", szPlayers );
			}
		}
	}

	if ( nRecords >= 1 && pszTok && *pszTok && !_strcmpi( pszTok, "more-in-list" ) )
		SendRequest( szGamedir );
}
