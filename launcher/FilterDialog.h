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
// Purpose: declares CFilterDialog, the server-browser filter editor.
//
// $NoKeywords: $
//=============================================================================

#ifndef FILTER_DIALOG_H
#define FILTER_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "ODComboBox.h"
#include "ODMenu.h"
#include "BorderlessEdit.h"
#include "resource_dlg.h"

class CServerInfo;

// One entry of the de-duplicated game-name list the dialog builds before it
// fills the game combo.  264 bytes; the link sits after the name.
struct filtergame_t
{
	char			szName[260];
	filtergame_t*	pNext;
};

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog dialog

class CFilterDialog : public CDlgBase
{
// Construction
public:
	CFilterDialog( const char* pszSection, CServerInfo* pServerList, CWnd* pParent = NULL );

// Dialog Data
	//{{AFX_DATA(CFilterDialog)
	enum { IDD = IDD_FILTER };
	// The three value controls are not on the template; OnInitDialog creates
	// them.  A combo is created at its *dropped* height and never moved --
	// MoveWindow afterwards desynchronises ShowDrop's closed-rect compare and
	// it stops opening.
	CODPingComboBox		m_cbPing;			// +224
	CODComboBox			m_cbGame;			// +436
	CBorderLessEdit		m_editMapName;		// +632
	CODBlendCheckBox	m_chkByOS;			// +752   IDC_FILTER_BYOS
	CODBlendCheckBox	m_chkByMap;			// +1056  IDC_FILTER_BYMAP
	CODBlendCheckBox	m_chkDedicated;		// +1360  IDC_FILTER_BYDEDICATED
	CODBlendCheckBox	m_chkByGame;		// +1664  IDC_FILTER_BYGAME
	CODBlendCheckBox	m_chkFavorites;		// +1968  IDC_FILTER_ONFAVORITES
	CODBlendCheckBox	m_chkPing;			// +2272  IDC_FILTER_RESPONSETIME
	CODBlendCheckBox	m_chkResponded;		// +2576  IDC_FILTER_RESPONDING
	CODBlendCheckBox	m_chkNotFull;		// +2880  IDC_FILTER_NOTFULL
	CODBlendCheckBox	m_chkNotEmpty;		// +3184  IDC_FILTER_NOTEMPTY
	CODBlendCheckBox	m_chkIsProxy;		// +3488  IDC_FILTER_ISPROXY
	CODBlendCheckBox	m_chkIsNotProxy;	// +3792  IDC_FILTER_ISNOTPROXY
	CODStatic			m_lblHeading;		// +4096  IDC_FILTER_HEADING
	CODBlendBtn			m_btnCancel;		// +4192  IDCANCEL
	CODBlendBtn			m_btnOK;			// +4432  IDOK
	//}}AFX_DATA

// Attributes
public:
	// The persisted criteria, cached out of the checkboxes -- OnOK writes the
	// profile from these, not from the controls.
	int					m_nFilterPingMax;	// +4672  "Filter PingMax" (def -1)
	int					m_bFilterPing;		// +4676  "Filter Ping"
	int					m_bFilterEmpty;		// +4680  "Filter Empty"
	int					m_bFilterFull;		// +4684  "Filter Full"
	int					m_bFilterResponded;	// +4688  "Filter Responded"
	int					m_bFilterFavorite;	// +4692  "Filter Favorite" (sic: not ctor-zeroed)
	int					m_bFilterGame;		// +4696  "Filter Game"
	int					m_bFilterMap;		// +4700  "Filter Map"
	int					m_bFilterDedicated;	// +4704  "Filter Dedicated"
	int					m_bFilterOS;		// +4708  "Filter OS"
	int					m_bFilterIsProxy;	// +4712  "Filter IsProxy"
	int					m_bFilterIsNotProxy;// +4716  "Filter IsNotProxy"
	char				m_szGameDir[260];	// +4720  selected gamedir (ctor "valve")
	CString				m_strFilterMapName;	// +4980  set by DoDataExchange
	int					m_bFilterAnyProxy;	// +4984  either proxy criterion is on
	const char*			m_pszSection;		// +4988  registry profile section

// Overrides
	//{{AFX_VIRTUAL(CFilterDialog)
	public:
	virtual ~CFilterDialog();
	protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual int		RMLPreIdle();
	virtual void	OnOK();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Size the two skinned buttons from the loaded "head_filter" strip.
	void	SetupHeaderButtons();

	// Add a gamedir to the de-duplicated name list, skipping the engine's own.
	void	AddGameUnique( const char* pszGame );

	CBrush		m_brushStatic;		// +4992  CTLCOLOR_STATIC face
	CBrush		m_brushField;		// +5000  CTLCOLOR_EDIT/_LISTBOX face
	CFont		m_font;				// +5008  Arial -11, weight 400
	// Six ctor-seeded colours; only the middle pair is read back, by OnCtlColor.
	COLORREF	m_clrFieldText;		// +5016
	COLORREF	m_clrFieldBk;		// +5020  m_brushField is made from this
	COLORREF	m_clrText;			// +5024
	COLORREF	m_clrBk;			// +5028
	COLORREF	m_clrStaticText;	// +5032
	COLORREF	m_clrStaticBk;		// +5036  m_brushStatic is made from this
	HGLOBAL		m_headerLoaded;		// +5040  the "head_filter" button strip
	int			m_headerStride;		// +5044
	int			m_headerW;			// +5048  one strip cell
	int			m_headerH;			// +5052
	filtergame_t*	m_pGameNames;	// +5056  head of the game-name list
	CServerInfo*	m_pServerList;	// +5060  the browser's queried servers

	// Generated message map functions
	//{{AFX_MSG(CFilterDialog)
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg HBRUSH	OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg void	OnSelectPing();
	afx_msg void	OnSelectGame();
	afx_msg void	OnIsNotProxy();
	afx_msg void	OnIsProxy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // FILTER_DIALOG_H
