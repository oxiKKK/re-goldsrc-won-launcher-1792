#ifndef _Crc_H
#define _Crc_H

/* crc.h */

// MD5 Hash
typedef struct
{
	unsigned int buf[4];
    unsigned int bits[2];
    unsigned char in[64];
} MD5Context_t;

typedef unsigned long CRC32_t;
void CRC32_Init( CRC32_t* pulCRC );
CRC32_t CRC32_Final( CRC32_t pulCRC );
void CRC32_ProcessByte( CRC32_t* pulCRC, unsigned char ch );
void CRC32_ProcessBuffer( CRC32_t* pulCRC, void* pBuffer, int nBuffer );

void MD5Init( MD5Context_t* ctx );
void MD5Update( MD5Context_t* ctx, unsigned char const* buf,
               unsigned int len );
void MD5Final( unsigned char* digest, MD5Context_t* ctx );
void MD5Transform( unsigned int buf[4], unsigned int const in[16] );

int MD5_Hash_File( unsigned char* digest, char* pszFileName, int bSeed, unsigned int* seed );
int MD5_Hash_CachedFile( unsigned char digest[16], unsigned char* pCache, int nFileSize, int bSeed, unsigned int seed[4] );

#endif // _Crc_H
