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
// Purpose: declares the connect/launch entry points CDlgConnectableBase
//          drives.
//
// $NoKeywords: $
//=============================================================================

#ifndef CONNECT_H
#define CONNECT_H
#ifdef _WIN32
#pragma once
#endif

class CServerInfo;
class CNetGameDlg;

int		Launcher_ConnectAndLaunch( CNetGameDlg* pSheet, CServerInfo* pInfo );
int		Launcher_HandleConnectFailure( void );

// Drive a candidate list to a live connection; returns nonzero once joined.
int		Connect_Run( CNetGameDlg* pSheet, CServerInfo* pInfo );

#endif // CONNECT_H
