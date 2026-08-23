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
// Purpose: declares CHLMainDlg, the main-menu dialog (IDD_MAIN).
//
// $NoKeywords: $
//=============================================================================

#ifndef LAUNCHER_DLG_H
#define LAUNCHER_DLG_H
#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#include "DlgBase.h"
#include "ODButton.h"
#include "ODStatic.h"
#include "resource.h"
#include "resource_dlg.h"

/////////////////////////////////////////////////////////////////////////////
// CHLMainDlg dialog

class CHLMainDlg : public CDlgBase
{
// Construction
public:
	CHLMainDlg( CWnd* pParent = NULL );
	virtual ~CHLMainDlg();

// Dialog Data
	//{{AFX_DATA(CHLMainDlg)
	enum { IDD = IDD_MAIN };
	CODBlendBtn     m_btnQuickStart;        // +224
	CODBlendBtn     m_btnChatRooms;         // +464
	CODBlendBtn     m_btnInternetGames;     // +704
	CODBlendBtn     m_btnLanGames;          // +944
	CODStatic       m_lblQuickStart;        // +1184
	CODStatic       m_lblLanGames;          // +1280
	CODStatic       m_lblInternetGames;     // +1376
	CODStatic       m_lblChatRooms;         // +1472
	CODBlendBtn     m_btnFriends;           // +1568
	CODStatic       m_lblCustomHelp;        // +1808
	CODStatic       m_lblPreviewsHelp;      // +1904
	CODBlendBtn     m_btnPreviews;          // +2000
	CODBitmapButton m_stcMin;               // +2240  IDC 1173 (minimise; "min_" skin)
	CODBitmapButton m_stcClose;             // +2328  IDC 1174 (close; "cls_" skin)
	CODStatic       m_lblOrderHelp;         // +2416
	CODBlendBtn     m_btnOrderHalfLife;     // +2512
	CODBlendBtn     m_btnConsole;           // +2752
	CODStatic       m_lblReadmeHelp;        // +2992
	CODStatic       m_lblQuitHelp;          // +3088
	CODStatic       m_lblReturnHelp;        // +3184
	CODStatic       m_lblNewGameHelp;       // +3280
	CODStatic       m_lblTrainingHelp;      // +3376
	CODStatic       m_lblLoadHelp;          // +3472
	CODStatic       m_lblLoadSaveHelp;      // +3568
	CODStatic       m_lblMultiplayerHelp;   // +3664
	CODStatic       m_lblConfigureHelp;     // +3760
	CODStatic       m_lblUnused1;           // +3856 (constructed, not wired)
	CODBlendBtn     m_btnReturnToGame;      // +3952
	CODBlendBtn     m_btnNewGame;           // +4192
	CODBlendBtn     m_btnHazardCourse;      // +4432
	CODBlendBtn     m_btnLoadGame;          // +4672
	CODBlendBtn     m_btnLoadOrSaveGame;    // +4912
	CODBlendBtn     m_btnConfigureHalfLife; // +5152
	CODBlendBtn     m_btnViewReadme;        // +5392
	CODBlendBtn     m_btnOK;                // +5632  IDOK
	CODBlendBtn     m_btnMultiplayer;       // +5872
	CODBlendBtn     m_btnUnused1;           // +6112 (constructed, not wired)
	CODBlendBtn     m_btnCustomGame;        // +6352
	CODBlendBtn     m_btnTfcManual;         // +6592
	CODStatic       m_lblManualHelp;        // +6832
	//}}AFX_DATA

// Attributes
public:
	int             m_bEngineWasActive;     // +6928  engine rendered this frame (cleared in RMLPostPump)
	int             m_bLogoOpened;          // +6932  logo.avi opened
	int             m_bLogoPlaying;         // +6936  logo.avi playing
	int             m_bMenuRefreshPending;  // +6940  re-show the menu page on the next tick
	CAnimateCtrl*   m_pLogoAnim;            // +6944  the menu logo animation control
	BYTE            m_pad6948[4];           // +6948
	int             m_defW;                 // +6952  cached default window width
	int             m_defH;                 // +6956  cached default window height
	BYTE            m_pad6960[4];           // +6960
	HGLOBAL         m_hMenuStrip;           // +6964  btns_main strip DIB (HeaderLoaded)
	int             m_nMenuStripRows;       // +6968  rows in the strip   (HeaderStride)
	// The strip cell size, kept as a pair because RefreshDialogSkin hands its
	// address straight to every SetDIBData call.
	CSize           m_menuCell;             // +6972  strip cell size    (HeaderSize)
	int             m_unk6980;              // +6980  zeroed by the ctor, never read
	int             m_bInGameMenu;          // +6984  last layout was the in-game variant
	int             m_bReady;               // +6988  dialog fully laid out
	DWORD           m_lastActivity;         // +6992  GetTickCount of last page close

// Operations
public:
	void 			RefreshDialogSkin( void );
	void			LayoutMainMenu( int bInGame, int bForce );
	static int		InGame();

// Overrides
	//{{AFX_VIRTUAL(CHLMainDlg)
public:
	virtual int		RMLPreIdle();
	virtual void	RMLPostPump();
	virtual void	OnOK();
	virtual BOOL	PreTranslateMessage( MSG* pMsg );
protected:
	virtual void	DoDataExchange( CDataExchange* pDX );
	virtual BOOL	OnInitDialog();
	virtual void	DrawDialogOverlay( CDC* pDC, RECT* prc );
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CHLMainDlg)
	afx_msg void	OnLaunchModalPage();  									// 1015
	afx_msg LRESULT	OnDisplayChange( WPARAM wParam, LPARAM lParam );
	afx_msg void	OnNewGame();  											// 1016   (CNewGameDlg)
	afx_msg void	OnTraining();  											// 1223
	afx_msg void	OnCreateGame();  											// 1022   (CLoadSaveDlg)
	afx_msg void	OnLoadGame();  											// 1021   (CLoadDlg)
	afx_msg void	OnConfiguration();  										// 1017
	afx_msg void	OnPaint();
	afx_msg void	OnReadmeDialog();  										// 1018
	afx_msg BOOL	OnEraseBkgnd( CDC* pDC );
	afx_msg void	OnActivateApp( BOOL bActive, DWORD dwThreadID );
	afx_msg void	OnMultiplayer();  										// 1019
	afx_msg void	OnActivate( UINT nState, CWnd* pWndOther, BOOL bMin );
	afx_msg void	OnConsole();  											// 1167   (thunk -> OnMultiplayer)
	afx_msg void	OnShowWindow( BOOL bShow, UINT nStatus );
	afx_msg void	OnSysCommand( UINT nID, LPARAM lParam );
	afx_msg void	OnSize( UINT nType, int cx, int cy );
	afx_msg void	OnMinimizeButton();  										// 1173   (title-bar minimise glyph)
	afx_msg void	OnCloseButton();  										// 1174   (title-bar close glyph)
	afx_msg void	OnUnusedButton();  										// 1020
	afx_msg void	OnPreviews();  											// 1024
	afx_msg void	OnSysKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void	OnCustomGame();  											// 1025
	afx_msg void	OnFriends();  											// 1221   (calls the no-op; nothing in this build)
	afx_msg void	OnReportError();  										// 121
	afx_msg void	OnTfcManual();  											// 1029
	//}}AFX_MSG
#ifdef LAUNCHER_FIXES
	afx_msg void	OnExitSizeMove();
#endif

	void	PlayLogo();								// logo.avi playback
	void	CreateLogoAnim();
	void	ShowMenuPage( int bShow );
	void	LaunchGameCmd( int bSetState, int nReserved, const char* pszFmt, ... );
	int		ShowMultiplayerPage( int bSetState, int nReserved );
	void	RefreshAfterConfig();

	static LRESULT CALLBACK	LauncherKeyboardHook( int code, WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};

int		Eng_GetDeferOpenManual( void );
int		Eng_GetDeferRelayout2( void );
int		Eng_GetDeferRelayout1( void );
void	Eng_ClearDeferRelayout1( void );
int		Eng_GetDeferMenuButton( void );

// Launcher_StartEngine( 0 ), as a 0-argument wrapper.
void	Launcher_StartEngineFg( void );

int		Launcher_AppOwnsForeground( void );  							// (foreground owned by launcher/engine?)
void	Launcher_SyncEngineWindow( CWnd* pDlg );  							// (sync engine window while in-game)
HWND	Launcher_CreateEngineWindow( HWND hParent, HINSTANCE hInst );

extern int	g_bConsoleMode;	// (4E2154) -console
extern int	g_bNoPrompt;	// (4E2158) -noprompt

extern HHOOK	g_hKeyboardHook;

extern int		g_bNoFly;	// (4E214C) menu fly-in animation disabled

#endif // LAUNCHER_DLG_H
