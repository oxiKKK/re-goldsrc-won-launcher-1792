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
// Purpose: declares CODEdit, the owner-draw rich-text panel.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODEDIT_H
#define ODEDIT_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include <afxcmn.h>

class CODScrollBar;
class CServerInfo;

int __stdcall ODList_CompareRefreshOrder( const CServerInfo* a, const CServerInfo* b, int );

/////////////////////////////////////////////////////////////////////////////
// CODEdit window

class CODEdit : public CWnd
{
// Construction
public:
	CODEdit();

// Attributes
public:
	COLORREF	m_clrBg;			// +60  panel background (RGB 56,56,56)
	int			m_bScrollVisible;	// +64  scrollbar currently shown
	int			m_nLineHeight;		// +68  one text line in pixels (tmHeight-3)
	CFont		m_font;				// +72  Arial 11 text font
	CODScrollBar*	m_pScrollbar;	// +80  scrollbar child (end cap)
	COLORREF	m_clrText;			// +84  text colour (RGB 128,128,128)
	CRichEditCtrl*	m_pRichEdit;	// +88  embedded rich-edit child

// Operations
public:
	void	SetText( const char* psz );
	void	Finalize();

// Implementation
public:
	virtual ~CODEdit();

protected:
	int		VisibleLines();

	// Generated message map functions
	//{{AFX_MSG(CODEdit)
	afx_msg void	OnPaint();
	afx_msg int		OnCreate( LPCREATESTRUCT lpCreateStruct );
	afx_msg void	OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void	OnSize( UINT nType, int cx, int cy );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // ODEDIT_H
