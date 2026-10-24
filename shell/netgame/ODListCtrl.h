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
// Purpose: declares CODListCtrl, the owner-draw report-list base.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODLISTCTRL_H
#define ODLISTCTRL_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include <afxcmn.h>
#include "ODScrollBar.h"

// One report column handed to CODListCtrl::AddColumn (built on the stack.
typedef struct odcolumn_s
{
	char	title[32];	// [+0]  heading text
	int		width;		// [+32] pixel width (the report paint reads column dword 8)
} odcolumn_t;

// One row in an owner-draw report list -- a 16-byte node out of the control's
// own pool.  There is no per-column text: the record the caller handed AddRow
// lands in `record`, and the per-list DrawRow override formats columns from it.
typedef struct odrow_s
{
	// The pool threads every row through one of two rings (free / live); the
	// links are the first two fields of the record itself.
	struct odrow_s*	pNext;	// +0
	struct odrow_s*	pPrev;	// +4
	void*			record;	// +8   the record AddRow was given, opaque to the base
	int				flags;	// +12  bit0 = selected, bit1 = focused
} odrow_t;

// CODListCtrl -- owner-draw report list base; AddColumn appends one column (sub_44C080)
typedef int ( __stdcall *odrowcmp_t )( const void* a, const void* b, int flags );

// Not a CListCtrl despite the name: the ctor stores CODListCtrl's own vftable
// over a CWnd, the dtor (0x44A680) tail-calls CWnd::~CWnd, Create (0x44A7C0)
// registers "CODListCtrlCls" and goes through CWnd::CreateEx, and the message
// map chains to CWnd's (0x4B45C8).  Everything is painted by hand.
class CODListCtrl : public CWnd
{
public:
	CODListCtrl();
	virtual	~CODListCtrl();

	// Persist this list's column order under its profile entry.
	void	SaveSortOrder();
	void	InitMembers();

	// 0x44A7C0 -- vtable slot 48 in the binary.
	BOOL	Create( DWORD dwStyle, const RECT& rc, CWnd* pParent, UINT nID );

	void	AddColumn( odcolumn_t* pCol );
	void	SetHeaderTransparent( int bOn );
	void	SetDrawFrame( int bOn );
	void	SetTransparent( int bOn );
	void	SetHighlight( COLORREF clr );			// selected-row highlight colour
	virtual void	DrawHeader( CDC* pDC );
	void	SetHeaderFont( int nSize, int cWeight );
	void	SetRowHeight( int h );
	void	SetSortKey( const char* pszReg );

	virtual void	DrawRow( CDC* pDC, int iRow );
	virtual void	AddRow( void* record );
	void	InsertItem( int item, void* data );
	void	DeleteItem( int item );
	int		RowFromPoint( POINT* pt );
	int		ColumnFromPoint( POINT* pt );
	int		GetCellRect( int iRow, int iCol, RECT* prc );
	int		GetItemFlags( int item );
	int		GetCurSel();
	void	ToggleHeader();
	void	SetBorderColor( COLORREF clr );
	void	ResetContent();
	int		GetRowCount();
	int		GetColumnCount();
	virtual void	SelectItem( int item, int bClearOthers );
	void*	GetItemData( int item );
	int		GetVisibleRows();

	// Keyboard scroll navigation (the scrollbar drives the visible window).
	void	ScrollPageUp();
	void	ScrollPageDown();
	void	ScrollLineUp();
	void	ScrollLineDown();

#ifdef LAUNCHER_FIXES
	// Keyboard selection navigation: move the selected row and follow it.
	void	NavSelect( int item );
	void	EnsureVisible( int item );
	void	ActivateSelection();
#endif

	// 0x44C520 -- hit-test a point against row `iRow`.
	int		HitTestCell( int iRow, int x, RECT* prc, int* pCol );

	void	RefitScrollbar();
	void	BeginUpdate( int bBegin, int bEnd );
	void	FreeRow( odrow_t* pRow );
	void	SortRows( odrowcmp_t cmp, int count );

	// The layout, in offset order -- declaration order is what produces it.
	odcolumn_t	m_cols[32];		// +60    column array, 36 bytes each
	HGLOBAL		m_hSortAsc;		// +1212  ascending sort-arrow DIB
	HGLOBAL		m_hSortDesc;	// +1216  descending sort-arrow DIB
	int			m_bSortEnabled;	// +1220  sort indicator active
	char		m_sortKeyName[256];	// +1224  profile value name
	char		m_sortSpec[256];	// +1480  the saved key list ("1;" default)
	int			m_nSortKey;		// +1736  primary sort key (signed 1-based column)
	int			m_redrawSuppress;	// +1740  batch-update latch
	COLORREF	m_clrBg;		// +1744  header fill colour (opaque header only)
	int			m_bDrawFrame;	// +1748  draw the 3px client frame
	int			m_bHdrTransparent;	// +1752  header row shows the parent skin
	COLORREF	m_clrFrame;		// +1756  frame colour, unfocused
	int			m_bTransparent;	// +1760  row area shows the parent skin
	COLORREF	m_clrRowText;	// +1764  normal row text colour
	int			m_unk1768;		// +1768
	COLORREF	m_clrRowBg;		// +1772  row text background colour
	COLORREF	m_clrSelText;	// +1776  selected-row text colour
	COLORREF	m_clrHighlight;	// +1780  selected-row highlight colour
	int			m_unk1784;		// +1784
	COLORREF	m_clrFrameFocus;	// +1788  frame colour, focused
	int			m_nCols;		// +1792  column count
	int			m_headerHeight;	// +1796  header-row height
	int			m_bHeaderVisible;	// +1800  header row shown
	CBrush		m_brBg;			// +1804  row-area fill brush
	CBrush		m_brHighlight;	// +1812  selection-bar brush
	CWnd*		m_pParent;		// +1820  the parent Create was given
	CFont		m_headerFont;	// +1824  header / row font
	int			m_nRows;		// +1832  row count
	int			m_rowHeight;	// +1836  row pixel height
	int			m_topRow;		// +1840  first visible row (scroll offset)
	int			m_bHasScrollbar;	// +1844  vertical scrollbar present
	int			m_curSel;		// +1848  current selected/focus row (-1 = none)
	// Owned by the window, not by this object: Create news it and the bar frees
	// itself from its own OnNcDestroy, so ~CODListCtrl leaves it alone.
	CODScrollBar*	m_pScrollbar;	// +1852  companion owner-draw scrollbar
	odrow_t**	m_rows;			// +1856  array of row records
	int			m_bScrollbarAlways;	// +1860  keep the bar up regardless of row count
	int			m_bWheelScroll;	// +1864  mouse-wheel line scroll enabled
	int			m_nRowsMax;		// +1868  m_rows capacity, doubled by the grow path
	odrow_t		m_freeRows;		// +1872  ring sentinel: unused pool nodes
	odrow_t		m_liveRows;		// +1888  ring sentinel: nodes handed out
	odrow_t*	m_pRowPool;		// +1904  m_nRowsMax contiguous row nodes

	void	InitRowPool();
	odrow_t*	AllocRow();
	void	GrowRows();
	void	LinkRow( odrow_t* pNode, odrow_t* pHead );
	void	UnlinkRow( odrow_t* pNode, odrow_t* pHead );

	// 0x44BE40 -- show/hide the companion bar for the current row count; bForce
	// re-applies the state even when it has not changed.
	void	UpdateScrollbar( int bForce );

	// 0x44C3D0 -- swallow the mouse wheel for line scrolling before the default
	// dispatch when wheel scrolling is enabled.
	virtual BOOL	PreTranslateMessage( MSG* pMsg );

protected:
	afx_msg void	OnNcDestroy();
	afx_msg int		OnCreate( LPCREATESTRUCT lpcs );
	afx_msg void	OnPaint();
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint pt );
	afx_msg void	OnLButtonUp( UINT nFlags, CPoint pt );
	afx_msg void	OnLButtonDblClk( UINT nFlags, CPoint pt );
	afx_msg UINT	OnGetDlgCode();
	afx_msg void	OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void	OnKillFocus( CWnd* pNewWnd );
	afx_msg void	OnSize( UINT nType, int cx, int cy );
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	// 0x44C420 -- base report-list key handler: PageUp/PageDown/Up/Down scroll the
	// view, TAB moves dialog focus, everything else falls through to CWnd::Default.
	afx_msg void	OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
#ifdef LAUNCHER_FIXES
	afx_msg BOOL	OnMouseWheel( UINT nFlags, short zDelta, CPoint pt );
#endif
	DECLARE_MESSAGE_MAP()
};

// Mod chooser list -- adds row height + a persisted sort order.
class CODModListCtrl : public CODListCtrl
{
public:
	// Two-line mod row: 8 liblist columns over a website line.  The row
	// record (odrow_t.text) is the mod_t itself, as in the binary.
	virtual void	DrawRow( CDC* pDC, int iRow );

protected:
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint point );

	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CODSaveGameListCtrl window
//
// The load/save page's list: timestamp, comment and elapsed time per row.

class CODSaveGameListCtrl : public CODListCtrl
{
public:
	virtual void	DrawRow( CDC* pDC, int iRow );

protected:
	//{{AFX_MSG(CODSaveGameListCtrl)
	afx_msg void	OnLButtonDblClk( UINT nFlags, CPoint point );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// 0x44C1D0 -- fit lpString into maxWidth pixels (after an indent), truncating
// with a trailing "..." when it overflows.
const char*	CODList_EllipsizeText( CDC* pDC, const char* lpString, int maxWidth, int indent );

/////////////////////////////////////////////////////////////////////////////
// CODColorPane window
//
// Dead code: it sits in band 57 with no xref, no vftable and no construction
// site, so the binary preserves no name for it -- CODColorPane is ours.  It
// colours an edit child and paints its own background from one brush.

class CODColorPane : public CWnd
{
public:
	CBrush		m_brBg;			// +80  background / edit-control brush
	COLORREF	m_clrText;		// +92  edit text colour

protected:
	//{{AFX_MSG(CODColorPane)
	afx_msg HBRUSH	OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // ODLISTCTRL_H
