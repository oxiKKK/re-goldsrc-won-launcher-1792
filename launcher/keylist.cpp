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
// Purpose: the list-control sort-key stack (KeyList_*).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

#define KEYLIST_MAX		14

// A key is a 1-based column number, negated for a descending sort; the stack
// keeps the older keys as tiebreakers behind the newest one.
static int	s_aiKeys[16];			// 0x4F94B4  the keys (+ 2 guard slots)
static int	s_nKeys;				// 0x4F94F4  count in use
static char	s_szKeyList[256];		// 0x4F93B4  serialized form

/*
==================
KeyList_Remove (0x464680)
==================
*/
void KeyList_Remove( int iKey )
{
	int	iWanted;
	int	i, j;

	iWanted = abs( iKey );

	for ( i = s_nKeys; i >= 1; i-- )
	{
		if ( abs( s_aiKeys[i - 1] ) == iWanted )
		{
			for ( j = i; j <= s_nKeys; j++ )	// shift the tail down, guard slot included
				s_aiKeys[j - 1] = s_aiKeys[j];

			s_aiKeys[s_nKeys] = 0;
			s_nKeys--;
		}
	}
}

/*
==================
KeyList_Add (0x464710)
==================
*/
int KeyList_Add( int iKey )
{
	int	i;

	if ( s_nKeys >= KEYLIST_MAX )
		return s_nKeys;

	for ( i = s_nKeys + 1; i > 0; i-- )		// shift [0..nKeys] up to [1..nKeys+1]
		s_aiKeys[i] = s_aiKeys[i - 1];

	s_nKeys++;
	s_aiKeys[0] = iKey;
	return iKey;
}

/*
==================
KeyList_Set (0x464750)
==================
*/
int KeyList_Set( int iKey )
{
	KeyList_Remove( iKey );
	return KeyList_Add( iKey );
}

/*
==================
KeyList_Append (0x464770)
==================
*/
int KeyList_Append( int iKey )
{
	if ( s_nKeys >= KEYLIST_MAX )
		return s_nKeys;

	s_aiKeys[s_nKeys + 1] = s_aiKeys[s_nKeys];		// carry the guard slot up
	s_aiKeys[s_nKeys]     = iKey;
	s_nKeys++;
	return s_nKeys;
}

/*
==================
KeyList_GetKey (0x4647A0)
==================
*/
int KeyList_GetKey( int index )
{
	return s_aiKeys[index];
}

/*
==================
KeyList_Clear (0x4647B0)
==================
*/
void KeyList_Clear( void )
{
	s_aiKeys[15] = 0;
	s_nKeys      = 0;
	s_aiKeys[0]  = 0;
}

/*
==================
KeyList_FromString (0x4647D0)
==================
*/
char* KeyList_FromString( char* pszList )
{
	char*	pszTok;

	KeyList_Clear();
	for ( pszTok = strtok( pszList, ";" ); pszTok; pszTok = strtok( NULL, ";" ) )
		KeyList_Append( atoi( pszTok ) );

	return pszTok;
}

/*
==================
KeyList_ToString (0x464810)
==================
*/
char* KeyList_ToString( void )
{
	char	szTmp[16];
	int		i;

	memset( s_szKeyList, 0, sizeof( s_szKeyList ) );
	for ( i = 0; i < s_nKeys; i++ )
	{
		sprintf( szTmp, "%i;", s_aiKeys[i] );
		strcat( s_szKeyList, szTmp );
	}
	return s_szKeyList;
}
