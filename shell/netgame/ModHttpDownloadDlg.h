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
// Purpose: declares the HTTP mod-download progress popup
//          (CModHttpDownloadDlg).
//
// $NoKeywords: $
//=============================================================================

#ifndef MODHTTPDOWNLOAD_DLG_H
#define MODHTTPDOWNLOAD_DLG_H
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

// One queued object to fetch.
typedef struct httpfile_s
{
	char			szUrl[260];			// +0    source URL
	char			szDest[260];		// +260  "<gamedir>/<filename>" destination
	int				cbLength;			// +520  content length the server reported
	struct httpfile_s* pNext;			// +524  next node
} httpfile_t;

/////////////////////////////////////////////////////////////////////////////
// CModHttpDownloadDlg dialog
//
// The HTTP half of the mod installer: one object per URL rather than a
// directory walk, otherwise the same pump as the FTP dialog.

class CModHttpDownloadDlg : public CDlgPopupBase
{
// Construction
public:
	CModHttpDownloadDlg( mod_t* pMod, CWnd* pParent = NULL );

// Dialog Data
	//{{AFX_DATA(CModHttpDownloadDlg)
	enum { IDD = IDD_MODDOWNLOAD };
	CODBlendBtn	m_btnCancel;	// +104  IDOK
	CODStatic	m_lblTime;		// +344  IDC_MODDOWNLOAD_TIME
	CODStatic	m_lblTitle;		// +440  IDC_MODDOWNLOAD_TITLE
	CODStatic	m_lblStatus;	// +536  IDC_MODDOWNLOAD_STATUS
	//}}AFX_DATA

// Attributes
public:
	mod_t*		m_pMod;			// +632  the mod being installed
	httpfile_t*	m_pFileList;	// +636  head of the enumerated object list
	httpfile_t*	m_pCurFile;		// +640  object currently being transferred
	FILE*		m_fpLocal;		// +644  local "wb" file for the current transfer
	CInternetFile*	m_pHttpFile;// +648  open remote object (NULL between files)
	int			m_cbThisFile;	// +652  bytes written for the current file
	int			m_cbReceived;	// +656  total bytes received so far
	int			m_cbExpected;	// +660  total expected size
	CInternetSession* m_pSession;	// +664  the WININET session
	int			m_bDownloadOK;	// +668  transfer active / succeeded
	int			m_bInitialized;	// +672  OnInitDialog finished
	float		m_flStartTime;	// +676  Sys_FloatTime at dialog open
	HGLOBAL		m_headerLoaded;	// +680  the loaded button strip
	int			m_headerStride;	// +684
	int			m_headerW;		// +688
	int			m_headerH;		// +692

// Overrides
	//{{AFX_VIRTUAL(CModHttpDownloadDlg)
	public:
	virtual ~CModHttpDownloadDlg();
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual int		RMLPreIdle();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	StartDownload();
	void	ServiceDownload();
	void	ReadChunk();
	void	NextFileOrFinish();
	int		BuildFileList( const char* pszUrl );
	void	ExtractArchive();
	BOOL	IsUrlReachable( const char* pszUrl );
	void	SetStatus( float flElapsed, const char* pszFmt, ... );

	// Generated message map functions
	//{{AFX_MSG(CModHttpDownloadDlg)
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // MODHTTPDOWNLOAD_DLG_H
