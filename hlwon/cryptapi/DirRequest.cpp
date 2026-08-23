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
// Purpose: DirRequest, the WON directory transaction, and the reply-record
//          parser that walks its payload.
//
// $NoKeywords: $
//=============================================================================

#include <windows.h>

#include "DirRequest.h"
#include "ReadBuffer.h"
#include "WriteBuffer.h"
#include "TitanRequest.h"

extern void	Console_Printf( char* fmt, ... );

// DirRequest::getDirectory (0x409390)
int DirRequest::getDirectory( const std::wstring& wsDir, ReadBuffer* pReply )
{
	WriteBuffer		wb( 0x100 );
	unsigned short	uStatus = 0, uCount = 0;
	BOOL			bOk;

	wb.appendLong( 0 );			// length placeholder @ offset 0
	wb.appendLong( 30 );		// service type
	wb.appendLong( 2 );			// message type = request
	wb.appendWString( wsDir );
	wb.appendByte( 0 );			// trailing flag

	wb.setLong( 0, wb.getSize() );	// back-patch length

	if ( !request( &wb, 30, 3, pReply, 0 ) )
		return 0;	// transport failure (no message)

	bOk  = pReply->readUShort( &uStatus );
	bOk &= pReply->readUShort( &uCount );
	if ( !bOk )
	{
		Console_Printf( "Invalid message received from directory server.\n" );
		return 0;
	}

	if ( uStatus != 0 )
	{
		Console_Printf( "Status error getting directory: %d\n", uStatus );
		return 0;
	}

	return (int)uCount;
}

// WON_ParseDirReply (0x4095E0)
BOOL WON_ParseDirReply( ReadBuffer* pMsg, direntry_t* pOut )
{
	BOOL			bOk = pMsg->readUByte( &pOut->m_type );
	unsigned long	ulRest;

	bOk &= pMsg->readWString( pOut->m_wsField04, 256 );
	bOk &= pMsg->readWString( pOut->m_wsField14, 256 );
	bOk &= pMsg->readWString( pOut->m_wsName,    256 );	// +0x24 -- the displayed name
	bOk &= pMsg->readULong( &pOut->m_field6C );
	bOk &= pMsg->readULong( &pOut->m_field70 );

	if ( pOut->m_type == 68 )	// 'D' -- leaf/dir entry: one trailing byte
		return pMsg->readUByte( &pOut->m_extraD ) & bOk;

	bOk &= pMsg->readWString( pOut->m_wsField34, 256 );
	bOk &= pMsg->readWString( pOut->m_wsField44, 256 );
	bOk &= pMsg->readWString( pOut->m_wsField54, 256 );

	// 0x409684 reads this short into the (reused) parameter slot, so the value is
	// discarded; only the second short survives, as the port.
	unsigned short	uDiscarded = 0;
	bOk &= pMsg->readUShort( &uDiscarded );

	bOk &= pMsg->readUShort( &pOut->m_port );	// +0x68 -- the port FetchRoomList uses
	bOk &= pMsg->readULong( &pOut->m_addr );	// +0x64 -- the address it pairs with
	bOk &= pMsg->readUShort( &pOut->m_cbData );

	pOut->m_pData = pMsg->getDataPtr();		// blob ptr
	ulRest = pOut->m_cbData;
	return pMsg->skipBytes( ulRest ) & bOk;	// consume the blob
}
