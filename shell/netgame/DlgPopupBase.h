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
// Purpose: declares CDlgPopupBase, the base for the skinned popup dialogs.
//
// $NoKeywords: $
//=============================================================================

#ifndef DLGPOPUPBASE_H
#define DLGPOPUPBASE_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>

/////////////////////////////////////////////////////////////////////////////
// CDlgPopupBase dialog
//
// vftable 0x4ADA1C -- the popups do not derive from CDlgBase; slot 52 is the
// painter each derived popup's WM_PAINT thunk dispatches into, and 54-59 are
// the frame protocol, the CDlgBase one with no VGui_Frame step.

class CDlgPopupBase : public CDialog
{
// Construction
public:
	CDlgPopupBase( UINT nIDTemplate, CWnd* pParent = NULL );

// Operations
public:
	int		RunModalLoop( DWORD dwFlags );

	// CDlgPopupBase::SetModalProgressPopup (0x40BDE0)
	void	SetModalProgressPopup( DWORD dwValue )	{ m_bModalProgressPopup = dwValue; }

	// Each popup stamps itself here; OnPaint no-ops until it is set.
	void	SetPaintWnd( CWnd* pWnd )				{ m_pPaintWnd = pWnd; }

// Overrides
	//{{AFX_VIRTUAL(CDlgPopupBase)
	public:
#if defined(_MSC_VER) && (_MSC_VER < 1300)
	virtual int     DoModal();
#else
	virtual INT_PTR DoModal();
#endif
	virtual BOOL	PreTranslateMessage( MSG* pMsg );
	//}}AFX_VIRTUAL

	virtual void	OnPaint();
	virtual void	DrawPopupContent( CDC* pDC, RECT* prc )	{}

	// The popup's frame protocol.
	virtual void	RMLSetup()		{}
	virtual int		RMLPreIdle()	{ return 0; }
	virtual void	RMLIdle()		{}
	virtual void	RMLPrePump()	{}
	virtual void	RMLPump()		{}
	virtual void	RMLPostPump()	{}

// Attributes
protected:
	CWnd*	m_pPaintWnd;			// +92  window CPaintDC binds to
	DWORD	m_bModalProgressPopup;	// +96  keep ticking instead of blocking

// Implementation
public:
	virtual ~CDlgPopupBase();

protected:
	// Shared per-frame engine pump, wired through each popup's timer thunk.
	int		OnEngineFrame();
};

#endif // DLGPOPUPBASE_H
