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
// Purpose: declares CODScrollBar, the owner-draw vertical scrollbar.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODSCROLLBAR_H
#define ODSCROLLBAR_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>

/////////////////////////////////////////////////////////////////////////////
// CODScrollBar window

class CODScrollBar : public CWnd
{
// Construction
public:
	CODScrollBar();
	void	InitMembers();						// loads the 7 DIBs + defaults

// Attributes
public:
	void	SetRange( int nMin, int nMax );
	void	SetPos( int nPos );
	void	SetRowHeight( int nRowHeight );		// repage from the client height
	int		GetPos();
#ifdef LAUNCHER_FIXES
	// The highest position that still moves the owner: m_nMax less one page.
	int		ScrollMax();
#endif

	void	GetUpArrowRect( RECT* prc );
	void	GetDnArrowRect( RECT* prc );
	void	GetTrackRect( RECT* prc );			// gutter between the caps
	void	GetThumbRect( RECT* prc );

	// OnPaint orchestrates these into a double buffer.
	void	DrawUpArrow( CDC* pDC, int bPressed, int bHover );
	void	DrawDnArrow( CDC* pDC, int bPressed, int bHover );
	void	DrawTrack( CDC* pDC, int bActive );
	// (sic) the three range values are passed but the body reads the members
	void	DrawThumb( CDC* pDC, int nPos, int nMin, int nMax,
					   int bHover, int bActive );

	// Clamp, repaint, then notify the owner via WM_VSCROLL.
	void	LineUp();
	void	LineDown();

	int			m_nPageRows;		// +60   rows per page (Create / SetRowHeight)
	int			m_nRowHeight;		// +64   one row in pixels
	int			m_bUseParentCapture;	// +68   capture through the parent list (Create)
	int			m_nPageDir;			// +72   page direction latched for timer 3
	int			m_bActive;			// +76   gutter-page repeat active
	int			m_nDragStartPos;	// +80   m_nPos captured at drag start
	int			m_ptDragStartX;		// +84   cursor x at drag start
	int			m_ptDragStartY;		// +88   cursor y at drag start
	HGLOBAL		m_hThumb;			// +92   gfx/shell/thumb.bmp
	HGLOBAL		m_hUpArrowF;		// +96   uparrowf.bmp (hover)
	HGLOBAL		m_hDnArrowF;		// +100  dnarrowf.bmp
	HGLOBAL		m_hUpArrowD;		// +104  uparrowd.bmp (default)
	HGLOBAL		m_hDnArrowD;		// +108  dnarrowd.bmp
	HGLOBAL		m_hDnArrowP;		// +112  dnarrowp.bmp (pressed)
	HGLOBAL		m_hUpArrowP;		// +116  uparrowp.bmp
	CWnd*		m_pOwner;			// +120  the list this bar scrolls
	int			m_nPos;				// +124  (ctor default 50)
	int			m_nMin;				// +128  (0)
	int			m_nMax;				// +132  (100)
	int			m_bEnabled;			// +136  (ctor default 1)
	int			m_bHoverTimer;		// +140  hover-leave watchdog running (id 2)
	int			m_bDragging;		// +144  thumb drag in progress
	int			m_bArrowRepeat;		// +148  arrow auto-repeat active (timer id 1)
	int			m_nArrowDir;		// +152  arrow repeat direction (+1 up, -1 down)

// Operations
protected:
	void	Page( BOOL bUp );					// page by m_nPageRows

	BOOL	HitTestThumb( POINT* ppt );			// thumb, inflated to bar width
	BOOL	HitTestUpArrow( POINT* ppt );
	BOOL	HitTestDnArrow( POINT* ppt );
	BOOL	HitTestGutter( POINT* ppt, int* pbPageUp );

	// Capture follows the parent list when this bar is its child decoration.
	void	SetScrollCapture();
	BOOL	ReleaseScrollCapture();
	BOOL	HasScrollCapture();

// Overrides
	//{{AFX_VIRTUAL(CODScrollBar)
	public:
	virtual BOOL	Create( CWnd* pParent, RECT* prc, int nRowHeight );
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CODScrollBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CODScrollBar)
	afx_msg void	OnNcDestroy();
	afx_msg void	OnPaint();
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint pt );
	afx_msg void	OnLButtonUp( UINT nFlags, CPoint pt );
	afx_msg void	OnMouseMove( UINT nFlags, CPoint pt );
	afx_msg void	OnTimer( UINT_PTR nIDEvent );
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnSetFocus( CWnd* pOldWnd );
	//}}AFX_MSG
#ifdef LAUNCHER_FIXES
	afx_msg BOOL	OnMouseWheel( UINT nFlags, short zDelta, CPoint pt );
#endif

	DECLARE_MESSAGE_MAP()
};

#endif // ODSCROLLBAR_H
