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
// Purpose: declares the chat room list and CChatClient, the WON chat
//          connection/identity record.
//
// $NoKeywords: $
//=============================================================================

#ifndef CHATCLIENT_H
#define CHATCLIENT_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>

class CFavorites;
class CNetGameDlg;
class CServerAddr;
class CChatUser;

// One chat-room directory record; the room list control paints it directly.
struct chatroom_t
{
	chatroom_t();

	int		m_dwFlagsA;			// +0
	int		m_nPlayers;			// +4    live occupancy (the list's second column)
	int		m_nIndex;			// +8    directory index (set by the fetch)
	int		m_nProbesLeft;		// +12   UDP status probes still to send (3 out of the ctor)
	DWORD	m_dwLastProbe;		// +16   tick of the last probe (the ctor leaves it alone)
	int		m_bAnswered;		// +20   the room replied, stop probing it
	int		m_bHidden;			// +24   filtered out of the list this pass
	int		m_nGroup;			// +28   group/category, -1 = ungrouped (paint accent)
	char	m_szTopic[128];		// +32   "No topic" default; the list's second line
	char	m_szAddress[128];	// +160  "ip:port"
	char	m_szName[64];		// +288  room name (find key)
	char	m_szId[32];			// +352  room id / password
	int		m_nPlayerCount;		// +384
	chatroom_t*	m_pNext;		// +388  circular next  (sentinel-rooted)
	chatroom_t*	m_pPrev;		// +392  circular prev  (-> sizeof 0x18C)
};

static_assert( sizeof( chatroom_t ) == 0x18C, "chatroom_t sizeof must be 396" );

// The head of a room ring: a room record whose links close onto itself.
class CRoomList : public chatroom_t
{
public:
	CRoomList();

	// find-or-create over the ring; refreshes m_szId on a match, allocates and
	// links on a miss.
	chatroom_t*	AddRoom( const char* pszName, const char* pszTopic, int nPlayers,
						 int dwFlagsA, const char* pszAddr, int nPlayerCount,
						 const char* pszId );

	void		Clear();
	void		Link( chatroom_t* pRoom );
	void		Unlink( chatroom_t* pRoom );
	chatroom_t*	FindByName( const char* pszName );
	chatroom_t*	FindByAddress( const char* pszAddr, int nPort );
};

// The 0x548-byte chat connection/identity record; vftable 0x4ACF2C, whose one
// slot is the deleting destructor 0x4046E0.
class CChatClient
{
public:
	CChatClient( CNetGameDlg* pOwner, CFavorites* pFavorites );
	virtual ~CChatClient();

	// The chat/IRC server this record talks to (+4), and the local status word
	// the join reply stamps in (+828).
	CServerAddr*	GetServerAddr();
	unsigned short	GetServerPort();

	const char*	GetChatHost();
	int			GetChatPort();
	const char*	GetPlayerName();
	long		GetSelfStatusRaw();
	void		SetSelfStatus( long lStatus );

	// The chat cursor walks the Titan block of woncomm.lst; a server that does
	// not answer is flagged so the next pass skips it.
	CServerAddr*	ResetChatServer( void );
	int			NextChatServer( void );
	void		SetChatServerFlag( int flag );

	const char*	GetDirHost() const
	{
		return m_szHostDir;
	}
	int			GetDirPort() const
	{
		return m_nPortDir;
	}
	const char*	GetAuthToken() const
	{
		return m_szAuthToken;
	}
	void		SetAuthToken( const char* psz )
	{
		strcpy( m_szAuthToken, psz );
	}

protected:
	// -- connection identity / config (all filled by the constructor)
		CServerAddr*	m_pIrcEntry;	// +4    CFavorites IRC server entry
	CServerAddr*	m_pChatEntry;	// +8    CFavorites chat server entry (the cursor)
	CServerAddr*	m_pDirEntry;	// +12   CFavorites directory entry
	CFavorites*		m_pFavorites;	// +16   the server table (ctor a3)
	CNetGameDlg*	m_pOwner;		// +20   the sheet that created us (ctor a2)
	char		m_szAuthToken[32];	// +24   auth/session token strcpy'd on login (memset 0x20)

	// -- live-session state; only the live network handler fills these in, so on a
	// plain identity record they stay zero (the ctor memset zeroes the whole span).
	unsigned char	m_pad56[28];	// +56   (zeroed; unidentified)
	int			m_bConnecting;	// +84   connect-in-progress guard
	unsigned char	m_pad88[20];	// +88   (zeroed; unidentified)
	int			m_unk108;		// +108  (zeroed; unidentified)
	chatroom_t*	m_pRoomList;	// +112  circular room-list head
	void*		m_pChatWindow;	// +116  unused on an identity record (the session
								//       fields live on CNetGameDlg at these offsets)
	int			m_unk120;		// +120  (zeroed; unidentified)
	CChatUser*	m_pUserList;	// +124  ditto -- the roster lives on the sheet
	CChatClient*	m_pSelfIdentity;	// +128  ditto -- the sheet points here, not vice versa
	unsigned char	m_pad132[8];	// +132  (zeroed; unidentified)
	chatroom_t*	m_pCurrentRoom;	// +140  joined room (FindByAddress result)
	int			m_unk144;		// +144  (zeroed; unidentified)
	int			m_unk148;	// +148  session flag set on (re)connect
	int			m_unk152;	// +152  session flag
	int			m_bReconnect;	// +156  reconnect-requested flag
	unsigned char	m_pad160[152];	// +160  (zeroed; unidentified -> +312)

	// -- three chat servers, each {port, host[256]} (IRC / chat / directory)
	int			m_nPortIrc;		// +312  default 6667
	char		m_szHostIrc[256];	// +316  IRC server host (-> +572)
	char		m_szPlayerName[256];// +572  local player name (sprintf "%s")
	long		m_lSelfStatus;	// +828  local WON member status
	int			m_nPortChat;	// +832  default 2667
	char		m_szHostChat[256];	// +836  chat server host (-> +1092)
	int			m_nPortDir;		// +1092 default 6002
	char		m_szHostDir[256];	// +1096 directory server host (-> +1352, sizeof 0x548)
};

static_assert( sizeof( CChatClient ) == 0x548, "CChatClient sizeof must be 1352" );

// One chat-room member; sizeof 0x10C, vftable 0x4ACF54 with the deleting
// destructor 0x404810 in its one slot.  Intrusive singly-linked list off the
// sheet.
class CChatUser
{
public:
	CChatUser( const char* pszNick, long lStatus );
	virtual ~CChatUser();

	char		m_szNick[256];	// +4    nickname (raw C-string)
	long		m_lStatus;		// +260  WON member status/flags (unconfirmed semantics)
	CChatUser*	m_pNext;		// +264  intrusive next
};

static_assert( sizeof( CChatUser ) == 0x10C, "CChatUser sizeof must be 268" );

#endif // CHATCLIENT_H
