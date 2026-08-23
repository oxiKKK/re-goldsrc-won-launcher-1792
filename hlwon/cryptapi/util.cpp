// util

#include <stdlib.h>
#include <time.h>

typedef struct authserver_s
{
	authserver_s *next;
	char address[64];
	unsigned short port;
} authserver_t;

extern char g_szServerFile[256];

void *Crypt_ParseServers( const char *pszFileName );

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void *ReadAuthList( void )
{
	authserver_t *pList;
	authserver_t *pCurrent;
	authserver_t **ppList;
	int count;
	int i;
	int j;
	int k;
	authserver_t *pTemp;

	pList = (authserver_t *)Crypt_ParseServers( g_szServerFile );
	if ( !pList )
		return NULL;

	srand( clock() );
	srand( clock() );
	srand( clock() );

	pCurrent = pList;
	count = 0;
	do
	{
		pCurrent = pCurrent->next;
		++count;
	}
	while ( pCurrent );

	if ( count != 1 )
	{
		ppList = new authserver_t *[count];
		pCurrent = pList;
		i = 0;
		do
		{
			ppList[i] = pCurrent;
			pCurrent = pCurrent->next;
			++i;
		}
		while ( pCurrent );

		k = 100;
		do
		{
			i = rand() % count;
			j = rand() % count;
			pTemp = ppList[i];
			ppList[i] = ppList[j];
			ppList[j] = pTemp;
			--k;
		}
		while ( k );

		for ( i = 0; i < count - 1; ++i )
			ppList[i]->next = ppList[i + 1];

		ppList[count - 1]->next = NULL;
		pList = ppList[0];
		delete[] ppList;
	}

	return pList;
}
