#ifndef WON_DIR_H
#define WON_DIR_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>
#include <string>

class CWONMsg;
class TitanRequest;

// The room directory FetchRoomList walks (0x4d0a38).
#define WON_DIR_PUBLIC	L"/Half-Life/Public"

// Directory record types.  The WON SDK's own names, from
// TitanApi/msg/Dir/DirEntity.h.
#define ET_DIRECTORY	'D'
#define ET_SERVICE		'S'

// The room servers answer a small UDP status protocol on their own port, used by
// the room-occupancy probe and the player search.  Every message is a fixed 03 01
// header, then one of these opcodes, then a short room index.  It is neither one
// of Valve's proto_oob queries nor a WON Titan message, and the WON SDK does not
// name it, so these names are ours -- shared so the launcher and wonserver cannot
// drift apart on the wire.
#define ROOMQ_HEADER0				3
#define ROOMQ_HEADER1				1
#define ROOMQ_PLAYERCOUNT_REQUEST	1	// + short room index
#define ROOMQ_PLAYERCOUNT_REPLY		2	// + short room index + short players
#define ROOMQ_FINDPLAYER_REQUEST		3	// + short room index + byte 0 + wide nick
#define ROOMQ_FINDPLAYER_REPLY		4	// + short room index

// The directory LaunchChatServer lists to find the factory servers (0x4D0AD8).
// Distinct from WON_DIR_PUBLIC (0x4D0A38), which it then hands to the factory.
#define WON_DIR_HALFLIFE	L"/Half-Life"

// One parsed WON directory entry (mirrors the struct in won_dirparse.cpp;
// offsets from sub_4095E0) (sub_4095E0)
struct direntry_t
{
	BYTE		m_type;			// +0    record type ('S' = service/room leaf, 'D' = dir)
	wchar_t		m_wsField04[256];	// +4   first wstring
	wchar_t		m_wsField14[256];	// +0x14 second wstring
	wchar_t		m_wsName[256];		// +0x24 third wstring -- the displayed name
	wchar_t		m_wsField34[256];	// +0x34 (non-'D')
	wchar_t		m_wsField44[256];	// +0x44 (non-'D')
	wchar_t		m_wsField54[256];	// +0x54 (non-'D')
	WORD		m_port;			// +0x68 network-order port (the SECOND short)
	DWORD		m_addr;			// +0x64 network-order IPv4 (struct in_addr)
	DWORD		m_field6C;
	DWORD		m_field70;
	WORD		m_cbData;
	const BYTE*	m_pData;
	BYTE		m_extraD;
};

// Build a WON directory request (svc 30 / msg 2), run the transaction, and return
// the directory entry count (0 on any failure).
int		WONComm_GetDirectory( TitanRequest* pRequest, const std::wstring& wsDir, CWONMsg* pReply );	// 0x409390

// Parse one directory reply record off pMsg into pOut.  TRUE iff every read ok.
BOOL	WON_ParseDirReply( CWONMsg* pMsg, direntry_t* pOut );	// 0x4095E0

// Build a WON factory "start process" request (svc 10 / msg 2) for a chat/game
// server and run the transaction; returns the spawned server's port, or 0 on
// failure.
int		WONComm_StartProcess( TitanRequest* pRequest,				// 0x40F330
							  const std::string& sServerName,	// "HLChatServ"
							  const std::string& sDirAddr,		// the directory, "host:port"
							  const std::wstring& wsDirectory,	// WON_DIR_PUBLIC
							  const std::wstring& wsArgs,		// the room's public args
							  const std::string& sPassword );	// "-password <pw>", or ""

// 0x437780 -- widens one char at a time (ws += s[i]).  Not the SDK's
// StringToWString, which allocates and calls AsciiToWide.
std::wstring	WON_ToWideString( const std::string& s );

#endif // WON_DIR_H
