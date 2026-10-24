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
// Purpose: declares CODSlider, the owner-draw horizontal slider.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODSLIDER_H
#define ODSLIDER_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>

/////////////////////////////////////////////////////////////////////////////
// CODSlider window

class CODSlider : public CWnd
{
// Construction
public:
	CODSlider();
	void	InitMembers();				// load slider.bmp + defaults

// Attributes
public:
	void	SetRange( int nMin, int nMax );		// max forced > min
	void	SetPos( int nPos );					// clamps + repaint
	int		GetPos() const;

	int		m_cyThumb;		// +60  slider.bmp height
	int		m_cxThumb;		// +64  slider.bmp width
	HGLOBAL	m_hThumbDib;	// +68  gfx/shell/slider.bmp
	CWnd*	m_pOwner;		// +72  HL_WM_SCROLL target (Create's pParent)
	int		m_nPos;			// +76  (ctor default 50)
	int		m_nMin;			// +80  (0)
	int		m_nMax;			// +84  (100)
	int		m_bHover;		// +88
	int		m_bDragging;	// +92  thumb capture active
	int		m_nDragOffX;	// +96  click x at drag start
	int		m_nDragOffY;	// +100 click y at drag start

// Operations
protected:
	void	GetThumbRect( RECT* prc );
	int		HitTestThumb( POINT* pt );
	void	DrawTrack( CDC* pDC );
	// (sic) the three values are passed but the body reads the members
	void	DrawThumb( CDC* pDC, int nPos, int nMin, int nMax );

// Overrides
	// Registers "CODSliderCls" and CreateEx's over prc, hiding CWnd::Create.
	//{{AFX_VIRTUAL(CODSlider)
	public:
	virtual BOOL	Create( CWnd* pParent, RECT* prc );
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CODSlider();			// frees the thumb DIB

	// Generated message map functions
protected:
	//{{AFX_MSG(CODSlider)
	afx_msg void	OnNcDestroy();
	afx_msg void	OnPaint();
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint pt );
	afx_msg void	OnLButtonUp( UINT nFlags, CPoint pt );
	afx_msg void	OnMouseMove( UINT nFlags, CPoint pt );
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg UINT	OnGetDlgCode();
	afx_msg void	OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void	OnKillFocus( CWnd* pNewWnd );
	afx_msg void	OnSetFocus( CWnd* pOldWnd );
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // ODSLIDER_H
