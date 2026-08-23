#ifndef __WON_WRITEBUFFER_H__
#define __WON_WRITEBUFFER_H__

#include <string>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class WriteBuffer
{
public:
	WriteBuffer( const WriteBuffer &other );
	WriteBuffer( int size );
	~WriteBuffer();

	WriteBuffer &operator=( const WriteBuffer &other );
	void appendString( const std::string &value );
	void appendWString( const std::wstring &value );
	void rewind( void );
	void setShort( int offset, short value );
	int getSize( void ) const;
	unsigned char *getBuffer( void ) const;
	void setBuf( int offset, const void *data, unsigned int len );
	void append( const void *data, unsigned int len );
	void appendByte( unsigned char value );
	void appendShort( unsigned short value );
	void appendLong( unsigned int value );
	void setLong( int offset, unsigned int value );

private:
	void reallocAndCopy( unsigned int size );

	unsigned char *mBuffer;
	unsigned int mCapacity;
	unsigned int mSize;
};

#endif // __WON_WRITEBUFFER_H__
