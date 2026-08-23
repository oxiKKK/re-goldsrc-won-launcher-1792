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
// Purpose: CHLMasterAsyncSocket, the master-server query socket.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Beyond this many servers the browser stops taking unregistered entries.
#define MAX_UNREGISTERED_SERVERS	200

static int	g_bOutOfDatePrompted;		// 0x4E2200  prompt-once latch

// Parse scratch for the two reply kinds; each is cleared per record.
static char	g_szMasterIp[128];			// 0x4E2180
static int	g_nMasterPort;				// 0x4E217C
static char	g_szVersionIp[128];			// 0x4E2204
static int	g_nVersionPort;				// 0x4E2178

int			g_bEnforceServerCap;		// 0x4EA8F0

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::CHLMasterAsyncSocket (0x41A1A0)

CHLMasterAsyncSocket::CHLMasterAsyncSocket( CNetGameDlg* pSheet )
{
	m_pBrowserDoc = pSheet;
	m_dwListDone  = 0;
	m_pBuffer     = new CMessageBuffer( 0x2000 );
	m_flLastSend  = engineapi.Sys_FloatTime();
	m_nServers    = 0;
	m_szFilter[0] = 0;
	Reset();
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::~CHLMasterAsyncSocket (0x41A260)

CHLMasterAsyncSocket::~CHLMasterAsyncSocket()
{
	m_pBuffer->SZ_Clear();
	if ( m_pBuffer )
	{
		delete m_pBuffer;
		m_pBuffer = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::OnReceive (0x41A2C0)
//
// CAsyncSocket slot 7.

void CHLMasterAsyncSocket::OnReceive( int nErrorCode )
{
	int		cmd;

	CAsyncSocket::OnReceive( nErrorCode );
	if ( nErrorCode )
	{
		m_nLastResult = 0;
		return;
	}

	m_nLastResult = Receive( m_pBuffer->GetData(), m_pBuffer->GetMaxSize(), 0 );
	if ( !m_nLastResult )
		return;

	Reset();
	m_flLastSend = engineapi.Sys_FloatTime();
	m_pBuffer->SetCurSize( m_nLastResult );
	m_pBuffer->MSG_BeginReading();

	if ( m_pBuffer->MSG_ReadLong() != -1 )		// connectionless header
	{
		m_nLastResult = 0;
		return;
	}

	cmd = m_pBuffer->MSG_ReadByte();
	if ( cmd == M2A_SERVER_BATCH )
		ParseServerList();
	else if ( cmd == M2A_MASTERSERVERS )
		ParseVersionReply();
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::SetListDone (0x41A380)

void CHLMasterAsyncSocket::SetListDone( DWORD dwDone )
{
	m_dwListDone = dwDone;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::ParseServerList (0x41A390)

void CHLMasterAsyncSocket::ParseServerList()
{
	BYTE	ip[4];
	long	lResume;
	int		nRecords;
	int		nVisible;
	int		rec;
	int		j;

	m_pBuffer->MSG_ReadByte();						// consume the echoed 'f'
	lResume  = m_pBuffer->MSG_ReadLong();			// resume token (raw)
	nRecords = ( m_nLastResult - 6 ) / 6;			// 6 bytes per IP:port record

	nVisible = m_pBrowserDoc ? m_pBrowserDoc->CountVisible() : 0;

	for ( rec = 0; rec < nRecords; rec++ )
	{
		memset( g_szMasterIp, 0, sizeof( g_szMasterIp ) );

		for ( j = 0; j < 4; j++ )
			ip[j] = (BYTE)m_pBuffer->MSG_ReadByte();
		sprintf( g_szMasterIp, "%i.%i.%i.%i", ip[0], ip[1], ip[2], ip[3] );

		g_nMasterPort = BigShort( (short)m_pBuffer->MSG_ReadShort() );

		++nVisible;
		if ( m_pBrowserDoc
		  && ( !g_bEnforceServerCap || nVisible <= MAX_UNREGISTERED_SERVERS ) )
			m_pBrowserDoc->AddServer( g_szMasterIp, g_nMasterPort, TRUE );

		++m_nServers;
	}

	if ( lResume )
		RequestServerBatch( lResume );		// ask for the next page
	else
		SetListDone( 1 );					// master signalled end of list
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::SetFilter (0x41A4C0)

void CHLMasterAsyncSocket::SetFilter( const char* pszFilter )
{
	m_szFilter[0] = 0;
	if ( pszFilter && *pszFilter )
		strcpy( m_szFilter, pszFilter );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::RequestServerBatch (0x41A500)

int CHLMasterAsyncSocket::RequestServerBatch( long lLastAddr )
{
	BOOL	bFiltered = ( m_szFilter[0] != 0 );

	m_pBuffer->SZ_Clear();
	m_pBuffer->MSG_WriteChar( bFiltered ? A2M_GET_SERVERS_BATCH2 : A2M_GET_SERVERS_BATCH );
	m_pBuffer->MSG_WriteLong( lLastAddr );
	if ( bFiltered )
		m_pBuffer->MSG_WriteString( m_szFilter );

	AsyncSelect( FD_READ );
	return Send( m_pBuffer->GetData(), m_pBuffer->GetCurSize(), 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::ParseVersionReply (0x41A580)
//
// Either an "outofdate" notice or three groups of {4 IP octets, 2-byte
// network-order port} separated by an all-ones address.

void CHLMasterAsyncSocket::ParseVersionReply()
{
	const char*	pszTail;
	BYTE		ip[4];
	int			nGroup;
	int			nRemain;
	int			j;

	m_pBuffer->MSG_ReadByte();		// advance past the command byte position
	pszTail = (const char*)m_pBuffer->GetData() + m_pBuffer->GetReadCount();

	if ( pszTail && *pszTail && !_strnicmp( pszTail, "outofdate", 9 ) )
	{
		if ( !g_bOutOfDatePrompted )
		{
			CPromptDlg	dlgNotice( 2, NULL );
			dlgNotice.SetMessage( Launcher_LoadString( IDS_NET_CORRUPT ) );
			if ( dlgNotice.DoModal() == IDOK )
			{
				CPromptDlg	dlgConfirm( 2, NULL );
				dlgConfirm.SetMessage( Launcher_LoadString( IDS_RUN_PATCH ) );
				if ( dlgConfirm.DoModal() == IDOK )
					Launcher_SetRestartFlag( 1 );
			}
			g_bOutOfDatePrompted = TRUE;
		}
	}
	else
	{
		nGroup  = 0;								// separators seen
		nRemain = ( m_nLastResult - 18 ) / 6 - 1;

		if ( ( m_nLastResult - 18 ) / 6 > 0 )
		{
			do
			{
				if ( nGroup > 2 )
					break;

				for ( j = 0; j < 4; j++ )
					ip[j] = (BYTE)m_pBuffer->MSG_ReadByte();

				if ( ip[0] == 0xFF && ip[1] == 0xFF && ip[2] == 0xFF && ip[3] == 0xFF )
				{
					++nGroup;						// group separator
					++nRemain;
				}
				else
				{
					memset( g_szVersionIp, 0, sizeof( g_szVersionIp ) );
					sprintf( g_szVersionIp, "%i.%i.%i.%i", ip[0], ip[1], ip[2], ip[3] );

					g_nVersionPort = BigShort( (short)m_pBuffer->MSG_ReadShort() );

					switch ( nGroup )
					{
					case 0:	gFavorites->AddServerAuth( g_szVersionIp, g_nVersionPort ); break;
					case 1:	gFavorites->AddServerTitan( g_szVersionIp, g_nVersionPort ); break;
					case 2:	gFavorites->AddServerMaster( g_szVersionIp, g_nVersionPort ); break;
					default: break;
					}
				}
			}
			while ( nRemain-- > 0 );
		}
	}

	if ( m_pBrowserDoc )
	{
		m_pBrowserDoc->SetDirty( 1 );

		if ( m_pBrowserDoc->m_pPage )
			m_pBrowserDoc->m_pPage->RebuildVisibleList();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::BeginFetch (0x41A980)

void CHLMasterAsyncSocket::BeginFetch()
{
	m_flLastSend = engineapi.Sys_FloatTime();
	m_nServers   = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::DecTries (0x41A9B0)

void CHLMasterAsyncSocket::DecTries()
{
	--m_nTries;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::HasTries (0x41A9C0)

BOOL CHLMasterAsyncSocket::HasTries()
{
	return m_nTries >= 0;
}

/////////////////////////////////////////////////////////////////////////////
// CHLMasterAsyncSocket::FlushSend (0x41A9D0)

void CHLMasterAsyncSocket::FlushSend()
{
	if ( !m_pBuffer || !m_pBuffer->GetCurSize() )
		return;

	AsyncSelect( FD_READ | FD_WRITE );
	Send( m_pBuffer->GetData(), m_pBuffer->GetCurSize(), 0 );
	m_flLastSend = engineapi.Sys_FloatTime();
}
