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
// Purpose: CMessageBuffer, the launcher's serialize / network-message IO
//          buffer.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/*
==================
CMessageBuffer::CMessageBuffer (0x429010)
==================
*/
CMessageBuffer::CMessageBuffer( int cbInitial )
{
	m_bOverFlowed    = FALSE;
	m_pData          = NULL;
	m_nMaxSize       = 0;
	m_nCurSize       = 0;
	m_bAllowOverflow = TRUE;

	SZ_Alloc( cbInitial );
}

/*
==================
CMessageBuffer::~CMessageBuffer (0x429040)

Vftable slot 0 is the scalar deleting dtor at 0x428FF0, which is compiler glue
and belongs to no translation unit.
==================
*/
CMessageBuffer::~CMessageBuffer()
{
	SZ_Free();
}

/*
==================
CMessageBuffer::MSG_WriteChar (0x429050)

Folded with MSG_WriteByte -- the two differ only in the signedness the caller
reads back.
==================
*/
void* CMessageBuffer::MSG_WriteChar( char c )
{
	BYTE*	p = (BYTE*)SZ_GetSpace( 1 );

	*p = c;
	return p;
}

/*
==================
CMessageBuffer::MSG_WriteLong (0x429060)

Explicit little-endian, byte at a time -- not a dword store.
==================
*/
void* CMessageBuffer::MSG_WriteLong( int n )
{
	BYTE*	p = (BYTE*)SZ_GetSpace( 4 );

	p[0] = (BYTE)( n       );
	p[1] = (BYTE)( n >>  8 );
	p[2] = (BYTE)( n >> 16 );
	p[3] = (BYTE)( n >> 24 );
	return p;
}

/*
==================
CMessageBuffer::MSG_WriteString (0x429090)

Writes the terminator too; a NULL string writes just the terminator.
==================
*/
unsigned CMessageBuffer::MSG_WriteString( const char* psz )
{
	if ( psz )
		return SZ_Write( psz, strlen( psz ) + 1 );
	return SZ_Write( "", 1 );
}

/*
==================
CMessageBuffer::MSG_BeginReading (0x4290D0)
==================
*/
void CMessageBuffer::MSG_BeginReading()
{
	m_nMsgReadCount = 0;
	m_bMsgBadRead   = FALSE;
}

/*
==================
CMessageBuffer::MSG_ReadChar (0x4290E0)
==================
*/
int CMessageBuffer::MSG_ReadChar()
{
	int		c;

	if ( m_nMsgReadCount + 1 > m_nCurSize )
	{
		m_bMsgBadRead = TRUE;
		return -1;
	}

	c = (signed char)m_pData[m_nMsgReadCount];
	m_nMsgReadCount++;
	return c;
}

/*
==================
CMessageBuffer::MSG_ReadByte (0x429110)
==================
*/
int CMessageBuffer::MSG_ReadByte()
{
	int		c;

	if ( m_nMsgReadCount + 1 > m_nCurSize )
	{
		m_bMsgBadRead = TRUE;
		return -1;
	}

	c = m_pData[m_nMsgReadCount];
	m_nMsgReadCount++;
	return c;
}

/*
==================
CMessageBuffer::MSG_ReadShort (0x429140)

Signed 16-bit, little-endian.
==================
*/
int CMessageBuffer::MSG_ReadShort()
{
	int		v;

	if ( m_nMsgReadCount + 2 > m_nCurSize )
	{
		m_bMsgBadRead = TRUE;
		return -1;
	}

	v = (short)( m_pData[m_nMsgReadCount] | ( m_pData[m_nMsgReadCount + 1] << 8 ) );
	m_nMsgReadCount += 2;
	return v;
}

/*
==================
CMessageBuffer::MSG_ReadLong (0x429180)

32-bit, little-endian.
==================
*/
int CMessageBuffer::MSG_ReadLong()
{
	int		v;

	if ( m_nMsgReadCount + 4 > m_nCurSize )
	{
		m_bMsgBadRead = TRUE;
		return -1;
	}

	v = m_pData[m_nMsgReadCount]
		| ( m_pData[m_nMsgReadCount + 1] << 8 )
		| ( m_pData[m_nMsgReadCount + 2] << 16 )
		| ( m_pData[m_nMsgReadCount + 3] << 24 );
	m_nMsgReadCount += 4;
	return v;
}

/*
==================
CMessageBuffer::MSG_ReadFloat (0x4291D0)

The only reader with no bounds check, and the only one that goes through the
LittleLong function pointer.
==================
*/
float CMessageBuffer::MSG_ReadFloat()
{
	union { int i; float f; }	u;

	u.i = LittleLong( *(int*)( m_pData + m_nMsgReadCount ) );
	m_nMsgReadCount += 4;
	return u.f;
}

/*
==================
CMessageBuffer::MSG_ReadString (0x429220)
==================
*/
char* CMessageBuffer::MSG_ReadString()
{
	static char	string[2048];		// 0x4E6080
	int			c;
	int			l = 0;

	for ( c = MSG_ReadChar(); c != -1; c = MSG_ReadChar() )
	{
		if ( c == 0 )
			break;
		string[l++] = (char)c;
		if ( l >= sizeof( string ) - 1 )
			break;
	}
	string[l] = 0;

	return string;
}

/*
==================
CMessageBuffer::SZ_Alloc (0x429270)
==================
*/
void* CMessageBuffer::SZ_Alloc( int cb )
{
	if ( cb < 256 )
		cb = 256;

	m_pData    = (BYTE*)malloc( cb );
	m_nMaxSize = cb;
	m_nCurSize = 0;
	return m_pData;
}

/*
==================
CMessageBuffer::SZ_Free (0x4292A0)
==================
*/
void CMessageBuffer::SZ_Free()
{
	if ( m_pData )
		free( m_pData );
	m_pData    = NULL;
	m_nCurSize = 0;
}

/*
==================
CMessageBuffer::SZ_Clear (0x4292C0)
==================
*/
void CMessageBuffer::SZ_Clear()
{
	m_nCurSize = 0;
	memset( m_pData, 0, m_nMaxSize );
}

/*
==================
CMessageBuffer::SZ_GetSpace (0x4292F0)

Both overflow paths are fatal: one for a buffer that may not grow, one for a
request larger than the buffer will ever be.
==================
*/
void* CMessageBuffer::SZ_GetSpace( int cb )
{
	void*	pWrite;

	if ( m_nCurSize + cb > m_nMaxSize )
	{
		if ( !m_bAllowOverflow )
		{
			Launcher_ShowMessageById( 0, IDS_MSG_OVERFLOW );
			exit( 1 );
		}
		if ( cb > m_nMaxSize )
		{
			Launcher_ShowMessageById( 0, IDS_MSG_REQTOOBIG );
			exit( 1 );
		}
		m_bOverFlowed = TRUE;
		SZ_Clear();
	}

	pWrite = m_pData + m_nCurSize;
	m_nCurSize += cb;
	return pWrite;
}

/*
==================
CMessageBuffer::SZ_Write (0x429360)
==================
*/
int CMessageBuffer::SZ_Write( const void* pData, unsigned cb )
{
	void*	p = SZ_GetSpace( (int)cb );

	memcpy( p, pData, cb );
	return cb;
}
