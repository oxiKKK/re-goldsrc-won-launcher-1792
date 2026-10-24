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
// Purpose: declares CODListBox, the owner-draw list box.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODLISTBOX_H
#define ODLISTBOX_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "ODScrollBar.h"

class CODComboBox;		// forward (CODListBox holds the owning combo)

typedef struct odlbitem_s		// 40 bytes per item
{
	int		bPtr;			// +0   0 = use szInline, 1 = use pszText
	char*	pszText;		// +4   caller-owned text (when bPtr)
	char	szInline[32];	// +8   copied text (when !bPtr)
} odlbitem_t;

#define ODLB_MAX_ITEMS	4096

class CODListBox : public CWnd
{
	friend class CODComboBox;	// the drop-list forwards clicks to the list's handlers
public:
	CODListBox();
	virtual ~CODListBox();

	void	InitMembers();
	BOOL	Create( DWORD dwStyle, RECT* prc, CWnd* pParent, UINT nID );

	void	AddString( const char* psz );
	void	AddStringPtr( void* pData );
	void	ResetContent();
	int		GetCount()			{ return m_nCount; }
	const char*	GetText( int i );
	int		FindString( const char* psz );
	int		GetCurSel()			{ return m_curSel; }
	void	SetCurSel( int i );
	int		GetTopIndex()		{ return m_topRow; }
	int		HasScrollbar()		{ return m_bHasScrollbar; }
	CODScrollBar*	GetScrollbar()	{ return m_pScrollbar; }
	void	SetRowHeight( int h );
	int		GetVisibleRows();
	int		ClampTopIndex();
	void	UpdateScrollbar();

	// Keyboard navigation helpers -- each clamps into range, selects, and seeds the
	// scrollbar thumb when the new selection scrolls out of the visible page
	void	NavPageUp();
	void	NavPageDown();
	void	NavLineUp();
	void	NavLineDown();

	COLORREF	m_clrFocusFrame;	// +60   focus frame when focused
	COLORREF	m_clrText;			// +64   normal item text
	COLORREF	m_clrSelText;		// +68   selected item text
	CBrush		m_brSel;			// +72   selected-row fill
	COLORREF	m_clrFrameInactive;	// +80   focus frame when unfocused
	CODComboBox*	m_pOwnerCombo;	// +84   owning combo (collapse, focus, repaint)
	int			m_bTransparent;		// +88   paint over the parent background
	int			m_bAutoDelete;		// +92   OnNcDestroy deletes this when set
	CBrush		m_brBg;				// +96   background fill (black)
	int			m_unk100;			// +100
	CWnd*		m_pParent;			// +104  owning dialog
	CFont		m_font;				// +108  Arial 11/400 item font
	int			m_nCount;			// +116  item count
	int			m_rowHeight;		// +120  one row in pixels (ctor 15)
	int			m_topRow;			// +124  first visible item
	int			m_bHasScrollbar;	// +128  gutter scrollbar present
	int			m_curSel;			// +132  current selection (-1)
	CODScrollBar*	m_pScrollbar;	// +136  companion owner-draw scrollbar
	odlbitem_t*	m_pItems;			// +140  the item store

	void	PaintRow( CDC* pDC, int iItem );

protected:
	afx_msg void	OnNcDestroy();
	afx_msg void	OnPaint();
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint pt );
	afx_msg void	OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnLButtonUp( UINT nFlags, CPoint pt );
	afx_msg void	OnMouseMove( UINT nFlags, CPoint pt );
	afx_msg void	OnSetFocus( CWnd* pOldWnd );
	afx_msg void	OnKillFocus( CWnd* pNewWnd );
	afx_msg void	OnSize( UINT nType, int cx, int cy );
	afx_msg UINT	OnGetDlgCode();
	afx_msg int		OnCreate( LPCREATESTRUCT lpcs );
	afx_msg void	OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	//}}AFX_MSG
#ifdef LAUNCHER_FIXES
	afx_msg BOOL	OnMouseWheel( UINT nFlags, short zDelta, CPoint pt );
#endif
	DECLARE_MESSAGE_MAP()
};

#endif // ODLISTBOX_H
