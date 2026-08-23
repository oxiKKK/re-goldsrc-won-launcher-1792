// won_dirparse.cpp -- WON_ParseDirReply, the directory/room reply-record parser

#include <windows.h>
#include "won_msg.h"
#include "won_dir.h"

// WON_ParseDirReply (0x4095E0)
BOOL WON_ParseDirReply( CWONMsg* pMsg, direntry_t* pOut )
{
	BOOL	bOk = pMsg->ReadByte( &pOut->m_type );
	bOk &= pMsg->ReadWString( pOut->m_wsField04, 256 );
	bOk &= pMsg->ReadWString( pOut->m_wsField14, 256 );
	bOk &= pMsg->ReadWString( pOut->m_wsName,    256 );	// +0x24 -- the displayed name
	bOk &= pMsg->ReadLong( &pOut->m_field6C );
	bOk &= pMsg->ReadLong( &pOut->m_field70 );

	if ( pOut->m_type == 68 )	// 'D' -- leaf/dir entry: one trailing byte
		return pMsg->ReadByte( &pOut->m_extraD ) & bOk;

	bOk &= pMsg->ReadWString( pOut->m_wsField34, 256 );
	bOk &= pMsg->ReadWString( pOut->m_wsField44, 256 );
	bOk &= pMsg->ReadWString( pOut->m_wsField54, 256 );
	{
		// 0x409684 reads this short into the (reused) parameter slot, so the value
		// is discarded; only the second short survives, as the port.
		WORD	wDiscarded = 0;
		bOk &= pMsg->ReadShort( &wDiscarded );
	}
	bOk &= pMsg->ReadShort( &pOut->m_port );	// +0x68 -- the port FetchRoomList uses
	bOk &= pMsg->ReadLong( &pOut->m_addr );		// +0x64 -- the address it pairs with
	bOk &= pMsg->ReadShort( &pOut->m_cbData );

	pOut->m_pData = pMsg->GetCurrent();		// blob ptr
	pMsg->Skip( pOut->m_cbData );			// consume the blob
	return bOk;
}
