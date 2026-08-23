#ifndef WONSERVER_GAMESV_H
#define WONSERVER_GAMESV_H
#ifdef _WIN32
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Mock GoldSrc game servers so the Internet games page has rows to show.  They
// answer the browser's "infostring" query on their own UDP ports; the master
// (wonserver_master.cpp) hands their addresses out.
void			WonGameSv_Start( void );
int				WonGameSv_Count( void );
unsigned short	WonGameSv_Port( int i );

#ifdef __cplusplus
}
#endif

#endif // WONSERVER_GAMESV_H
