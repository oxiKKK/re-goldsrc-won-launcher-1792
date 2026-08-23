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
// Purpose: declares CServerInfo and CServerRule, the queried-server record
//          and its rule list.
//
// $NoKeywords: $
//=============================================================================

#ifndef SERVERINFO_H
#define SERVERINFO_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include <stdio.h>
#include "Token.h"

#define MAX_PING_SLOTS	32		// ping array is double[32]; memset 0x100 bytes

// Server-query state machine (CServerInfo::m_nStatus, +96).
enum SvQueryState
{
	SVQ_IDLE          = 0,	// idle / finished / torn down
	SVQ_QUEUED        = 1,	// enqueued for a refresh pass
	SVQ_SOCKET_OPEN   = 2,	// datagram socket created (OpenConnection)
	SVQ_CONNECT_RETRY = 3,	// connect failed outright, awaiting retry
	SVQ_CONNECTED     = 4,	// connect succeeded / OnConnect -- ready to query
	SVQ_PING_SENT     = 5,	// "ping" sent, awaiting the 'j' reply
	SVQ_PING_DONE     = 6,	// ping reply received
	SVQ_INFO_SENT     = 7,	// "infostring" sent, awaiting "infostringresponse"
	SVQ_INFO_DONE     = 8,	// info string received
	SVQ_PLAYERS_SENT  = 9,	// "players" sent, awaiting the 'D' list
	SVQ_PLAYERS_DONE  = 10,	// player list received
	SVQ_RULES_SENT    = 11,	// "rules" sent, awaiting the 'E' list
	SVQ_RULES_DONE    = 12,	// rules received -- query complete
	SVQ_DEAD          = 13	// unreachable / errored out
};

// One server rule ("sv_gravity" "800"); a node in CServerInfo's rules list.
// Plain non-polymorphic node, sizeof 0x0C.
class CServerRule
{
public:
	CServerRule( const char* pszKey, const char* pszValue );	// 0x460770
	~CServerRule();											// 0x461D20
	void	Save( FILE* fp );										// 0x460850

	// Parse a "{ key value ... }" block into a rule list; *ppBuf is advanced past
	// the closing brace on success.
	static BOOL	ParseRules( CServerRule** ppHead, char** ppBuf );	// 0x460880

	CString			m_strKey;	// +0
	CString			m_strValue;	// +4
	CServerRule*	m_pNext;	// +8
};

// One player record in a server's "players NN { ...
class CPlayerInfo
{
public:
	CPlayerInfo( const char* pszName );			// 0x452000
	virtual ~CPlayerInfo();						// 0x452090 (slot 0 = scalar dtor 0x452070)

	void	Save( int iIndex, FILE* fp );		// 0x451C70
	void	SetKey( const char* pszKey, const char* pszValue );	// 0x451D50
	BOOL	Parse( char** ppBuf );				// 0x451E20

	// Nothing in the image reads or writes +4; the ctor starts at +8.
	int		m_unk4;			// +4
	CString	m_strName;		// +8   "name" (init "unknown")
	int		m_iId;			// +12  "id"
	int		m_iColors;		// +16  "colors"
	int		m_iFrags;		// +20  "frags"
	int		m_iTime;		// +24  "time"
	// +28..+39 (0x1C..0x27): untouched by ctor/Save/Parse -- reserved.
	BYTE	m_reserved1C[12];	// +28
	double	m_dConnTime;	// +40  local arrival time (set by the player-list parser)
};

class CHLAsyncSocket;

class CServerInfo
{
public:
	CServerInfo( const char* pszAddress, int nUserData );	// 0x461AA0
	virtual ~CServerInfo();									// 0x461D00 (slot 0 = 0x461D70)

	void	SaveToFile( FILE* fp );						// 0x460B00
	void	SetKeyValue( const char* pszKey, const char* pszValue );	// 0x461040
	BOOL	LoadFromBuffer( char** ppBuf );				// 0x4616A0

	// - connection state-machine helpers (CServerConnection is this class) ---
	void	ClearPlayers();								// 0x461E60 (free m_ppPlayers + records)
	void	AllocPlayers( int nMaxPlayers );			// 0x461ED0 (realloc player array)
	BOOL	SendInfoRequest();							// 0x461F30 ("infostring\n", -> SVQ_INFO_SENT)
	void	SendPlayersRequest();						// 0x461FD0 ("players",     -> SVQ_PLAYERS_SENT)
	void	SendRulesRequest();							// 0x462070 ("rules",       -> SVQ_RULES_SENT)
	void	SendPingRequest();							// 0x462110 ("ping",        -> SVQ_PING_SENT)
	void	CloseSocket();								// 0x4621B0 (delete m_pSocket)
	void	ResetRetry();								// 0x4621D0
	BOOL	OpenConnection();							// 0x4621E0 (new CHLAsyncSocket + Create -> SVQ_SOCKET_OPEN)
	BOOL	Connect();									// 0x4622D0 (AsyncSelect + Connect -> SVQ_CONNECTED)
	BOOL	BeginPlayerQuery();							// 0x462330 (alloc players, -> SVQ_PLAYERS_SENT)
	void	SetFiltered( int bFiltered );				// 0x462370
	int		GetFiltered();								// 0x462390
	CServerRule* AddRule( const char* pszKey, const char* pszValue );	// 0x4623A0
	void	ClearRules();								// 0x462410 (free the rules list)
	void	ComputePingStats();							// 0x462450 (min ping -> m_dSvPing, packet loss)
	int		GetServerId();								// 0x4624F0
	void	SetPingTime( double dTime );				// 0x462500
	double	GetPingTime();								// 0x462520

public:
	// - offsets disasm-verified against ctor/SetKeyValue/SaveToFile ---
	// (the compiler-generated vptr occupies +0 via the virtual dtor)
	int			m_unk4;			// +4    never written or read in this band
	// An embedded rule the compiler tracks as a whole sub-object: the ctor
	// inlines CServerRule's own body over it with key and value both empty, and
	// its m_pNext at +16 is the rules-list head everything else walks.
	CServerRule	m_rules;			// +8
	CString		m_strAddress;		// +20   "address" (= ctor arg)
	int			m_nPort;			// +24   "port"  AND ctor userdata arg (shared field)
	CString		m_strName;			// +28   "name"  (def "?")
	CString		m_strMap;			// +32   "map"   (def "?")
	CString		m_strGame;			// +36   "game"  (def "Half-Life")
	CString		m_strDir;			// +40   "dir"   (def "VALVE")
	int			m_nMaxPlayers;		// +44   "maxplayers"
	int			m_nCurrentPlayers;	// +48   "currentplayers"
	int			m_nProxyMaxPlayers;	// +52   proxied slots, summed over the group
	int			m_nProxyCurPlayers;	// +56   proxied players, summed over the group
	int			m_nProtocol;		// +60   "protocol"
	int			m_bLan;				// +64   "lan"
	double		m_dSvPing;			// +72   "svping"
	int			m_bNoResponse;		// +80   "noresponse"
	int			m_nRetry;			// +84   connection retry/seq (reused as state-machine retry)
	double		m_dSendTime;		// +88   connection request send-time (overlays the +88/+92 slots)
	int			m_nStatus;			// +96   "status" / connection STATE field
	float		m_flPacketLoss;		// +100  "packetloss" (def 100.0)
	double		m_rgPing[MAX_PING_SLOTS];	// +104  "pingNN" (double[32])
	int			m_nNumPings;		// +360  = g_nNumPings
	CString		m_strUrl;			// +364  "url"
	CString		m_strDownload;		// +368  "dl"
	int			m_bMod;				// +372  "mod"
	int			m_nVersion;			// +376  "version"
	int			m_nSize;			// +380  "size"
	char		m_cSvType;			// +384  "svtype" (def 'l')
	char		m_cSvOs;			// +385  "svos"   (def 'w')
	BYTE		_pad386[2];			// +386
	int			m_bPassword;		// +388  "password"
	int			m_bSvSide;			// +392  "svside"
	int			m_bClDll;			// +396  "cldll"
	BYTE		m_bProxyTarget;		// +400  "proxytarget"
	BYTE		m_bProxy;			// +401  "proxy"
	BYTE		_pad402[2];			// +402
	CString		m_strProxyAddress;	// +404  "proxyaddress"
	DWORD		m_dwProxyIp;		// +408  inet_addr(host part)
	int			m_iProxyPort;		// +412  parsed from m_strProxyAddress (default 27015)
	char		m_szHLVersion[32];	// +416  "hlversion" strcpy (def g_szPatchVersion)
	CPlayerInfo** m_ppPlayers;		// +448  malloc(4*count)
	CHLAsyncSocket*	m_pSocket;			// +452  connection's CHLAsyncSocket back-pointer (=0 as a server record)
	CServerInfo* m_pNext;			// +456  next in the browser doc's server list
	CServerInfo*	m_pJoinNext;	// +460  join/sort chain (ServerBrowser_CollectJoinable, 0x43C910)
	int			m_bFavorite;		// +464  "favorite"
	int			m_bIpx;				// +468  "ipx"
	CObject*	m_pOwnedQuery;		// +472  ctor-zeroed; nothing in the band reads it
	union
	{
		int			m_iOrder;		// +476
		CServerInfo*	m_pBatchNext;	// +476
	};
	int			m_bFiltered;		// +480  "filtered"
	int			m_nFullMax;			// +484  "fullmax" (also player count in Load; def 64)
	int			m_nServerId;		// +488  = dword_4D1BFC++ (unique counter)
	int			m_unk492;		// +492
	double		m_dPingTime;		// +496  ctor sets -1.0 (getter 0x462520)
};
// sizeof == 0x1F8 (504)


#endif // SERVERINFO_H
