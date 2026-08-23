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
// Purpose: declares CPromptDlg, the launcher's skinned message / prompt
//          dialog.
//
// $NoKeywords: $
//=============================================================================

#ifndef PROMPT_DLG_H
#define PROMPT_DLG_H

#include "ODButton.h"
#include "DlgPopupBase.h"
#include "launcher.h"
#include "strings.h"
#include "resource_dlg.h"

class CPromptDlg : public CDlgPopupBase
{
public:
	CPromptDlg( int nStyle, CWnd* pParent = NULL );
	virtual ~CPromptDlg();

	void	SetMessage( const char* fmt, ... );		// > m_message

	int		SetPromptSize( int w, int h );		// > m_promptW/m_promptH
	int		SetTextAlign( int nAlign );

	void	SetMessageFont( int nHeight, int nWeight );		// (Arial, -nHeight)
	int		SetCheckboxShown( int bShown );
	int		IsCheckboxChecked();
	void	SetCheckboxText( const char* psz );
	void	SetTitle( const char* psz );		// > m_szTitle

	enum { IDD = IDD_PROMPT };									// 0xB7

	// - typed members (binary byte offsets in comments) ---
	CODBlendCheckBox	m_checkDontAsk;	// +104  IDC 1043, the "don't ask again" switch
	CODBlendBtn	m_btnOK;		// +408  strip slice 22, caption string id 0x118
	CODBlendBtn	m_btnCancel;	// +648  strip slice 14, caption string id 0xFB
	int			m_nTextAlign;	// +888  DrawText alignment; DT_CENTER by default
	int			m_promptH;		// +892  prompt height (ctor: StringHeight(0x1E8) + 160)
	int			m_promptW;		// +896  prompt width  (ctor default 320)
	int			m_nStyle;		// +900  bit0 => single OK button, else OK + Cancel
	CString		m_message;		// +904  the (formatted) prompt text (owner-drawn)
	CFont		m_msgFont;		// +912  message font (Arial, weight 900)
	COLORREF	m_clrTitle;		// +920  PROMPT_TITLE_COLOR
	CFont		m_titleFont;	// +924  title font (Arial -18, weight 900)
	int			m_bCheckboxShown;		// +952 "don't ask again" state
	char		m_szCheckboxText[512];		// +956 its caption
	char		m_szTitle[64];	// +1468 title text (drawn when m_nStyle < 0)
	COLORREF	m_clrText;		// +908  PROMPT_TEXT_COLOR

protected:

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual int  RMLPreIdle();		// per-frame engine pump (frame-protocol override)
	virtual void DrawPopupContent( CDC* pDC, RECT* prc );		// (slot 54)

	afx_msg void	OnCheckbox();
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );

	HGLOBAL		m_headerLoaded;	// +932
	int			m_headerStride;	// +936
	int			m_headerW;		// +940
	int			m_headerH;		// +944

	DECLARE_MESSAGE_MAP()
};

#endif // PROMPT_DLG_H
