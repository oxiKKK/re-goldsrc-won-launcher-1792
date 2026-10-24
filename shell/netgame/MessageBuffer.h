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
// Purpose: declares CMessageBuffer, the launcher's serialize / network-message
//          IO buffer.
//
// $NoKeywords: $
//=============================================================================

#ifndef MESSAGEBUFFER_H
#define MESSAGEBUFFER_H
#ifdef _WIN32
#pragma once
#endif

// The engine's SZ_/MSG_ primitives wrapped as one object; sizeof 0x20.
class CMessageBuffer
{
public:
	CMessageBuffer( int cbInitial );
	virtual ~CMessageBuffer();

	void*	SZ_Alloc( int cb );
	void	SZ_Free();
	void	SZ_Clear();
	void	MSG_BeginReading();

	void*	SZ_GetSpace( int cb );
	int		SZ_Write( const void* pData, unsigned cb );
	void*	MSG_WriteChar( char c );
	void*	MSG_WriteLong( int n );
	unsigned MSG_WriteString( const char* psz );

	// CMessageBuffer::SetCurSize (0x4293A0)
	void	SetCurSize( int n )		{ m_nCurSize = n; }
	// CMessageBuffer::GetCurSize (0x4293B0)
	int		GetCurSize() const		{ return m_nCurSize; }
	int		GetReadCount() const	{ return m_nMsgReadCount; }
	int		GetMaxSize() const		{ return m_nMaxSize; }
	// CMessageBuffer::GetData (0x429390)
	void*	GetData() const			{ return m_pData; }

	int		MSG_ReadChar();
	int		MSG_ReadByte();
	int		MSG_ReadShort();
	int		MSG_ReadLong();
	float	MSG_ReadFloat();
	char*	MSG_ReadString();

protected:
	// +0    vftable  (virtual ~CMessageBuffer)
	int		m_nMsgReadCount;	// +4   read cursor
	BOOL	m_bMsgBadRead;		// +8   set when a read runs past m_nCurSize
	BYTE*	m_pData;			// +12  malloc'd working buffer
	int		m_nMaxSize;			// +16  capacity (>=256; callers pass 0x2000)
	int		m_nCurSize;			// +20  bytes written so far (write cursor)
	BOOL	m_bAllowOverflow;	// +24  ctor sets TRUE; gates fatal-vs-wrap on overflow
	BOOL	m_bOverFlowed;		// +28  set when a write wrapped the buffer
};

#endif // MESSAGEBUFFER_H
