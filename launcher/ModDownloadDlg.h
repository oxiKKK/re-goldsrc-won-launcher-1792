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
// Purpose: declares the FTP mod-download progress popup (CModDownloadDlg).
//
// $NoKeywords: $
//=============================================================================

#ifndef MODDOWNLOAD_DLG_H
#define MODDOWNLOAD_DLG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include <afxinet.h>
#include "mod.h"
#include "DlgPopupBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "resource_dlg.h"

// One queued remote file.
typedef struct dlfile_s
{
	char			szName[260];		// +0    file name (no path)
	char			szSubDir[260];		// +260  remote sub-directory ("" at root)
	char			szLocalPath[260];	// +520  local destination
	int				cbLength;			// +780  remote file length
	struct dlfile_s* pNext;				// +784  next file in the queue
} dlfile_t;

// A remote sub-directory found during the walk.  Its own, smaller node: the
// walk only needs the name and the link.
typedef struct dldir_s
{
	char			szName[260];		// +0
	struct dldir_s*	pNext;				// +260
} dldir_t;

/////////////////////////////////////////////////////////////////////////////
// CModDownloadDlg dialog
//
// Walks the mod's FTP directory, queues every file it finds, then pulls them
// a kilobyte at a time out of the modal loop's idle hook.

class CModDownloadDlg : public CDlgPopupBase
{
// Construction
public:
	CModDownloadDlg( mod_t* pMod, CWnd* pParent = NULL );

// Dialog Data
	//{{AFX_DATA(CModDownloadDlg)
	enum { IDD = IDD_MODDOWNLOAD };
	CODBlendBtn	m_btnCancel;	// +104  IDOK
	CODStatic	m_lblTime;		// +344  IDC_MODDOWNLOAD_TIME
	CODStatic	m_lblTitle;		// +440  IDC_MODDOWNLOAD_TITLE
	CODStatic	m_lblStatus;	// +536  IDC_MODDOWNLOAD_STATUS
	//}}AFX_DATA

// Attributes
public:
	char		m_szRemoteDir[260];	// +632  current remote sub-directory

	mod_t*		m_pMod;			// +892  the mod being installed
	dlfile_t*	m_pFileList;	// +896  head of the enumerated remote-file list
	dlfile_t*	m_pCurFile;		// +900  file currently being transferred
	FILE*		m_fpLocal;		// +904  local "wb" file for the current transfer
	CInternetFile*	m_pFtpFile;	// +908  open remote file (NULL between files)
	int			m_cbThisFile;	// +912  bytes written for the current file
	int			m_cbReceived;	// +916  total bytes received so far
	int			m_cbTotal;		// +920  total download size
	CFtpConnection*	m_pFtpConn;	// +924  the FTP connection
	CInternetSession*	m_pSession;	// +928  the WININET session
	int			m_bDownloadOK;	// +932  transfer active / succeeded
	int			m_bInitialized;	// +936  OnInitDialog finished
	float		m_flStartTime;	// +940  Sys_FloatTime at dialog open
	HGLOBAL		m_headerLoaded;	// +944  the loaded button strip
	int			m_headerStride;	// +948
	int			m_headerW;		// +952
	int			m_headerH;		// +956

// Overrides
	//{{AFX_VIRTUAL(CModDownloadDlg)
	public:
	virtual ~CModDownloadDlg();
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual int		RMLPreIdle();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	ServiceDownload();
	void	StartOrFinishFile();
	void	PumpChunk();
	void	ExtractArchive();
	BOOL	Connect();
	BOOL	HasLibList( CFtpConnection* pConn, const char* pszDir );
	int		EnumDir( CFtpConnection* pConn, const char* pszDir );
	void	SetStatus( float flElapsed, const char* pszFmt, ... );

	// Generated message map functions
	//{{AFX_MSG(CModDownloadDlg)
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // MODDOWNLOAD_DLG_H
