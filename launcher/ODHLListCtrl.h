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
// Purpose: declares CODHLListCtrl, the server-browser report list.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODHLLISTCTRL_H
#define ODHLLISTCTRL_H
#ifdef _WIN32
#pragma once
#endif

#include "ODListCtrl.h"

class CServerInfo;

// The protocol this build speaks; rows on any other one are tinted.
extern int	g_nDefaultProtocol;		// 0x4CF8AC

// Ping -> dot count, from the table at 0x4D0DE8.  [lo,hi] is inclusive.  The
// filter dialog walks the same records for its max-ping choices, so the array
// is shared rather than duplicated.
struct pingband_t
{
	int		flag;
	int		count;
	int		lo;
	int		hi;
};

extern const pingband_t	g_pingBands[];
extern const int		g_numPingBands;

// Column glyphs, in the order LoadGlyphs fills them.
enum
{
	ODGLYPH_FAVORITE = 0,
	ODGLYPH_NONFAV,
	ODGLYPH_WINDOWS,
	ODGLYPH_LINUX,
	ODGLYPH_DEDICATED,
	ODGLYPH_PROXY,
	ODGLYPH_LISTEN,
	ODGLYPH_LOCK,
	ODGLYPH_UNLOCK,
	ODGLYPH_COUNT
};

/////////////////////////////////////////////////////////////////////////////
// CODHLListCtrl window
//
// The Internet/LAN server list: nine columns of glyphs and text over
// CODListCtrl's row pool, with its own right-click context menu.

class CODHLListCtrl : public CODListCtrl
{
// Construction
public:
	CODHLListCtrl( CWnd* pParent, int bOwnsSort );

// Attributes
public:
	CServerInfo*	m_pSelectedSv;	// +1972  right-click / double-click target

protected:
	HGLOBAL		m_hGlyphs[ODGLYPH_COUNT];	// +1908 .. +1940
	int			m_bNumericPing;	// +1944  -numericping cmdline
	int			m_bOwnsSort;	// +1948  the LAN page owns the sort
	CWnd*		m_pParent;		// +1952  host dialog
	int			m_unk1956;		// +1956  =0, set to cx by OnSize
	int			m_unk1960;		// +1960  =0
	COLORREF	m_clrPingLow;	// +1964  ping dots, "fast" colour
	COLORREF	m_clrPingHigh;	// +1968  ping dots, "slow" colour
	int			m_unk1976;		// +1976  =0
	COLORREF	m_clrProxyBg;	// +1980  proxy row text (unselected)
	COLORREF	m_clrProxyText;	// +1984  proxy row text (selected)

// Operations
public:
	CServerInfo*	GetSelectedServer();
	void	InsertRecord( void* pRecord, int iAt );
	void	ResortByRefreshOrder();

// Overrides
public:
	virtual void	DrawRow( CDC* pDC, int iRow );		// CODListCtrl slot 47

// Implementation
public:
	virtual ~CODHLListCtrl();

protected:
	void	LoadGlyphs();
	void	FreeGlyphs();
	void	DrawPingBars( CDC* pDC, const char* pszPing, RECT* prcText, RECT* prcBars );

	// Folded onto the shared empty stub; the focus handlers still call it.
	void	NotifyFocusChanged();

	// Generated message map functions
	//{{AFX_MSG(CODHLListCtrl)
	afx_msg void	OnSize( UINT nType, int cx, int cy );
	afx_msg void	OnRButtonUp( UINT nFlags, CPoint point );
	afx_msg void	OnSetFocus( CWnd* pOldWnd );
	afx_msg void	OnKillFocus( CWnd* pNewWnd );
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void	OnLButtonDblClk( UINT nFlags, CPoint point );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // ODHLLISTCTRL_H
