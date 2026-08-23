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
// Purpose: declares the chat room list (Rooms_Load, CODRoomListCtrl).
//
// $NoKeywords: $
//=============================================================================

#ifndef ROOMS_H
#define ROOMS_H
#ifdef _WIN32
#pragma once
#endif

#include "ODListCtrl.h"
#include "chatclient.h"

/////////////////////////////////////////////////////////////////////////////
// CODRoomListCtrl window

class CODRoomListCtrl : public CODListCtrl
{
public:
	virtual void	DrawRow( CDC* pDC, int iRow );	// slot 47
	void	AddRoomRow( chatroom_t* pRoom );		// slot 49, sorted insert
};

// rooms.lst loader.
char*	Rooms_Load( char** ppArray, int* pnCount );

#endif // ROOMS_H
