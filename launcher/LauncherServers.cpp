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
// Purpose: the WON server-list module: CServerAddr, CFavorites and the
//          woncomm.lst parser.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

CFavorites*	gFavorites;

/*
==================
CServerAddr::CServerAddr (0x424CF0)
==================
*/
CServerAddr::CServerAddr( void )
{
	host_name[0] = 0;
	port         = 0;
	reserved1    = 0;
	reserved2    = 0;
	sort_flag    = 0;
	next         = NULL;
}

/*
==================
CFavorites::CFavorites (0x424D10)
==================
*/
CFavorites::CFavorites( void )
{
	BeginMasterList();
	BeginModList();
}

/*
==================
CFavorites::~CFavorites (0x424DA0)

Four explicit Free calls, not a loop: entries[SERVERLIST_IRC] is never populated
from woncomm.lst and is not freed here.
==================
*/
CFavorites::~CFavorites( void )
{
	Free( &entries[SERVERLIST_TITAN] );
	Free( &entries[SERVERLIST_AUTH] );
	Free( &entries[SERVERLIST_MASTER] );
	Free( &entries[SERVERLIST_MODSERVER] );
}

/*
==================
CFavorites::Initialize (0x424E40)
==================
*/
void CFavorites::Initialize( void )
{
	CheckParm( "wipe", NULL );
	ParseServers();

	if ( !CheckParm( "-usefirstmaster", NULL ) )
	{
		// ShuffleMasters consumes rand(), so this sequence is load-bearing.
		ShuffleMasters( &entries[SERVERLIST_AUTH] );
		ShuffleMasters( &entries[SERVERLIST_MASTER] );
		ShuffleMasters( &entries[SERVERLIST_TITAN] );
		ShuffleMasters( &entries[SERVERLIST_MODSERVER] );
	}

	// base_dir holds the launcher base directory; chdir into it.
	memset( base_dir, 0, sizeof( base_dir ) );
	strcpy( base_dir, COM_GetBaseDir() );
	_chdir( base_dir );
}

/*
==================
CFavorites::Free (0x424EF0)
==================
*/
void CFavorites::Free( CServerAddr* pBucket )
{
	CServerAddr*	pNextNode;

	for ( CServerAddr* p = pBucket->next; p; p = pNextNode )
	{
		pNextNode = p->next;
		p->next = NULL;
		delete p;
	}
	pBucket->next = NULL;
}

/*
==================
CFavorites::RotateLast (0x424F30)
==================
*/
void CFavorites::RotateLast( CServerAddr* pBucket )
{
	CServerAddr*	last = pBucket->next;
	if ( !last || !last->next )
		return;

	// >= 2 nodes here, so the walk always reassigns prev before it is read.
	CServerAddr*	prev = last;
	while ( last->next )
	{
		prev = last;
		last = last->next;
	}
	prev->next    = last->next;
	last->next    = pBucket->next;
	pBucket->next = last;
}

/*
==================
CFavorites::ShuffleMasters (0x424F70)
==================
*/
void CFavorites::ShuffleMasters( CServerAddr* pBucket )
{
	int	count = 0;
	for ( CServerAddr* p = pBucket->next; p; p = p->next )
		++count;

	if ( count >= 2 )
	{
		int	rot = rand() & 63;
		while ( rot-- > 0 )
			RotateLast( pBucket );
	}
}

/*
==================
CFavorites::Dedup (0x424FC0)
==================
*/
void CFavorites::Dedup( CServerAddr* pBucket )
{
	CServerAddr*	pKeep = NULL;
	CServerAddr*	pNextNode;

	for ( CServerAddr* p = pBucket->next; p; p = pNextNode )
	{
		pNextNode = p->next;

		// keep p only if no later node duplicates it
		BOOL		bDup = FALSE;
		for ( CServerAddr* q = pNextNode; q; q = q->next )
		{
			if ( !_strcmpi( q->host_name, p->host_name ) && q->port == p->port )
			{
				bDup = TRUE;
				break;
			}
		}

		if ( bDup )
		{
			delete p;
		}
		else
		{
			p->next = pKeep;
			pKeep = p;
		}
	}

	// reverse the survivor stack back into the bucket (restores original order)
	pBucket->next = NULL;
	while ( pKeep )
	{
		pNextNode     = pKeep->next;
		pKeep->next   = pBucket->next;
		pBucket->next = pKeep;
		pKeep         = pNextNode;
	}
}

/*
==================
CFavorites::Insert (0x425050)
==================
*/
void CFavorites::Insert( CServerAddr* pBucket, CServerAddr* pNode )
{
	if ( !pBucket->next )
	{
		pBucket->next = pNode;
		return;
	}

	CServerAddr*	pTail = NULL;
	for ( CServerAddr* p = pBucket->next; p; p = p->next )
	{
		if ( !_strcmpi( pNode->host_name, p->host_name ) && pNode->port == p->port )
		{
			delete pNode;
			return;
		}
		pTail = p;
	}

	pNode->next = NULL;
	pTail->next = pNode;
}

/*
==================
CFavorites::Resolve (0x4250E0)
==================
*/
void CFavorites::Resolve( CServerAddr* pBucket )
{
	char	szBuf[64];

	for ( CServerAddr* p = pBucket->next; p; p = p->next )
	{
		sockaddr_in	adr;

		strcpy( szBuf, p->host_name );
		memset( &adr, 0, sizeof( adr ) );

		if ( NET_StringToAdr( p->host_name, &adr ) )
			sprintf( szBuf, "%s", inet_ntoa( adr.sin_addr ) );

		strcpy( p->host_name, szBuf );
	}
}

/*
==================
CFavorites::ResolveMasterLists (0x425190)
==================
*/
void CFavorites::ResolveMasterLists( void )
{
	Resolve( &entries[SERVERLIST_TITAN] );
	Resolve( &entries[SERVERLIST_AUTH] );
	Resolve( &entries[SERVERLIST_MASTER] );
	Resolve( &entries[SERVERLIST_MODSERVER] );

	Dedup( &entries[SERVERLIST_TITAN] );
	Dedup( &entries[SERVERLIST_AUTH] );
	Dedup( &entries[SERVERLIST_MASTER] );
	Dedup( &entries[SERVERLIST_MODSERVER] );
}

/*
==================
CFavorites::AddServerTitan (0x425200)
==================
*/
void CFavorites::AddServerTitan( const char* pszAddr, unsigned short port )
{
	CServerAddr*	pNode = new CServerAddr;

	strncpy( pNode->host_name, pszAddr, sizeof( pNode->host_name ) - 1 );
	pNode->port = port;
	Insert( &entries[SERVERLIST_TITAN], pNode );
}

/*
==================
CFavorites::AddServerAuth (0x425250)
==================
*/
void CFavorites::AddServerAuth( const char* pszAddr, unsigned short port )
{
	CServerAddr*	pNode = new CServerAddr;

	strncpy( pNode->host_name, pszAddr, sizeof( pNode->host_name ) - 1 );
	pNode->port = port;
	Insert( &entries[SERVERLIST_AUTH], pNode );
}

/*
==================
CFavorites::AddServerMaster (0x4252A0)
==================
*/
void CFavorites::AddServerMaster( const char* pszAddr, unsigned short port )
{
	CServerAddr*	pNode = new CServerAddr;

	strncpy( pNode->host_name, pszAddr, sizeof( pNode->host_name ) - 1 );
	pNode->port = port;
	Insert( &entries[SERVERLIST_MASTER], pNode );
}

/*
==================
CFavorites::AddServerMod (0x4252F0)
==================
*/
void CFavorites::AddServerMod( const char* pszAddr, unsigned short port )
{
	CServerAddr*	pNode = new CServerAddr;

	strncpy( pNode->host_name, pszAddr, sizeof( pNode->host_name ) - 1 );
	pNode->port = port;
	Insert( &entries[SERVERLIST_MODSERVER], pNode );
}

/*
==================
CFavorites::GetMasterAddr (0x425340)
==================
*/
char* CFavorites::GetMasterAddr( void )
{
	static char	addr[256];

	if ( master_cursor == &entries[SERVERLIST_MASTER] )
		sprintf( addr, WON_MASTER_DEFAULT_HOST );
	else
		sprintf( addr, master_cursor->host_name );
	return addr;
}

/*
==================
CFavorites::GetMasterPort (0x425380)
==================
*/
unsigned short CFavorites::GetMasterPort( void )
{
	if ( master_cursor == &entries[SERVERLIST_MASTER] )
		return PORT_WON_MASTER;
	return (unsigned short)master_cursor->port;
}

/*
==================
CFavorites::GetModAddr (0x4253A0)
==================
*/
char* CFavorites::GetModAddr( void )
{
	static char	addr[256];

	if ( mod_cursor == &entries[SERVERLIST_MODSERVER] )
		sprintf( addr, WON_MASTER_DEFAULT_HOST );
	else
		sprintf( addr, mod_cursor->host_name );
	return addr;
}

/*
==================
CFavorites::GetModPort (0x4253E0)
==================
*/
unsigned short CFavorites::GetModPort( void )
{
	if ( mod_cursor == &entries[SERVERLIST_MODSERVER] )
		return PORT_WON_MODSERVER;
	return (unsigned short)mod_cursor->port;
}

/*
==================
CFavorites::Partition (0x425400)
==================
*/
void CFavorites::Partition( CServerAddr* pBucket )
{
	CServerAddr*	pUnflagged = NULL;
	CServerAddr*	pFlagged   = NULL;
	CServerAddr*	pNextNode;

	for ( CServerAddr* p = pBucket->next; p; p = pNextNode )
	{
		pNextNode = p->next;
		if ( p->sort_flag )
		{
			p->next = pFlagged;
			pFlagged = p;
			p->sort_flag = 0;
		}
		else
		{
			p->next = pUnflagged;
			pUnflagged = p;
		}
	}

	pBucket->next = NULL;

	// reverse the flagged stack in first, then the unflagged, so unflagged ends up front
	while ( pFlagged )
	{
		pNextNode     = pFlagged->next;
		pFlagged->next = pBucket->next;
		pBucket->next = pFlagged;
		pFlagged      = pNextNode;
	}
	while ( pUnflagged )
	{
		pNextNode       = pUnflagged->next;
		pUnflagged->next = pBucket->next;
		pBucket->next   = pUnflagged;
		pUnflagged      = pNextNode;
	}
}

/*
==================
CFavorites::PartitionList (0x425480)

Partitions entries[SERVERLIST_TITAN], the Titan/room list.
==================
*/
void CFavorites::PartitionList( void )
{
	Partition( &entries[SERVERLIST_TITAN] );
}

/*
==================
CFavorites::ParseServers (0x425490)
==================
*/
void CFavorites::ParseServers( void )
{
	char	path[260];
	char*	val;
	FILE*	fp;

	sprintf( path, "woncomm.lst" );
	if ( CheckParm( "-comm", &val ) && val )
		strcpy( path, val );

	if ( COM_FindFile( path, NULL, &fp ) == -1 )
	{
		LOG( "\"%s\" not found -- no WON servers configured", path );
		return;
	}
	fclose( fp );

	char*	file = (char*)COM_LoadMallocFile( path );
	if ( !file )
	{
		LOG( "\"%s\" found but could not be loaded", path );
		return;
	}
	LOG( "parsing \"%s\"", path );

	CToken	tok( file );
	tok.SetCommentMode( 1 );

	for ( ;; )
	{
		tok.ParseNextToken();
		if ( !strlen( tok.token ) )
			break;

		int				idx;
		unsigned short	defPort;

		if ( !_strcmpi( tok.token, "Titan" ) )
		{
			idx = SERVERLIST_TITAN;
			defPort = PORT_WON_TITAN;
		}
		else if ( !_strcmpi( tok.token, "Auth" ) )
		{
			idx = SERVERLIST_AUTH;
			defPort = PORT_WON_AUTH;
		}
		else if ( !_strcmpi( tok.token, "Master" ) )
		{
			idx = SERVERLIST_MASTER;
			defPort = PORT_WON_MASTER;
		}
		else if ( !_strcmpi( tok.token, "ModServer" ) )
		{
			idx = SERVERLIST_MODSERVER;
			defPort = PORT_WON_MODSERVER;
		}
		else
		{
			Launcher_ShowMessageByIdEx( 0, IDS_SETTINGS_SERVERTYPEINVALID, path );
			break;
		}

		tok.ParseNextToken();
		if ( !strlen( tok.token ) || _strcmpi( tok.token, "{" ) )
		{
			Launcher_ShowMessageByIdEx( 0, IDS_TOKEN_EXPECTLEFTBRACE, path );
			break;
		}

		BOOL	bClosed = FALSE;
		for ( ;; )
		{
			tok.ParseNextToken();
			if ( !strlen( tok.token ) )
				break;							// EOF mid-block
			if ( !_strcmpi( tok.token, "}" ) )
			{
				bClosed = TRUE;
				break;
			}

			char	host[64];
			int		port;
			COM_ParseHostPort( tok.token, host, &port, defPort );

			switch ( idx )
			{
			case SERVERLIST_TITAN:
				AddServerTitan( host, (unsigned short)port );
				break;
			case SERVERLIST_AUTH:
				AddServerAuth( host, (unsigned short)port );
				break;
			case SERVERLIST_MASTER:
				AddServerMaster( host, (unsigned short)port );
				break;
			case SERVERLIST_MODSERVER:
				AddServerMod( host, (unsigned short)port );
				break;
			}
		}

		if ( !bClosed )
			Launcher_ShowMessageByIdEx( 0, IDS_TOKEN_EXPECTRIGHTBRACE, path );
	}

	free( file );

	LOG( "titan=%p auth=%p master=%p mod=%p (list heads' first entry)",
		 entries[SERVERLIST_TITAN].next,
		 entries[SERVERLIST_AUTH].next,
		 entries[SERVERLIST_MASTER].next,
		 entries[SERVERLIST_MODSERVER].next );
}

/*
==================
CFavorites::NextMasterList (0x4257B0)
==================
*/
BOOL CFavorites::NextMasterList( void )
{
	if ( !master_cursor )
		return FALSE;
	master_cursor = master_cursor->next;
	return master_cursor != NULL;
}

/*
==================
CFavorites::BeginMasterList (0x4257D0)
==================
*/
CServerAddr* CFavorites::BeginMasterList( void )
{
	master_cursor = &entries[SERVERLIST_MASTER];
	return master_cursor;
}

/*
==================
CFavorites::NextModList (0x4257E0)
==================
*/
BOOL CFavorites::NextModList( void )
{
	if ( !mod_cursor )
		return FALSE;
	mod_cursor = mod_cursor->next;
	return mod_cursor != NULL;
}

/*
==================
CFavorites::BeginModList (0x425800)
==================
*/
CServerAddr* CFavorites::BeginModList( void )
{
	mod_cursor = &entries[SERVERLIST_MODSERVER];
	return mod_cursor;
}

/*
==================
CFavorites::GetBaseDir (0x425810)
==================
*/
const char* CFavorites::GetBaseDir( void )
{
	return base_dir;
}

/*
==================
CFavorites::GetIrcServers (0x425820)

entries[0..2] heads, handed to the chat client: IRC, Titan and Auth.
==================
*/
CServerAddr* CFavorites::GetIrcServers( void )
{
	return &entries[SERVERLIST_IRC];
}

/*
==================
CFavorites::GetTitanServers (0x425830)
==================
*/
CServerAddr* CFavorites::GetTitanServers( void )
{
	return &entries[SERVERLIST_TITAN];
}

/*
==================
CFavorites::GetAuthServers (0x425840)
==================
*/
CServerAddr* CFavorites::GetAuthServers( void )
{
	return &entries[SERVERLIST_AUTH];
}

/*
==================
CServerAddr::~CServerAddr

ICF-folded onto NullStub (0x40E460); the compiler still emits real calls to it
from Insert, Free, Dedup and ~CFavorites.
==================
*/
CServerAddr::~CServerAddr( void )
{
}
