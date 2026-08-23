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
// Purpose: declares the owner-draw skin buttons: CODBitmapButton, CODBlendBtn,
//          CODBlendCheckBox.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODBUTTON_H
#define ODBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>

// Copy the owning dialog's background under a child control's rect (the shared
// owner-draw "transparent" background fill).  Body in ODButton.cpp.

// btns_main.bmp strip-slice indices -- the `idx` argument to
// CODBlendBtn::SetDIBData (stored in m_states).
enum ButtonStripCell
{
	BTNSTRIP_NEW_GAME			= 0,
	BTNSTRIP_RESUME_GAME		= 1,	// "Resume"/"Return to game"/filter row (same cell)
	BTNSTRIP_HAZARD_COURSE		= 2,
	BTNSTRIP_CONFIGURATION		= 3,
	BTNSTRIP_LOAD_GAME			= 4,
	BTNSTRIP_LOAD_SAVE_GAME		= 5,
	BTNSTRIP_VIEW_README		= 6,
	BTNSTRIP_OK_NARROW			= 7,
	BTNSTRIP_MULTIPLAYER		= 8,
	BTNSTRIP_EASY				= 9,
	BTNSTRIP_MEDIUM				= 10,
	BTNSTRIP_DIFFICULT			= 11,
	BTNSTRIP_ROW_ODD			= 12,
	BTNSTRIP_ROW_EVEN			= 13,
	BTNSTRIP_BACK				= 14,	// back / cancel / previous-page
	BTNSTRIP_VIDEO				= 16,
	BTNSTRIP_AUDIO				= 17,
	BTNSTRIP_CONTROLS			= 18,
	BTNSTRIP_DONE				= 19,	// done / next-page / return / ok (wide)
	BTNSTRIP_QUICK_START		= 20,
	BTNSTRIP_DEFAULTS			= 21,
	BTNSTRIP_OK					= 22,
	BTNSTRIP_OPTIONS			= 23,
	BTNSTRIP_MODES				= 24,
	BTNSTRIP_ADVANCED			= 25,
	BTNSTRIP_LOAD_SAVE			= 27,	// load / save (same-width cell)
	BTNSTRIP_INTERNET_GAMES		= 28,	// internet games / browse
	BTNSTRIP_CHAT_ROOMS			= 29,	// chat rooms / chat
	BTNSTRIP_LAN_GAMES			= 30,	// lan games / lan
	BTNSTRIP_CUSTOMIZE			= 31,
	BTNSTRIP_REFRESH			= 35,
	BTNSTRIP_FILTER				= 36,
	BTNSTRIP_OK_WIDE			= 37,
	BTNSTRIP_CREATE				= 39,
	BTNSTRIP_CREATEROOM			= 41,
	BTNSTRIP_FIND				= 42,
	BTNSTRIP_JOIN				= 44,
	BTNSTRIP_CREATE_GAME		= 46,
	BTNSTRIP_CONNECT			= 47,
	BTNSTRIP_SPECTATE			= 51,
	// The server-info button wears the same strip cell as spectate.
	BTNSTRIP_SERVERINFO			= 51,
	BTNSTRIP_UPDATE				= 52,
	BTNSTRIP_ADDSERVER			= 53,
	BTNSTRIP_DISCONNECT			= 54,	// disconnect / leave
	BTNSTRIP_CONSOLE			= 55,
	BTNSTRIP_GORE				= 56,
	BTNSTRIP_AUTO_UPDATE		= 57,	// "autopatch"
	BTNSTRIP_PREVIEWS			= 59,
	BTNSTRIP_ADVANCED_WIDE		= 60,
	BTNSTRIP_3D_INFO_SITE		= 61,
	BTNSTRIP_CUSTOM_GAME		= 62,
	BTNSTRIP_ACTIVATE			= 63,
	BTNSTRIP_INSTALL			= 64,
	BTNSTRIP_VISIT_MOD_SITE		= 65,
	BTNSTRIP_REFRESH_LIST		= 66,
	BTNSTRIP_DEACTIVATE			= 67,
	BTNSTRIP_SPECTATE_JOIN		= 69,	// the cell the spectate page's Join button wears
	BTNSTRIP_SPECTATE_WIDE		= 70
};

// CODBlendBtn -- an owner-draw menu button (BS_OWNERDRAW).
class CODBlendBtn : public CButton
{
public:
	CODBlendBtn();

	// - creation overrides (force BS_OWNERDRAW) ---
	BOOL			Create( LPCTSTR lpszCaption, DWORD dwStyle, const RECT& rect,
						CWnd* pParentWnd, UINT nID );
	virtual void	PreSubclassWindow();

	// - owner-draw (bodies in ODButton.cpp) ---
	virtual void	DrawItem( LPDRAWITEMSTRUCT lpDIS );
	virtual void	SetFontSize( int nSize, int nWeight );
	virtual void	DrawButtonFace( CDC* pDC, CBitmap* pBmp, RECT* prc,
						int bFocus, int bSelected, int bDisabled );
	void	DrawStripButton( LPDRAWITEMSTRUCT pdis );
	void	DrawStripFace( CDC* pDC, CBitmap* pBmp, RECT* prc, int bSel, int bFocus, int bDis );
	int		BlendSlice( CDC* pDstDC, CBitmap* pBaseBmp, int a4, CDC* pOverlayDC,
				CBitmap* pOverlayBmp, int level );
	void*	BlitStripSlice( CDC* pDstDC, int slice, DWORD rop );
	static void	BlendStates( RECT* prc, CDC* pSrcDC, CDC* pDstDC,
				CBitmap* pOverlay, CBitmap* pBase, int mode );

	// - helpers whose bodies are external in this build (declared so the
	//     reconstructed methods can call them by name) ---
	int		IsHighlighted();
	void	SetHighlight( int bOn );
	void	SetTransparent( BOOL bOn );
	void	DrawDefault();
	int		EnsureSkinLoaded();
	int		EnsureStripSkin();
	int		PrimeBlendBuffers( CDC* pDC, CBitmap* pBmp, RECT* prc );
	// 0x43F110 reads its first argument as a pointer to the {w,h} pair and the
	// call sites push one dword, so the size arrives by reference, not by value.
	void	SetDIBData( const CSize& size, int nIndex, HGLOBAL hDib );
	// Mark the cached skin dirty and drop the two blended bitmaps.
	void	FreeSkinBitmaps();

	void	SetHasArrow( int bOn );
	void	SetTextColor( COLORREF clr );
	void	SetBkColor( COLORREF clr );
	void	SetLeftAlign();
	void	SetRightAlign();

	// - state (binary byte offsets in comments) ---
	int			m_unk60;			// +60   ctor-zeroed; nothing in the band reads it
	HGLOBAL		m_hFaceDib;			// +64   per-button face DIB, freed by the dtor
	int			m_bHighlight;		// +68   forced-highlight (multiplayer-only dim)
	int			m_bTwoBitmap;		// +72   uses the alt overlay/face pair
	int			m_bHasArrow;		// +76   draws an arrow glyph
	int			m_bTransparent;		// +80   shows the parent background
	int			m_textYOffset;		// +84   caption vertical nudge
	UINT		m_textFlags;		// +88   DrawText format
	int			m_cellW;			// +92   strip cell width
	int			m_cellH;			// +96   strip cell height
	int			m_states;			// +100  this button's slice index into the strip
	int			m_bStripMode;		// +104  face comes from the shared strip
	HGLOBAL		m_hStripDib;		// +108  btns_main strip DIB
	CFont		m_shadowFont;		// +112  drop-shadow caption font
	int			m_faceH;			// +120  client height cached by EnsureSkinLoaded
	int			m_faceW;			// +124  client width  cached by EnsureSkinLoaded
	// Cached faces, rendered client-sized by EnsureSkinLoaded/EnsureStripSkin.
	CBitmap		m_bmpOverlay;		// +128  normal face cache / glow overlay
	CBitmap		m_bmpOverlayAlt;	// +136  alt face cache / glow overlay
	int			m_bSkinDirty;		// +144  skin needs (re)loading
	CBitmap		m_bmpFace;			// +148  blended face cache
	COLORREF	m_clrArrowSel;		// +156  focus-bracket colour (pressed)
	COLORREF	m_clrArrowNorm;		// +160  focus-bracket colour (normal)
	COLORREF	m_clrText;			// +164  base text colour
	int			m_bFade;			// +168  mouse-over fade enabled
	float		m_fadeEnd;			// +172  fade end time
	float		m_fadeStart;		// +176  fade start time
	double		m_timeCur;			// +184  current animation time
	double		m_timeStart;		// +192  animation start time
	CFont		m_mainFont;			// +200  caption font
	COLORREF	m_clrDown;			// +208  pressed/disabled text colour
	COLORREF	m_clrBg;			// +212  background colour
	COLORREF	m_clrHover;			// +216  hover text colour
	BYTE*		m_blendBufBase;		// +220  BlendSlice scratch (base)
	BYTE*		m_blendBufOverlay;	// +224  BlendSlice scratch (overlay)
	int			m_blendCapBase;		// +228  scratch capacity (base)
	int			m_blendCapOverlay;	// +232  scratch capacity (overlay)
	int			m_b3State;			// +236  paint all three strip slices

	virtual ~CODBlendBtn();

	// Mouse-over glow.
protected:
	//{{AFX_MSG( CODBlendBtn )
	afx_msg void	OnMouseMove( UINT nFlags, CPoint pt );
	afx_msg void	OnTimer( UINT_PTR nIDEvent );
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnSetFocus( CWnd* pOldWnd );
	afx_msg void	OnDestroy();
	afx_msg UINT	OnGetDlgCode();
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint pt );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// CODBitmapButton -- the skinned close/minimise glyph (three state DIBs).
// RTTI names it, and its base-class array gives CButton: DrawItem is the
// vtbl+184 virtual, not a reflected handler.
class CODBitmapButton : public CButton
{
public:
	CODBitmapButton();
	virtual ~CODBitmapButton();

	// The paging arrows are owner-draw BUTTONs; a STATIC gets no WM_DRAWITEM.
	BOOL	CreateGlyph( DWORD dwStyle, const RECT& rc, CWnd* pParent, UINT nID );
	void	SetSkin( const char* n, const char* d, const char* f );

	int			m_bCapturing;		// +60  mouse captured between down/up
	int			m_bHovering;		// +64  hover tick armed
	int			m_stateIndex;		// +68  current state
	int			m_bSkinLoaded;		// +72  DIBs loaded
	HGLOBAL		m_dibNormal;		// +76  normal DIB
	HGLOBAL		m_dibDown;			// +80  down DIB
	HGLOBAL		m_dibFocus;			// +84  focus DIB

	// The owner-draw glyph painter (vtbl+184).
	virtual void	DrawItem( LPDRAWITEMSTRUCT lpDIS );

protected:
	//{{AFX_MSG( CODBitmapButton )
	afx_msg void	OnLButtonUp( UINT nFlags, CPoint pt );
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint pt );
	afx_msg void	OnTimer( UINT_PTR nIDEvent );
	afx_msg void	OnMouseMove( UINT nFlags, CPoint pt );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// CODBlendStatic -- the hover-glow label that shares band 50.  Its handler set
// is CODBlendBtn's minus everything interactive, and the binary preserves no
// name for it (no GetMessageMap returns its map at 0x4B0E70).
class CODBlendStatic : public CButton
{
public:
	int			m_bFade;		// +68  fade timer armed
	float		m_fadeEnd;		// +72  fade duration
	float		m_fadeStart;	// +76
	double		m_timeCur;		// +80  this tick
	double		m_timeStart;	// +88  last tick the cursor was inside

protected:
	//{{AFX_MSG( CODBlendStatic )
	afx_msg void	OnMouseMove( UINT nFlags, CPoint pt );
	afx_msg void	OnTimer( UINT_PTR nIDEvent );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// CODBlendCheckBox -- the owner-draw check box (ctor 0x442010, vftable 0x4B0DAC,
// RTTI ".?AVCODBlendCheckBox@@" 0x4D0D18, 304 bytes).
class CODBlendCheckBox : public CODBlendBtn
{
public:
	CODBlendCheckBox();
	virtual ~CODBlendCheckBox();

	// Owner-draw entry: paints the glyph + caption.
	virtual void	DrawItem( LPDRAWITEMSTRUCT lpDIS );
	// Swap the caption font (size + weight).
	virtual void	SetFontSize( int nSize, int nWeight );

	int			m_bSkinLoaded;	// +240  glyph DIB cache primed
	int			m_bTransparent;	// +244  paint the parent background (ctor 1)
	int			m_bCapturing;	// +248  mouse captured between down/up
	int			m_bChecked;		// +252  current check state (dialogs read this)
	UINT		m_textFlags;	// +256  DrawText caption format (ctor 32 = DT_SINGLELINE)
	HGLOBAL		m_dibChecked;	// +260  gfx/shell/cb_checked.bmp
	HGLOBAL		m_dibDown;		// +264  gfx/shell/cb_down.bmp
	HGLOBAL		m_dibEmpty;		// +268  gfx/shell/cb_empty.bmp
	HGLOBAL		m_dibOver;		// +272  gfx/shell/cb_over.bmp
	HGLOBAL		m_dibDisabled;	// +276  gfx/shell/cb_disabled.bmp
	COLORREF	m_clrBg;		// +280  opaque background fill (when !m_bTransparent)
	CFont		m_font;			// +284  Arial 12 caption font
	CFont		m_smallFont;	// +292  Arial 11

	int		GetCheck() const	{ return m_bChecked; }
	void	SetCheck( int bChecked );

	void	SetTransparent( BOOL bOn );
	void	SetBkColor( COLORREF clr );

	// Mark the cached skin dirty and drop the two blended bitmaps.
	void	FreeSkinBitmaps();

protected:
	void	EnsureSkin();
	void	FreeSkin();

	afx_msg void	OnLButtonDown( UINT nFlags, CPoint pt );
	afx_msg void	OnLButtonUp( UINT nFlags, CPoint pt );
	DECLARE_MESSAGE_MAP()
};

#endif // ODBUTTON_H
