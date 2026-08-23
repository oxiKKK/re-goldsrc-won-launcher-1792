#ifndef WONSERVER_MASTER_H
#define WONSERVER_MASTER_H
#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Start the master responder on the given port in background threads.
void	WonMaster_Start( unsigned short nPort, unsigned short nServicePort );

#ifdef __cplusplus
}
#endif

#endif // WONSERVER_MASTER_H
