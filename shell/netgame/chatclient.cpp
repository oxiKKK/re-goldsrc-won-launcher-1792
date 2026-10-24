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
// Purpose: the chat room list and CChatClient, the WON chat connection record.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/*
==================
chatroom_t::chatroom_t (0x4041E0)
==================
*/
chatroom_t::chatroom_t()
{
	memset( m_szName, 0, sizeof( m_szName ) );
	memset( m_szTopic, 0, sizeof( m_szTopic ) );
	memset( m_szAddress, 0, sizeof( m_szAddress ) );
	m_nPlayers   = 0;
	m_dwFlagsA   = 0;
	m_nIndex     = 0;
	m_pNext      = NULL;
	m_pPrev      = NULL;
	m_bAnswered  = 0;
	m_bHidden    = 0;
	m_nProbesLeft = 3;
	m_nGroup     = -1;
}

/*
==================
CRoomList::CRoomList (0x404240)
==================
*/
CRoomList::CRoomList()
{
	m_pPrev = this;
	m_pNext = this;
}

/*
==================
CRoomList::AddRoom (0x404270)
==================
*/
chatroom_t* CRoomList::AddRoom( const char* pszName, const char* pszTopic, int nPlayers,
								int dwFlagsA, const char* pszAddr, int nPlayerCount,
								const char* pszId )
{
	// search the existing ring (the sentinel is this list).
	for ( chatroom_t* p = m_pNext; p != this; p = p->m_pNext )
	{
		if ( !_strcmpi( pszName, p->m_szName ) )
		{
			if ( pszId && _strcmpi( pszId, p->m_szId ) )
			{
				strncpy( p->m_szId, pszId, sizeof( p->m_szId ) );
				p->m_szId[sizeof( p->m_szId ) - 1] = 0;
			}
			return p;	// existing match
		}
	}

	chatroom_t*	pNew = new chatroom_t;
	if ( !pNew )
		return NULL;

	strncpy( pNew->m_szName, pszName, sizeof( pNew->m_szName ) );
	pNew->m_szName[sizeof( pNew->m_szName ) - 1] = 0;
	strncpy( pNew->m_szTopic, pszTopic, sizeof( pNew->m_szTopic ) );
	pNew->m_szTopic[sizeof( pNew->m_szTopic ) - 1] = 0;
	if ( pszId )
	{
		strncpy( pNew->m_szId, pszId, sizeof( pNew->m_szId ) );
		pNew->m_szId[sizeof( pNew->m_szId ) - 1] = 0;
	}
	else
	{
		memset( pNew->m_szId, 0, sizeof( pNew->m_szId ) );
	}
	pNew->m_nPlayers     = nPlayers;
	pNew->m_dwFlagsA     = dwFlagsA;
	pNew->m_nPlayerCount = nPlayerCount;
	if ( pszAddr )
	{
		strcpy( pNew->m_szAddress, pszAddr );
		pNew->m_szAddress[sizeof( pNew->m_szAddress ) - 1] = 0;
	}
	else
	{
		strcpy( pNew->m_szAddress, "" );
	}

	Link( pNew );	// link at the head of the ring
	return pNew;
}

/*
==================
CRoomList::Clear (0x404400)
==================
*/
void CRoomList::Clear()
{
	chatroom_t*	p = m_pNext;
	while ( p != this )
	{
		chatroom_t*	pNext = p->m_pNext;
		Unlink( p );
		delete p;
		p = pNext;
	}
}

/*
==================
CRoomList::Link (0x404430)
==================
*/
void CRoomList::Link( chatroom_t* pRoom )
{
	if ( pRoom->m_pNext || pRoom->m_pPrev )
	{
		Launcher_ShowMessageById( 0, IDS_CHAT_IGNORE_MISLINKED );
		return;
	}
	pRoom->m_pNext   = m_pNext;
	m_pNext->m_pPrev = pRoom;
	m_pNext          = pRoom;
	pRoom->m_pPrev   = this;
}

/*
==================
CRoomList::Unlink (0x404480)
==================
*/
void CRoomList::Unlink( chatroom_t* pRoom )
{
	if ( pRoom->m_pNext && pRoom->m_pPrev )
	{
		pRoom->m_pNext->m_pPrev = pRoom->m_pPrev;
		pRoom->m_pPrev->m_pNext = pRoom->m_pNext;
		pRoom->m_pNext = NULL;
		pRoom->m_pPrev = NULL;
	}
	else
	{
		Launcher_ShowMessageById( 0, IDS_CHAT_IGNORE_UNLINKED );
	}
}

/*
==================
CRoomList::FindByName (0x4044E0)
==================
*/
chatroom_t* CRoomList::FindByName( const char* pszName )
{
	for ( chatroom_t* p = m_pNext; p != this; p = p->m_pNext )
	{
		if ( !_strcmpi( pszName, p->m_szName ) )
			return p;
	}
	return NULL;
}

/*
==================
CRoomList::FindByAddress (0x404530)
==================
*/
chatroom_t* CRoomList::FindByAddress( const char* pszAddr, int nPort )
{
	char	szWant[256];

	if ( !pszAddr || !*pszAddr )
		return NULL;

	sprintf( szWant, "%s:%i", pszAddr, nPort );

	for ( chatroom_t* p = m_pNext; p != this; p = p->m_pNext )
	{
		if ( !_strcmpi( szWant, p->m_szAddress ) )
			return p;
	}
	return NULL;
}

/*
==================
CChatClient::NextChatServer (0x4045B0)
==================
*/
int CChatClient::NextChatServer( void )
{
	if ( !m_pChatEntry )
		return 0;

	m_pChatEntry = m_pChatEntry->next;
	if ( !m_pChatEntry )
		return 0;

	strcpy( m_szHostChat, m_pChatEntry->host_name );
	m_nPortChat = m_pChatEntry->port;
	return 1;
}

/*
==================
CChatClient::CChatClient (0x404610)
==================
*/
CChatClient::CChatClient( CNetGameDlg* pOwner, CFavorites* pFavorites )
{
	m_pFavorites = pFavorites;
	m_pOwner     = pOwner;

	// One sweep over the whole live-session span (+56 .. +311), then the token
	// buffer, then each server's host buffer paired with its default port.
	memset( m_pad56, 0, 256 );
	memset( m_szAuthToken, 0, sizeof( m_szAuthToken ) );

	memset( m_szHostIrc, 0, sizeof( m_szHostIrc ) );
	m_nPortIrc = PORT_IRC;
	memset( m_szHostChat, 0, sizeof( m_szHostChat ) );
	m_nPortChat = PORT_CHAT;
	memset( m_szHostDir, 0, sizeof( m_szHostDir ) );
	m_nPortDir = PORT_DIR;

	sprintf( m_szPlayerName, "%s", g_pServerBrowser->GetPlayerName() );

	m_pIrcEntry  = pFavorites->GetIrcServers();		// &entries[0]
	m_pChatEntry = pFavorites->GetTitanServers();	// &entries[1]
	m_pDirEntry  = pFavorites->GetAuthServers();	// &entries[2]

	m_lSelfStatus = 0;
}

/*
==================
CChatClient::~CChatClient (0x404700)
==================
*/
CChatClient::~CChatClient()
{
}

/*
==================
CChatClient::GetServerAddr (0x404710)
==================
*/
CServerAddr* CChatClient::GetServerAddr()
{
	return m_pIrcEntry;
}

/*
==================
CChatClient::GetServerPort (0x404720)
==================
*/
unsigned short CChatClient::GetServerPort()
{
	return m_pIrcEntry->port;
}

/*
==================
CChatClient::GetChatHost (0x404730)
==================
*/
const char* CChatClient::GetChatHost()
{
	return m_szHostChat;
}

/*
==================
CChatClient::GetChatPort (0x404740)
==================
*/
int CChatClient::GetChatPort()
{
	return m_nPortChat;
}

/*
==================
CChatClient::GetPlayerName (0x404750)
==================
*/
const char* CChatClient::GetPlayerName()
{
	return m_szPlayerName;
}

/*
==================
CChatClient::GetSelfStatusRaw (0x404760)
==================
*/
long CChatClient::GetSelfStatusRaw()
{
	return m_lSelfStatus;
}

/*
==================
CChatClient::SetSelfStatus (0x404770)
==================
*/
void CChatClient::SetSelfStatus( long lStatus )
{
	m_lSelfStatus = lStatus;
}

/*
==================
CChatClient::ResetChatServer (0x404780)
==================
*/
CServerAddr* CChatClient::ResetChatServer( void )
{
	m_pChatEntry = m_pFavorites->GetTitanServers();
	return m_pChatEntry;
}

/*
==================
CChatClient::SetChatServerFlag (0x404790)
==================
*/
void CChatClient::SetChatServerFlag( int flag )
{
	if ( !m_pChatEntry )
		return;

	if ( m_pChatEntry == m_pFavorites->GetTitanServers() )
		return;		// the head entry is left alone

	m_pChatEntry->sort_flag = flag;
}

/*
==================
CChatUser::CChatUser (0x4047C0)
==================
*/
CChatUser::CChatUser( const char* pszNick, long lStatus )
{
	m_szNick[0] = 0;
	strcpy( m_szNick, pszNick );
	m_lStatus = lStatus;
	m_pNext   = NULL;
}

/*
==================
CChatUser::~CChatUser (0x404830)
==================
*/
CChatUser::~CChatUser()
{
}
