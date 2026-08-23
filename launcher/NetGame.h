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
// Purpose: declares CNetGameDlg, CNetGameDoc, CServerBrowserDlg, netfilter_t
//          and the server-browser backend interface.
//
// $NoKeywords: $
//=============================================================================

#ifndef NETGAME_H
#define NETGAME_H
#ifdef _WIN32
#pragma once
#endif

#include "Profile.h"

void	AntiCheat_DecodeString( void* pData, int len, int key );
void	AntiCheat_EncodeString( void* pData, int len, int key );

// The server-browser document (g_pServerBrowser, 0x4F4DF8).
class CServerInfo;
class CWnd;
class CHLMasterAsyncSocket;
class CNetGameDlg;

class CServerBrowser	// 0x2CC0 bytes
{
public:
	CServerBrowser();

	~CServerBrowser();
	void 			SaveFavoriteServers( void );
	char*			GetPlayerName( void );

	char	m_szPersistFile[132];		// +0      "<favorites base>/favsvrs.dat"
	char	m_pad132[128];				// +132
	CGameClientConfig	m_playerConfig;// +260    player config block (Profile.h, 0x2A78)
	char	m_szLogoName[260];			// +11132  "Settings\Logo"       (default "None")
	char	m_szLogoColor[64];			// +11392  "Settings\Logo Color" (default "Orange")
};

// List / query helpers that operate on a raw CServerInfo chain (not a whole
// document), so they stay free functions.
void	ServerBrowser_PruneServers( CServerInfo** ppHead, int bKeepFavorites );
char*	NET_CleanServerName( const char* pszName );
int		ServerBrowser_CompareAddr( const void* a, const void* b );
int __stdcall ServerBrowser_CompareInfo( const CServerInfo* a, const CServerInfo* b, int unused );	// retn 0Ch
CServerInfo*	ServerBrowser_SplitFavorites( CNetGameDlg* pSheet, int bKeepFavorites );
void	ServerBrowser_MergeIncoming( CNetGameDlg* pSheet );
void	ServerBrowser_DedupeByAddr( CNetGameDlg* pSheet );
void	ServerBrowser_CreateMasterSocket( CNetGameDlg* pSheet, CHLMasterAsyncSocket** ppOut );
void	ServerBrowser_CollectJoinable( CNetGameDlg* pSheet, CServerInfo** ppChain,
			CServerInfo** ppBest, int* pnCount, int bSkipLocal );
CServerInfo*	ServerBrowser_UnlinkServer( CServerInfo** ppHead, CServerInfo* pInfo );
int		ServerBrowser_BuildFilterInfo( const struct netfilter_t* pFilter, char* pszInfo, int nMaxSize );
void	Rate_ApplyFromConfig( void );
void	ServerBrowser_RequestPings( CServerInfo* pHead );
int		ServerBrowser_RefreshActiveTimes( CServerInfo* pHead );
void	ServerBrowser_SortList( int nCount, CServerInfo* pHead );	// sort the +460 chain
void	ServerBrowser_CopyConfig( CServerBrowser* pDest, const CServerBrowser* pSrc );

extern int	g_bWonLoginRequired;

extern int	g_bHubNeedsRefresh;
extern int	g_bNetGameSheetOpened;

#include <afxdlgs.h>
#include "DlgConnectableBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "RoomDialog.h"
#include "LogoDlg.h"
#include "resource_dlg.h"

class CChatClient;
class CWONMsg;
class CRoomList;
struct chatroom_t;
struct CChatUser;

class CHLLanAsyncSocket;

CNetGameDlg*	NetGame_CreateSheet();
char*	Rooms_StripHiddenPrefix( char** ppszName );
class CServerBrowserDlg;

/////////////////////////////////////////////////////////////////////////////
// CNetGameDlg

class CNetGameDlg : public CPropertySheet
{
// Construction
public:
	CNetGameDlg( CServerBrowser* pDoc, int nReserved );

// Attributes
public:
	int		m_unk80;			// +80
	int		m_bConnecting;		// +84
	int		m_nRetries;			// +88
	BYTE	m_pad92[4];			// +92
	double	m_dTimeout;			// +96
	int		m_nMaxSockets;		// +104
	CRoomList*	m_pMsgRing;		// +108
	CRoomList*	m_pRoomList;	// +112
	CServerBrowserDlg*	m_pPage;	// +116
	CHLMasterAsyncSocket*	m_pMaster;	// +120
	CChatUser*	m_pUserList;	// +124
	CChatClient*	m_pSelfIdentity;	// +128
	CServerInfo*	m_pServerListHead;	// +132
	CServerInfo*	m_pIncoming;	// +136
	chatroom_t*	m_pCurrentRoom;	// +140
	int		m_bDirty;			// +144
	int		m_bJoinAnnounced;	// +148
	int		m_bTitanGotData;	// +152
	int		m_bNeedReconnect;	// +156
	CServerBrowser*	m_pDoc;		// +160
	int		m_bListDirty;		// +164
	int		m_nQueryGeneration;	// +168
	int		m_unk172;			// +172
	char	m_szStatus[256];	// +176
	BYTE	m_pad432[768];		// +432
	int		m_nReconnectTries;	// +1200
	BYTE	m_pad1204[272];		// +1204
	char	m_msgBuffer[0x8000];	// +1476
	int		m_nChatSessionId;	// +34244
	class EasyTitanSocket*	m_pTitanSocket;	// +34248
	CHLLanAsyncSocket*	m_pLanSocket0;	// +34252
	CHLLanAsyncSocket*	m_pLanSocket1;	// +34256
	masterfetch_t*	m_pPendingQuery;	// +34260

// Operations
public:
	int		ComputeMaxSockets();
	CServerBrowser*	GetDoc();
	CServerBrowser*	LoadDoc();
	void	StartLanQuery( BOOL bEnable );
	int		StopLanQuery();

	CServerInfo*	AddServer( const char* pszAddr, int nPort, int bForceNew );
	void			ClearServers( int bKeepFavorites );
	int				CountVisible();
	int				CountPlayers();
	int				IsDirty();
	void			SetDirty( int bDirty );
	void			BuildFilter( struct netfilter_t* pFilter );
	void			QueryMaster( const struct netfilter_t* pFilter );
	void			SetPage( CServerBrowserDlg* pPage );
	void			RemoveServer( CServerInfo* pInfo );
	void			ConnectMaster( const char* pszHost, unsigned int nPort );
	int				FetchRoomList( int nFlag );
	int				LaunchChatServer( char* pszArgs );
	void			SetStatusText( const char* pszFormat, ... );
	void			ChatPrintf( const char* pszFormat, ... );
	// CNetGameDlg::GetCurrentRoom (0x43cd90)
	chatroom_t*		GetCurrentRoom()	{ return m_pCurrentRoom; }
	void			SetCurrentRoom( chatroom_t* pRoom );
	// CNetGameDlg::IsListDirty (0x43cfd0)
	int				IsListDirty()		{ return m_bListDirty; }

	// CNetGameDlg::SetListDirty (0x43cfe0)
	//
	// MFC's byte-identical CPropertySheet::EnableStackedTabs folded onto this
	// address; calling that one writes MFC's own field.
	void			SetListDirty( int bDirty )	{ m_bListDirty = bDirty; }

// Implementation
public:
	void			RefreshPageIfDirty();
	BOOL			HasPendingQuery();
	void			SetQueryGeneration( int nGen );
	void			OnMasterQueryDone( int bFailed );
	void			ServicePendingQuery();
	void			ServiceChat();

	CChatUser*		AddUser( const char* pszNick, long lStatus );
	void			RemoveAllUsers();
	CChatUser*		FindUser( long lUserId );
	CChatUser*		FindAndUnlinkUser( long lUserId );
	void			OnMemberList( CWONMsg* pMsg, BOOL bResetList );
	BOOL			OnUsersLeft( CWONMsg* pMsg );					// opcode 5
	BOOL			OnChatText( CWONMsg* pMsg );					// opcode 7
	int				ReceiveTitanMsg( class EasyTitanSocket* pSocket, void* pBuffer, class CWONMsg* pMsg,
						unsigned int* pnService, unsigned int* pnMsgType, unsigned int nMax );
	BOOL			JoinRoom( CChatClient* pSelf );
	void			SendChatText( const void* pText, int cbText );
	void			RequestRoomList();
	void			ListRooms();
	void			JoinRoomByName( const char* pszName );
	BOOL			Authenticate( int bForce );
	void			FindPlayer( const char* pszNick );
	void			Pump();
	void			CloseTitanSocket();
	void			CloseSockets();
};

class CODChatEdit;
class CODComboBox;
class CODHLListCtrl;
class CODIRCUserListCtrl;
class CHLChatLineCtrl;

// The browser's server filter, loaded from the page's profile section by
// CServerBrowserDlg::LoadFilter and applied row by row by ApplyFilter.
struct netfilter_t				// 376 bytes (0x178)
{
	int		m_bHideNoResponse;	// +0    "Filter Responded"
	int		m_bHideEmpty;		// +4    "Filter Empty"
	int		m_bHideFull;		// +8    "Filter Full"
	int		m_bFavoritesOnly;	// +12   "Filter Favorite"
	int		m_bLimitPing;		// +16   "Filter Ping"
	int		m_nPingMax;			// +20   "Filter PingMax" (-1 = no limit)
	int		m_bByGame;			// +24   "Filter Game"
	char	m_szGame[260];		// +28   "Filter Game Name" (def the engine gamedir)
	int		m_bByMap;			// +288  "Filter Map"
	char	m_szMap[64];		// +292  "Filter Map Name"
	int		m_bLinuxOnly;		// +356  "Filter OS"
	int		m_bDedicatedOnly;	// +360  "Filter Dedicated"
	int		m_bFilterProxies;	// +364  set when either proxy flag is on
	int		m_bProxiesOnly;		// +368  "Filter IsProxy"
	int		m_bHideProxies;		// +372  "Filter IsNotProxy"
};

void	ChatUserList_AddRow( CServerBrowserDlg* pPage, CChatUser* pUser );
void	ChatUserList_RemoveRow( CServerBrowserDlg* pPage, CChatUser* pUser );
void	ChatUserList_Reseed( CServerBrowserDlg* pPage );

/////////////////////////////////////////////////////////////////////////////
// CServerBrowserDlg dialog

class CServerBrowserDlg : public CDlgConnectableBase
{
	friend class CNetGameDlg;
	friend class CODHLListCtrl;
	friend void	ChatUserList_AddRow( CServerBrowserDlg*, CChatUser* );
	friend void	ChatUserList_RemoveRow( CServerBrowserDlg*, CChatUser* );
	friend void	ChatUserList_Reseed( CServerBrowserDlg* );

// Construction
public:
	// nMode: 1 = internet browse / spectate, 0 = chat rooms.
	CServerBrowserDlg( int nMode, CWnd* pParent = NULL );
	virtual ~CServerBrowserDlg();

// Dialog Data
protected:
	//{{AFX_DATA(CServerBrowserDlg)
	enum { IDD = IDD_DLG156 };
	CODStatic		m_lblVersion;		// +240  RGB( 150, 90, 0 )
	CODStatic		m_lblStatus;		// +336  RGB( 200, 200, 200 )
	CODBlendBtn		m_btnDisconnect;	// +432
	CODBlendBtn		m_btnResume;		// +672
	CODBitmapButton	m_btnMinimize;		// +912
	CODBlendBtn		m_btnServerInfo;	// +1000
	CODBlendBtn		m_btnUpdate;		// +1240
	CODBlendBtn		m_btnAddServer;		// +1480
	CRoomStatic		m_lblConnecting;	// +1720
	CODBlendBtn		m_btnSwitchPage;	// +1832
	CODBlendBtn		m_btnJoin;			// +2072
	CODBlendBtn		m_btnSpare;			// +2312
	CODStatic		m_lblChatPrompt;	// +2552
	CODBlendBtn		m_btnRefresh;		// +2648
	CODBlendBtn		m_btnFind;			// +2888
	CODBlendBtn		m_btnCreateRoom;	// +3128
	CODBlendBtn		m_btnFilter;		// +3368
	CODBlendBtn		m_btnCreateServer;	// +3608
	CODBlendBtn		m_btnDone;			// +3848
	//}}AFX_DATA

// Attributes
protected:
	netfilter_t		m_filter;			// +4088
	int				m_bReady;			// +4464
	int				m_bFavoritesOnlyView;// +4468
	BYTE			m_pad4472[4];		// +4472
	CNetGameDlg*	m_pBrowserEngine;	// +4476
	HGLOBAL			m_headerLoaded;		// +4480
	int				m_headerStride;		// +4484
	int				m_headerW;			// +4488
	int				m_headerH;			// +4492
	CBrush			m_bkBrush;			// +4496
	CODChatEdit*	m_pChatText;		// +4504
	CHLChatLineCtrl*	m_pChatInput;	// +4508
	CODIRCUserListCtrl*	m_pUserList;// +4512
	CFont			m_fontChatInput;	// +4516
	CFont			m_fontChatText;		// +4524
	int				m_unk4532;			// +4532
	int				m_nSidebarW;			// +4536
	CODHLListCtrl*	m_pServerList;		// +4540
	BYTE			m_bSelSpectate;		// +4544
	BYTE			m_pad4545[3];		// +4545
	int				m_iSelRow;			// +4548
	int				m_unk4552;			// +4552
	int				m_unk4556;			// +4556
	CODStatic*		m_pLblSpeed;			// +4560
	CODComboBox*	m_pComboSpeed;		// +4564
	BYTE			m_pad4568[4];		// +4568
	int				m_nMode;				// +4572
	int				m_unk4576;			// +4576
	int				m_nRowHeight;			// +4580
	CFont*			m_pFontArial9;			// +4584
	BYTE			m_pad4588[4];		// +4588
	int				m_yBanner;				// +4592
	int				m_unk4596;			// +4596
	int				m_wChatPrompt;			// +4600
	int				m_xButtons;				// +4604
	int				m_unk4608;			// +4608
	int				m_wButtons;				// +4612
	int				m_unk4616;			// +4616
	int				m_xServerList;			// +4620
	int				m_yServerList;			// +4624
	int				m_xRight;				// +4628
	int				m_unk4632;			// +4632
	int				m_unk4636;			// +4636
	int				m_unk4640;			// +4640
	int				m_unk4644;			// +4644
	int				m_hChatInput;			// +4648
	int				m_yBottom;				// +4652
	int				m_unk4656;			// +4656
	int				m_unk4660;			// +4660

// Operations
public:

	// (Re)allocate the hosted property-sheet engine and run the CLoginDlg gate when
	// the chat transport requires a WON login.  Nonzero to proceed.
	int		Run();

	BOOL	HasControls();
	CODChatEdit*	GetChatText();
	CNetGameDlg*	GetBrowserEngine()	{ return m_pBrowserEngine; }
	void	RebuildVisibleList();
	void	BuildFilterHelp();
	void	EnterRoom( chatroom_t* pRoom );
	void	UpdateRoomBanner();

protected:
	void	SaveConnectionSpeed();
	void	ConnectSelected();
	void	SetFavoriteOnSelection( int bFavorite );
	void	SelectMatchingRows( const char* pszNeedle );
	int		CountCheckedRows();
	void	MaybeLoadRoomList();
	void	FindUserOnServers( const char* pszNick );

	// Generated message map functions
	//{{AFX_MSG(CServerBrowserDlg)
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnRefresh();
	afx_msg int		OnCreate( LPCREATESTRUCT lpcs );
	afx_msg HBRUSH	OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void	OnDoneClicked();
	afx_msg void	OnCreateServer();
	afx_msg void	OnPaint();
	afx_msg void	OnJoin();
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThread );
	afx_msg void	OnSwitchPage();
	afx_msg void	OnMinimize();
	afx_msg void	OnDisconnect();
	afx_msg void	OnResume();
	afx_msg void	OnLButtonUp( UINT nFlags, CPoint point );
	afx_msg void	OnUserListValidate();
	afx_msg void	OnSpeedChanged();
	afx_msg void	OnFind();
	afx_msg void	OnCreateRoom();
	afx_msg void	OnChatSend();
	afx_msg void	OnInsertUserNick();
	afx_msg void	OnServerInfo();
	afx_msg void	OnUpdateFromMaster();
	afx_msg void	OnEditFilter();
	afx_msg void	OnAddByAddress();
	afx_msg void	OnMarkFavorite();
	afx_msg void	OnClearFavorite();
	afx_msg void	OnDeleteSelected();
	afx_msg void	OnRefreshSelected();
	afx_msg void	OnResortList();
	//}}AFX_MSG

	afx_msg void	OnRefreshCriteria();

// Overrides
	//{{AFX_VIRTUAL(CServerBrowserDlg)
protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();
	virtual void	OnCancel();
	virtual int		RMLPreIdle();
	virtual void	InitButtonStrips();
	virtual void	Relayout();
	virtual BOOL	HasCreateGameButton();
	virtual void	ApplyFilter( netfilter_t* pFilter );
	virtual void	LoadFilter( netfilter_t* pFilter );
	virtual const char*	GetSettingsSection();
	virtual const char*	GetHeaderBitmap();
	virtual UINT	GetJoinCaptionId();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	LayoutPage( int bBrowseMode );

	DECLARE_MESSAGE_MAP()
};

#endif // NETGAME_H
