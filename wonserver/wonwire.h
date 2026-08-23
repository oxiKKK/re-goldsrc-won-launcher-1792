#ifndef WONSERVER_WONWIRE_H
#define WONSERVER_WONWIRE_H
#ifdef _WIN32
#pragma once
#endif

#include <string>

// Both ends of the wire serialize through the WON WriteBuffer / ReadBuffer
// classes (hlwon/cryptapi); this header only carries what the server needs on
// top of them.

// Widen an ASCII/ANSI literal for WriteBuffer::appendWString, which puts the WON
// [u16 charCount][UTF-16LE] encoding on the wire.
std::wstring	WonWide( const char* psz );

// Pull the player nick out of a room-join payload (svc 50 msg 0), which
// Chat_BuildJoinRequest lays out as [u16 charCount][UTF-16LE nick][byte 1]
// [optional password string].  Writes "" when the payload is malformed.
void	WonWire_ReadJoinNick( const unsigned char* pPayload, int cbPayload,
							  char* pszOut, int cbOut );

void	WonWire_ReadJoin( const unsigned char* pPayload, int cbPayload,
						  char* pszNick, int cbNick, char* pszPassword, int cbPassword );

#endif // WONSERVER_WONWIRE_H
