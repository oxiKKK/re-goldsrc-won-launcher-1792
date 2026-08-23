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
// Purpose: shell artwork loading and the DIB helpers.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/*
==================
DIB_LoadBitmapFile (0x409090)
==================
*/
HGLOBAL WINAPI DIB_LoadBitmapFile( const char* pszName )
{
	fileinfo_t	fi;
	int			size;
	char*		pData;
	HGLOBAL		hDIB;

	if ( !pszName || !*pszName )
		return NULL;

	size = COM_FindFile( pszName, &fi, NULL );
	if ( size == -1 )
	{
		Launcher_ShowMessageByIdEx( 0, IDS_DIB_OPENFAIL, pszName );
		return NULL;
	}
	COM_CloseFile( fi );

	pData = (char*)COM_LoadMallocFile( pszName );
	if ( !pData )
		return NULL;

	if ( ( (BITMAPFILEHEADER*)pData )->bfType == DIB_HEADER_MARKER )
	{
		hDIB = GlobalAlloc( GHND, size );
		if ( hDIB )
		{
			memcpy( GlobalLock( hDIB ), pData + sizeof( BITMAPFILEHEADER ),
				size - sizeof( BITMAPFILEHEADER ) );
			GlobalUnlock( hDIB );
			free( pData );
			return hDIB;
		}
	}

	free( pData );
	return NULL;
}

/*
==================
DIB_BlitDib (0x409190)
==================
*/
BOOL WINAPI DIB_BlitDib( HDC hdc, RECT* prcDst, HGLOBAL hDib, RECT* prcSrc )
{
	BITMAPINFO*	pbmi;
	void*		pBits;
	int			h, dw, dh, sw, sh;
	BOOL		bSuccess;

	if ( !hDib )
		return FALSE;

	pbmi  = (BITMAPINFO*)GlobalLock( hDib );
	pBits = DIB_FindBits( &pbmi->bmiHeader );
	h     = DIB_Height( &pbmi->bmiHeader );

	dw = prcDst->right  - prcDst->left;
	dh = prcDst->bottom - prcDst->top;
	sw = prcSrc->right  - prcSrc->left;
	sh = prcSrc->bottom - prcSrc->top;

	SetStretchBltMode( hdc, COLORONCOLOR );

	if ( dw == sw && dh == sh )
		bSuccess = SetDIBitsToDevice( hdc, prcDst->left, prcDst->top, dw, dh,
			prcSrc->left, h - prcSrc->top - sh, 0, (WORD)h, pBits, pbmi, DIB_RGB_COLORS );
	else
		bSuccess = StretchDIBits( hdc, prcDst->left, prcDst->top, dw, dh,
			prcSrc->left, h - prcSrc->top - sh, sw, sh, pBits, pbmi, DIB_RGB_COLORS, SRCCOPY );

	GlobalUnlock( hDib );

	return bSuccess;
}

/*
==================
DIB_FindBits (0x4092A0)
==================
*/
void* WINAPI DIB_FindBits( LPBITMAPINFOHEADER pDIB )
{
	return (char*)pDIB + pDIB->biSize + DIB_PaletteSize( pDIB );
}

/*
==================
DIB_Width (0x4092C0)
==================
*/
DWORD WINAPI DIB_Width( LPBITMAPINFOHEADER pDIB )
{
	// Anything that is not a BITMAPINFOHEADER is a legacy BITMAPCOREHEADER.
	if ( pDIB->biSize == sizeof( BITMAPINFOHEADER ) )
		return pDIB->biWidth;

	return ( (BITMAPCOREHEADER*)pDIB )->bcWidth;
}

/*
==================
DIB_Height (0x4092E0)
==================
*/
DWORD WINAPI DIB_Height( LPBITMAPINFOHEADER pDIB )
{
	if ( pDIB->biSize == sizeof( BITMAPINFOHEADER ) )
		return pDIB->biHeight;

	return ( (BITMAPCOREHEADER*)pDIB )->bcHeight;
}

/*
==================
DIB_PaletteSize (0x409300)
==================
*/
WORD WINAPI DIB_PaletteSize( LPBITMAPINFOHEADER pDIB )
{
	return (WORD)( DIB_NumColors( pDIB ) *
		( pDIB->biSize == sizeof( BITMAPINFOHEADER ) ? sizeof( RGBQUAD ) : sizeof( RGBTRIPLE ) ) );
}

/*
==================
DIB_NumColors (0x409320)
==================
*/
WORD WINAPI DIB_NumColors( LPBITMAPINFOHEADER pDIB )
{
	WORD	bits;

	if ( pDIB->biSize == sizeof( BITMAPINFOHEADER ) )
	{
		if ( pDIB->biClrUsed )
			return (WORD)pDIB->biClrUsed;
		bits = (WORD)pDIB->biBitCount;
	}
	else
		bits = ( (BITMAPCOREHEADER*)pDIB )->bcBitCount;

	switch ( bits )
	{
	case 1:  return 2;
	case 4:  return 16;
	case 8:  return 256;
	default: return 0;
	}
}
