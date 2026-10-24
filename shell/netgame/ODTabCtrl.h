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
// Purpose: declares CODTabCtrl, the owner-draw tab strip.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODTABCTRL_H
#define ODTABCTRL_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>

#define MAX_OD_TABS		4096		// the ctor's new[] is 196608 / 48 entries

typedef struct tabentry_s
{
	int			bIsPtr;		// +0   0 = inline szText, 1 = pszPtr
	const char*	pszPtr;		// +4   caller's string when bIsPtr
	char		szText[32];	// +8   inline copy
	int			cxSmall;	// +40  width in the normal (11pt) font + 10
	int			cxBig;		// +44  width in the selected (20pt) font + 10
} tabentry_t;				// 48 bytes

/////////////////////////////////////////////////////////////////////////////
// CODTabCtrl window

class CODTabCtrl : public CWnd
{
// Construction
public:
	CODTabCtrl();
	void	InitMembers();

// Attributes
public:
	void		EnableStackedTabs( int bStacked );
	void		AddTabPtr( const char* pszText );	// stores the pointer, no copy
	void		AddTab( const char* pszText );
	void		SetCurSel( int iTab, int bNotify );
	int			GetCurSel();
	const char*	GetTabText( int iTab );
	int			GetTabCount();

	CFont		m_fontSel;			// +60   -20 Arial 400 (selected tab)
	BYTE		m_pad68[4];			// +68   never touched by the band
	COLORREF	m_clrTextNorm;		// +72   RGB( 255, 180, 24 )
	COLORREF	m_clrTextSel;		// +76   RGB( 255, 255, 255 )
	CBrush		m_brHot;			// +80   RGB( 84, 45, 0 )
	COLORREF	m_clrLine;			// +88   RGB( 127, 127, 127 ) (divider pen)
	int			m_bAutoDelete;		// +92   OnNcDestroy deletes this when set
	CBrush		m_brBg;				// +96   black
	CWnd*		m_pTabParent;		// +104  owner (set in Create)
	CFont		m_fontNorm;			// +108  -11 Arial 400 (unselected tabs)
	int			m_nTabCount;		// +116
	int			m_nCurSel;			// +120  -1 == none
	tabentry_t*	m_pTabs;			// +124
	int			m_bStacked;			// +128

// Operations
protected:
	void	MeasureTab( const char* pszText, int* pcxSmall, int* pcxBig );
	void	GetTabRectSingleRow( int iTab, RECT* prc, int* pbLast, int* pcyText );
	void	GetTabRect( int iTab, RECT* prc, int* pbLast, int* pcyText );

// Overrides
	// Create registers "CODTabCtrlCls" and CreateEx's, hiding CWnd::Create.
	//{{AFX_VIRTUAL(CODTabCtrl)
	public:
	virtual BOOL	Create( DWORD dwStyle, RECT* prc, CWnd* pParent, UINT nID );
	protected:
	virtual void	DrawTab( CDC* pDC, int iTab );
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CODTabCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CODTabCtrl)
	afx_msg void	OnNcDestroy();
	afx_msg void	OnPaint();
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint pt );
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg UINT	OnGetDlgCode();
	afx_msg int		OnCreate( LPCREATESTRUCT lpcs );
	afx_msg void	OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // ODTABCTRL_H
