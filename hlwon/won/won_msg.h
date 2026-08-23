#ifndef WON_MSG_H
#define WON_MSG_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>

class CWONMsg
{
public:
	CWONMsg();									// 0x45AB30
	CWONMsg( const BYTE* pchData, int iEnd );	// 0x45AB50 (buffer ptr, end offset)

	BOOL	ReadByte( BYTE* pbOut );						// 0x45ABE0
	BOOL	ReadShort( WORD* pwOut );						// 0x45ABB0 (unsigned 16, LE)
	BOOL	ReadLong( DWORD* pdwOut );						// 0x45AB80 (32, LE)
	BOOL	ReadWString( wchar_t* pwszOut, int cchMax );	// 0x45AC10 (u16 count + UTF-16)
	const BYTE*	ReadRemaining( int* piLenOut );				// 0x45AD10 (ptr + count, consumes)
	const BYTE*	GetCurrent() const { return m_pchData + m_iReadPos; }	// 0x45AD60
	void	Skip( int cb ) { m_iReadPos += cb; }				// 0x45AD30

	// Re-point this message at a fresh payload span.
	void	SetBuffer( const BYTE* pchData, int iEnd );		// 0x45AB60

private:
	const BYTE*	m_pchData;	// +0
	int			m_iReadPos;	// +4
	int			m_iEndPos;	// +8 (absolute end offset)
};

#endif // WON_MSG_H
