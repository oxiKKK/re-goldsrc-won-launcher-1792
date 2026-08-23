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
// Purpose: declares CCreateRoomDialog, the chat-room creation dialog.
//
// $NoKeywords: $
//=============================================================================

#ifndef CREATEROOM_DIALOG_H
#define CREATEROOM_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "BorderlessEdit.h"
#include "resource_dlg.h"

/////////////////////////////////////////////////////////////////////////////
// CCreateRoomDialog dialog

class CCreateRoomDialog : public CDlgBase
{
// Construction
public:
	CCreateRoomDialog( CWnd* pParent = NULL );

// Dialog Data
	//{{AFX_DATA(CCreateRoomDialog)
	enum { IDD = IDD_CREATEROOM };
	CODStatic		m_lblRoomPassword;	// +224   IDC_CREATEROOM_ROOMPASSWORD
	CODStatic		m_lblRoomName;		// +320   IDC_CREATEROOM_ROOMNAME
	CODBlendBtn		m_btnCancel;		// +416   IDCANCEL
	CODBlendBtn		m_btnOK;			// +656   IDOK
	//}}AFX_DATA

// Attributes
public:
	// The two entry fields are not on the template; OnInitDialog creates them
	// and DoDataExchange pulls their text into the two strings below, which the
	// room list reads back after DoModal returns IDOK.
	CBorderLessEdit	m_editRoomName;		// +896
	CBorderLessEdit	m_editRoomPassword;	// +1012
	CString			m_strRoomName;		// +1128
	CString			m_strRoomPassword;	// +1132

// Overrides
	//{{AFX_VIRTUAL(CCreateRoomDialog)
	public:
	virtual ~CCreateRoomDialog();
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual int		RMLPreIdle();
	//}}AFX_VIRTUAL

// Implementation
protected:
	void	SetupButtonStrips();

	HGLOBAL	m_headerLoaded;		// +1136  the "head_createroom" button strip
	int		m_headerStride;		// +1140
	int		m_headerW;			// +1144  one strip cell
	int		m_headerH;			// +1148

	// Generated message map functions
	//{{AFX_MSG(CCreateRoomDialog)
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // CREATEROOM_DIALOG_H
