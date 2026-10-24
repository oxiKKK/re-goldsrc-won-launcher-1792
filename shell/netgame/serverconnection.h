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
// Purpose: declares CServerConnection, a typedef alias of CServerInfo.
//
// The alias is the right model, not a collapse of two classes: the binary holds
// exactly one server-record type descriptor, ".?AVCServerInfo@@" at 0x4D1D38,
// and there is no ".?AVCServerConnection@@".  Every offset CHLAsyncSocket reaches
// through this pointer (+72, +84, +88, +96) is a named CServerInfo field used in
// its documented role.
//
// $NoKeywords: $
//=============================================================================

#ifndef SERVERCONNECTION_H
#define SERVERCONNECTION_H
#ifdef _WIN32
#pragma once
#endif

#include "ServerInfo.h"

typedef CServerInfo CServerConnection;

#endif // SERVERCONNECTION_H
