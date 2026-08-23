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
// Purpose: declares the WON server-list module: CServerAddr, CFavorites and the
//          woncomm.lst parser.
//
// $NoKeywords: $
//=============================================================================

#ifndef LAUNCHERSERVERS_H
#define LAUNCHERSERVERS_H
#ifdef _WIN32
#pragma once
#endif

// One entry in a server bucket.  A bucket is itself a CServerAddr whose `next`
// is the head of the chain, so the list helpers all live on CFavorites and take
// the bucket as their first argument.
class CServerAddr
{
public:
	CServerAddr();
	~CServerAddr();

	char			host_name[64];	// +0   host string
	unsigned short	port;			// +64
	short			pad;			// +66  (never written)
	DWORD			reserved1;		// +68  (zeroed; unused)
	DWORD			reserved2;		// +72  (zeroed; unused)
	DWORD			sort_flag;		// +76  partition scratch flag
	CServerAddr*	next;			// +80
};

static_assert( sizeof( CServerAddr ) == 0x54, "CServerAddr must match the binary's 84-byte record" );

enum EServerList
{
	SERVERLIST_IRC			= 0,	// IRC servers (no woncomm.lst block)
	SERVERLIST_TITAN		= 1,	// Titan block    (WON chat / community servers)
	SERVERLIST_AUTH			= 2,	// Auth block     (WON directory servers)
	SERVERLIST_MASTER		= 3,	// Master block   (game master list)
	SERVERLIST_MODSERVER	= 4,	// ModServer block
	SERVERLIST_COUNT
};

class CFavorites
{
public:
	CFavorites();
	~CFavorites();

public:
	void			Initialize( void );

	CServerAddr*	BeginMasterList( void );
	CServerAddr*	BeginModList( void );
	BOOL			NextMasterList( void );
	BOOL			NextModList( void );

	char*			GetMasterAddr( void );
	unsigned short	GetMasterPort( void );
	char*			GetModAddr( void );
	unsigned short	GetModPort( void );

	// special servers
	void			AddServerAuth( const char* pszAddr, unsigned short port );
	void			AddServerTitan( const char* pszAddr, unsigned short port );
	void			AddServerMaster( const char* pszAddr, unsigned short port );
	void			AddServerMod( const char* pszAddr, unsigned short port );

	CServerAddr*	GetIrcServers( void );
	CServerAddr*	GetTitanServers( void );
	CServerAddr*	GetAuthServers( void );

	// 0x425810 -- the launcher's base directory, used to build favsvrs.dat's path.
	// base_dir is at offset 0, so the body is just `mov eax, ecx`.
	const char*		GetBaseDir( void );

	// lists
	void			ResolveMasterLists( void );
	void			PartitionList( void );
	void			ParseServers( void );

private:
	// The bucket-walking helpers.  Every one of these is a __thiscall CFavorites
	// member taking the bucket on the stack, not a CServerAddr member: the
	// callers load ecx with the CFavorites and push the bucket address.
	void	Free( CServerAddr* pBucket );
	void	RotateLast( CServerAddr* pBucket );
	void	ShuffleMasters( CServerAddr* pBucket );
	void	Dedup( CServerAddr* pBucket );
	void	Insert( CServerAddr* pBucket, CServerAddr* pNode );
	void	Resolve( CServerAddr* pBucket );
	void	Partition( CServerAddr* pBucket );

public:
	char			base_dir[256];				// +0
	CServerAddr*	master_cursor;				// +256
	CServerAddr*	mod_cursor;					// +260
	CServerAddr		entries[SERVERLIST_COUNT];	// +264
};

extern CFavorites*	gFavorites;

#endif // LAUNCHERSERVERS_H
