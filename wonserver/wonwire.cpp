// wonwire.cpp -- server-side helpers over the WON WriteBuffer/ReadBuffer classes.

#include <windows.h>
#include "wonwire.h"

std::wstring WonWide( const char* psz )
{
	std::wstring	ws;

	for ( const char* p = psz; p && *p; ++p )
		ws += (wchar_t)(unsigned char)*p;

	return ws;
}

// WonWire_ReadJoinNick -- see wonwire.h.
void WonWire_ReadJoinNick( const unsigned char* pPayload, int cbPayload,
						   char* pszOut, int cbOut )
{
	char password[256];
	WonWire_ReadJoin( pPayload, cbPayload, pszOut, cbOut, password, sizeof( password ) );
}

void WonWire_ReadJoin( const unsigned char* pPayload, int cbPayload,
						char* pszNick, int cbNick, char* pszPassword, int cbPassword )
{
	int nChars;
	int offset;
	int nPassword;

	if ( pszPassword && cbPassword > 0 )
		pszPassword[0] = 0;
	if ( !pszNick || cbNick <= 0 )
		return;
	pszNick[0] = 0;
	if ( !pPayload || cbPayload < 2 )
		return;

	nChars = pPayload[0] | ( pPayload[1] << 8 );
	if ( nChars <= 0 || 2 + 2 * nChars > cbPayload )
		return;
	if ( nChars > cbNick - 1 )
		nChars = cbNick - 1;

	WideCharToMultiByte( CP_ACP, 0, (const wchar_t*)( pPayload + 2 ), nChars,
						 pszNick, cbNick - 1, NULL, NULL );
	pszNick[nChars] = 0;

	offset = 2 + ( pPayload[0] | ( pPayload[1] << 8 ) ) * 2;
	if ( !pszPassword || cbPassword <= 0 || offset + 3 > cbPayload )
		return;

	++offset;
	nPassword = pPayload[offset] | ( pPayload[offset + 1] << 8 );
	offset += 2;
	if ( nPassword < 0 || offset + nPassword > cbPayload )
		return;
	if ( nPassword > cbPassword - 1 )
		nPassword = cbPassword - 1;
	memcpy( pszPassword, pPayload + offset, nPassword );
	pszPassword[nPassword] = 0;
}
