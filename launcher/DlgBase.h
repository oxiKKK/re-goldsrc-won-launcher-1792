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
// Purpose: declares CDlgBase, the skinned dialog base and its slide animation.
//
// $NoKeywords: $
//=============================================================================

#ifndef DLGBASE_H
#define DLGBASE_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>

#ifdef LAUNCHER_RE
// Not in the original: the reconstruction's top-left build marker.
void	Launcher_DrawBuildMarker( CDC* pDC );

// The same marker over the engine's window.  Call the update every frame while
// the engine owns the screen; it creates, parks, shows and hides the overlay on
// its own.  See the block comment on the definition for what it can and cannot
// reach -- we have no hook into the engine's present, so this is a cooperating
// window, not a composited overlay.
void	Launcher_UpdateGameOverlay( void );
void	Launcher_DestroyGameOverlay( void );
#endif

class CODBlendBtn;

/////////////////////////////////////////////////////////////////////////////
// CDlgBase dialog
//
// vftable 0x4AD7F0 -- every skinned page derives from this.  Slots 52-54 are
// the draw protocol, 55-60 the RunModalLoop frame protocol.

class CDlgBase : public CDialog
{
// Construction
public:
	CDlgBase( UINT nIDTemplate, CWnd* pParent = NULL );

// Operations
public:
	int		RunModalLoop( DWORD dwFlags );
	int		SetFramePump( int bOn );
	void*	LoadHeaderBitmap( const char* pszName, RECT* prcOverride );
	HGLOBAL	FreeHeaderDib();
	void	InitChildDialog( CDlgBase* pChildPage, CODBlendBtn* pSlideBtn );
	void	RestoreAfterModal();
	void	PrepareTransition( int bRestore, RECT* prcTarget,
							   HGLOBAL hHdrDib, CODBlendBtn* pSlideBtn );
	void	RunSlide( int bOpen );
	void	FinishTransition( int bShow, int bForce );

// Overrides
	//{{AFX_VIRTUAL(CDlgBase)
	public:
#if defined(_MSC_VER) && (_MSC_VER < 1300)
	virtual int     DoModal();
#else
	virtual INT_PTR DoModal();
#endif
	virtual BOOL	PreTranslateMessage( MSG* pMsg );
	//}}AFX_VIRTUAL

	// The draw protocol: PaintSkinnedDialog composites the page, calling out to
	// the content and overlay slots each page overrides.
	virtual void	DrawDialogContent( CDC* pDC );
	virtual void	PaintSkinnedDialog();
	virtual void	DrawDialogOverlay( CDC* pDC, RECT* prc );

	// The RunModalLoop frame protocol.
	virtual void	RMLSetup()		{}
	virtual int		RMLPreIdle()	{ return 0; }
	virtual void	RMLIdle()		{}
	virtual void	RMLPrePump()	{}
	virtual void	RMLPump()		{}
	virtual void	RMLPostPump()	{}

// Attributes
public:
	// Fly-in transition context: built by PrepareTransition, consumed by
	// RunSlide, torn down by FinishTransition.
	CDC*		m_pdcParent;		// +92   parent client DC (heap CClientDC)
	CDC*		m_pdcWork;			// +96   per-frame compositing DC
	CDC*		m_pdcSaved;			// +100  saved background DC
	CBitmap*	m_pbmWork;			// +104
	CBitmap*	m_pbmWorkOld;		// +108  previously selected bitmap
	CBitmap*	m_pbmSaved;			// +112
	CBitmap*	m_pbmSavedOld;		// +116
	CWnd*		m_pWndSlide2;		// +120  the window whose image slides
	RECT		m_rcUnion;			// +124  slide rect and target rect combined
	RECT		m_rcHdrDst;			// +140  target (parent header) rect copy
	HGLOBAL		m_hHdrDibDst;		// +156  target header DIB
	RECT		m_rcHdrOwn;			// +160  this page's header rect copy
	HGLOBAL		m_hHdrDibOwn;		// +176  this page's header DIB copy
	int			m_nHdrPad;			// +180  nonzero => skip the header-content draw
	RECT		m_rcHeader;			// +184  this page's header rect
	HGLOBAL		m_hHeaderDib;		// +200  this page's header DIB

	CWnd*		m_pSelfWnd;			// +204  the page itself; the paint sentinel
	CODBlendBtn*	m_pSlideWnd;	// +208  the button the page flies out of
	RECT*		m_prcTargetHdr;		// +212  fly-in target header rect
	HGLOBAL*	m_phTargetHdrDib;	// +216  fly-in target header dib
	BOOL		m_bFramePump;		// +220  keep ticking instead of blocking

// Implementation
public:
	virtual ~CDlgBase();
};

void	Dlg_CenterWindow( CWnd* pDlg );
#ifdef LAUNCHER_FIXES
void	Dlg_ApplyTitleBar( CWnd* pDlg, const char* pszTitle );
int		Dlg_WheelScrollLines( void );
int		Dlg_RouteMouseWheel( MSG* pMsg );
void	Dlg_CenterPopup( CWnd* pDlg, int w, int h );
void	Dlg_SaveWindowPos( CWnd* pDlg );
#endif
void	SetWindowTextSafe( CWnd* pWnd, const char* psz );

#endif // DLGBASE_H
