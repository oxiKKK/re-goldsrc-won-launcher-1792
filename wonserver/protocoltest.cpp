// protocoltest.cpp -- socket-level coverage for every launcher-facing service.

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <vector>

static void Put16( std::vector<unsigned char>& b, unsigned short value )
{
	b.push_back( (unsigned char)value );
	b.push_back( (unsigned char)( value >> 8 ) );
}

static void Put32( std::vector<unsigned char>& b, unsigned long value )
{
	b.push_back( (unsigned char)value );
	b.push_back( (unsigned char)( value >> 8 ) );
	b.push_back( (unsigned char)( value >> 16 ) );
	b.push_back( (unsigned char)( value >> 24 ) );
}

static void PutString( std::vector<unsigned char>& b, const char* value )
{
	int len = (int)strlen( value );
	Put16( b, (unsigned short)len );
	b.insert( b.end(), value, value + len );
}

static void PutWString( std::vector<unsigned char>& b, const char* value )
{
	int len = (int)strlen( value );
	Put16( b, (unsigned short)len );
	for ( int i = 0; i < len; i++ )
	{
		b.push_back( (unsigned char)value[i] );
		b.push_back( 0 );
	}
}

static unsigned short Get16( const unsigned char* p )
{
	return (unsigned short)( p[0] | ( p[1] << 8 ) );
}

static unsigned long Get32( const unsigned char* p )
{
	return (unsigned long)p[0] | ( (unsigned long)p[1] << 8 )
		| ( (unsigned long)p[2] << 16 ) | ( (unsigned long)p[3] << 24 );
}

static bool SendAll( SOCKET s, const unsigned char* p, int len )
{
	int sent = 0;
	while ( sent < len )
	{
		int wrote = send( s, (const char*)p + sent, len - sent, 0 );
		if ( wrote <= 0 )
			return false;
		sent += wrote;
	}
	return true;
}

static bool RecvAll( SOCKET s, unsigned char* p, int len )
{
	int got = 0;
	while ( got < len )
	{
		int read = recv( s, (char*)p + got, len - got, 0 );
		if ( read <= 0 )
			return false;
		got += read;
	}
	return true;
}

static SOCKET ConnectTcp( unsigned short port )
{
	SOCKET s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	struct sockaddr_in addr;
	memset( &addr, 0, sizeof( addr ) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
	addr.sin_port = htons( port );
	if ( s == INVALID_SOCKET || connect( s, (struct sockaddr*)&addr, sizeof( addr ) ) )
	{
		if ( s != INVALID_SOCKET )
			closesocket( s );
		return INVALID_SOCKET;
	}
	DWORD timeout = 2000;
	setsockopt( s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof( timeout ) );
	return s;
}

static std::vector<unsigned char> TitanFrame( unsigned long svc, unsigned long msg,
	const std::vector<unsigned char>& payload )
{
	std::vector<unsigned char> frame;
	Put32( frame, (unsigned long)( 12 + payload.size() ) );
	Put32( frame, svc );
	Put32( frame, msg );
	frame.insert( frame.end(), payload.begin(), payload.end() );
	return frame;
}

static bool RecvTitan( SOCKET s, unsigned long* svc, unsigned long* msg,
	std::vector<unsigned char>* payload )
{
	unsigned char header[12];
	if ( !RecvAll( s, header, sizeof( header ) ) )
		return false;
	unsigned long total = Get32( header );
	if ( total < 12 || total > 0x100000 )
		return false;
	*svc = Get32( header + 4 );
	*msg = Get32( header + 8 );
	payload->resize( total - 12 );
	return payload->empty() || RecvAll( s, &(*payload)[0], (int)payload->size() );
}

static bool TitanTransact( unsigned short port, unsigned long svc, unsigned long msg,
	const std::vector<unsigned char>& request, unsigned long* replySvc,
	unsigned long* replyMsg, std::vector<unsigned char>* reply )
{
	SOCKET s = ConnectTcp( port );
	if ( s == INVALID_SOCKET )
		return false;
	std::vector<unsigned char> frame = TitanFrame( svc, msg, request );
	bool ok = SendAll( s, &frame[0], (int)frame.size() )
		&& RecvTitan( s, replySvc, replyMsg, reply );
	closesocket( s );
	return ok;
}

static int UdpQuery( unsigned short port, const unsigned char* request, int requestLen,
	unsigned char* reply, int replyCap )
{
	SOCKET s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	struct sockaddr_in addr;
	memset( &addr, 0, sizeof( addr ) );
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
	addr.sin_port = htons( port );
	DWORD timeout = 2000;
	setsockopt( s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof( timeout ) );
	if ( sendto( s, (const char*)request, requestLen, 0,
		(struct sockaddr*)&addr, sizeof( addr ) ) != requestLen )
	{
		closesocket( s );
		return 0;
	}
	int fromLen = sizeof( addr );
	int got = recvfrom( s, (char*)reply, replyCap, 0,
		(struct sockaddr*)&addr, &fromLen );
	closesocket( s );
	return got > 0 ? got : 0;
}

static bool HasBytes( const unsigned char* p, int len, const char* value )
{
	int valueLen = (int)strlen( value );
	for ( int i = 0; i + valueLen <= len; i++ )
		if ( !memcmp( p + i, value, valueLen ) )
			return true;
	return false;
}

static bool HasWide( const std::vector<unsigned char>& b, const char* value )
{
	std::vector<unsigned char> wide;
	for ( const char* p = value; *p; p++ )
	{
		wide.push_back( (unsigned char)*p );
		wide.push_back( 0 );
	}
	for ( size_t i = 0; i + wide.size() <= b.size(); i++ )
		if ( !memcmp( &b[i], &wide[0], wide.size() ) )
			return true;
	return false;
}

static bool TestMasterAndGame()
{
	unsigned char reply[4096];
	unsigned char listQuery[5] = { 'e', 0, 0, 0, 0 };
	int len = UdpQuery( 27010, listQuery, sizeof( listQuery ), reply, sizeof( reply ) );
	if ( len < 16 || Get32( reply ) != 0xFFFFFFFF || reply[4] != 'f' )
		return false;

	unsigned char versionQuery[] = { 'v', '1', '.', '1', '.', '0', '.', '8', 0, 0 };
	len = UdpQuery( 27010, versionQuery, sizeof( versionQuery ), reply, sizeof( reply ) );
	if ( len != 36 || Get32( reply ) != 0xFFFFFFFF || reply[4] != 'w' )
		return false;

	const char* commands[] = { "ping", "infostring\n", "players", "rules" };
	const char expected[] = { 'j', 'i', 'D', 'E' };
	for ( int i = 0; i < 4; i++ )
	{
		std::vector<unsigned char> query( 4, 0xFF );
		query.insert( query.end(), commands[i], commands[i] + strlen( commands[i] ) + 1 );
		len = UdpQuery( 27023, &query[0], (int)query.size(), reply, sizeof( reply ) );
		if ( len < 5 || Get32( reply ) != 0xFFFFFFFF )
			return false;
		if ( i == 1 )
		{
			if ( !HasBytes( reply, len, "infostringresponse" ) )
				return false;
		}
		else if ( reply[4] != expected[i] )
			return false;
	}
	return true;
}

static bool TestModServices()
{
	unsigned char reply[4096];
	unsigned char query[5] = { 'n', 0, 0, 0, 0 };
	int len = UdpQuery( 27011, query, sizeof( query ), reply, sizeof( reply ) );
	if ( len < 16 || Get32( reply ) != 0xFFFFFFFF || reply[4] != 'o'
	  || !HasBytes( reply, len, "Counter-Strike" ) )
		return false;

	SOCKET s = ConnectTcp( 27010 );
	if ( s == INVALID_SOCKET )
		return false;
	const char request[] = "x\r\nstart-of-list\r\n";
	bool ok = SendAll( s, (const unsigned char*)request, sizeof( request ) );
	len = ok ? recv( s, (char*)reply, sizeof( reply ), 0 ) : 0;
	closesocket( s );
	return len > 8 && Get32( reply ) == 0xFFFFFFFF && reply[4] == 'y'
		&& HasBytes( reply, len, "end-of-list" );
}

static bool TestDirectoryAndFactory( unsigned short* factoryPort )
{
	std::vector<unsigned char> request;
	std::vector<unsigned char> reply;
	unsigned long svc, msg;

	PutWString( request, "/Half-Life/Public" );
	request.push_back( 0 );
	if ( !TitanTransact( 6002, 30, 2, request, &svc, &msg, &reply )
	  || svc != 30 || msg != 3 || reply.size() < 4 || Get16( &reply[0] ) != 0
	  || Get16( &reply[2] ) != 8 || !HasWide( reply, "Lobby" ) )
		return false;

	request.clear();
	PutWString( request, "/Half-Life" );
	request.push_back( 0 );
	if ( !TitanTransact( 6002, 30, 2, request, &svc, &msg, &reply )
	  || reply.size() < 4 || Get16( &reply[2] ) != 1
	  || !HasWide( reply, "TitanFactoryServer" ) )
		return false;

	request.clear();
	PutString( request, "HLChatServ" );
	request.push_back( 1 );
	PutString( request, "-password swordfish" );
	Put32( request, 232 );
	PutString( request, "127.0.0.1:6002" );
	PutWString( request, "Protocol Room" );
	PutWString( request, "/Half-Life/Public" );
	request.push_back( 1 );
	request.push_back( 1 );
	request.push_back( 0 );
	Put16( request, 0 );
	if ( !TitanTransact( 6002, 10, 2, request, &svc, &msg, &reply )
	  || svc != 10 || msg != 1 || reply.size() != 5
	  || Get16( &reply[0] ) != 2 || reply[2] != 1 )
		return false;
	*factoryPort = Get16( &reply[3] );
	return *factoryPort >= 27100 && *factoryPort < 27116;
}

static std::vector<unsigned char> JoinPayload( const char* nick, const char* password )
{
	std::vector<unsigned char> payload;
	PutWString( payload, nick );
	payload.push_back( 1 );
	if ( password )
		PutString( payload, password );
	return payload;
}

static bool TestRoom( unsigned short port, const char* nick, const char* password,
	bool expectPasswordRetry )
{
	SOCKET s = ConnectTcp( port );
	if ( s == INVALID_SOCKET )
		return false;
	unsigned long svc, msg;
	std::vector<unsigned char> payload;
	std::vector<unsigned char> frame;
	std::vector<unsigned char> reply;

	if ( expectPasswordRetry )
	{
		payload = JoinPayload( nick, NULL );
		frame = TitanFrame( 50, 0, payload );
		if ( !SendAll( s, &frame[0], (int)frame.size() )
		  || !RecvTitan( s, &svc, &msg, &reply ) || reply.size() != 6
		  || (short)Get16( &reply[0] ) != -1103 )
		{
			closesocket( s );
			return false;
		}
	}

	payload = JoinPayload( nick, password );
	frame = TitanFrame( 50, 0, payload );
	if ( !SendAll( s, &frame[0], (int)frame.size() )
	  || !RecvTitan( s, &svc, &msg, &reply ) || svc != 50 || msg != 1
	  || reply.size() != 6 || Get16( &reply[0] ) != 0 || Get32( &reply[2] ) == 0 )
	{
		closesocket( s );
		return false;
	}
	unsigned long memberId = Get32( &reply[2] );
	if ( !RecvTitan( s, &svc, &msg, &reply ) || svc != 50 || msg != 18 )
	{
		closesocket( s );
		return false;
	}

	std::vector<unsigned char> find;
	find.push_back( 3 ); find.push_back( 1 ); find.push_back( 3 );
	Put16( find, 7 );
	find.push_back( 0 );
	PutWString( find, nick );
	unsigned char udpReply[64];
	int len = UdpQuery( port, &find[0], (int)find.size(), udpReply, sizeof( udpReply ) );
	if ( len != 6 || udpReply[2] != 4 || Get16( udpReply + 4 ) != 7 )
	{
		closesocket( s );
		return false;
	}

	payload.clear();
	Put32( payload, memberId );
	Put16( payload, 0xFFFE );
	const char text[] = "protocol test";
	payload.insert( payload.end(), text, text + strlen( text ) );
	frame = TitanFrame( 50, 7, payload );
	if ( !SendAll( s, &frame[0], (int)frame.size() )
	  || !RecvTitan( s, &svc, &msg, &reply ) || svc != 50 || msg != 7
	  || reply.size() < 6 || Get32( &reply[0] ) != memberId )
	{
		closesocket( s );
		return false;
	}
	closesocket( s );
	return true;
}

int main()
{
	WSADATA wsa;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsa ) )
		return 1;

	unsigned short factoryPort = 0;
	if ( !TestMasterAndGame() ) { printf( "FAIL: master/game services\n" ); return 1; }
	printf( "OK: master bootstrap/list and ping/info/players/rules\n" );
	if ( !TestModServices() ) { printf( "FAIL: mod services\n" ); return 1; }
	printf( "OK: UDP mod catalog and TCP mod statistics\n" );
	if ( !TestDirectoryAndFactory( &factoryPort ) ) { printf( "FAIL: directory/factory\n" ); return 1; }
	printf( "OK: public/factory directories and room creation\n" );
	if ( !TestRoom( 27015, "ProtocolTest", NULL, false ) ) { printf( "FAIL: public room\n" ); return 1; }
	printf( "OK: room join, roster, find-player and chat relay\n" );
	if ( !TestRoom( factoryPort, "PrivateTest", "swordfish", true ) )
		{ printf( "FAIL: private room\n" ); return 1; }
	printf( "OK: private-room password challenge and retry\n" );
	printf( "\n=== WON PROTOCOL SURFACE OK ===\n" );
	return 0;
}
