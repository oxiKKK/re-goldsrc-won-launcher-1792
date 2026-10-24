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
// Purpose: declares the intro logo dialog (CLogoDlg, IDD 202) and the
//          master-list fetch object.
//
// $NoKeywords: $
//=============================================================================

#ifndef LOGO_DLG_H
#define LOGO_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "resource_dlg.h"

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg dialog
//
// vftable 0x4AF8DC -- the intro screen.  It is a plain CDialog, not a skinned
// page, and carries its own copy of the frame protocol in slots 52-57.

class CLogoDlg : public CDialog
{
// Construction
public:
	CLogoDlg( CWnd* pParent = NULL );

	enum { IDD = IDD_LOGO };				// 202

// Operations
public:
	int		RunModalLoop( DWORD dwFlags );
	void	PlayIntroSequence();
	void	FlushInputAndClose();

// Overrides
	//{{AFX_VIRTUAL(CLogoDlg)
	public:
#if defined(_MSC_VER) && (_MSC_VER < 1300)
	virtual int     DoModal();
#else
	virtual INT_PTR DoModal();
#endif
	virtual BOOL	OnInitDialog();
	//}}AFX_VIRTUAL

	// The frame protocol; only the timed DIB tick has a body.
	virtual void	RMLSetup()		{}
	virtual int		RMLPreIdle();
	virtual void	RMLIdle()		{}
	virtual void	RMLPrePump()	{}
	virtual void	RMLPump()		{}
	virtual void	RMLPostPump()	{}

// Attributes
public:
	int			m_bDibLogo;		// +92   draw the static DIB pair instead of AVI
	HGLOBAL		m_hDib1;		// +96   logo frame DIB (freed in the dtor)
	HGLOBAL		m_hDib2;		// +100  alternate frame DIB
	float		m_flDelay;		// +104  inter-frame delay (ctor: 2.0)
	float		m_flLastTime;	// +108  Sys_FloatTime of the last tick
	int			m_nActiveDib;	// +112  1 = m_hDib1 (ctor default); advances to 2

// Implementation
public:
	virtual ~CLogoDlg();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLogoDlg)
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnLogoStart();
	afx_msg void	OnChar( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

// The synchronous MCI AVI player (alias "sierravideo"; ESC breaks).  Plays
// media\<name>.avi centred in a 640x480 frame inside pWnd.
void	Launcher_PlayAVI( CWnd* pWnd, const char* pszName, int* pWH );

class CNetGameDlg;
class CHLMasterAsyncSocket;

// The master-list fetch state machine; the internet-games sheet owns one while
// a batch query is in flight.
typedef struct masterfetch_s
{
	void	( *m_pfnDone )( void* pOwner, int nResult );							// +0
	void	( *m_pfnStatus )( void* pOwner, const char* pszFormat, ... );		// +4
	CNetGameDlg*	m_pOwner;			// +8
	CHLMasterAsyncSocket*	m_pSocket;	// +12
	double			m_flLastSend;		// +16   Sys_FloatTime of the last request
	int				m_bBusy;			// +24
	char			m_szHost[1024];		// +28
	int				m_nPort;			// +1052
} masterfetch_t;

void	MasterFetch_Init( masterfetch_t* p, CNetGameDlg* pOwner,
			void ( *pfnDone )( void* pOwner, int nResult ),
			void ( *pfnStatus )( void* pOwner, const char* pszFormat, ... ) );
void	MasterFetch_CloseSocket( masterfetch_t* p );
// pszFilter is the master-query criteria infostring (NULL => plain query).
void	MasterFetch_Request( masterfetch_t* p, const char* pszHost,
			unsigned int nPort, const char* pszFilter );
void	MasterFetch_Start( masterfetch_t* p, const char* pszFilter );
void	MasterFetch_Service( masterfetch_t* p );

#endif // LOGO_DLG_H
