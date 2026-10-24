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
// Purpose: declares the launcher's file / search-path layer (Quake common.c
//          lineage).
//
// $NoKeywords: $
//=============================================================================

#ifndef COMMON_H
#define COMMON_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>
#include <stdio.h>

#define MAX_OSPATH	260

// A file opened through the COM_* layer: a pak member is { offset, length,
// shared-pak handle }; a loose file uses handle alone.

typedef struct fileinfo_s
{
	int		filepos;
	int		filelen;
	int		handle;
} fileinfo_t;

extern char	com_gamedir[MAX_OSPATH];	// e.g. "<basedir>/valve"
extern int	com_filesize;			// size of the last file opened

// Byte-order swap primitives (Quake common.c lineage).
short	ShortSwap( short l );
short	ShortNoSwap( short l );
int		LongSwap( int l );
int		LongNoSwap( int l );
float	FloatSwap( float f );
float	FloatNoSwap( float f );

extern float	(*LittleFloat)	( float l );
extern float	(*BigFloat)		( float l );
extern int		(*LittleLong)	( int l );
extern int		(*BigLong)		( int l );
extern short	(*LittleShort)	( short l );
extern short	(*BigShort)		( short l );
extern int		bigendien;

// Engine Sys_File* primitives (sys_win.c fork).
void	Sys_Error( const char* error, ... );
int		Sys_FileOpenRead( const char* path, int* pHandle );
void	Sys_FileClose( int handle );
void	Sys_FileSeek( int handle, int position );
int		Sys_FileRead( int handle, void* dest, int count );
int		Sys_FileTime( const char* path );

// COM_* search-path filesystem (common.c fork).
char*	COM_GetBaseDir( void );
void	COM_ResetGameDirectories( void );
void	COM_Shutdown( void );				// free the whole search path
void	COM_AddGameDirectory( int bBase, const char* pszBaseDir, const char* pszDir );
void	COM_FileBase( const char* in, char* out );	// base name, no dir/ext
char*	COM_SkipPath( char* pathname );		// the file-name part
void	COM_FixSlashes( char* p );
char*	COM_FindPath( const char* pszRel );
int		COM_InitFilesystem( void );
int		COM_FindFile( const char* filename, fileinfo_t* pfi, FILE** file );
int		COM_OpenFile( const char* filename, fileinfo_t* pfi );
void	COM_CloseFile( fileinfo_t fi );
void*	COM_LoadMallocFile( const char* path );
int		COM_CreatePath( char* pszPath );

int		COM_ParseHostPort( const char* src, char* hostOut, int* portOut, int defPort );

struct sockaddr_in;

// 0x467EF0, defined in this TU.
int		NET_StringToAdr( const char* pszString, sockaddr_in* padr );

struct pack_s;

typedef struct mapinfo_s
{
	char			name[260];				// +0    "maps/<name>.bsp" (pak/loose path)
	char			gamedir[260];			// +260  owning game dir (for filtering)
	unsigned char	flags;					// +520  MAPINFO_FROMPAK | MAPINFO_BASE
	char			pad521[3];				// +521  (alignment to +524)
	struct pack_s*	pack;					// +524  owning pak (NULL for loose files)
	char			pad528[260];			// +528  unused engine slack
	struct mapinfo_s* next;					// +788
} mapinfo_t;

// mapinfo_t::flags bits.
#define MAPINFO_FROMPAK	1	// candidate came from a pak (not a loose file)
#define MAPINFO_BASE	2	// base "valve" search path (the last list element)

// COM_GetMapList fFlags: which map sources to enumerate (OR together).
#define GETMAPS_LOOSE	1	// loose "<dir>/maps/*.bsp" files
#define GETMAPS_PAKS	2	// "maps/*.bsp" members inside pak files

int		COM_GetMapList( mapinfo_t** ppList, int fFlags, int bFilter );
void	COM_FreeMapList( mapinfo_t** ppList );

// The installed player models, in the same records (freed by COM_FreeMapList).
int		COM_GetPlayerModelList( mapinfo_t** ppList );

#endif // COMMON_H
