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
// Purpose: declares DirRequest, the WON directory transaction, and the reply
//          record it parses.
//
// $NoKeywords: $
//=============================================================================

#ifndef __WON_DIRREQUEST_H__
#define __WON_DIRREQUEST_H__

#include <windows.h>
#include <string>

#include "TitanRequest.h"

class ReadBuffer;

// The room directory FetchRoomList walks (0x4d0a38).
#define WON_DIR_PUBLIC	L"/Half-Life/Public"

// One parsed WON directory entry; offsets from 0x4095E0.
struct direntry_t
{
	BYTE		m_type;				// +0    record type ('S' = service/room leaf, 'D' = dir)
	wchar_t		m_wsField04[256];	// +4    first wstring
	wchar_t		m_wsField14[256];	// +0x14 second wstring
	wchar_t		m_wsName[256];		// +0x24 third wstring -- the displayed name
	wchar_t		m_wsField34[256];	// +0x34 (non-'D')
	wchar_t		m_wsField44[256];	// +0x44 (non-'D')
	wchar_t		m_wsField54[256];	// +0x54 (non-'D')
	WORD		m_port;				// +0x68 network-order port (the SECOND short)
	DWORD		m_addr;				// +0x64 network-order IPv4 (struct in_addr)
	DWORD		m_field6C;
	DWORD		m_field70;
	WORD		m_cbData;
	const BYTE*	m_pData;
	BYTE		m_extraD;
};

// vftable 0x4ADB10 -> 0x4AD7E8; the directory service's TitanRequest.
class DirRequest : public TitanRequest
{
public:
	DirRequest( const char* pszAddr, int nPort )
		: TitanRequest( pszAddr ? pszAddr : "", nPort )
	{
	}

	// Build the svc 30 / msg 2 request, run the transaction, and return the
	// directory entry count (0 on any failure).
	int	getDirectory( const std::wstring& wsDir, ReadBuffer* pReply );	// 0x409390
};

// Parse one directory reply record off pMsg into pOut.  TRUE iff every read ok.
BOOL	WON_ParseDirReply( ReadBuffer* pMsg, direntry_t* pOut );			// 0x4095E0

#endif // __WON_DIRREQUEST_H__
