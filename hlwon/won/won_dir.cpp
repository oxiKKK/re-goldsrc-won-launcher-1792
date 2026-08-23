// won_dir.cpp -- WONComm_GetDirectory, the WON directory request driver

#include <windows.h>
#include "won_msg.h"
#include "won_dir.h"
#include "TitanRequest.h"
#include "WriteBuffer.h"

extern void	Console_Printf( char* fmt, ... );	// launcher logger

// WONComm_GetDirectory (0x409390)
int WONComm_GetDirectory( TitanRequest* pRequest, const std::wstring& wsDir, CWONMsg* pReply )
{
	WriteBuffer	wb( 0x100 );

	wb.appendLong( 0 );			// length placeholder @ offset 0
	wb.appendLong( 30 );		// service type
	wb.appendLong( 2 );			// message type = request
	wb.appendWString( wsDir );
	wb.appendByte( 0 );			// trailing flag

	wb.setLong( 0, wb.getSize() );	// back-patch length

	if ( !pRequest->request( &wb, 30, 3, pReply, 0 ) )
		return 0;	// transport failure (no message)

	WORD	wStatus = 0, wCount = 0;
	BOOL	bOk = pReply->ReadShort( &wStatus );
	bOk &= pReply->ReadShort( &wCount );
	if ( !bOk )
	{
		Console_Printf( "Invalid message received from directory server.\n" );
		return 0;
	}

	if ( wStatus != 0 )
	{
		Console_Printf( "Status error getting directory: %d\n", wStatus );
		return 0;
	}

	return (int)wCount;
}

/*
==================
WON_ToWideString (0x437780)

Widens one char at a time, which is what the launcher's own COMDAT does -- not
the SDK's StringToWString, which allocates and calls AsciiToWide.
==================
*/
std::wstring WON_ToWideString( const std::string& s )
{
	std::wstring	ws;

	for ( size_t i = 0; i < s.size(); i++ )
		ws += (wchar_t)(unsigned char)s[i];

	return ws;
}
