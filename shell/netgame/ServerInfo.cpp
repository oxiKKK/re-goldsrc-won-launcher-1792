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
// Purpose: CServerInfo and CServerRule, the queried-server record and its
//          rule list.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// CServerInfo is a launcher-internal class; all access is through named members.
static_assert( sizeof( CServerInfo ) == 0x1F8, "CServerInfo sizeof must be 504" );

// CServerRule::CServerRule (0x460770)
CServerRule::CServerRule( const char* pszKey, const char* pszValue )
{
	m_strKey   = pszKey;
	m_strValue = pszValue;
	m_pNext    = NULL;
}

/*
==================
SaveRules (0x4607f0)
==================
*/
static void SaveRules( CServerRule* pHead, FILE* fp )
{
	if ( !fp )
		return;
	fprintf( fp, "%srules\r\n", "\t\t" );
	fprintf( fp, "%s{\r\n", "\t\t" );
	for ( CServerRule* p = pHead; p; p = p->m_pNext )
		p->Save( fp );
	fprintf( fp, "%s}\r\n", "\t\t" );
}

// CServerRule::Save (0x460850)
void CServerRule::Save( FILE* fp )
{
	if ( fp )
		fprintf( fp, "%s\t\"%s\" \"%s\"\r\n", "\t\t",
			(LPCSTR)m_strKey, (LPCSTR)m_strValue );
}

// CServerRule::ParseRules (0x460880)
BOOL CServerRule::ParseRules( CServerRule** ppHead, char** ppBuf )
{
	CString	strKey;
	CString	strValue;
	CToken	tok( *ppBuf );

	tok.SetQuoteMode( TRUE );
	tok.SetCommentMode( TRUE );

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
		return FALSE;

	if ( strcmp( tok.token, "{" ) )
	{
		Launcher_ErrorMessageBox( 0, "Expecting '{', got '%s'", tok.token );
		return FALSE;
	}

	for ( ;; )
	{
		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			break;
		if ( !_strcmpi( tok.token, "}" ) )
		{
			*ppBuf = tok.GetData();
			return TRUE;
		}

		strKey = tok.token;
		tok.ParseNextToken();
		strValue = tok.token;

		CServerRule*	pRule = new CServerRule( strKey, strValue );
		pRule->m_pNext = *ppHead;
		*ppHead = pRule;
	}
	return FALSE;
}

/*
==================
StripQuotes (0x460aa0)

A record's own quoting is added when it is written, so any quote that came in
with a value has to come back out first.
==================
*/
static void StripQuotes( CString& str )
{
	str.Remove( '"' );
}

/*
==================
TrimStrings (0x460ab0)
==================
*/
static void TrimStrings( CServerInfo* p )
{
	StripQuotes( p->m_strName );
	StripQuotes( p->m_strAddress );
	StripQuotes( p->m_strMap );
	StripQuotes( p->m_strGame );
	StripQuotes( p->m_strDir );
	StripQuotes( p->m_strUrl );
	StripQuotes( p->m_strDownload );
}

// CServerInfo::SaveToFile (0x460B00)
void CServerInfo::SaveToFile( FILE* fp )
{
	if ( !fp )
		return;

	fprintf( fp, "%sserver\r\n", "\t" );
	fprintf( fp, "%s{\r\n", "\t" );

	TrimStrings( this );

	fprintf( fp, "%s\t\"%s\" \"%s\"\r\n", "\t", "address", (LPCSTR)m_strAddress );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "port",    m_nPort );
	fprintf( fp, "%s\t\"%s\" \"%s\"\r\n", "\t", "name",    (LPCSTR)m_strName );
	fprintf( fp, "%s\t\"%s\" \"%s\"\r\n", "\t", "map",     (LPCSTR)m_strMap );
	fprintf( fp, "%s\t\"%s\" \"%s\"\r\n", "\t", "game",    (LPCSTR)m_strGame );
	fprintf( fp, "%s\t\"%s\" \"%s\"\r\n", "\t", "dir",     (LPCSTR)m_strDir );
	fprintf( fp, "%s\t\"%s\" \"%s\"\r\n", "\t", "url",     (LPCSTR)m_strUrl );
	fprintf( fp, "%s\t\"%s\" \"%s\"\r\n", "\t", "dl",      (LPCSTR)m_strDownload );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "maxplayers",     m_nMaxPlayers );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "currentplayers", m_nCurrentPlayers );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "protocol", m_nProtocol );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "favorite", m_bFavorite != 0 );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "ipx",      m_bIpx != 0 );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "mod",      m_bMod != 0 );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "version",  m_nVersion );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "size",     m_nSize );
	fprintf( fp, "%s\t\"%s\" \"%c\"\r\n", "\t", "svtype",   m_cSvType );
	fprintf( fp, "%s\t\"%s\" \"%c\"\r\n", "\t", "svos",     m_cSvOs );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "password", m_bPassword != 0 );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "svside",   m_bSvSide != 0 );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "cldll",    m_bClDll != 0 );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "lan",      m_bLan != 0 );
	fprintf( fp, "%s\t\"%s\" \"%f\"\r\n", "\t", "svping",   m_dSvPing );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "noresponse", m_bNoResponse != 0 );
	fprintf( fp, "%s\t\"%s\" \"%f\"\r\n", "\t", "packetloss", m_flPacketLoss );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "status",   m_nStatus );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "filtered", m_bFiltered != 0 );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "fullmax",  m_nFullMax );
	fprintf( fp, "%s\t\"%s\" \"%s\"\r\n", "\t", "hlversion", m_szHLVersion );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "proxy",       m_bProxy != 0 );
	fprintf( fp, "%s\t\"%s\" \"%i\"\r\n", "\t", "proxytarget", m_bProxyTarget != 0 );
	fprintf( fp, "%s\t\"%s\" \"%s\"\r\n", "\t", "proxyaddress", (LPCSTR)m_strProxyAddress );

	for ( int i = 0; i < MAX_PING_SLOTS; i++ )
		if ( m_rgPing[i] != 0.0 )
			fprintf( fp, "%s\t\"%s%02i\" \"%f\"\r\n", "\t", "ping", i, m_rgPing[i] );

	if ( m_rules.m_pNext )
		SaveRules( m_rules.m_pNext, fp );

	if ( m_ppPlayers )
	{
		fprintf( fp, "%splayers %02i\r\n", "\t", m_nFullMax );
		fprintf( fp, "%s{\r\n", "\t" );
		for ( int i = 0; i < m_nFullMax; i++ )
			if ( m_ppPlayers[i] )
				m_ppPlayers[i]->Save( i, fp );
		fprintf( fp, "%s}\r\n", "\t" );
	}

	fprintf( fp, "%s}\r\n", "\t" );
}

// CServerInfo::SetKeyValue (0x461040)
void CServerInfo::SetKeyValue( const char* pszKey, const char* pszValue )
{
	if ( !_strcmpi( pszKey, "address" ) )			m_strAddress = pszValue;
	else if ( !_strcmpi( pszKey, "port" ) )			m_nPort = atoi( pszValue );
	else if ( !_strcmpi( pszKey, "name" ) )			m_strName = pszValue;
	else if ( !_strcmpi( pszKey, "map" ) )			m_strMap = pszValue;
	else if ( !_strcmpi( pszKey, "game" ) )			m_strGame = pszValue;
	else if ( !_strcmpi( pszKey, "dir" ) )			m_strDir = pszValue;
	else if ( !_strcmpi( pszKey, "maxplayers" ) )	m_nMaxPlayers = atoi( pszValue );
	else if ( !_strcmpi( pszKey, "currentplayers" ) )	m_nCurrentPlayers = atoi( pszValue );
	else if ( !_strcmpi( pszKey, "protocol" ) )		m_nProtocol = atoi( pszValue );
	else if ( !_strcmpi( pszKey, "lan" ) )			m_bLan = ( atoi( pszValue ) != 0 );
	else if ( !_strcmpi( pszKey, "svping" ) )		m_dSvPing = atof( pszValue );
	else if ( !_strcmpi( pszKey, "noresponse" ) )	m_bNoResponse = ( atoi( pszValue ) != 0 );
	else if ( !_strcmpi( pszKey, "status" ) )		m_nStatus = atoi( pszValue );
	else if ( !_strcmpi( pszKey, "packetloss" ) )	m_flPacketLoss = (float)atof( pszValue );
	else if ( !_strcmpi( pszKey, "url" ) )			m_strUrl = pszValue;
	else if ( !_strcmpi( pszKey, "dl" ) )			m_strDownload = pszValue;
	else if ( !_strcmpi( pszKey, "mod" ) )			m_bMod = ( atoi( pszValue ) != 0 );
	else if ( !_strcmpi( pszKey, "version" ) )		m_nVersion = atoi( pszValue );
	else if ( !_strcmpi( pszKey, "size" ) )			m_nSize = atoi( pszValue );
	else if ( !_strcmpi( pszKey, "svtype" ) )		m_cSvType = *pszValue;
	else if ( !_strcmpi( pszKey, "svos" ) )			m_cSvOs = *pszValue;
	else if ( !_strcmpi( pszKey, "password" ) )		m_bPassword = ( atoi( pszValue ) != 0 );
	else if ( !_strcmpi( pszKey, "svside" ) )		m_bSvSide = ( atoi( pszValue ) != 0 );
	else if ( !_strcmpi( pszKey, "cldll" ) )		m_bClDll = ( atoi( pszValue ) != 0 );
	else if ( !_strcmpi( pszKey, "proxytarget" ) )	m_bProxyTarget = ( atoi( pszValue ) != 0 );
	else if ( !_strcmpi( pszKey, "proxy" ) )		m_bProxy = ( atoi( pszValue ) != 0 );
	else if ( !_strcmpi( pszKey, "favorite" ) )		m_bFavorite = ( atoi( pszValue ) != 0 );
	else if ( !_strcmpi( pszKey, "ipx" ) )			m_bIpx = ( atoi( pszValue ) != 0 );
	else if ( !_strcmpi( pszKey, "filtered" ) )		m_bFiltered = ( atoi( pszValue ) != 0 );
	else if ( !_strcmpi( pszKey, "fullmax" ) )		m_nFullMax = atoi( pszValue );
	else if ( !_strcmpi( pszKey, "hlversion" ) )	strcpy( m_szHLVersion, pszValue );
	else if ( !_strcmpi( pszKey, "proxyaddress" ) )
	{
		char	szHost[64];
		m_strProxyAddress = pszValue;
		COM_ParseHostPort( (LPCSTR)m_strProxyAddress, szHost, &m_iProxyPort, 27015 );
		m_dwProxyIp = inet_addr( szHost );
	}
	else if ( !_strnicmp( pszKey, "ping", 4 ) && strlen( pszKey ) == 6 )
	{
		int	iSlot = atoi( pszKey + 4 );
		if ( iSlot >= 0 && iSlot < MAX_PING_SLOTS )
			m_rgPing[iSlot] = atof( pszValue );
	}
	// unknown keys ignored
}

// CServerInfo::LoadFromBuffer (0x4616A0)
BOOL CServerInfo::LoadFromBuffer( char** ppBuf )
{
	CString	strKey;
	CString	strValue;
	CToken	tok( *ppBuf );

	tok.SetQuoteMode( TRUE );
	tok.SetCommentMode( TRUE );

	tok.ParseNextToken();
	if ( !strlen( tok.token ) )
		return FALSE;

	if ( strcmp( tok.token, "{" ) )
	{
		Launcher_ErrorMessageBox( 0, "Expecting '{', got '%s'", tok.token );
		return FALSE;
	}

	for ( ;; )
	{
		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			break;
		if ( !_strcmpi( tok.token, "}" ) )
		{
			*ppBuf = tok.GetData();
			return TRUE;
		}

		strKey = tok.token;

		if ( !_strcmpi( strKey, "rules" ) )
		{
			// A nested "rules { "k" "v" ... }" gets its own parse pass; the
			// list it builds replaces whatever was there, it does not chain on.
			CServerRule*	pRules = NULL;

			*ppBuf = tok.GetData();
			CServerRule::ParseRules( &pRules, ppBuf );
			tok.SetData( *ppBuf );
			m_rules.m_pNext = pRules;
		}
		else if ( !_strcmpi( strKey, "players" ) )
		{
			// "players NN { player <idx> { ... } ... }".  Only the closing brace
			// returns to the key loop; every other way out of the block ends the
			// record.
			tok.ParseNextToken();
			if ( strlen( tok.token ) )
			{
				m_nFullMax  = atoi( tok.token );
				m_ppPlayers = new CPlayerInfo*[ m_nFullMax ];
				memset( m_ppPlayers, 0, sizeof( CPlayerInfo* ) * m_nFullMax );

				tok.ParseNextToken();
				if ( !_strcmpi( tok.token, "{" ) )
				{
					for ( ;; )
					{
						tok.ParseNextToken();
						if ( !strlen( tok.token ) )
							break;
						if ( !_strcmpi( tok.token, "}" ) )
							goto nextKey;
						if ( _strcmpi( tok.token, "player" ) )
							break;

						tok.ParseNextToken();
						if ( !strlen( tok.token ) )
							break;

						int	iIdx = atoi( tok.token );
						if ( iIdx < 0 || iIdx >= m_nFullMax )
							break;

						CPlayerInfo*	pPlayer = new CPlayerInfo( "unknown" );

						*ppBuf = tok.GetData();
						pPlayer->Parse( ppBuf );
						tok.SetData( *ppBuf );
						m_ppPlayers[iIdx] = pPlayer;
					}
				}
			}
			break;
		}
		else
		{
			tok.ParseNextToken();
			strValue = tok.token;
			SetKeyValue( strKey, strValue );
		}
nextKey:
		;
	}
	return FALSE;
}

// CServerInfo::CServerInfo (0x461AA0)
CServerInfo::CServerInfo( const char* pszAddress, int nUserData )
	: m_rules( "", "" )	// inlined over the sub-object at +8
{
	static int	s_nNextId;		// dword_4D1BFC

	m_rules.m_pNext = NULL;		// written again by the body
	m_strAddress = pszAddress;
	m_nPort      = nUserData;	// ctor stores the userdata arg into the port field
	m_strName    = "?";
	m_strMap     = "?";
	m_strGame    = "Half-Life";
	m_strDir     = "VALVE";
	m_nMaxPlayers     = 0;
	m_nCurrentPlayers = 0;
	m_nProxyMaxPlayers   = 0;
	m_nProxyCurPlayers   = 0;
	m_nProtocol  = 0;
	m_bLan       = 0;
	m_dSvPing    = 0.0;
	m_bNoResponse= 0;
	m_nRetry     = 0;
	m_dSendTime  = 0.0;
	m_nStatus    = SVQ_QUEUED;
	m_flPacketLoss = 100.0f;
	memset( m_rgPing, 0, sizeof( m_rgPing ) );
	m_nNumPings  = g_nNumPings;
	m_strUrl     = "";
	m_bMod       = 0;
	m_nVersion   = 0;
	m_nSize      = 0;
	m_cSvType    = 'l';
	m_cSvOs      = 'w';
	m_bPassword  = 0;
	m_bSvSide    = 1;
	m_bClDll     = 0;
	m_bProxyTarget = 0;
	m_bProxy     = 0;
	m_strProxyAddress = "";
	m_dwProxyIp  = 0;
	m_iProxyPort = 0;
	strcpy( m_szHLVersion, g_szPatchVersion );
	m_ppPlayers  = NULL;
	m_pSocket    = NULL;
	m_pNext      = NULL;
	m_pJoinNext  = NULL;
	m_bFavorite  = 0;
	m_bIpx       = 0;
	m_pOwnedQuery = NULL;
	m_bFiltered  = 0;
	m_nFullMax   = 64;
	m_nServerId  = s_nNextId++;
	m_dPingTime  = -1.0;
}

// CServerRule::~CServerRule (0x461D20)
CServerRule::~CServerRule()
{
	// m_strKey / m_strValue release through their own dtors.
}

// CServerInfo::~CServerInfo (0x461D70)
CServerInfo::~CServerInfo()
{
	ClearPlayers();
	ClearRules();

	if ( m_pSocket )
		delete m_pSocket;

	// The CStrings and the embedded rule release through their own dtors.
}

// CServerInfo::ClearPlayers (0x461E60)
void CServerInfo::ClearPlayers()
{
	if ( !m_ppPlayers )
		return;

	for ( int i = 0; i < m_nFullMax; i++ )
	{
		if ( m_ppPlayers[i] )
			delete m_ppPlayers[i];
		m_ppPlayers[i] = NULL;
	}

	delete[] m_ppPlayers;
	m_ppPlayers = NULL;
}

// CServerInfo::AllocPlayers (0x461ED0)
void CServerInfo::AllocPlayers( int nMaxPlayers )
{
	ClearPlayers();

	m_ppPlayers = new CPlayerInfo*[ m_nFullMax ];
	if ( !m_ppPlayers )
		exit( 0 );

	memset( m_ppPlayers, 0, sizeof( CPlayerInfo* ) * m_nFullMax );
	m_nMaxPlayers = nMaxPlayers;
}

// CServerInfo::SendInfoRequest (0x461F30)
BOOL CServerInfo::SendInfoRequest()
{
	CHLAsyncSocket*	pSock = m_pSocket;
	if ( !pSock )
		return FALSE;

	CMessageBuffer*	buf = pSock->m_pBuffer;
	buf->SZ_Clear();
	buf->MSG_WriteLong( -1 );
	buf->MSG_WriteString( "infostring\n" );

	pSock->AsyncSelect( FD_READ );
	m_nStatus = SVQ_INFO_SENT;
	int nSent = pSock->Send( buf->GetData(), buf->GetCurSize(), 0 );
	LOG( "SendInfoRequest: %s:%d len=%d sent=%d err=%d",
		 (LPCSTR)m_strAddress, m_nPort, buf->GetCurSize(), nSent,
		 nSent < 0 ? GetLastError() : 0 );
	buf->SZ_Clear();

	m_dSendTime = engineapi.Sys_FloatTime();
	return TRUE;
}

// CServerInfo::SendPlayersRequest (0x461FD0)
void CServerInfo::SendPlayersRequest()
{
	CHLAsyncSocket*	pSock = m_pSocket;
	if ( !pSock )
		return;

	CMessageBuffer*	buf = pSock->m_pBuffer;
	buf->SZ_Clear();
	buf->MSG_WriteLong( -1 );
	buf->MSG_WriteString( "players" );

	pSock->AsyncSelect( FD_READ );
	m_nStatus = SVQ_PLAYERS_SENT;
	pSock->Send( buf->GetData(), buf->GetCurSize(), 0 );
	buf->SZ_Clear();

	m_dSendTime = engineapi.Sys_FloatTime();
}

// CServerInfo::SendRulesRequest (0x462070)
void CServerInfo::SendRulesRequest()
{
	CHLAsyncSocket*	pSock = m_pSocket;
	if ( !pSock )
		return;

	CMessageBuffer*	buf = pSock->m_pBuffer;
	buf->SZ_Clear();
	buf->MSG_WriteLong( -1 );
	buf->MSG_WriteString( "rules" );

	pSock->AsyncSelect( FD_READ );
	m_nStatus = SVQ_RULES_SENT;
	pSock->Send( buf->GetData(), buf->GetCurSize(), 0 );
	buf->SZ_Clear();

	m_dSendTime = engineapi.Sys_FloatTime();
}

// CServerInfo::SendPingRequest (0x462110)
void CServerInfo::SendPingRequest()
{
	CHLAsyncSocket*	pSock = m_pSocket;
	if ( !pSock )
		return;

	CMessageBuffer*	buf = pSock->m_pBuffer;
	buf->SZ_Clear();
	buf->MSG_WriteLong( -1 );
	buf->MSG_WriteString( "ping" );

	pSock->AsyncSelect( FD_READ );
	m_nStatus = SVQ_PING_SENT;
	pSock->Send( buf->GetData(), buf->GetCurSize(), 0 );
	buf->SZ_Clear();

	m_dSendTime = engineapi.Sys_FloatTime();
}

// CServerInfo::CloseSocket (0x4621B0)
void CServerInfo::CloseSocket()
{
	if ( m_pSocket )
		delete m_pSocket;
	m_pSocket = NULL;
}

// CServerInfo::ResetRetry (0x4621D0)
void CServerInfo::ResetRetry()
{
	m_nRetry = 0;
}

// CServerInfo::OpenConnection (0x4621E0)
BOOL CServerInfo::OpenConnection()
{
	CHLAsyncSocket*	pSock = new CHLAsyncSocket( this );

	m_pSocket = pSock;
	if ( !pSock )
	{
		m_pSocket = NULL;
		m_nStatus = SVQ_IDLE;
		return FALSE;
	}

	// 0x3F = FD_READ|FD_WRITE|FD_OOB|FD_ACCEPT|FD_CONNECT|FD_CLOSE
	if ( !pSock->Create( 0, SOCK_DGRAM, 0x3F, NULL ) )
	{
		CloseSocket();
		m_nStatus = SVQ_IDLE;
		return FALSE;
	}

	memset( m_rgPing, 0, sizeof( double ) * g_nNumPings );
	m_nNumPings = g_nNumPings;
	m_nStatus   = SVQ_SOCKET_OPEN;
	return TRUE;
}

// CServerInfo::Connect (0x4622D0)
BOOL CServerInfo::Connect()
{
	CHLAsyncSocket*	pSock = m_pSocket;
	if ( !pSock )
		return FALSE;

	if ( !pSock->AsyncSelect( FD_CONNECT ) )
	{
		CloseSocket();
		m_nStatus = SVQ_IDLE;
		return FALSE;
	}

	if ( !pSock->Connect( m_strAddress, m_nPort ) )
	{
		m_nStatus = SVQ_CONNECT_RETRY;
		return FALSE;
	}

	m_nStatus = SVQ_CONNECTED;
	return TRUE;
}

// CServerInfo::BeginPlayerQuery (0x462330)
BOOL CServerInfo::BeginPlayerQuery()
{
	if ( !m_pSocket )
		return FALSE;
	if ( m_nMaxPlayers <= 0 )
		return FALSE;

	AllocPlayers( m_nMaxPlayers );
	if ( !m_nCurrentPlayers )
		return FALSE;

	m_nStatus = SVQ_PLAYERS_SENT;
	return TRUE;
}

// CServerInfo::SetFiltered  (0x462370)
void CServerInfo::SetFiltered( int bFiltered )
{
	m_bFiltered = bFiltered != 0;
}

// CServerInfo::GetFiltered  (0x462390)
int CServerInfo::GetFiltered()
{
	return m_bFiltered;
}

// CServerInfo::AddRule (0x4623A0)
CServerRule* CServerInfo::AddRule( const char* pszKey, const char* pszValue )
{
	CServerRule*	pRule = new CServerRule( pszKey, pszValue );
	pRule->m_pNext = m_rules.m_pNext;
	m_rules.m_pNext       = pRule;
	return pRule;
}

// CServerInfo::ClearRules (0x462410)
void CServerInfo::ClearRules()
{
	for ( CServerRule* p = m_rules.m_pNext; p; )
	{
		CServerRule* pNext = p->m_pNext;
		delete p;
		p = pNext;
	}
	m_rules.m_pNext = NULL;
}

// CServerInfo::ComputePingStats (0x462450)
void CServerInfo::ComputePingStats()
{
	double	flMin   = 9999.0;
	int		nReplies = 0;
	double	flSum   = 0.0;
	int		nValid  = 0;

	if ( g_nNumPings > 0 )
	{
		for ( int i = 0; i < g_nNumPings; i++ )
		{
			if ( m_rgPing[i] != 0.0 )
			{
				flSum += m_rgPing[i];
				if ( flMin > m_rgPing[i] )
					flMin = m_rgPing[i];
				++nReplies;
			}
		}

		nValid = nReplies;
		if ( nReplies )
			m_dSvPing = flMin;
		else
			m_dSvPing = 0.0;		// +72 / +76 cleared as a pair
	}
	else
	{
		m_dSvPing = 0.0;			// +72 / +76 cleared as a pair
	}

	m_flPacketLoss = (float)( ( (double)g_nNumPings - (double)nValid ) * 100.0 / (double)g_nNumPings );
}

// CServerInfo::GetServerId  (0x4624F0)
int CServerInfo::GetServerId()
{
	return m_nServerId;
}

// CServerInfo::SetPingTime  (0x462500) -- stamped when a reply lands
void CServerInfo::SetPingTime( double dTime )
{
	m_dPingTime = dTime;
}

// CServerInfo::GetPingTime  (0x462520)
double CServerInfo::GetPingTime()
{
	return m_dPingTime;
}
