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
// Purpose: declares CBorderLessEdit and CInputEdit, the frameless skinned
//          edit controls.
//
// $NoKeywords: $
//=============================================================================

#ifndef BORDERLESSEDIT_H
#define BORDERLESSEDIT_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>

/////////////////////////////////////////////////////////////////////////////
// CBorderLessEdit window

class CBorderLessEdit : public CWnd
{
// Construction
public:
	CBorderLessEdit();

// Attributes
public:
	UINT		m_nID;				// +60  control id, stored by Create
	int			m_bActive;			// +64  gates focus forwarding to the inner edit
	COLORREF	m_clrEditBk;		// +68  inner-edit background colour (ctor 0 = black)
	COLORREF	m_clrBorder;		// +72  border colour without focus (ctor 0; owners poke 56,56,56)
	COLORREF	m_clrFocus;			// +76  border colour with focus (ctor 128,128,128)
	CBrush		m_brBack;			// +80  inner-edit background brush (black)
	COLORREF	m_clrEditText;		// +88  inner-edit text colour (240,127,24)
	CEdit*		m_pEdit;			// +92  embedded edit child
	int			m_bPassword;		// +96  -> ES_PASSWORD
	int			m_bAutoHScroll;		// +100 -> ES_AUTOHSCROLL
	BYTE		m_pad104[4];		// +104
	CFont		m_font;				// +108 Arial -12, weight 400

// Operations
public:
	void	SetBorderColor( COLORREF clr );
	void	SetEditTextColor( COLORREF clr );
	void	SetActive( int bActive );
	void	SetText( const char* psz );
	void	SetAutoHScroll();
	void	SetPasswordMode();

// Overrides
	//{{AFX_VIRTUAL(CBorderLessEdit)
	public:
	virtual BOOL	Create( DWORD dwStyle, RECT* prc, CWnd* pParent, UINT nID );
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBorderLessEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CBorderLessEdit)
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg HBRUSH	OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void	OnSetFocus( CWnd* pOldWnd );
	afx_msg void	OnKillFocus( CWnd* pNewWnd );
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void	OnChar( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void	OnSize( UINT nType, int cx, int cy );
	afx_msg void	OnEditFocusChanged();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CInputEdit window
//
// vftable 0x4AE93C -- a CBorderLessEdit bound to a userinfo key (the
// player-setup dialog uses it).

class CInputEdit : public CBorderLessEdit
{
public:
	CInputEdit( CWnd* pOwner = NULL );
	virtual ~CInputEdit();

// Overrides
	//{{AFX_VIRTUAL(CInputEdit)
	public:
	virtual BOOL	PreTranslateMessage( MSG* pMsg );
	//}}AFX_VIRTUAL

	CWnd*	m_pOwnerDlg;		// +116  the dialog whose IDOK ENTER fires

	DECLARE_MESSAGE_MAP()
};

#endif // BORDERLESSEDIT_H
