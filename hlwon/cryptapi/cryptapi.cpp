// cryptapi.c -- WON cryptographics functions

#include <time.h>

#include "cryptapi.h"
#include "WON_CryptKeyBase.h"
#include "WON_BFSymmetricKey.h"
#include "WON_CryptFactory.h"
#include "WON_EGPublicKey.h"
#include "WON_EGPrivateKey.h"
#include "WON_AuthFamilyBuffer.h"
#include "WON_AuthCertificate1.h"
#include "WON_AuthFactory.h"
#include "AuthRequest.h"
#include "ReadBuffer.h"
#include "WriteBuffer.h"
#include "resource.h"
#include "crc.h"
#include "msg/Auth/TMsgTypesAuth.h"

void *ReadAuthList( void );

qboolean	g_authIsServer;
AuthRequest*	gAuthRequest;
WON_BFSymmetricKey* gConnectionKey;

typedef struct authserver_s
{
	authserver_s* next;
	char address[64];
	unsigned short port;
} authserver_t;

static char g_szGUID[64];		// (4FD7A0)
char g_szServerFile[256];
char g_szBaseDir[256];
char g_szExeName[256];

#define AUTH_REFRESH_THRESHOLD 15.0
#define AUTH_KEY_LEN 8

// callbacks
void		(*Callback_Printf)( char*, ... );
void		(*Callback_AuthFailure)( void );
char*		(*Callback_GetLocalizedString)( unsigned int );

void DeleteAuthObject( void );

static void Crypt_SetAuthData( authchallenge_s* challenge, WriteBuffer* buffer )
{
	if (challenge->data)
		free(challenge->data);

	challenge->data = malloc(buffer->getSize() + 1);
	memcpy(challenge->data, buffer->getBuffer(), buffer->getSize());
	challenge->dataSize = buffer->getSize();
}

char* STUB_Callback_GetLocalizedString( unsigned int code )
{
	static char net_corrupt[] = "Your executable is out of date.";
	static char cdkey_bad[] = "Your Half-Life CD Key is invalid.";
	static char auth_failure[] = "Unable to authenticate with WON servers.";
	static char cdkey_in_use[] = "Your Half-Life CD Key is currently in use.  Please try again later.";
	static char banned[] = "Your CD key cannot be used on the WON.net system.";
	static char auth_error[] = "Auth Error";

	switch (code)
	{
	case IDS_NET_CORRUPT:
		return net_corrupt;
	case IDS_CDKEY_BAD:
		return cdkey_bad;
	case IDS_WON_AUTHFAILURE:
		return auth_failure;
	case IDS_WON_CDINUSE:
		return cdkey_in_use;
	case IDS_WON_BANNED:
		return banned;
	default:
		return auth_error;
	}
}

void Crypt_Initialize( crypt_parms_t* parms )
{
	g_authIsServer = parms->authType == CRYPT_AUTHTYPE_SERVER ? TRUE : FALSE;

	strcpy(g_szGUID, parms->pszGUID);
	strcpy(g_szServerFile, parms->pszServerFile);
	strcpy(g_szBaseDir, parms->pszBaseDir);
	strcpy(g_szExeName, parms->pszExeName);

	Callback_Printf = parms->pfnPrintf;
	Callback_AuthFailure = parms->pfnAuthFailure;

	Callback_GetLocalizedString = parms->pfnGetLocalizedString;
	if (!Callback_GetLocalizedString)
		Callback_GetLocalizedString = STUB_Callback_GetLocalizedString;
}

void Crypt_Shutdown( void )
{
	DeleteAuthObject();
}

int Crypt_MD5_File( unsigned char* hash, char* filename )
{
	return MD5_Hash_File(hash, filename, FALSE, FALSE);
}

// Crypt_ReturnAPI
extern crypt_api_t cryptapi;
qboolean Crypt_ReturnAPI( int version, crypt_api_t* api )
{
	if (version != CRYPT_API_VERSION)
		return FALSE;

	memcpy(api, &cryptapi, sizeof(cryptapi));
	return TRUE;
}

int Crypt_CreateKey( int keyType, const unsigned char* keyData, void** keyHandle )
{
	WON_BFSymmetricKey* result;

	if (!keyHandle)
		return 0;

	result = WON_CryptFactory::NewBFSymmetricKey((unsigned short)keyType, keyData);
	if (!result)
		return 0;

	*keyHandle = result;
	return 1;
}

void KillSessionKey( void** keyHandle )
{
	if (!keyHandle || !*keyHandle)
		return;

	WON_CryptFactory::DeleteBFSymmetricKey((WON_BFSymmetricKey*)*keyHandle);
	*keyHandle = NULL;
}

int Crypt_Encrypt( const void* key, const void* input, int inputSize, void* output, int* outputSize )
{
	WON_CryptKeyBase::CryptReturn cryptResult;

	if (!key)
		return 0;

	cryptResult = ((WON_BFSymmetricKey*)key)->Encrypt(input, inputSize);
	if (!cryptResult.GetData())
		return 0;

	memcpy(output, cryptResult.GetData(), cryptResult.GetLen());
	*outputSize = cryptResult.GetLen();
	return 1;
}

int Crypt_Decrypt( const void* key, const void* input, int inputSize, void* output, int* outputSize )
{
	WON_CryptKeyBase::CryptReturn cryptResult;

	if (!key)
		return 0;

	cryptResult = ((WON_BFSymmetricKey*)key)->Decrypt((const unsigned char*)input, inputSize);
	if (!cryptResult.GetData())
		return 0;

	memcpy(output, cryptResult.GetData(), cryptResult.GetLen());
	*outputSize = cryptResult.GetLen();
	return 1;
}

void DeleteAuthObject( void )
{
	if (gAuthRequest)
	{
		delete gAuthRequest;
		gAuthRequest = NULL;
	}

	if (gConnectionKey)
	{
		WON_CryptFactory::DeleteBFSymmetricKey(gConnectionKey);
		gConnectionKey = NULL;
	}
}

int Crypt_GetUserId( void )
{
	WON_AuthCertificate1* pCertificate;

	if (!gAuthRequest)
		return -1;

	pCertificate = gAuthRequest->GetCertificate();
	if (!pCertificate)
		return -1;

	return pCertificate->GetUserId();
}

int Crypt_GetCertificate( void* cert, int* size )
{
	WON_AuthCertificate1* pCertificate;
	unsigned short nLength;

	if (!gAuthRequest)
		return 0;

	pCertificate = gAuthRequest->GetCertificate();
	if (!pCertificate)
		return 1;

	nLength = pCertificate->GetRawLen();
	*size = nLength;

	if (!nLength)
		return 1;

	memcpy(cert, pCertificate->GetRaw(), nLength);
	return 2;
}

int Crypt_GetNewCertificate( void )
{
	authserver_s* pAuthList;
	authserver_s* pCurServer;
	authserver_s* pNextServer;
	int bResult;
	int nTries;

	bResult = 0;
	nTries = 2;

	pAuthList = (authserver_s*)ReadAuthList();

	while (!bResult)
	{
		pCurServer = pAuthList;
		while (pCurServer)
		{
			if (gAuthRequest)
			{
				gAuthRequest->setAddrAndPort(pCurServer->address, pCurServer->port);
			}
			else
			{
				gAuthRequest = new AuthRequest(g_szExeName, g_szGUID, pCurServer->address, pCurServer->port);
			}

			if (gAuthRequest && gAuthRequest->getAuthVerifierKey("kver.kp"))
			{
				if (gAuthRequest->getCertificate(TRUE) && gAuthRequest->GetCertificate())
				{
					bResult = TRUE;
					break;
				}

				if (gAuthRequest->ReceivedResponse())
					nTries = 0;
			}

			pCurServer = pCurServer->next;
		}

		if (--nTries < 0)
			break;
	}

	if (gAuthRequest && !bResult)
	{
		if (Callback_Printf)
			Callback_Printf("%s\n", gAuthRequest->GetLastError());

		if (Callback_AuthFailure)
			Callback_AuthFailure();
	}

	while (pAuthList)
	{
		pNextServer = pAuthList->next;
		delete pAuthList;
		pAuthList = pNextServer;
	}

	return bResult != 0;
}

void* Crypt_GetAuthObject( void )
{
	return gAuthRequest;
}

void Crypt_InstanceAuthObject( void )
{
	struct authserver_s
	{
		authserver_s* next;
		char address[64];
		unsigned short port;
	};

	authserver_s* pAuthList;
	authserver_s* pNextServer;

	if (!gAuthRequest)
	{
		pAuthList = (authserver_s*)ReadAuthList();
		if (pAuthList)
			gAuthRequest = new AuthRequest(g_szExeName, g_szGUID, pAuthList->address, pAuthList->port);

		while (pAuthList)
		{
			pNextServer = pAuthList->next;
			delete pAuthList;
			pAuthList = pNextServer;
		}
	}
}

void Crypt_DestroyAuthObject( void )
{
	if (gAuthRequest)
	{
		delete gAuthRequest;
		gAuthRequest = NULL;
	}
}

int Crypt_IsAuthenticated( void )
{
	return gAuthRequest && gAuthRequest->GetCertificate();
}

// -----------------------------------------------------
// New file ?

int Crypt_GetRawBFKey( void* key, void* buffer, int* size )
{
	WON_CryptKeyBase* cryptKey;
	unsigned short keyLen;

	if (!key)
		return 0;

	cryptKey = (WON_CryptKeyBase*)key;
	keyLen = cryptKey->GetKeyLen();
	memcpy(buffer, cryptKey->GetKey(), keyLen);
	*size = keyLen;
	return 2;
}

void Crypt_DeleteAuthData( struct authchallenge_s* challenge )
{
	if (challenge->key)
	{
		WON_CryptFactory::DeleteBFSymmetricKey((WON_BFSymmetricKey*)challenge->key);
		challenge->key = NULL;
	}

	if (challenge->data)
	{
		free(challenge->data);
		challenge->data = NULL;
	}
}

int Crypt_GetConnectionKey( void* key, int* size )
{
	unsigned short keyLen;

	if (!gConnectionKey)
		return 0;

	keyLen = gConnectionKey->GetKeyLen();
	memcpy(key, gConnectionKey->GetKey(), keyLen);
	*size = keyLen;
	return 2;
}

int Crypt_AuthRequest( void* data, int size, struct authchallenge_s* challenge )
{
	WON_AuthCertificate1* pCertificate;
	WriteBuffer buffer(0x100);

	if (!gAuthRequest)
		return 0;

	pCertificate = gAuthRequest->GetCertificate();
	if (!pCertificate)
		return 0;

	buffer.appendLong(0xFFFFFFFF);
	buffer.appendByte(WONMsg::Auth1Challenge1);
	buffer.appendShort(pCertificate->GetRawLen());
	buffer.append(pCertificate->GetRaw(), pCertificate->GetRawLen());
	Crypt_SetAuthData(challenge, &buffer);
	return 2;
}

int Crypt_AuthChallenge1( void* data, int size, struct authchallenge_s* challenge )
{
	WON_AuthCertificate1* pCertificate;
	WON_AuthCertificate1* pPeerCertificate;
	WON_EGPublicKey* pPeerKey;
	WON_BFSymmetricKey* pChallengeKey;
	WON_CryptKeyBase::CryptReturn encryptedKey;
	WriteBuffer buffer(0x100);
	const unsigned char* pData;
	unsigned short nCertLen;

	if (!gAuthRequest)
		return 0;

	pCertificate = gAuthRequest->GetCertificate();
	if (!pCertificate)
		return 0;

	pData = (const unsigned char*)data;
	nCertLen = *(unsigned short*)pData;
	pData += sizeof(unsigned short);

	pPeerCertificate = WON_AuthFactory::NewAuthCertificate1(pData, nCertLen);
	if (!pPeerCertificate)
		return 1;

	if (!pPeerCertificate->IsValid() || !gAuthRequest->verifyAuthStuff(pPeerCertificate))
	{
		WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
		return 1;
	}

	challenge->userId = pPeerCertificate->GetUserId();
	pPeerKey = WON_CryptFactory::NewEGPublicKey(pPeerCertificate->GetPubKeyLen(), pPeerCertificate->GetPubKey());
	if (!pPeerKey)
	{
		WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
		return 1;
	}

	pChallengeKey = WON_CryptFactory::NewBFSymmetricKey(AUTH_KEY_LEN, NULL);
	if (!pChallengeKey)
	{
		WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
		WON_CryptFactory::DeleteEGPublicKey(pPeerKey);
		return 1;
	}

	encryptedKey = pPeerKey->Encrypt(pChallengeKey->GetKey(), pChallengeKey->GetKeyLen());
	if (!encryptedKey.GetData())
	{
		WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
		WON_CryptFactory::DeleteEGPublicKey(pPeerKey);
		WON_CryptFactory::DeleteBFSymmetricKey(pChallengeKey);
		return 1;
	}

	if (challenge->key)
	{
		WON_CryptFactory::DeleteBFSymmetricKey((WON_BFSymmetricKey*)challenge->key);
		challenge->key = NULL;
	}
	challenge->key = pChallengeKey;

	buffer.appendLong(0xFFFFFFFF);
	buffer.appendByte(WONMsg::Auth1Challenge2);
	buffer.appendShort(pCertificate->GetRawLen());
	buffer.append(pCertificate->GetRaw(), pCertificate->GetRawLen());
	buffer.appendShort(encryptedKey.GetLen());
	buffer.append(encryptedKey.GetData(), encryptedKey.GetLen());
	Crypt_SetAuthData(challenge, &buffer);

	WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
	WON_CryptFactory::DeleteEGPublicKey(pPeerKey);

	return 2;
}

int Crypt_AuthChallenge2( void* data, int size, struct authchallenge_s* challenge )
{
	WON_AuthCertificate1* pPeerCertificate;
	WON_EGPublicKey* pPeerKey;
	WON_EGPrivateKey* pPrivateKey;
	WON_BFSymmetricKey* pChallengeKey;
	WON_CryptKeyBase::CryptReturn decryptedKey;
	WON_CryptKeyBase::CryptReturn encryptedKey;
	WriteBuffer secret(0x100);
	WriteBuffer buffer(0x100);
	const unsigned char* pData;
	unsigned short nCertLen;
	unsigned short nEncryptedLen;

	if (!gAuthRequest)
		return 0;

	if (!gAuthRequest->GetCertificate())
		return 0;

	pData = (const unsigned char*)data;
	nCertLen = *(unsigned short*)pData;
	pData += sizeof(unsigned short);

	pPeerCertificate = WON_AuthFactory::NewAuthCertificate1(pData, nCertLen);
	if (!pPeerCertificate)
		return 1;
	pData += nCertLen;

	if (!pPeerCertificate->IsValid() || !gAuthRequest->verifyAuthStuff(pPeerCertificate))
	{
		WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
		return 1;
	}

	pPeerKey = WON_CryptFactory::NewEGPublicKey(pPeerCertificate->GetPubKeyLen(), pPeerCertificate->GetPubKey());
	if (!pPeerKey)
	{
		WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
		return 1;
	}

	pPrivateKey = gAuthRequest->GetPrivateKey();
	if (!pPrivateKey)
	{
		WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
		WON_CryptFactory::DeleteEGPublicKey(pPeerKey);
		return 1;
	}

	nEncryptedLen = *(unsigned short*)pData;
	pData += sizeof(unsigned short);
	decryptedKey = pPrivateKey->Decrypt(pData, nEncryptedLen);
	if (!decryptedKey.GetData() || pPrivateKey->GetLastError())
	{
		WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
		WON_CryptFactory::DeleteEGPublicKey(pPeerKey);
		return 1;
	}

	if (gConnectionKey)
	{
		WON_CryptFactory::DeleteBFSymmetricKey(gConnectionKey);
		gConnectionKey = NULL;
	}
	gConnectionKey = WON_CryptFactory::NewBFSymmetricKey((unsigned short)decryptedKey.GetLen(), decryptedKey.GetData());
	if (!gConnectionKey)
	{
		WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
		WON_CryptFactory::DeleteEGPublicKey(pPeerKey);
		return 1;
	}

	pChallengeKey = WON_CryptFactory::NewBFSymmetricKey(AUTH_KEY_LEN, NULL);
	if (!pChallengeKey)
	{
		WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
		WON_CryptFactory::DeleteEGPublicKey(pPeerKey);
		return 1;
	}

	secret.appendShort(decryptedKey.GetLen());
	secret.append(decryptedKey.GetData(), decryptedKey.GetLen());
	secret.appendShort(pChallengeKey->GetKeyLen());
	secret.append(pChallengeKey->GetKey(), pChallengeKey->GetKeyLen());

	if (challenge->key)
	{
		WON_CryptFactory::DeleteBFSymmetricKey((WON_BFSymmetricKey*)challenge->key);
		challenge->key = NULL;
	}
	challenge->key = pChallengeKey;

	encryptedKey = pPeerKey->Encrypt(secret.getBuffer(), secret.getSize());
	if (!encryptedKey.GetData() || pPeerKey->GetLastError())
	{
		WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
		WON_CryptFactory::DeleteEGPublicKey(pPeerKey);
		WON_CryptFactory::DeleteBFSymmetricKey(pChallengeKey);
		challenge->key = NULL;
		return 1;
	}

	buffer.appendLong(0xFFFFFFFF);
	buffer.appendByte(WONMsg::Auth1Complete);
	buffer.appendShort(encryptedKey.GetLen());
	buffer.append(encryptedKey.GetData(), encryptedKey.GetLen());
	Crypt_SetAuthData(challenge, &buffer);

	WON_AuthFactory::DeleteAuthCertificate1(pPeerCertificate);
	WON_CryptFactory::DeleteEGPublicKey(pPeerKey);
	return 2;
}

int Crypt_AuthComplete( void* data, int size, struct authchallenge_s* challenge )
{
	WON_EGPrivateKey* pPrivateKey;
	WON_BFSymmetricKey* pChallengeKey;
	WON_CryptKeyBase::CryptReturn decrypted;
	ReadBuffer reader;
	WriteBuffer buffer(0x100);
	const unsigned char* pData;
	const unsigned char* pKey;
	const unsigned char* pResponse;
	unsigned short nEncryptedLen;
	short nKeyLen;
	short nResponseLen;

	if (!gAuthRequest)
		return 0;

	if (!gAuthRequest->GetCertificate())
		return 0;

	pPrivateKey = gAuthRequest->GetPrivateKey();
	if (!pPrivateKey)
		return 1;

	pData = (const unsigned char*)data;
	nEncryptedLen = *(unsigned short*)pData;
	pData += sizeof(unsigned short);

	decrypted = pPrivateKey->Decrypt(pData, nEncryptedLen);
	if (!decrypted.GetData() || pPrivateKey->GetLastError() || !challenge->key)
		return 1;

	pChallengeKey = (WON_BFSymmetricKey*)challenge->key;
	reader.setBuffer((const char*)decrypted.GetData(), decrypted.GetLen());
	if (!reader.readShort(&nKeyLen))
		return 1;

	pKey = reader.getDataPtr();
	if (!reader.skipBytes((unsigned short)nKeyLen) ||
		(unsigned short)nKeyLen != pChallengeKey->GetKeyLen() ||
		memcmp(pChallengeKey->GetKey(), pKey, (unsigned short)nKeyLen))
	{
		return 1;
	}

	if (!reader.readShort(&nResponseLen))
		return 1;
	pResponse = reader.getDataPtr();
	if (!reader.skipBytes((unsigned short)nResponseLen))
		return 1;

	buffer.appendLong(0xFFFFFFFF);
	buffer.appendByte(WONMsg::Auth1CompleteHL); // Half-Life specific message
	buffer.appendShort((unsigned short)nResponseLen);
	buffer.append(pResponse, (unsigned short)nResponseLen);
	Crypt_SetAuthData(challenge, &buffer);
	return 2;
}

int Crypt_DecodeAuthComplete( void* data, int size, struct authchallenge_s* challenge )
{
	WON_BFSymmetricKey* pChallengeKey;
	unsigned short keyLen;

	if (!challenge->key)
		return 1;

	pChallengeKey = (WON_BFSymmetricKey*)challenge->key;
	keyLen = *(unsigned short*)data;
	if (keyLen != pChallengeKey->GetKeyLen())
		return 1;

	return memcmp((unsigned char*)data + sizeof(unsigned short), pChallengeKey->GetKey(), keyLen) == 0 ? 2 : 1;
}

void Crypt_ServiceAuthRefresh( double time )
{
	static double tLastService = 0.0;

	if (!gAuthRequest)
		return;

	if (time - tLastService >= AUTH_REFRESH_THRESHOLD)
	{
		tLastService = time;

		gAuthRequest->HandleAuthRefresh();
	}
}

crypt_api_t cryptapi =
{
	sizeof(crypt_api_t),
	CRYPT_API_VERSION,
	Crypt_Initialize,
	Crypt_Shutdown,
	Crypt_ServiceAuthRefresh,
	Crypt_GetRawBFKey,
	Crypt_DeleteAuthData,
	Crypt_GetConnectionKey,
	Crypt_AuthRequest,
	Crypt_AuthChallenge1,
	Crypt_AuthChallenge2,
	Crypt_AuthComplete,
	Crypt_DecodeAuthComplete,
	Crypt_GetUserId,
	Crypt_GetCertificate,
	Crypt_GetNewCertificate,
	Crypt_IsAuthenticated,
	Crypt_GetAuthObject,
	Crypt_InstanceAuthObject,
	Crypt_DestroyAuthObject,
	Crypt_MD5_File,
	Crypt_CreateKey,
	KillSessionKey,
	Crypt_Encrypt,
	Crypt_Decrypt,
};
