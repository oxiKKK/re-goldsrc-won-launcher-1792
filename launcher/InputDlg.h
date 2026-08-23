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
// Purpose: declares CInputDlg, the skinned modal text-entry dialog (IDD 200).
//
// $NoKeywords: $
//=============================================================================

#ifndef INPUT_DLG_H
#define INPUT_DLG_H

#include "DlgBase.h"
#include "DlgPopupBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "BorderlessEdit.h"
#include "resource_dlg.h"

class CInputDlg : public CDlgPopupBase
{
public:
	CInputDlg( CWnd* pParent = NULL );
	virtual ~CInputDlg();

	void	SetPrompt( const char* psz );		// > m_strPrompt
	void	SetPasswordMode( int bOn );		// > m_bPasswordMode

	enum { IDD = IDD_INPUT };							// 0xC8

	// - typed members (binary byte offsets in comments) ---
	CInputEdit*	m_pInput;		// +100  entry field (child id 103), built in OnInitDialog
	CString		m_strPrompt;	// +104  prompt/message text (set by SetPrompt, before DoModal)
	CString		m_strInput;		// +108  text the user entered (filled by DoDataExchange)
	CODBlendBtn	m_btnOK;		// +112  strip slice 22, caption string id 0x118
	CODBlendBtn	m_btnCancel;	// +352  strip slice 14, caption string id 0xFB
	CODStatic	m_promptLabel;	// +592  the prompt label
	int			m_bPasswordMode;// +688  echo the entry field as dots (set by SetPasswordMode)

protected:
	void	InitMembers();		// (bind OK/Cancel to the button strip)

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual int  RMLPreIdle();		// per-frame engine pump (frame-protocol override)

	// Both paint entries repaint the popup panel and then re-stamp the prompt
	// label; the label owns the text, so it has to be refreshed with it.
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );

	DECLARE_MESSAGE_MAP()
};

#endif // INPUT_DLG_H
