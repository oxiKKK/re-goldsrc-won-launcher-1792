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
// Purpose: declares the Controls / key-binding dialog (CKeyboardDlg, IDD 0xA3
//          = 163) and CODKeySearchComboBox.
//
// $NoKeywords: $
//=============================================================================

#ifndef KEYBOARD_DLG_H
#define KEYBOARD_DLG_H
#pragma once

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "ODListCtrl.h"
#include "ODComboBox.h"
#include "resource.h"
#include "resource_dlg.h"

// One key from gfx/shell/kb_keys.lst (the dialog keeps 256 of these, 132 bytes
// each).
typedef struct kbkey_s
{
	char		szName[64];		// +0    engine key name ("ENTER", "MOUSE1", ...)
	char		szDisplay[64];	// +64   display name shown in the list
	COLORREF	color;			// +128  list text colour
} kbkey_t;						// 132 bytes (0x84)

// One key slot of the player-config binding block (g_pServerBrowser+260,
// indexed by kb_keys.lst order, 40 bytes per entry).
typedef struct kbbinding_s
{
	char	szKeyName[32];		// +0   engine key name ("<UNKNOWN...>" when empty)
	int		cmdLen;				// +32  strlen(pszCommand)+1
	char*	pszCommand;			// +36  malloc'd bind command ("+attack", ...)
} kbbinding_t;					// 40 bytes (0x28)

class CODKeySearchComboBox;

class CKeyboardDlg;

// CODKeyBindingRow -- one action row of the binding list (0x1C bytes, newed by
// the dialog and kept on its m_pRows singly-linked list; the list control only
// references them).
class CODKeyBindingRow
{
public:
	// bodies external (OD control library)
	CODKeyBindingRow();
	~CODKeyBindingRow();

	void		Init( const char* pszText, const char* pszCommand,
					  const char* pszKey, COLORREF clr );
	const char*	GetCommand();		// bind command ("+attack")
	const char*	GetPrimaryKey();
	const char*	GetAltKey();
	void		ClearAlternate();
	void		ClearPrimary();
	void		SetKey( const char* pszKey, COLORREF clr, int bAlternate );
	void		AssignKey( const char* pszKey, COLORREF clr );		// fill primary, then alternate
	const char*	GetText();		// +0 action label ("Attack")
	COLORREF	GetTextColor();		// +16 primary colour
	COLORREF	GetAltColor();		// +20 alternate colour

	char*		m_pszText;			// +0   action label (malloc'd)
	char*		m_pszCommand;		// +4   bind command
	char*		m_pszPrimaryKey;	// +8   primary key name (uppercased)
	char*		m_pszAltKey;		// +12  alternate key name (uppercased)
	COLORREF	m_clrPrimary;		// +16  primary key text colour (default white)
	COLORREF	m_clrAlt;			// +20  alternate key text colour (default white)
	CODKeyBindingRow*	m_pNext;	// +24  the dialog's m_pRows chain
};

// CODKeyBindingCtrl -- the bindings report list (vftable 0x4AEDCC; the binary
// news a 0x79C block and constructs it with 0x41C7B0).
class CODKeyBindingCtrl : public CODListCtrl
{
public:
	// bodies external (OD control library)
	CODKeyBindingCtrl( CKeyboardDlg* pOwner );
	virtual ~CODKeyBindingCtrl();

	void	AttachSearchCombo( CODKeySearchComboBox* pCombo );		// > m_pSearchCombo
	int		BeginCellEdit( int iRow, int iCol );		// (latch the pick cell)

	// Owner-draw paint: three columns (action / primary / alternate) with the
	// pick cell flashed while the page captures a key (0x41CD60).
	virtual void	DrawRow( CDC* pDC, int iRow );

	// 0x420090 -- the "press a key" callout drawn over the pick cell.
	void	DrawCapturePrompt( CDC* pDC, int, int );

protected:
	afx_msg void	OnPaint();

	// Mouse.
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint pt );
	afx_msg void	OnLButtonDblClk( UINT nFlags, CPoint pt );

	// Cell-column cursor cycling (LEFT / RIGHT wrap across the two key columns).
	void	CycleColLeft();
	void	CycleColRight();

	// Selection navigation that keeps the chosen row scrolled into view.
	void	SelectPrevRow();		// (UP)
	void	SelectNextRow();		// (DOWN)
	void	SelectPrevPage();		// (PAGE UP)
	void	SelectNextPage();		// (PAGE DOWN)

	// Key handler: ENTER/arrows drive the cell cursor + selection, BACKSPACE/DELETE
	// clear the focused cell, all else defers to the base OnKeyDown (0x41CB60).
	afx_msg void	OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );

	DECLARE_MESSAGE_MAP()
public:

	// - state (binary byte offsets in comments) ---
	CKeyboardDlg*	m_pOwnerDlg;	// +1908  the owning page (ctor arg)
	COLORREF	m_clrCell;			// +1912  cell colour (ctor 0x609C)
	COLORREF	m_clrCellHot;		// +1916  hot cell colour (ctor 0xB8E8)
	CODKeySearchComboBox*	m_pSearchCombo;	// +1920  attached key-search combo
	int			m_iPickRow;			// +1924  row whose key cell is being edited (-1)
	int			m_iPickCol;			// +1928  1 = primary key, 2 = alternate key (-1)
	int			m_iCurCol;			// +1932  column the keyboard cursor currently targets (ctor 0)
	int			m_bPickPending;		// +1936  a combo pick awaits commit
	CFont		m_cellFont;			// +1940  Arial 12 weight-900 cell font

};

// CODKeySearchComboBox -- the "search for a key" drop list the binding list pops
// over a key cell (the binary news a 0xD4 block and constructs it with
// 0x41BE30).
class CODKeySearchComboBox : public CODComboBox
{
public:
	CODKeySearchComboBox( CODKeyBindingCtrl* pList, CKeyboardDlg* pOwner );
	virtual ~CODKeySearchComboBox();

	CODKeyBindingCtrl*	m_pList2;		// +196  the owning binding list (ctor a2)
	CKeyboardDlg*		m_pOwner;		// +200  owner dialog (ctor a3)
	int					m_curSel2;		// +204  cached selection (-1)
	char				m_editable;		// +208  editable flag (ctor 'a')

protected:
	// 0x41BE80 -- the drop-list key pump.
	afx_msg void	OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	DECLARE_MESSAGE_MAP()
};

class CKeyboardDlg : public CDlgBase
{
	// The OD list / search combo call back into the page's protected handlers
	// while a cell is edited (OnSearchSelChange, RememberClearedKey, SetCapturing).
	friend class CODKeySearchComboBox;
	friend class CODKeyBindingCtrl;

public:
	CKeyboardDlg( CWnd* pParent = NULL );
	virtual ~CKeyboardDlg();		// (deleting) ->(body)

	// The OD list calls back into the page while a cell is edited.
	void		SetDirty();		// > m_bDirty = 1
	const char*	GetKeyDisplayName( const char* pszKey );		// (m_keys lookup; "" when unknown)
	int			IsCapturing();		// > m_bCapturing (drives the cell flash)

// Dialog data (the binary byte offset is noted next to each control member).
	CODKeyBindingRow*	m_pRows;		// +224   kb_act.lst action rows (singly linked)
	CODStatic			m_lblHelp;		// +228   IDC_CONTROLS_KEYHELP (1149)
	CODBlendBtn			m_btnAdvanced;	// +328   IDC 34, strip row 25 (IDS_BTN_ADVANCED)
	CODBlendBtn			m_btnCancel;	// +568   IDC 25, strip row 14 (IDS_BTN_REVERT)
	CODBlendBtn			m_btnOK;		// +808   IDOK,   strip row 22 (IDS_BTN_OK)
	CODBlendBtn			m_btnDefaults;	// +1048  IDC 21, strip row 21 (IDS_BTN_RESTORE)

protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual BOOL	PreTranslateMessage( MSG* pMsg );		// (the key-capture pump)
	virtual void	OnOK();		// (write + save the bindings)

	// Command handlers (message map 0x4AEBC0)
	afx_msg void	OnCancelPage();		// cmd 25 (also ESC; confirms when dirty)
	afx_msg void	OnUseDefaults();		// cmd 21 (restore default bindings)
	afx_msg void	OnAdvancedOptions();		// cmd 34 (CGameOptionsDlg page)
	afx_msg void	OnBindListSelChange();		// id 1030, notify 1
	afx_msg void	OnSearchSelChange();		// id 113, CBN_SELCHANGE (commit a combo pick)
	afx_msg HBRUSH	OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void	OnTimer( UINT_PTR nIDEvent );		// (50ms capture tick -> Default)
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg void	OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );		// (folded stub)
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg void	OnPaint();		// (shared skin-paint thunk)
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );		// (shared skin thunk)

	// kb list parsing + binding work (private helpers in the binary)
	void	LoadKeyList();		// gfx/shell/kb_keys.lst -> m_keys + search pool
	void	LoadActionList();		// gfx/shell/kb_act.lst -> m_pRows + list rows
	CODKeyBindingRow*	FindRowByCommand( const char* pszCommand );
	COLORREF	GetKeyColor( const char* pszKey );		// (m_keys lookup; 0xFFFFFF default)
	void	RememberClearedKey( const char* pszKey );		// dedup-append to m_clearedKeys
	void	CommitPickedKey( char* pszKey );		// move the captured key into the pick cell
	void	SetCapturing( int bOn );		// capture latch + 50ms timer + UI sound

// Skinned-dialog state mirrored from the binary ctor.
	CBrush		m_bkBrush;		// +1288  black dialog background
	HGLOBAL		m_headerLoaded;		// +1296 header strip present
	int			m_headerStride;		// +1300 header DIB stride
	int			m_headerW;		// +1304 header width [0])
	int			m_headerH;		// +1308 header height [1])

// Binding work state (the fixed arrays are ctor-zeroed).
	kbkey_t		m_keys[256];			// +1312   kb_keys.lst key table (binding indices)
	int			m_nKeys;				// +35104  keys parsed (max 256)
	kbkey_t		m_clearedKeys[256];		// +35108  keys unbound while the page was up (name only)
	BYTE		m_pad68900[101376];			// +68900
	int			m_nClearedKeys;			// +170276
	CODKeyBindingCtrl*		m_pBindList;	// +170280  the binding list (id 1030)
	CODKeySearchComboBox*	m_pSearchCombo;	// +170284  the key-search combo (id 113)
	int			m_bDirty;				// +170288  a binding changed (confirm on cancel)
	int			m_bCapturing;			// +170292  PreTranslateMessage is eating the next key

	DECLARE_MESSAGE_MAP()
};

#endif // KEYBOARD_DLG_H
