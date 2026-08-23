#ifndef WONSERVER_UDP_H
#define WONSERVER_UDP_H
#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

// (Re)publish the current room set to the responder.
void	WonUdp_Sync( void );

// Register one room port -> player count with the responder (called by the room
// registry as rooms are added).
void	WonUdp_SetRoom( unsigned short nPort, int nPlayers );
void	WonUdp_Clear( void );

#ifdef __cplusplus
}
#endif

#endif // WONSERVER_UDP_H
