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
// Purpose: declares the list-control sort-key stack (KeyList_*).
//
// $NoKeywords: $
//=============================================================================

#ifndef KEYLIST_H
#define KEYLIST_H
#ifdef _WIN32
#pragma once
#endif

void	KeyList_Remove( int iKey );
int		KeyList_Add( int iKey );
int		KeyList_Set( int iKey );				// remove any copy, re-add at the head
int		KeyList_Append( int iKey );
int		KeyList_GetKey( int index );			// raw index, no bounds check
void	KeyList_Clear( void );
char*	KeyList_FromString( char* pszList );
char*	KeyList_ToString( void );

#endif // KEYLIST_H
