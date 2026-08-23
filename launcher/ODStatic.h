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
// Purpose: declares CODStatic, the self-painted help label.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODSTATIC_H
#define ODSTATIC_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>

/////////////////////////////////////////////////////////////////////////////
// CODStatic window

class CODStatic : public CStatic
{
// Construction
public:
	CODStatic();

// Attributes
public:
	void	SetFontSize( int nSize, int nWeight );
	void	SetWindowText( const char* psz );
	void	SetWindowText( const CString& str );
	void	SetTextColor( COLORREF clr );
	void	SetBgColor( COLORREF clr );
	void	SetTransparent( BOOL bOn );
	void	SetCentered( BOOL bCenter );		// (sic) the argument is ignored
	void	SetOffsets( int cx, int cy );

	CSize		m_szOffsets;	// +60  indent added to the text rect
	CString		strText;		// +68  the drawn text
	BOOL		m_bCenterText;	// +72  DT_CENTER when nonzero
	COLORREF	m_clrBgnd;		// +76  opaque-fill background colour
	BOOL		m_bTransparent;	// +80  paint over the parent background instead
	CFont		m_hStaticFont;	// +84  Arial caption font
	COLORREF	m_clrText;		// +92  text colour

// Implementation
public:
	virtual ~CODStatic();

	// Generated message map functions
protected:
	//{{AFX_MSG(CODStatic)
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnDrawItem( int nIDCtl, LPDRAWITEMSTRUCT lpDIS );
	afx_msg void	OnNcPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif // ODSTATIC_H
