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
// Purpose: declares ReadBuffer, the WON message reader.
//
// $NoKeywords: $
//=============================================================================

#ifndef __WON_READBUFFER_H__
#define __WON_READBUFFER_H__

#include <string>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class ReadBuffer
{
public:
	ReadBuffer( const char *data, int len );			// 0x45AB40
	ReadBuffer();									// 0x45AB30

	void setBuffer( const char *data, unsigned long len );	// 0x45AB60
	int readString( std::string *value );
	int readUByte( unsigned char *value );			// 0x45ABE0
	int readByte( char *value );
	int readUShort( unsigned short *value );			// 0x45ABB0
	int readShort( short *value );
	int readULong( unsigned long *value );			// 0x45AB80
	int readLong( long *value );
	int readWString( wchar_t *value, int cchMax );	// 0x45AC10
	const unsigned char *getTheRest( unsigned long *len );	// 0x45AD10
	int skipBytes( unsigned long len );				// 0x45AD30
	unsigned char *getDataPtr( void );				// 0x45AD60
	unsigned long getRemainingSize( void );			// 0x45AD70

private:
	unsigned char *mBuffer;
	unsigned long mReadPos;
	unsigned long mBufferSize;
};

#endif // __WON_READBUFFER_H__
