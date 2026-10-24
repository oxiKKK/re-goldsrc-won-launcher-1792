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
// Purpose: CHLLanAsyncSocket, the LAN server-query socket.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The query sweep walks PORT_SERVER .. PORT_SERVER + NUM_QUERY_PORTS - 1.
#define PORT_SERVER			27015
#define NUM_QUERY_PORTS		10

/////////////////////////////////////////////////////////////////////////////
// CHLLanAsyncSocket::CHLLanAsyncSocket (0x4155B0)

CHLLanAsyncSocket::CHLLanAsyncSocket( CNetGameDlg* pSheet )
{
	m_pBrowserDoc = pSheet;
	m_unk24       = 0;
	m_bQuerySent  = 0;
	m_pBuffer     = new CMessageBuffer( 0x2000 );	// 8 KB datagram buffer
	m_bOpen       = 0;
	m_bIpx        = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CHLLanAsyncSocket::~CHLLanAsyncSocket (0x415660)

CHLLanAsyncSocket::~CHLLanAsyncSocket()
{
	m_pBuffer->SZ_Clear();
	if ( m_pBuffer )
	{
		delete m_pBuffer;
		m_pBuffer = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLLanAsyncSocket::OnSend (0x4156C0)
//
// CAsyncSocket slot 8.

void CHLLanAsyncSocket::OnSend( int nErrorCode )
{
	CAsyncSocket::OnSend( nErrorCode );
}

/////////////////////////////////////////////////////////////////////////////
// CHLLanAsyncSocket::OnReceive (0x4156D0)
//
// CAsyncSocket slot 7.

void CHLLanAsyncSocket::OnReceive( int nErrorCode )
{
	SOCKADDR_IN	from;
	const char*	pszTok;
	int			fromlen;

	if ( nErrorCode )
	{
		CAsyncSocket::OnReceive( nErrorCode );
		m_nLastResult = 0;
		return;
	}

	memset( &from, 0, sizeof( from ) );
	fromlen = sizeof( from );

	m_nLastResult = ReceiveFrom( m_pBuffer->GetData(), m_pBuffer->GetMaxSize(),
		(SOCKADDR*)&from, &fromlen, 0 );
	if ( m_nLastResult <= 0 )
		return;

	m_pBuffer->SetCurSize( m_nLastResult );
	m_pBuffer->MSG_BeginReading();

	if ( m_pBuffer->MSG_ReadLong() != -1 )		// connectionless header
	{
		m_nLastResult = 0;
		return;
	}

	pszTok = m_pBuffer->MSG_ReadString();
	if ( pszTok && *pszTok && !_strcmpi( pszTok, "infostringresponse" ) )
	{
		PublishLanServer( &from );
		CAsyncSocket::OnReceive( 0 );		// re-arm
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLLanAsyncSocket::PublishLanServer (0x4157C0)

void CHLLanAsyncSocket::PublishLanServer( SOCKADDR_IN* pFrom )
{
	SOCKADDR_IPX*	pIpx;
	CServerInfo*	pInfo;
	const char*		pszInfo;
	char			szAddr[256];
	char			szHost[64];
	char			info[2048];
	int				nPort;

	if ( m_bIpx )
	{
		// IPX: net number and node address, hex-encoded, stand in for the
		// dotted quad.
		pIpx = (SOCKADDR_IPX*)pFrom;
		sprintf( szAddr, "%02x%02x%02x%02x:%02x%02x%02x%02x%02x%02x",
			pIpx->sa_netnum[0], pIpx->sa_netnum[1], pIpx->sa_netnum[2], pIpx->sa_netnum[3],
			pIpx->sa_nodenum[0], pIpx->sa_nodenum[1], pIpx->sa_nodenum[2],
			pIpx->sa_nodenum[3], pIpx->sa_nodenum[4], pIpx->sa_nodenum[5] );
		nPort = ntohs( pIpx->sa_socket );
	}
	else
	{
		strcpy( szAddr, inet_ntoa( pFrom->sin_addr ) );
		nPort = ntohs( pFrom->sin_port );
	}

	pInfo   = m_pBrowserDoc->AddServer( szAddr, nPort, FALSE );
	pszInfo = m_pBuffer->MSG_ReadString();
	if ( pInfo && pszInfo && *pszInfo )
	{
		strncpy( info, pszInfo, 2047 );
		info[2047] = 0;

		pInfo->m_bIpx            = m_bIpx;
		pInfo->m_strName         = Info_ValueForKey( info, "hostname" );
		pInfo->m_strMap          = Info_ValueForKey( info, "map" );
		pInfo->m_strDir          = Info_ValueForKey( info, "gamedir" );
		pInfo->m_strGame         = Info_ValueForKey( info, "description" );
		pInfo->m_nCurrentPlayers = atoi( Info_ValueForKey( info, "players" ) );
		pInfo->m_nMaxPlayers     = atoi( Info_ValueForKey( info, "max" ) );
		pInfo->m_nProtocol       = atoi( Info_ValueForKey( info, "protocol" ) );
		pInfo->m_cSvType         = *Info_ValueForKey( info, "type" );
		pInfo->m_cSvOs           = *Info_ValueForKey( info, "os" );
		pInfo->m_bPassword       = ( atoi( Info_ValueForKey( info, "password" ) ) != 0 );
		pInfo->m_bProxy          = ( pInfo->m_cSvType == 'p' );
		pInfo->m_bProxyTarget    = ( atoi( Info_ValueForKey( info, "proxytarget" ) ) != 0 );

		pInfo->m_strProxyAddress = Info_ValueForKey( info, "proxyaddress" );
		COM_ParseHostPort( pInfo->m_strProxyAddress, szHost, &pInfo->m_iProxyPort, PORT_SERVER );
		pInfo->m_dwProxyIp = inet_addr( szHost );

		pInfo->m_strUrl         = "";
		pInfo->m_strDownload    = "";
		pInfo->m_szHLVersion[0] = 0;
		pInfo->m_nVersion       = atoi( Info_ValueForKey( info, "modversion" ) );
		pInfo->m_bMod           = 0;
		pInfo->m_nSize          = 0;
		pInfo->m_bSvSide        = 0;
		pInfo->m_bClDll         = 0;

		pInfo->SetPingTime( engineapi.Sys_FloatTime() );	// the record is now fresh
	}

	m_pBuffer->SZ_Clear();
}

/////////////////////////////////////////////////////////////////////////////
// CHLLanAsyncSocket::BroadcastQuery (0x415B10)

int CHLLanAsyncSocket::BroadcastQuery()
{
	SOCKADDR_IPX	addr;
	int				nResult = 0;
	int				nPort;
	int				i;

	m_pBuffer->SZ_Clear();
	m_pBuffer->MSG_WriteLong( -1 );				// connectionless header
	m_pBuffer->MSG_WriteString( "infostring\n" );

	m_dSendTime  = engineapi.Sys_FloatTime();
	m_bQuerySent = 1;

	for ( i = 0; i < NUM_QUERY_PORTS; i++ )
	{
		nPort = PORT_SERVER + i;
		if ( m_bIpx )
		{
			memset( &addr, 0, sizeof( addr ) );
			addr.sa_family = AF_IPX;
			memset( addr.sa_nodenum, 0xFF, sizeof( addr.sa_nodenum ) );	// broadcast node
			addr.sa_socket = htons( (u_short)nPort );

			nResult = SendTo( m_pBuffer->GetData(), m_pBuffer->GetCurSize(),
				(SOCKADDR*)&addr, sizeof( addr ), 0 );
		}
		else
		{
			nResult = SendTo( m_pBuffer->GetData(), m_pBuffer->GetCurSize(),
				(u_short)nPort, NULL, 0 );		// NULL host -> INADDR_BROADCAST
		}
	}

	m_pBuffer->SZ_Clear();
	return AsyncSelect( FD_READ | FD_WRITE );
}

/////////////////////////////////////////////////////////////////////////////
// CHLLanAsyncSocket::Open (0x415C20)
//
// MFC's Create cannot reach AF_IPX, so that path builds and binds its own
// socket and attaches it afterwards.

BOOL CHLLanAsyncSocket::Open()
{
	SOCKADDR_IPX	name;
	u_long			argp   = 1;
	int				optval = 1;
	u_long			bReuse = 1;

	if ( m_bOpen )
		return TRUE;

	if ( m_bIpx )
	{
		m_hRawSocket = socket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );
		if ( m_hRawSocket == INVALID_SOCKET )
			return FALSE;

		memset( &name, 0, sizeof( name ) );
		name.sa_family = AF_IPX;
		name.sa_socket = htons( (u_short)PORT_SERVER );

		if ( bind( m_hRawSocket, (SOCKADDR*)&name, sizeof( SOCKADDR_IPX ) ) )
		{
			closesocket( m_hRawSocket );
			return FALSE;
		}

		if ( !Attach( m_hRawSocket,
				FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE ) )
			return FALSE;
	}
	else
	{
		if ( !Create( 0, SOCK_DGRAM,
				FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE, NULL ) )
			return FALSE;
	}

	if ( ioctlsocket( m_hSocket, FIONBIO, &argp ) == -1 )
		return FALSE;
	if ( setsockopt( m_hSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&optval, sizeof( optval ) ) == -1 )
		return FALSE;
	if ( setsockopt( m_hSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuse, sizeof( bReuse ) ) == -1 )
		return FALSE;

	AsyncSelect( FD_READ | FD_WRITE );
	m_bOpen = 1;
	return TRUE;
}
