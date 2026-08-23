// serverlist

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>

typedef struct authserver_s
{
	authserver_s *next;
	char address[64];
	unsigned short port;
} authserver_t;

extern char g_szBaseDir[256];

static char g_ParseToken[1024];

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int Crypt_ResolveStringAddress( const char *pszAddress, void *pSockAddr )
{
	struct sockaddr_in *pAddr;
	char szAddress[1024];
	char *pch;
	int iPort;

	pAddr = (struct sockaddr_in *)pSockAddr;
	memset( pAddr, 0, sizeof( *pAddr ) );
	pAddr->sin_family = AF_INET;

	strcpy( szAddress, pszAddress );

	for ( pch = szAddress; *pch; ++pch )
	{
		if ( *pch == ':' )
		{
			*pch = 0;
			iPort = atoi( pch + 1 );
			pAddr->sin_port = htons( iPort );
		}
	}

	if ( szAddress[0] >= '0' && szAddress[0] <= '9' && strstr( szAddress, "." ) )
	{
		pAddr->sin_addr.s_addr = inet_addr( szAddress );
		return 1;
	}

	hostent *pHost = gethostbyname( szAddress );
	if ( pHost )
	{
		pAddr->sin_addr = **(in_addr **)pHost->h_addr_list;
		return 1;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
char *Crypt_Parse( char *data )
{
	int c;
	int len;

	len = 0;
	g_ParseToken[0] = 0;

	if ( !data )
		return NULL;

	c = *data;
	while ( 1 )
	{
		if ( c <= ' ' )
		{
			while ( c )
			{
				c = *++data;
				if ( c > ' ' )
					goto parse_token;
			}

			return NULL;
		}

parse_token:
		if ( c != '/' )
			break;
		if ( data[1] != '/' )
			goto copy_token_char;

		if ( *data )
		{
			do
			{
				if ( c == '\n' )
					break;
				c = *++data;
			}
			while ( c );
		}
	}

	if ( c == '"' )
	{
		++data;
		while ( 1 )
		{
			c = *data++;
			if ( c == '"' || !c )
				break;
			g_ParseToken[len++] = (char)c;
		}

		g_ParseToken[len] = 0;
		return data;
	}

	if ( c != '{' && c != '}' && c != ')' && c != '(' && c != '\'' && c != ':' )
	{
		do
		{
copy_token_char:
			++data;
			g_ParseToken[len++] = (char)c;
			c = *data;
		}
		while ( c != '{' && c != '}' && c != ')' && c != '(' && c != '\'' && c != ':' && c > ' ' );

		g_ParseToken[len] = 0;
		return data;
	}

	g_ParseToken[0] = (char)c;
	g_ParseToken[1] = 0;
	return ++data;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void *Crypt_ParseServers( const char *pszFileName )
{
	authserver_t *pList;
	char szPath[1024];
	char szAddress[1024];
	char szAddrPort[1024];
	char *data;
	char *next;
	FILE *fp;
	long nFileSize;
	int skipSection;
	int iPort;
	int iDefaultPort;
	struct sockaddr_in addr;
	char *pMem;

	pList = NULL;
	sprintf( szPath, "%s/%s", g_szBaseDir, pszFileName );
	fp = fopen( szPath, "rb" );
	if ( !fp )
	{
		sprintf( szPath, "valve/%s", pszFileName );
		fp = fopen( szPath, "rb" );
		if ( !fp )
			return NULL;
	}

	fseek( fp, 0, 2 );
	nFileSize = ftell( fp );
	fseek( fp, 0, 0 );
	if ( nFileSize <= 0 || nFileSize > 0x4000 )
	{
		fclose( fp );
		return NULL;
	}

	pMem = (char *)malloc( nFileSize + 1 );
	if ( !pMem )
	{
		fclose( fp );
		return NULL;
	}

	fread( pMem, nFileSize, 1, fp );
	fclose( fp );
	pMem[nFileSize] = 0;

	data = pMem;
	while ( 1 )
	{
		next = Crypt_Parse( data );
		if ( !strlen( g_ParseToken ) )
			break;

		skipSection = 1;
		if ( !_strcmpi( g_ParseToken, "Auth" ) )
		{
			iDefaultPort = 7001;
			skipSection = 0;
		}

		data = Crypt_Parse( next );
		if ( !strlen( g_ParseToken ) || _strcmpi( g_ParseToken, "{" ) )
			break;

		while ( 1 )
		{
			data = Crypt_Parse( data );
			if ( !strlen( g_ParseToken ) )
				break;
			if ( !_strcmpi( g_ParseToken, "}" ) )
				break;

			sprintf( szAddress, "%s", g_ParseToken );
			data = Crypt_Parse( data );
			if ( !strlen( g_ParseToken ) )
				break;
			if ( _strcmpi( g_ParseToken, ":" ) )
				break;

			data = Crypt_Parse( data );
			if ( !strlen( g_ParseToken ) )
				break;

			iPort = atoi( g_ParseToken );
			sprintf( szAddrPort, "%s:%i", szAddress, iPort );

			memset( &addr, 0, sizeof( addr ) );
			if ( Crypt_ResolveStringAddress( szAddrPort, &addr ) )
			{
				iPort = ntohs( addr.sin_port );
				if ( !iPort )
					iPort = iDefaultPort;
				sprintf( szAddress, "%s", inet_ntoa( addr.sin_addr ) );
			}

			if ( !skipSection )
			{
				authserver_t *pServer;

				pServer = new authserver_t;
				strcpy( pServer->address, szAddress );
				pServer->port = (unsigned short)iPort;
				pServer->next = pList;
				pList = pServer;
			}
		}
	}

	free( pMem );
	return pList;
}
