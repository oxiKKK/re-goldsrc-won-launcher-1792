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
// Purpose: declares the owner-draw combo boxes (CODComboBox,
//          CODDriverComboBox, CODColorComboBox).
//
// $NoKeywords: $
//=============================================================================

#ifndef ODCOMBOBOX_H
#define ODCOMBOBOX_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>

class CODListBox;
class CODScrollBar;

// One CODDriverComboBox row record, as the video page builds them.
typedef struct drivrow_s	// 160 bytes
{
	char	label[32];		// +0
	char	desc[128];		// +32
} drivrow_t;

/////////////////////////////////////////////////////////////////////////////
// CODComboBox window
//
// A 15px closed face over a CODListBox drop; the list is a child window that
// paints nothing itself and dirties the combo instead.

class CODComboBox : public CComboBox
{
// Construction
public:
	CODComboBox();

	// vtbl+192: registers "CODComboBoxCls" + CreateEx; hides CComboBox::Create.
	virtual BOOL	Create( DWORD dwStyle, RECT* prc, CWnd* pParent, UINT nID );

// Attributes
public:
	CODListBox*	m_pList;		// +60   the id-101 dropdown / item store
	int			m_bTracking;	// +64   arrow-hover timer (1) running
	int			m_dropHeight;	// +68   open-drop pixel height (60)
	CString		m_curText;		// +72   text mirrored from the selection
	int			m_bEditable;	// +76   editable (drop grows when set)
	CBitmap		m_frame;		// +80   face frame bitmap
	CEdit*		m_pEdit;		// +88   hidden focus/edit child (id 102)
	int			m_bEditOpen;	// +92   the hidden CEdit is currently shown
	int			m_bEditing;		// +96   inline edit in progress (commit on close)
	CBrush		m_brHot;		// +100  hot-row brush
	COLORREF	m_clrBk;		// +108  base/background colour
	COLORREF	m_clrText;		// +112  row / face text colour
	COLORREF	m_clrFrame;		// +116  client frame, unfocused
	COLORREF	m_clrFrameFocus;	// +120  client frame, focused
	int			m_curSel;		// +124  cached selection (-1)
	CFont		m_faceFont;		// +128  Arial 12/400 face font
	int			m_bAutoDelete;	// +136  OnNcDestroy deletes this when set
	HGLOBAL		m_dnArrow;		// +140  gfx/shell/sm_dnarw.bmp
	HGLOBAL		m_dnArrowF;		// +144  gfx/shell/sm_dnarf.bmp
	CBrush		m_brText;		// +148  face/row fill brush
	CWnd*		m_pOwner;		// +156  the parent Create was given
	CFont		m_textFont;		// +160  Arial 11/400 row font
	int			m_rowHeight;	// +168  one drop-list row in pixels (15)
	int			m_bDropped;		// +176  drop list currently expanded
	RECT		m_rcClosed;		// +180  closed-state rect

// Operations
public:
	int		AddItem( void* pRecord );
	int		AddString( const char* psz );
	void	SetCurSel( int i );
	const char*	GetString( int i );
	int		GetCount();
	int		GetCurSel();
	int		FindString( const char* psz );
	int		IsDropped();
	void	SetAutoDelete( int bAuto );
	void	SetFaceColor( COLORREF clr );
	void	SetRowHeight( int h );
	void	MoveTo( RECT* prc, int bRepaint );
	void	ShowDrop( int bShow );
	CODScrollBar*	GetScrollbar();

	// Collapse the open drop and repaint both this combo and the menu behind it.
	void	Collapse();
	// Open the drop list when the arrow gutter / closed face is clicked.
	void	DropDown();

	void	DrawArrow( CDC* pDC, RECT* prc );

	// CODComboBox::SetDropHeight (0x444180)
	void	SetDropHeight( int h )			{ m_dropHeight = h; }
	// CODComboBox::SetFrameColor (0x445CE0)
	void	SetFrameColor( COLORREF clr )	{ m_clrFrame = clr; }
	// CODComboBox::SetTextColor (0x4462E0)
	void	SetTextColor( COLORREF clr )	{ m_clrText = clr; }

// Overrides
public:
	// vtbl+184: the full owner-draw repaint -- closed face band, focus frame and,
	// when dropped, every visible row through DrawRow.  Derived combos override it.
	virtual void	Paint();
	virtual void	DrawRow( CDC* pDC, int iRow );		// vtbl+188

// Implementation
public:
	virtual ~CODComboBox();

protected:
	// NOTE(ox): not in the binary.  There every CODComboBox is built through
	// Create, so m_pList is live by the first paint; a combo attached by
	// DDX_Control instead is subclassed, never Created, and would paint with
	// m_pList still NULL.
	virtual void	PreSubclassWindow();

	// Generated message map functions
	//{{AFX_MSG(CODComboBox)
	afx_msg UINT	OnGetDlgCode();
	afx_msg void	OnNcDestroy();
	afx_msg void	OnPaint();
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint pt );
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnLButtonUp( UINT nFlags, CPoint pt );
	afx_msg void	OnMouseMove( UINT nFlags, CPoint pt );
	afx_msg void	OnTimer( UINT_PTR nIDEvent );
	afx_msg void	OnKillFocus( CWnd* pNewWnd );
	afx_msg void	OnSetFocus( CWnd* pOldWnd );
	afx_msg HBRUSH	OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void	OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void	OnSize( UINT nType, int cx, int cy );
	afx_msg void	OnGetMinMaxInfo( MINMAXINFO* lpMMI );
	afx_msg void	OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void	OnNcCalcSize( BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp );
	afx_msg int		OnCreate( LPCREATESTRUCT lpcs );
	afx_msg void	OnNotifyParent();
	afx_msg void	OnEditCommit();
	afx_msg void	OnEditUpdate();
	afx_msg void	OnParentNotify( UINT message, LPARAM lParam );
	//}}AFX_MSG
#ifdef LAUNCHER_FIXES
	afx_msg BOOL	OnMouseWheel( UINT nFlags, short zDelta, CPoint pt );
#endif
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CODDriverComboBox window
//
// The video page's renderer picker: each row record is a 160-byte label +
// description pair, and both faces paint the description.

class CODDriverComboBox : public CODComboBox
{
public:
	CODDriverComboBox();

	virtual void	DrawRow( CDC* pDC, int iRow );		// vtbl+188
	virtual void	Paint();							// vtbl+184

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CODColorComboBox window
//
// vftable 0x4B2B84, RTTI ".?AVCODColorComboBox@@" -- the player-customize
// colour picker.  It has no message map of its own; its face is a Paint
// override, not an OnPaint handler.

class CODColorComboBox : public CODComboBox
{
public:
	CODColorComboBox();

	COLORREF	CurrentSwatch();

	virtual void	Paint();							// vtbl+184
	virtual void	DrawRow( CDC* pDC, int iRow );		// vtbl+188
};

COLORREF	Color_NameToRGB( const char* pszName );		// 0x455020

#endif // ODCOMBOBOX_H
