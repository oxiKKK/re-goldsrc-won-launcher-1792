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
// Purpose: CHLAsyncSocket, the launcher's async socket base.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Default query port; a proxy address without one is assumed to be here.
#define PORT_SERVER		27015

float	g_flLastReceiveTime;	// 0x4E3914  last server-reply timestamp (shared w/ master socket)

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket::CHLAsyncSocket (0x4148C0)

CHLAsyncSocket::CHLAsyncSocket( CServerConnection* pConn )
{
	m_pConnection = pConn;
	m_pBuffer     = new CMessageBuffer( 0x2000 );	// 8 KB read buffer
}

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket::~CHLAsyncSocket (0x414960)

CHLAsyncSocket::~CHLAsyncSocket()
{
	m_pBuffer->SZ_Free();
	delete m_pBuffer;
}

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket::OnReceive (0x4149C0)
//
// CAsyncSocket slot 7.

void CHLAsyncSocket::OnReceive( int nErrorCode )
{
	CServerConnection*	pConn;

	g_flLastReceiveTime = (float)engineapi.Sys_FloatTime();

	pConn = m_pConnection;
	if ( !pConn || !pConn->m_nStatus )
	{
		CAsyncSocket::OnReceive( nErrorCode );
		return;
	}

	if ( nErrorCode == WSAENETDOWN )
	{
		Launcher_ShowMessageById( 0, IDS_SOCKET_CONNECTIONFAILURE );
		CAsyncSocket::OnReceive( WSAENETDOWN );
		return;
	}

	// Latency: once a request was sent and latency not yet recorded.
	if ( pConn->m_dSendTime != 0.0 && pConn->m_dSvPing == 0.0 )
		pConn->m_dSvPing = g_flLastReceiveTime - pConn->m_dSendTime;

	switch ( pConn->m_nStatus )
	{
	case SVQ_PING_SENT:    ParsePingReply( pConn );    break;
	case SVQ_INFO_SENT:    ParseInfoResponse( pConn ); break;
	case SVQ_PLAYERS_SENT: ParsePlayerList( pConn );   break;
	case SVQ_RULES_SENT:   ParseRules( pConn );        break;
	default: break;
	}

	CAsyncSocket::OnReceive( nErrorCode );
}

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket::OnConnect (0x414AE0)
//
// CAsyncSocket slot 11.

void CHLAsyncSocket::OnConnect( int nErrorCode )
{
	if ( nErrorCode == 0 )
	{
		m_pConnection->m_nStatus = SVQ_CONNECTED;
		m_pConnection->m_nRetry  = 0;
	}
	CAsyncSocket::OnConnect( nErrorCode );
}

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket::ParseInfoResponse (0x414B10)
//
// SVQ_INFO_SENT -> SVQ_INFO_DONE.

void CHLAsyncSocket::ParseInfoResponse( CServerConnection* pCnx )
{
	char		szInfo[2048];
	char		szHost[64];
	const char*	pszCmd;
	const char*	pszInfo;
	int			nRecv;

	nRecv = Receive( m_pBuffer->GetData(), m_pBuffer->GetMaxSize() );
	if ( !nRecv )
		return;
	m_pBuffer->SetCurSize( nRecv );
	m_pBuffer->MSG_BeginReading();

	if ( m_pBuffer->MSG_ReadLong() != -1 )
		return;

	pszCmd = m_pBuffer->MSG_ReadString();
	if ( !pszCmd || !*pszCmd || _strcmpi( pszCmd, "infostringresponse" ) )
		return;

	pszInfo = m_pBuffer->MSG_ReadString();
	if ( !pszInfo || !*pszInfo )
		return;
	strncpy( szInfo, pszInfo, 2047 );
	szInfo[2047] = 0;

	pCnx->m_strName         = Info_ValueForKey( szInfo, "hostname" );
	pCnx->m_strMap          = Info_ValueForKey( szInfo, "map" );
	pCnx->m_strDir          = Info_ValueForKey( szInfo, "gamedir" );
	pCnx->m_strGame         = Info_ValueForKey( szInfo, "description" );
	pCnx->m_nCurrentPlayers = atoi( Info_ValueForKey( szInfo, "players" ) );
	pCnx->m_nMaxPlayers     = atoi( Info_ValueForKey( szInfo, "max" ) );
	pCnx->m_nProtocol       = atoi( Info_ValueForKey( szInfo, "protocol" ) );
	pCnx->m_cSvType         = *Info_ValueForKey( szInfo, "type" );
	pCnx->m_cSvOs           = *Info_ValueForKey( szInfo, "os" );
	pCnx->m_bPassword       = ( atoi( Info_ValueForKey( szInfo, "password" ) ) != 0 );
	pCnx->m_bProxy          = ( pCnx->m_cSvType == 'p' );
	pCnx->m_bProxyTarget    = ( atoi( Info_ValueForKey( szInfo, "proxytarget" ) ) != 0 );

	pCnx->m_strProxyAddress = Info_ValueForKey( szInfo, "proxyaddress" );
	COM_ParseHostPort( pCnx->m_strProxyAddress, szHost, &pCnx->m_iProxyPort, PORT_SERVER );
	pCnx->m_dwProxyIp = inet_addr( szHost );

	pCnx->m_strUrl         = "";
	pCnx->m_strDownload    = "";
	pCnx->m_szHLVersion[0] = 0;
	pCnx->m_nVersion       = atoi( Info_ValueForKey( szInfo, "modversion" ) );
	pCnx->m_bMod           = 0;
	pCnx->m_nSize          = 0;
	pCnx->m_bSvSide        = 0;
	pCnx->m_bClDll         = 0;

	pCnx->m_nStatus = SVQ_INFO_DONE;
	pCnx->m_nRetry  = 0;
	pCnx->SetPingTime( engineapi.Sys_FloatTime() );		// the record is now fresh
}

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket::ParsePlayerList (0x414DD0)
//
// SVQ_PLAYERS_SENT -> SVQ_PLAYERS_DONE.

void CHLAsyncSocket::ParsePlayerList( CServerConnection* pCnx )
{
	CPlayerInfo*	pPlayer;
	int				nRecv;
	int				nCount;
	int				i;

	nRecv = Receive( m_pBuffer->GetData(), m_pBuffer->GetMaxSize() );
	if ( !nRecv )
		return;
	m_pBuffer->SetCurSize( nRecv );
	m_pBuffer->MSG_BeginReading();

	if ( m_pBuffer->MSG_ReadLong() != -1 || m_pBuffer->MSG_ReadByte() != 'D' )
		return;

	nCount = m_pBuffer->MSG_ReadByte();
	if ( nCount > 0 && nCount <= pCnx->m_nCurrentPlayers && pCnx->m_ppPlayers )
	{
		for ( i = 0; i < nCount && i < pCnx->m_nCurrentPlayers; i++ )
		{
			pPlayer = pCnx->m_ppPlayers[i];
			if ( !pPlayer )
			{
				pPlayer = new CPlayerInfo( "unknown" );
				if ( !pPlayer )
				{
					Launcher_ShowMessageById( 0, IDS_PLAYERINFO_NOMEM );
					return;
				}
				pCnx->m_ppPlayers[i] = pPlayer;
			}
			pPlayer->m_iId       = m_pBuffer->MSG_ReadByte();	// record index
			pPlayer->m_strName   = m_pBuffer->MSG_ReadString();
			pPlayer->m_iColors   = 0;
			pPlayer->m_iFrags    = m_pBuffer->MSG_ReadLong();
			pPlayer->m_iTime     = (int)m_pBuffer->MSG_ReadFloat();
			pPlayer->m_dConnTime = engineapi.Sys_FloatTime();
		}
	}

	pCnx->m_nStatus = SVQ_PLAYERS_DONE;
	pCnx->m_nRetry  = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket::ParseRules (0x414F50)
//
// SVQ_RULES_SENT -> SVQ_RULES_DONE.

void CHLAsyncSocket::ParseRules( CServerConnection* pCnx )
{
	char	szKey[64];
	char	szVal[256];
	int		nRecv;
	int		nRules;
	int		i;

	nRecv = Receive( m_pBuffer->GetData(), m_pBuffer->GetMaxSize() );
	if ( !nRecv )
		return;
	m_pBuffer->SetCurSize( nRecv );
	m_pBuffer->MSG_BeginReading();

	if ( m_pBuffer->MSG_ReadLong() != -1 || m_pBuffer->MSG_ReadByte() != 'E' )
		return;

	nRules = m_pBuffer->MSG_ReadShort();
	if ( nRules > 0 && nRules <= 256 )
	{
		pCnx->ClearRules();
		for ( i = 0; i < nRules; i++ )
		{
			// The reader hands back a shared static, so the key has to be
			// copied out before the value is read over it.
			strcpy( szKey, m_pBuffer->MSG_ReadString() );
			strcpy( szVal, m_pBuffer->MSG_ReadString() );
			pCnx->AddRule( szKey, szVal );
		}
	}

	pCnx->m_nStatus = SVQ_RULES_DONE;
	pCnx->m_nRetry  = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CHLAsyncSocket::ParsePingReply (0x4150A0)
//
// SVQ_PING_SENT -> SVQ_PING_DONE.

void CHLAsyncSocket::ParsePingReply( CServerConnection* pCnx )
{
	int		nRecv;

	nRecv = Receive( m_pBuffer->GetData(), m_pBuffer->GetMaxSize() );
	if ( !nRecv )
		return;
	m_pBuffer->SetCurSize( nRecv );
	m_pBuffer->MSG_BeginReading();

	if ( m_pBuffer->MSG_ReadLong() == -1 && m_pBuffer->MSG_ReadByte() == A2A_ACK )
	{
		if ( pCnx->m_dSendTime != 0.0 )
			pCnx->m_dSvPing = engineapi.Sys_FloatTime() - pCnx->m_dSendTime;
		pCnx->m_nStatus = SVQ_PING_DONE;
		pCnx->m_nRetry  = 0;
	}
}
