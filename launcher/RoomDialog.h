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
// Purpose: declares CRoomDialog, the chat-room list shell, CRoomListCtrl and
//          CRoomStatic.
//
// $NoKeywords: $
//=============================================================================

#ifndef ROOM_DIALOG_H
#define ROOM_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include <afxcmn.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "BorderlessEdit.h"
#include "resource_dlg.h"
#include "rooms.h"

class CChatClient;
class CNetGameDlg;
class CODRoomListCtrl;

/////////////////////////////////////////////////////////////////////////////
// CRoomStatic window
//
// A CODStatic that draws two strings: a fixed 125px caption column in the
// shadow colour, then the value after it.  The internet-games page embeds one
// for its "Room:" banner.

class CRoomStatic : public CODStatic
{
public:
	CRoomStatic();

	void	SetText( const char* psz );

	COLORREF	m_clrShadow;	// +96   caption colour
	int			m_nBannerState;	// +100
	CString		m_banner;		// +104

	// Generated message map functions
protected:
	//{{AFX_MSG(CRoomStatic)
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CRoomListCtrl window
//
// Report-mode CListCtrl that owns its own background colour.  Build 1792 never
// instantiates it: only the six handlers and their map survive, so there is no
// vftable or RTTI record to take the name from.

class CRoomListCtrl : public CListCtrl
{
public:
	virtual void	DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );

	BYTE		m_pad60[16];	// +60
	int			m_nWidth;		// +76   client width, cached by OnSize
	COLORREF	m_clrBk;		// +80   LVM_SETBKCOLOR / LVM_SETTEXTBKCOLOR
	COLORREF	m_clrText;		// +84   LVM_SETTEXTCOLOR

	// Generated message map functions
protected:
	//{{AFX_MSG(CRoomListCtrl)
	afx_msg void	OnSize( UINT nType, int cx, int cy );
	afx_msg void	OnDrawItem( int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct );
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg LRESULT	OnSetTextColor( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT	OnSetBkColor( WPARAM wParam, LPARAM lParam );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CRoomDialog dialog

class CRoomDialog : public CDlgBase
{
// Construction
public:
	// Takes the property sheet, not a chat client: the page passes its
	// m_pBrowserEngine, and OnCreateGame calls LaunchChatServer/JoinRoom on
	// that same pointer and reads its m_pSelfIdentity.
	CRoomDialog( CNetGameDlg* pSheet, CWnd* pParent = NULL );

// Dialog Data
	// The two filter toggles are owner-draw checkboxes, not edits: the dialog
	// ORs BS_OWNERDRAW onto the template's BS_AUTOCHECKBOX and pokes each
	// one's m_bChecked directly.
	//{{AFX_DATA(CRoomDialog)
	enum { IDD = IDD_ROOM };
	CODBlendCheckBox	m_lblUserCreated;	// +224   IDC_ROOM_USER
	CODBlendCheckBox	m_lblPermanent;		// +528   IDC_ROOM_PERMANENT
	CODBlendBtn			m_btnCancel;		// +832   IDCANCEL              strip 14
	CODBlendBtn			m_btnCreate;		// +1072  IDC_ROOM_CREATE_ROOM  strip 46
	CODBlendBtn			m_btnJoin;			// +1312  IDOK                  strip 44
	//}}AFX_DATA

// Attributes
public:
	// Name of the row the user accepted, or NULL.
	const char*	GetPickedRoomName();

// Overrides
	//{{AFX_VIRTUAL(CRoomDialog)
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	OnOK();
	virtual void	OnCancel();
	virtual int		RMLPreIdle();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CRoomDialog();

protected:
	void	InitButtonStrips();
	static void	MatchSavedRoom( const char* pSavedNames, int nCount, chatroom_t* pRoom );
	void	PopulateRooms();		// walk the room list + filter -> AddRoomRow

	CODBlendBtn		m_btnSpare1;		// +1552  (ctor-only; role unknown)
	CODBlendBtn		m_btnSpare2;		// +1792
	BOOL			m_bShowUserCreated;	// +2032
	BOOL			m_bShowPermanent;	// +2036
	CNetGameDlg*	m_pSheet;			// +2040  the hosting property sheet
	int				m_unk2044;			// +2044

	// Ctor-seeded row palette; nothing in the image reads the four colours.
	COLORREF		m_clrListBg;		// +2048
	COLORREF		m_clrListSel;		// +2052
	const char*		m_pszPickedRoom;	// +2056  name of the row OnOK accepted
	COLORREF		m_clrListText;		// +2060
	COLORREF		m_clrListInfo;		// +2064

	HGLOBAL			m_hStripDib;		// +2068  the "head_rooms" button strip
	int				m_nStripStride;		// +2072
	int				m_stripWH[2];		// +2076  one strip cell
	CRoomList*		m_pRoomListHead;	// +2084  the sheet's circular room list
	CODRoomListCtrl* m_pRoomList;		// +2088  created in OnInitDialog

	// Generated message map functions
	//{{AFX_MSG(CRoomDialog)
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg void	OnPaint();
	afx_msg void	OnCreateGame();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg void	OnTogglePermanent();
	afx_msg void	OnRoomListSelect();
	afx_msg void	OnToggleUserCreated();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // ROOM_DIALOG_H
