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
// Purpose: the launcher's file / search-path layer (Quake common.c lineage).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

typedef struct
{
	char	name[56];
	int		filepos;
	int		filelen;
} packfile_t;					// 64 bytes, as on disk (dpackfile_t)

typedef struct pack_s
{
	char		filename[MAX_OSPATH];	// +0
	int			handle;					// +260  Sys_File* handle on the .pak
	int			numfiles;				// +264
	packfile_t*	files;					// +268
} pack_t;

typedef struct searchpath_s
{
	int					base;					// +0    survives gamedir changes
	char				filename[MAX_OSPATH];	// +4    directory (empty for paks)
	char				gamedir[MAX_OSPATH];	// +264  owning game dir name
	pack_t*				pack;					// +524  only one of filename/pack used
	struct searchpath_s* next;					// +528
} searchpath_t;							// 0x214 bytes

static searchpath_t*	com_searchpaths;	// 0x4F9E1C
int				com_filesize;		// 0x4F9E20

char			com_gamedir[MAX_OSPATH];	// 0x4F9CF0  e.g. "<basedir>/valve"

#define MAX_HANDLES		10

static FILE*	sys_handles[MAX_HANDLES];	// 0x4F9DF4
static int		com_errorEntered;			// 0x4FB358
static int		com_fsInitialized;			// 0x4FB35C

/*
==================
Sys_Error (0x4671B0)
==================
*/
void Sys_Error( const char* error, ... )
{
	va_list	argptr;
	char	text[1024];

	va_start( argptr, error );
	vsprintf( text, error, argptr );
	va_end( argptr );

	if ( com_errorEntered )
	{
		fprintf( stderr, "%s\n", text );
		exit( 2 );
	}

	com_errorEntered = 1;
	Launcher_ErrorMessageBox( 0, text );
	exit( 3 );
}

/*
==================
findhandle (0x467220)
==================
*/
static int findhandle( void )
{
	int		i;

	for ( i = 1; i < MAX_HANDLES; i++ )
	{
		if ( !sys_handles[i] )
			return i;
	}

	Sys_Error( "out of handles" );
	return -1;
}

/*
==================
Qfilelength (0x467250)
==================
*/
static int Qfilelength( FILE* f )
{
	int		pos;
	int		end;

	pos = ftell( f );
	fseek( f, 0, SEEK_END );
	end = ftell( f );
	fseek( f, pos, SEEK_SET );

	return end;
}

/*
==================
Sys_FileOpenRead (0x467290)
==================
*/
int Sys_FileOpenRead( const char* path, int* pHandle )
{
	FILE*	f;
	int		i;

	i = findhandle();

	f = fopen( path, "rb" );
	if ( !f )
	{
		*pHandle = -1;
		return -1;
	}

	sys_handles[i] = f;
	*pHandle = i;

	return Qfilelength( f );
}

/*
==================
Sys_FileClose (0x4672E0)
==================
*/
void Sys_FileClose( int handle )
{
	fclose( sys_handles[handle] );
	sys_handles[handle] = NULL;
}

/*
==================
Sys_FileSeek (0x467310)
==================
*/
void Sys_FileSeek( int handle, int position )
{
	fseek( sys_handles[handle], position, SEEK_SET );
}

/*
==================
Sys_FileRead (0x467330)
==================
*/
int Sys_FileRead( int handle, void* dest, int count )
{
	return fread( dest, 1, count, sys_handles[handle] );
}

/*
==================
Sys_FileTime (0x467360)
==================
*/
int Sys_FileTime( const char* path )
{
	FILE*	f;

	f = fopen( path, "rb" );
	if ( !f )
		return -1;

	fclose( f );
	return 1;
}

/*
==================
Sys_mkdir (0x467390)
==================
*/
static int Sys_mkdir( const char* path )
{
	return _mkdir( path );
}

/*
==================
COM_PackFileCmp (0x4673A0)
==================
*/
static int COM_PackFileCmp( const char* pakname, const char* name )
{
	for ( ;; )
	{
		if ( *pakname != '/' || *name != '\\' )
		{
			if ( tolower( *pakname ) != tolower( *name ) )
				return -1;
			if ( !*pakname )
				break;
		}

		pakname++;
		if ( !pakname[-1] )
			break;
		if ( !*name++ )
			break;
	}

	return 0;
}

/*
==================
COM_FindFile (0x467400)
==================
*/
int COM_FindFile( const char* filename, fileinfo_t* pfi, FILE** file )
{
	searchpath_t*	search;
	pack_t*			pak;
	char			netpath[260];
	int				i;

	com_filesize = -1;
	COM_InitFilesystem();

	if ( file )
	{
		if ( pfi )
			Sys_Error( "COM_FindFile: both phFile and file set" );
	}
	else if ( !pfi )
	{
		Sys_Error( "COM_FindFile: neither phFile or file set" );
	}

	// Search through the path, one element at a time.
	for ( search = com_searchpaths; search; search = search->next )
	{
		if ( strlen( search->filename ) )
		{
			// A directory tree element.
			sprintf( netpath, "%s/%s", search->filename, filename );
			if ( Sys_FileTime( netpath ) == -1 )
				continue;

			com_filesize = Sys_FileOpenRead( netpath, &i );
			if ( pfi )
			{
				pfi->handle = i;
				pfi->filepos = 0;
			}
			else
			{
				Sys_FileClose( i );
				*file = fopen( netpath, "rb" );
			}
			return com_filesize;
		}

		pak = search->pack;
		if ( !pak )
			continue;

		// Look through all the pak file elements.
		for ( i = 0; i < pak->numfiles; i++ )
		{
			if ( !COM_PackFileCmp( pak->files[i].name, filename ) )
				break;
		}
		if ( i >= pak->numfiles )
			continue;

		// Found it!
		if ( pfi )
		{
			pfi->handle = pak->handle;
			pfi->filepos = pak->files[i].filepos;
			pfi->filelen = pak->files[i].filelen;
			Sys_FileSeek( pak->handle, pak->files[i].filepos );
		}
		else
		{
			*file = fopen( pak->filename, "rb" );
			if ( *file )
				fseek( *file, pak->files[i].filepos, SEEK_SET );
		}

		com_filesize = pak->files[i].filelen;
		return com_filesize;
	}

	if ( pfi )
		pfi->handle = -1;		// not-found sentinel (callers test fi.handle == -1)
	else
		*file = NULL;

	com_filesize = -1;
	return -1;
}

/*
==================
COM_OpenFile (0x467650)
==================
*/
int COM_OpenFile( const char* filename, fileinfo_t* pfi )
{
	return COM_FindFile( filename, pfi, NULL );
}

/*
==================
COM_CloseFile (0x467670)
==================
*/
void COM_CloseFile( fileinfo_t fi )
{
	searchpath_t*	s;

	for ( s = com_searchpaths; s; s = s->next )
	{
		if ( s->pack && s->pack->handle == fi.handle )
			return;
	}

	Sys_FileClose( fi.handle );
}

/*
==================
COM_FileBase (0x4676B0)
==================
*/
void COM_FileBase( const char* in, char* out )
{
	char	c;
	int		len = (int)strlen( in ) + 1;
	int		end, i, start, n;

	if ( len == 1 )				// empty source
	{
		*out = 0;
		return;
	}

	end = len - 2;				// index of the last character

	// Walk back to the extension dot (stopping at a path separator first).
	i = end;
	if ( len != 2 )
	{
		while ( i )
		{
			c = in[i];
			if ( c == '.' || c == '/' || c == '\\' )
				break;
			--i;
		}
	}
	if ( in[i] == '.' )
		end = i - 1;			// drop ".ext"

	// Walk back to the path separator: the base name starts just after it.
	for ( i = len - 2; i >= 0; --i )
	{
		c = in[i];
		if ( c == '/' || c == '\\' )
			break;
	}
	start = ( i >= 0 && ( in[i] == '/' || in[i] == '\\' ) ) ? i + 1 : 0;

	n = end - start + 1;
	strncpy( out, &in[start], n );
	out[n] = 0;
}

/*
==================
COM_LoadMallocFile (0x467740)
==================
*/
void* COM_LoadMallocFile( const char* path )
{
	fileinfo_t	fi;
	int			len;
	char*		buf;

	len = COM_OpenFile( path, &fi );
	if ( fi.handle == -1 )
		return NULL;

	buf = (char*)malloc( len + 1 );
	if ( !buf )
	{
		COM_CloseFile( fi );
		return NULL;
	}

	buf[len] = 0;
	Sys_FileRead( fi.handle, buf, len );
	COM_CloseFile( fi );
	return buf;
}

/*
==================
COM_LoadPackFile (0x4677E0)
==================
*/
static pack_t* COM_LoadPackFile( const char* packfile )
{
	struct
	{
		char	id[4];
		int		dirofs;
		int		dirlen;
	} header;
	packfile_t*	files;
	pack_t*		pak;
	int			handle;
	int			numfiles;

	if ( Sys_FileOpenRead( packfile, &handle ) == -1 )
		return NULL;

	Sys_FileRead( handle, &header, sizeof( header ) );
	if ( header.id[0] != 'P' || header.id[1] != 'A' || header.id[2] != 'C' || header.id[3] != 'K' )
	{
		Sys_FileClose( handle );
		return NULL;
	}

	numfiles = header.dirlen / sizeof( packfile_t );
	if ( numfiles > 4096 )
	{
		Sys_FileClose( handle );
		return NULL;
	}

	files = (packfile_t*)malloc( numfiles * sizeof( packfile_t ) );
	memset( files, 0, numfiles * sizeof( packfile_t ) );
	Sys_FileSeek( handle, header.dirofs );
	Sys_FileRead( handle, files, header.dirlen );

	pak = (pack_t*)malloc( sizeof( pack_t ) );
	memset( pak, 0, sizeof( pack_t ) );
	strcpy( pak->filename, packfile );
	pak->files = files;
	pak->numfiles = numfiles;
	pak->handle = handle;

	return pak;
}

/*
==================
COM_Shutdown (0x467920)
==================
*/
void COM_Shutdown( void )
{
	searchpath_t*	sp;
	searchpath_t*	next;

	for ( sp = com_searchpaths; sp; sp = next )
	{
		next = sp->next;

		if ( sp->pack )
		{
			Sys_FileClose( sp->pack->handle );
			if ( sp->pack->files )
				free( sp->pack->files );
			free( sp->pack );
		}
		free( sp );
	}
}

/*
==================
COM_ReverseSearchPaths (0x467990)
==================
*/
static void COM_ReverseSearchPaths( void )
{
	searchpath_t*	sp = com_searchpaths;
	searchpath_t*	prev = NULL;
	searchpath_t*	next;

	while ( sp )
	{
		next = sp->next;
		sp->next = prev;
		prev = sp;
		sp = next;
	}

	com_searchpaths = prev;
}

/*
==================
COM_FileExistsOnDisk (0x4679C0)
==================
*/
static int COM_FileExistsOnDisk( const char* psz )
{
	return psz && *psz && _access( psz, 0 ) != -1;
}

/*
==================
COM_SkipPath (0x4679F0)
==================
*/
char* COM_SkipPath( char* pathname )
{
	char*	last = pathname;
	char*	p;

	for ( p = pathname; *p; p++ )
	{
		if ( *p == '/' )
			last = p + 1;
	}
	return last;
}

/*
==================
COM_FixSlashes (0x467A10)
==================
*/
void COM_FixSlashes( char* p )
{
	for ( ; *p; ++p )
		if ( *p == '/' )
			*p = '\\';
}

/*
==================
COM_FindPath (0x467A30)
==================
*/
char* COM_FindPath( const char* pszRel )
{
	static char		buf[260];		// 0x4F9BE4
	searchpath_t*	sp;

	for ( sp = com_searchpaths; sp; sp = sp->next )
	{
		if ( !sp->pack )
		{
			sprintf( buf, "%s/%s", sp->gamedir, pszRel );
			COM_FixSlashes( buf );
			if ( COM_FileExistsOnDisk( buf ) )
				return buf;
		}
	}

	sprintf( buf, "%s/%s", "valve", pszRel );
	COM_FixSlashes( buf );
	return buf;
}

/*
==================
COM_ResetGameDirectories (0x467AB0)
==================
*/
void COM_ResetGameDirectories( void )
{
	searchpath_t*	sp;
	searchpath_t*	next;
	searchpath_t*	kept = NULL;

	for ( sp = com_searchpaths; sp; sp = next )
	{
		next = sp->next;

		if ( sp->base )
		{
			sp->next = kept;
			kept = sp;
		}
		else
		{
			if ( sp->pack )
			{
				Sys_FileClose( sp->pack->handle );
				if ( sp->pack->files )
					free( sp->pack->files );
				free( sp->pack );
			}
			free( sp );
		}
	}

	com_searchpaths = kept;
	COM_ReverseSearchPaths();
	sprintf( com_gamedir, "valve" );
}

/*
==================
COM_AddSearchPath (0x467B50)
==================
*/
static void COM_AddSearchPath( searchpath_t** ppList, const char* pszDir,
	pack_t* pak, int bBase, const char* pszGameDir )
{
	searchpath_t*	sp;

	if ( !ppList )
		return;

	sp = (searchpath_t*)malloc( sizeof( searchpath_t ) );
	memset( sp, 0, sizeof( searchpath_t ) );

	sp->base = bBase;
	sp->pack = pak;
	if ( pszDir )
		strcpy( sp->filename, pszDir );
	if ( pszGameDir )
		strcpy( sp->gamedir, pszGameDir );

	sp->next = *ppList;
	*ppList = sp;
}

/*
==================
COM_AddGameDirectory (0x467BF0)
==================
*/
void COM_AddGameDirectory( int bBase, const char* pszBaseDir, const char* pszDir )
{
	char	dir[260];
	char	pakfile[260];
	pack_t*	pak;
	int		i;

	if ( !bBase )
		COM_ResetGameDirectories();

	if ( !pszDir )
		return;

	sprintf( dir, "%s/%s", pszBaseDir, pszDir );
	sprintf( com_gamedir, dir );		// (sic) binary passes dir as the format string

	for ( i = 0; ; i++ )
	{
		sprintf( pakfile, "%s/pak%i.pak", dir, i );
		pak = COM_LoadPackFile( pakfile );
		if ( !pak )
			break;
		COM_AddSearchPath( &com_searchpaths, NULL, pak, bBase, pszDir );
	}

	COM_AddSearchPath( &com_searchpaths, dir, NULL, bBase, pszDir );
}

static char	s_szBaseDir[260];		// 0x4F9AE0

/*
==================
COM_GetBaseDir (0x467CE0)
==================
*/
char* COM_GetBaseDir( void )
{
	char	filename[260];
	char*	slash;
	char	last;
	int		len;

	if ( GetModuleFileNameA( AfxGetInstanceHandle(), filename, sizeof( filename ) ) )
	{
		slash = strrchr( filename, '\\' );
		if ( slash )
			slash[1] = 0;
	}

	strcpy( s_szBaseDir, filename );

	len = strlen( s_szBaseDir );
	if ( len > 0 )
	{
		last = s_szBaseDir[len - 1];
		if ( last == '\\' || last == '/' )
			s_szBaseDir[len - 1] = 0;
	}

	return s_szBaseDir;
}

/*
==================
COM_InitFilesystem (0x467D80)
==================
*/
int COM_InitFilesystem( void )
{
	char*	pszGame = NULL;

	if ( com_fsInitialized )
		return 1;

	com_fsInitialized = 1;
	com_searchpaths = NULL;

	COM_AddGameDirectory( 1, COM_GetBaseDir(), "valve" );

	if ( CheckParm( "-game", &pszGame ) && pszGame )
		COM_AddGameDirectory( 0, COM_GetBaseDir(), pszGame );

	return com_fsInitialized;
}

float	(*LittleFloat)	( float l );	// 0x4FB334
float	(*BigFloat)		( float l );	// 0x4FB338
int		(*LittleLong)	( int l );		// 0x4FB33C
int		(*BigLong)		( int l );		// 0x4FB340
short	(*LittleShort)	( short l );	// 0x4FB344
short	(*BigShort)		( short l );	// 0x4FB348
int		bigendien;						// 0x4FB34C

/*
==================
ShortSwap (0x467DF0)
==================
*/
short ShortSwap( short l )
{
	unsigned char	b1 = l & 0xFF;
	unsigned char	b2 = ( l >> 8 ) & 0xFF;

	return ( b1 << 8 ) + b2;
}

/*
==================
ShortNoSwap (0x467E10)
==================
*/
short ShortNoSwap( short l )
{
	return l;
}

/*
==================
LongSwap (0x467E20)
==================
*/
int LongSwap( int l )
{
	unsigned char	b1 = l & 0xFF;
	unsigned char	b2 = ( l >> 8 ) & 0xFF;
	unsigned char	b3 = ( l >> 16 ) & 0xFF;
	unsigned char	b4 = ( l >> 24 ) & 0xFF;

	return ( (int)b1 << 24 ) + ( (int)b2 << 16 ) + ( (int)b3 << 8 ) + b4;
}

/*
==================
LongNoSwap (0x467E60)
==================
*/
int LongNoSwap( int l )
{
	return l;
}

/*
==================
FloatSwap (0x467E70)
==================
*/
float FloatSwap( float f )
{
	union { float f; unsigned char b[4]; } dat1, dat2;

	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

/*
==================
FloatNoSwap (0x467EA0)
==================
*/
float FloatNoSwap( float f )
{
	return f;
}

/*
==================
COM_CreatePath (0x467EB0)
==================
*/
int COM_CreatePath( char* pszPath )
{
	char*	p;
	char	c;
	int		result = 0;

	c = pszPath[1];
	for ( p = pszPath + 1; c; ++p )
	{
		if ( c == '/' || c == '\\' )
		{
			*p = 0;
			result = Sys_mkdir( pszPath );		// ignore "already exists"
			*p = c;
		}
		c = p[1];
	}
	return result;
}

/*
==================
NET_StringToAdr (0x467EF0)
==================
*/
int NET_StringToAdr( const char* pszString, sockaddr_in* padr )
{
	char	cp[128];
	char*	pColon;

	memset( padr, 0, sizeof( *padr ) );
	padr->sin_family = AF_INET;
	padr->sin_port   = 0;

	strcpy( cp, pszString );

	// split off ":port"
	for ( pColon = cp; *pColon; pColon++ )
	{
		if ( *pColon == ':' )
		{
			*pColon = 0;
			padr->sin_port = htons( (u_short)atoi( pColon + 1 ) );
		}
	}

	if ( cp[0] < '0' || cp[0] > '9' )
	{
		struct hostent*	pHost = gethostbyname( cp );
		if ( pHost )
		{
			padr->sin_addr.s_addr = *(DWORD*)pHost->h_addr_list[0];
			return 1;
		}
		return 0;
	}

	padr->sin_addr.s_addr = inet_addr( cp );
	return 1;
}

/*
==================
COM_ParseHostPort (0x467FD0)
==================
*/
int COM_ParseHostPort( const char* src, char* hostOut, int* portOut, int defPort )
{
	const char*	pColon = strstr( src, ":" );
	if ( pColon )
	{
		int	n = (int)( pColon - src );
		strncpy( hostOut, src, n );
		hostOut[n] = 0;
		*portOut = atoi( pColon + 1 );
		return *portOut;
	}

	strcpy( hostOut, src );
	*portOut = defPort;
	return defPort;
}

// dheader_t / lump directory (only the fields this probe touches).
typedef struct
{
	int		fileofs;
	int		filelen;
} dlump_t;

typedef struct
{
	int		version;		// 29 (Quake) or 30 (GoldSrc)
	dlump_t	lumps[15];		// lumps[0] is the entity lump
} dheader_t;				// 124 bytes (0x7C)

/*
==================
COM_BSPHasDeathmatchSpawn (0x468040)
==================
*/
static int COM_BSPHasDeathmatchSpawn( const char* filename )
{
	FILE*		file;
	dheader_t	header;
	long		basepos;
	int			len;
	char*		ents;
	const char*	pszEntity;
	int			bFound;

	COM_FindFile( filename, NULL, &file );
	if ( !file || com_filesize == -1 )
		return 0;

	basepos = ftell( file );
	if ( fread( &header, sizeof( header ), 1, file ) != 1
		|| ( header.version != 29 && header.version != 30 ) )
	{
		fclose( file );
		return 0;
	}

	len = header.lumps[0].filelen;
	if ( len > 0x1000000 )		// 16 MiB entity-lump sanity cap
	{
		fclose( file );
		return 0;
	}

	fseek( file, basepos + header.lumps[0].fileofs, SEEK_SET );
	ents = (char*)malloc( len + 1 );
	if ( !ents )
	{
		fclose( file );
		return 0;
	}

	fread( ents, 1, len, file );
	fclose( file );
	ents[len] = 0;

	pszEntity = NULL;
	if ( g_pCurrentMod )
		pszEntity = g_pCurrentMod->GetKey( "mpentity" );
	if ( !pszEntity )
		pszEntity = "info_player_deathmatch";

	bFound = ( strstr( ents, pszEntity ) != NULL );
	free( ents );
	return bFound;
}

/*
==================
COM_FreeMapList (0x468190)
==================
*/
void COM_FreeMapList( mapinfo_t** ppList )
{
	mapinfo_t*	node;
	mapinfo_t*	next;

	if ( !ppList )
		return;

	for ( node = *ppList; node; node = next )
	{
		next = node->next;
		delete node;
	}

	*ppList = NULL;
}

/*
==================
COM_GetMapList (0x4681C0)
==================
*/
int COM_GetMapList( mapinfo_t** ppList, int fFlags, int bFilter )
{
	searchpath_t*	search;
	pack_t*			pak;
	mapinfo_t*		list = NULL;
	mapinfo_t*		node;
	char			netpath[260];
	WIN32_FIND_DATAA	fd;
	HANDLE			hFind;
	int				i;

	*ppList = NULL;

	for ( search = com_searchpaths; search; search = search->next )
	{
		pak = search->pack;

		if ( pak )
		{
			if ( !( fFlags & GETMAPS_PAKS ) )
				continue;

			// Scan the pak directory for maps/*.bsp members.
			for ( i = 0; i < pak->numfiles; i++ )
			{
				const char*	pakname = pak->files[i].name;
				char		fname[128];
				char		ext[128];
				char		dir[128];

				if ( _strnicmp( "maps/", pakname, 4 ) )
					continue;

				_splitpath( pakname, NULL, dir, fname, ext );
				if ( _strcmpi( ext, ".bsp" ) )
					continue;

				// Skip pak members that also exist loose on disk.
				sprintf( netpath, "%s/%s", search->gamedir, pakname );
				hFind = FindFirstFileA( netpath, &fd );
				if ( hFind != INVALID_HANDLE_VALUE )
				{
					FindClose( hFind );
					continue;
				}

				node = new mapinfo_t;
				memset( node, 0, sizeof( mapinfo_t ) );
				sprintf( node->name, "maps/%s.bsp", fname );
				node->pack = pak;
				node->flags |= MAPINFO_FROMPAK;
				if ( !search->next )
					node->flags |= MAPINFO_BASE;
				strcpy( node->gamedir, search->gamedir );
				node->next = list;
				list = node;
			}
		}
		else if ( fFlags & GETMAPS_LOOSE )
		{
			// Loose maps/*.bsp tree.
			sprintf( netpath, "%s/maps/*.bsp", search->filename );
			hFind = FindFirstFileA( netpath, &fd );
			if ( hFind == INVALID_HANDLE_VALUE )
				continue;

			do
			{
				node = new mapinfo_t;
				memset( node, 0, sizeof( mapinfo_t ) );
				sprintf( node->name, "maps/%s", fd.cFileName );
				node->flags &= ~MAPINFO_FROMPAK;
				if ( !search->next )
					node->flags |= MAPINFO_BASE;
				strcpy( node->gamedir, search->gamedir );
				node->next = list;
				list = node;
			}
			while ( FindNextFileA( hFind, &fd ) );

			FindClose( hFind );
		}
	}

	if ( bFilter )
	{
		mapinfo_t*	kept = NULL;
		mapinfo_t*	next;

		for ( node = list; node; node = next )
		{
			next = node->next;
			if ( COM_BSPHasDeathmatchSpawn( node->name ) )
			{
				node->next = kept;
				kept = node;
			}
			else
			{
				delete node;
			}
		}

		*ppList = kept;
	}

	return 1;
}

/*
==================
COM_GetPlayerModelList (0x468510)
==================
*/
int COM_GetPlayerModelList( mapinfo_t** ppList )
{
	searchpath_t*	search;
	pack_t*			pak;
	mapinfo_t*		list = NULL;
	mapinfo_t*		node;
	char			gamedir[260];
	char			netpath[256];
	char			dir[128];
	char			fname[128];
	char			ext[128];
	WIN32_FIND_DATAA	fd;
	HANDLE			hFind;
	int				nOwned;
	int				i;

	*ppList = NULL;

	sprintf( gamedir, "valve" );		// (sic) the literal as the format string
	if ( g_pCurrentMod )
	{
		const char*	pszDir = g_pCurrentMod->GetKey( "gamedir" );
		if ( pszDir && *pszDir )
			strcpy( gamedir, pszDir );

		const char*	pszNoModels = g_pCurrentMod->GetKey( "nomodels" );
		if ( pszNoModels && *pszNoModels && atoi( pszNoModels ) )
			return 1;			// the mod ships no player models
	}

	for ( search = com_searchpaths; search; search = search->next )
	{
		pak = search->pack;

		if ( pak )
		{
			// Preview bitmaps inside the pak, "models/player/<name>/<name>.bmp".
			for ( i = 0; i < pak->numfiles; i++ )
			{
				const char*	pakname = pak->files[i].name;

				if ( _strnicmp( "models/player", pakname, 13 ) )
					continue;

				_splitpath( pakname, NULL, dir, fname, ext );
				if ( _strcmpi( ext, ".bmp" ) )
					continue;

				sprintf( netpath, "models/player/%s/%s", fname, fname );
				if ( _strnicmp( netpath, pakname, strlen( netpath ) ) )
					continue;

				// Skip the pak entry when the model also exists loose on disk.
				sprintf( netpath, "%s/%s/%s.mdl", search->gamedir, dir, fname );
				hFind = FindFirstFileA( netpath, &fd );
				if ( hFind != INVALID_HANDLE_VALUE )
				{
					FindClose( hFind );
					continue;
				}

				node = new mapinfo_t;
				memset( node, 0, sizeof( mapinfo_t ) );
				strcpy( node->name, pakname );
				_strlwr( node->name );
				strcpy( node->gamedir, search->gamedir );
				node->flags |= MAPINFO_FROMPAK;
				node->pack = pak;
				node->next = list;
				list = node;
			}
		}

		if ( !search->pack )
		{
			// Every subdirectory of the loose "<dir>/models/player" tree.
			sprintf( netpath, "%s/models/player/*.*", search->gamedir );
			hFind = FindFirstFileA( netpath, &fd );
			if ( hFind == INVALID_HANDLE_VALUE )
				continue;

			do
			{
				if ( !( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
					continue;
				if ( fd.cFileName[0] == '.' )
					continue;

				node = new mapinfo_t;
				memset( node, 0, sizeof( mapinfo_t ) );
				sprintf( node->name, "models/player/%s/%s.bmp", fd.cFileName, fd.cFileName );
				_strlwr( node->name );
				strcpy( node->gamedir, search->gamedir );
				node->flags &= ~MAPINFO_FROMPAK;
				node->next = list;
				list = node;
			}
			while ( FindNextFileA( hFind, &fd ) );

			FindClose( hFind );
		}
	}

	// Keep only the models of the running game dir -- unless none of them are.
	nOwned = 0;
	for ( node = list; node; node = node->next )
	{
		if ( !_strcmpi( node->gamedir, gamedir ) )
			nOwned++;
	}

	if ( nOwned )
	{
		mapinfo_t*	kept = NULL;
		mapinfo_t*	next;

		for ( node = list; node; node = next )
		{
			next = node->next;
			if ( _strcmpi( node->gamedir, gamedir ) )
			{
				delete node;
			}
			else
			{
				node->next = kept;
				kept = node;
			}
		}
		*ppList = kept;
	}
	else
	{
		*ppList = list;
	}

	return 1;
}
