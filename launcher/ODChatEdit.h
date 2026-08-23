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
// Purpose: declares CODChatEdit, the chat transcript control.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODCHATEDIT_H
#define ODCHATEDIT_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>

#define ODCHAT_MAX_LINES			4095
#define ODCHAT_RING_SLOTS			4096
#define ODCHAT_LINE_CONTINUATION	0x80000000

class CODScrollBar;
class CODChatEdit;

void	ChatWnd_Printf( CODChatEdit* pWnd, const char* pszNick, const char* pszFormat, ... );

typedef struct odchatline_s		// 24 bytes; one ring-buffer entry
{
	int			flags;			// +0  bit31 = continuation (wrapped) line
	char*		text;			// +4  wrapped display text (heap)
	COLORREF	bodyColor;		// +8  body text colour
	char*		raw;			// +12 full unwrapped source text (heap)
	COLORREF	nameColor;		// +16 "<nick>" prefix colour
	int			prefixWidth;	// +20 pixel width reserved for the "<nick>" tag
} odchatline_t;

/////////////////////////////////////////////////////////////////////////////
// CODChatEdit window
//
// vftable 0x4B101C -- the chat transcript pane; the companion CODScrollBar
// owns the scroll offset and this control adopts it in OnPaint.

class CODChatEdit : public CWnd
{
// Construction
public:
	CODChatEdit();

	void	InitMembers();
	BOOL	Create( DWORD dwStyle, RECT* prc, CWnd* pParent, UINT nID );

// Attributes
public:
	COLORREF	m_clrText;			// +60   default text
	int			m_unk64;			// +64
	char		m_szWhisperTarget[128];	// +68   whisper "-> target" name
	COLORREF	m_clrWhisper;		// +196  whisper text
	int			m_bWhisper;			// +200  next line is a whisper
	COLORREF	m_clrName;			// +204  self speaker
	COLORREF	m_clrOther;			// +208  other speaker
	char		m_szSelfNick[128];	// +212  own nick (self-vs-other colorize)
	BYTE		m_pad340[132];		// +340
	COLORREF	m_clrBorder;		// +472  normal border
	CBrush		m_brBack;			// +476  brown accent brush -- not the pane fill
	COLORREF	m_clrFrame;			// +484  unfocused frame
	int			m_bAutoDelete;		// +488  OnNcDestroy deletes this when set
	CBrush		m_brBlack;			// +492  pane background brush
	CWnd*		m_pChatOwner;		// +500  parent dialog (set by Create)
	CFont		m_font;				// +504  Arial -11/400
	int			m_nLines;			// +512  lines currently in the ring
	int			m_rowHeight;		// +516  line height (15)
	int			m_topLine;			// +520  first visible line
	int			m_bHasScrollbar;	// +524  16px gutter present
	int			m_curLine;			// +528  current/last-touched line (-1 = none)
	CODScrollBar*	m_pScrollbar;	// +532  companion scrollbar
	odchatline_t*	m_lines;		// +536  4096-entry ring
	COLORREF	m_clrFocus;			// +540  focus border

// Operations
public:
	void	AddChatLine( int, const char* pszSpeaker, const char* pszText );
	void	SetSelfNick( const char* pszNick );
	void	SetWhisperTarget( const char* pszTarget );
	void	AppendLine( const odchatline_t* pLine );
	void	ClearAllLines();
	void	ResetContent();
	void	SetCurSel( int iLine );
	int		GetCurSel();
	int		GetLineCount();
	int		GetTopLine();
	int		GetVisibleRows();
	void	DrawLine( CDC* pMemDC, int iLine );

	// Keyboard scroll navigation (the selection leads, the bar follows).
	void	ScrollPageUp();
	void	ScrollPageDown();
	void	ScrollLineUp();
	void	ScrollLineDown();

	void	SyncTopLine();

	// Show/hide the companion scrollbar when the ring overflows the visible
	// rows, then keep the thumb pinned to the bottom.
	void	UpdateScrollbarVisibility();
	BOOL	IsScrolledToBottom();

// Implementation
public:
	virtual ~CODChatEdit();

protected:
	// Generated message map functions
	//{{AFX_MSG(CODChatEdit)
	afx_msg void	OnNcDestroy();
	afx_msg void	OnPaint();
	afx_msg void	OnLButtonDown( UINT nFlags, CPoint pt );
	afx_msg void	OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pSB );
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnLButtonUp( UINT nFlags, CPoint pt );
	afx_msg void	OnMouseMove( UINT nFlags, CPoint pt );
	afx_msg void	OnSize( UINT nType, int cx, int cy );
	afx_msg UINT	OnGetDlgCode();
	afx_msg int		OnCreate( LPCREATESTRUCT lpcs );
	afx_msg void	OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // ODCHATEDIT_H
