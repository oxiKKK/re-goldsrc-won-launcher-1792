// exefuncs.h
#ifndef EXEFUNCS_H
#define EXEFUNCS_H

// Engine hands this to DLLs for functionality callbacks
typedef struct exefuncs_s
{
	int			fMMX;
	int			iCPUMhz;
	void		(*VID_LockBuffer)( void );
	void		(*VID_UnlockBuffer)( void );
	void		(*VID_Shutdown)( void );
	void		(*VID_Update)( struct vrect_s* rects );
	void		(*VID_ForceLockState)( int lk );
	int			(*EF_VID_ForceUnlockedAndReturnState)( void );
	void		(*EF_VID_ForceLockState)( void );
	char*		(*VID_GetExtModeDescription)( int mode );
	void		(*VID_GetVID)( struct viddef_s* pvid );
	void		(*D_BeginDirectRect)( int x, int y, byte* pbitmap, int width, int height );
	void		(*D_EndDirectRect)( int x, int y, int width, int height );
	void		(*AppActivate)( int fActive, int minimize );
	void		(*CDAudio_Play)( int track, int looping );
	void		(*CDAudio_Pause)( void );
	void		(*CDAudio_Resume)( void );
	void		(*CDAudio_Update)( void );
	void		(*InitCmds)( void );
	void        (*ErrorMessage)( int nLevel, const char* pszErrorMessage );
	int			(*D_SurfaceCacheForRes)( int width, int height );
	void        (*Console_Printf)( char* fmt, ... );
	char*		(*GetCDKey)( char* pszCDKey, int* nLength, int* bDedicated );
	void		(*unused1)( void );
	void		(*unused2)( void );
	void		(*unused3)( void );
	int			(*IsValidCD)( void );
	void		(*ChangeGameDirectory)( const char* pszNewDirectory );
	void		(*unused4)( void );
	void		(*AuthFailed)( void );
	char*		(*GetLocalizedString)( unsigned int uID );
} exefuncs_t;

#endif