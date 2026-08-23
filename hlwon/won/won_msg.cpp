// won_msg.cpp -- CWONMsg.

#include <windows.h>
#include "won_msg.h"

// CWONMsg::CWONMsg (0x45AB30)
CWONMsg::CWONMsg()
{
	m_pchData  = NULL;
	m_iReadPos = 0;
	m_iEndPos  = 0;
}

// CWONMsg::CWONMsg(data,end) (0x45AB50)
CWONMsg::CWONMsg( const BYTE* pchData, int iEnd )
{
	m_pchData  = pchData;
	m_iReadPos = 0;
	m_iEndPos  = iEnd;
}

// CWONMsg::SetBuffer (0x45AB60)
void CWONMsg::SetBuffer( const BYTE* pchData, int iEnd )
{
	m_pchData  = pchData;
	m_iReadPos = 0;
	m_iEndPos  = iEnd;
}

// CWONMsg::ReadByte (0x45ABE0) -- fail when pos == end
BOOL CWONMsg::ReadByte( BYTE* pbOut )
{
	if ( m_iEndPos == m_iReadPos )
		return FALSE;
	*pbOut = m_pchData[m_iReadPos];
	++m_iReadPos;
	return TRUE;
}

// CWONMsg::ReadShort (0x45ABB0) -- unsigned 16-bit, little-endian
BOOL CWONMsg::ReadShort( WORD* pwOut )
{
	if ( (unsigned int)( m_iEndPos - m_iReadPos ) < 2 )
		return FALSE;
	*pwOut = *(const WORD*)( m_pchData + m_iReadPos );
	m_iReadPos += 2;
	return TRUE;
}

// CWONMsg::ReadLong (0x45AB80) -- 32-bit, little-endian
BOOL CWONMsg::ReadLong( DWORD* pdwOut )
{
	if ( (unsigned int)( m_iEndPos - m_iReadPos ) < 4 )
		return FALSE;
	*pdwOut = *(const DWORD*)( m_pchData + m_iReadPos );
	m_iReadPos += 4;
	return TRUE;
}

// CWONMsg::ReadWString (0x45AC10)
BOOL CWONMsg::ReadWString( wchar_t* pwszOut, int cchMax )
{
	WORD	wCount;
	if ( !ReadShort( &wCount ) )
		return FALSE;

	int	cbNeeded = 2 * (int)wCount;
	if ( (unsigned int)( m_iEndPos - m_iReadPos ) < (unsigned int)cbNeeded )
		return FALSE;

	const wchar_t*	pSrc = (const wchar_t*)( m_pchData + m_iReadPos );
	int	cCopy = wCount;
	if ( cCopy > cchMax - 1 )
		cCopy = cchMax - 1;
	for ( int i = 0; i < cCopy; i++ )
		pwszOut[i] = pSrc[i];
	pwszOut[cCopy] = 0;

	m_iReadPos += cbNeeded;
	return TRUE;
}

// CWONMsg::ReadRemaining (0x45AD10)
const BYTE* CWONMsg::ReadRemaining( int* piLenOut )
{
	*piLenOut = m_iEndPos - m_iReadPos;
	const BYTE*	pchRet = m_pchData + m_iReadPos;
	m_iReadPos = m_iEndPos;
	return pchRet;
}
