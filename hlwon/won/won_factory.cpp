// won_factory.cpp -- WONComm_StartProcess, the WON factory "start process" driver

#include <windows.h>
#include "won_msg.h"
#include "won_dir.h"
#include "TitanRequest.h"
#include "WriteBuffer.h"
#include "ReadBuffer.h"

extern void	Console_Printf( char* fmt, ... );	// launcher logger

// WONComm_StartProcess (0x40F330)
int WONComm_StartProcess( TitanRequest* pRequest,
						  const std::string& sServerName,
						  const std::string& sArgs,
						  const std::wstring& wsDataA,
						  const std::wstring& wsDataB,
						  const std::string& sAddr )
{
	WriteBuffer		wb( 0x100 );
	ReadBuffer		reply;
	unsigned short	uStatus = 0, uPort = 0;
	unsigned char	uResultType = 0;
	BOOL			bOk, bPortOk;

	wb.appendLong( 0 );			// length placeholder @ offset 0
	wb.appendLong( 10 );		// service type = factory
	wb.appendLong( 2 );			// message type = start-process request
	wb.appendString( sServerName );
	wb.appendByte( 1 );
	wb.appendString( sAddr );
	wb.appendLong( 232 );		// 0xE8
	wb.appendString( sArgs );
	wb.appendWString( wsDataB );
	wb.appendWString( wsDataA );
	wb.appendByte( 1 );
	wb.appendByte( 1 );
	wb.appendByte( 0 );
	wb.appendShort( 0 );

	wb.setLong( 0, wb.getSize() );	// back-patch length

	if ( !pRequest->request( &wb, 10, 1, &reply, 0 ) )
		return 0;	// transport failure (no message)

	bOk = reply.readUShort( &uStatus );
	if ( bOk && (short)uStatus < 0 )
	{
		Console_Printf( "Status error in start process: %d\n", (short)uStatus );
		return 0;
	}

	bOk &= reply.readUByte( &uResultType );
	bPortOk = reply.readUShort( &uPort ) & bOk;

	if ( bPortOk && uResultType == 1 )
	{
		if ( (short)uStatus == 2 )		// success
			return (int)uPort;

		Console_Printf( "Factory error: %d\n", (short)uStatus );
	}
	else
	{
		Console_Printf( "Invalid message received from factory.\n" );
	}

	return 0;
}
