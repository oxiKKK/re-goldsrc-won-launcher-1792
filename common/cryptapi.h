#if !defined( CRYPTAPI_H )
#define CRYPTAPI_H
#ifdef _WIN32
#pragma once
#endif

typedef int qboolean;

#define CRYPT_AUTHTYPE_CLIENT	0
#define CRYPT_AUTHTYPE_SERVER	1

#define CRYPT_GUID	"2123437429222"

#define CRYPT_API_VERSION 1

// Internal auth mode selected by Crypt_Initialize.
extern int g_authIsServer;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct crypt_parms_s
{
	int	authType;
	char* pszBaseDir;
	char* pszGUID;
	char* pszServerFile;
	char* pszExeName;

	// Callbacks
	void (*pfnPrintf)(char* fmt, ...);		// dump to console
	void (*pfnAuthFailure)();
	char* (*pfnGetLocalizedString)(unsigned int code);
} crypt_parms_t;

typedef struct authchallenge_s
{
	unsigned char unknown[40];	// Preserved retail layout.
	int dataSize;
	void* data;
	void* key;
	int userId;
} authchallenge_t;

typedef struct crypt_api_s
{
	int		size;
	int		version;

	void	(*Initialize)(crypt_parms_t*);
	void	(*Shutdown)(void);
	void	(*ServiceAuthRefresh)(double);
	int		(*GetRawBFKey)(void*, void*, int*);
	void	(*DeleteAuthData)(struct authchallenge_s*);
	int		(*GetConnectionKey)(void*, int*);
	int		(*AuthRequest)(void*, int, struct authchallenge_s*);
	int		(*AuthChallenge1)(void*, int, struct authchallenge_s*);
	int		(*AuthChallenge2)(void*, int, struct authchallenge_s*);
	int		(*AuthComplete)(void*, int, struct authchallenge_s*);
	int		(*DecodeAuthComplete)(void*, int, struct authchallenge_s*);
	int		(*GetUserId)(void);
	int		(*GetCertificate)(void*, int*);
	int		(*GetNewCertificate)(void);
	int		(*IsAuthenticated)(void);
	void*	(*GetAuthObject)(void);
	void	(*InstanceAuthObject)(void);
	void	(*DestroyAuthObject)(void);
	int		(*MD5_File)(unsigned char*, char*);
	int		(*CreateKey)(int, const unsigned char*, void**);
	void	(*DeleteKey)(void**);
	int		(*Encrypt)(const void*, const void*, int, void*, int*);
	int		(*Decrypt)(const void*, const void*, int, void*, int*);
} crypt_api_t;

int Crypt_ReturnAPI(int version, crypt_api_t* api);

extern crypt_api_t crypt;

// The crypt-UI auth-status queries the launcher polls while an auth request is
// in flight (0x47B8C0 - 0x47B8F0, cryptapi band; defined in AuthRequest.cpp).
int   CryptApi_AuthHasError( void* pAuth );		// 0x47B8D0
int   CryptApi_AuthErrorState( void* pAuth );	// 0x47B8E0
int   CryptApi_AuthErrorCode( void* pAuth );	// 0x47B8F0
char* CryptApi_AuthErrorString( void* pAuth );	// 0x47B8C0

#ifdef __cplusplus
}
#endif

#endif // CRYPTAPI_H
