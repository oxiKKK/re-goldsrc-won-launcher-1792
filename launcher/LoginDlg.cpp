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
// Purpose: CLoginDlg, the WON login dialog.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The status text is formatted through this one buffer, so SetStatusLine can be
// called with the line it is about to overwrite.
static char		s_szLoginStatus[256];

// Throttle stamp for the "%.1f s. remaining" countdown line.
static double	s_flLastCountdown;

// Entries at 0x4AF7F0, base map 0x4B4398 = CDialog.
BEGIN_MESSAGE_MAP( CLoginDlg, CDialog )
	//{{AFX_MSG_MAP(CLoginDlg)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::CLoginDlg (0x427480)

CLoginDlg::CLoginDlg( CNetGameDlg* pNetGame, CWnd* pParent )
	: CDlgPopupBase( IDD_LOGIN, pParent )
{
	SetPaintWnd( this );
	m_nState = 2;

	// Touch the master lists so the connect task has them ready; the walk's
	// result is discarded here.
	gFavorites->BeginMasterList();
	gFavorites->NextMasterList();

	m_bDone         = 0;
	m_nConnectStage = 0;
	m_pNetGame      = pNetGame;

	m_brush.Attach( CreateSolidBrush( RGB( 0, 0, 0 ) ) );

	int	wh[2];

	m_hStripBmp    = Launcher_HeaderLoaded();
	Launcher_HeaderSize( wh );
	m_nStripWidth  = wh[0];
	m_nStripHeight = wh[1];
	m_nStripCount  = Launcher_HeaderStride();

	if ( m_hStripBmp )
		m_btnCancel.SetDIBData( CSize( wh[0], wh[1] ), BTNSTRIP_BACK, m_hStripBmp );

	SetModalProgressPopup( 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::~CLoginDlg (0x4275E0)

CLoginDlg::~CLoginDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::DoDataExchange (0x427680)

void CLoginDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_LOGIN_LINE_LOWER, m_lblLine1 );
	DDX_Control( pDX, IDC_LOGIN_LINE_UPPER, m_lblLine2 );
	DDX_Control( pDX, IDC_LOGIN_TITLE,  m_lblLogin );
	DDX_Control( pDX, IDCANCEL,              m_btnCancel );
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::OnInitDialog (0x4276e0)
//
// Fixed 300x170 popup, centred on the screen rather than on the launcher: the
// caption band on top, two status lines under it, Cancel bottom-right.

BOOL CLoginDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetStatusLine( "" );
	m_lblLine1.SetWindowText( "" );

	MoveWindow( 0, 0, 300, 170, FALSE );

	RECT	rcWnd;
	GetWindowRect( &rcWnd );

	int	w = rcWnd.right - rcWnd.left;
	int	h = rcWnd.bottom - rcWnd.top;

#ifdef LAUNCHER_FIXES
	Dlg_CenterPopup( this, w, h );
#else
	MoveWindow( ( GetSystemMetrics( SM_CXSCREEN ) - w ) / 2,
				( GetSystemMetrics( SM_CYSCREEN ) - h ) / 2, w, h, TRUE );
#endif

	RECT	rcClient;
	GetClientRect( &rcClient );

	// WON login caption -- centred, as tall as the locale's wrap needs.
	int		nLines = Launcher_StringHeight( IDS_GERMAN, 0 );
	RECT	rc;

	rc.left   = 10;
	rc.top    = 10;
	rc.right  = rcClient.right - rcClient.left - 10;
	rc.bottom = 20 * ( nLines + 2 );
	m_lblLogin.MoveWindow( 10, 10, rc.right - 10, rc.bottom - 10, TRUE );
	m_lblLogin.SetTextColor( RGB( 240, 180, 56 ) );
	m_lblLogin.SetBgColor( RGB( 56, 56, 56 ) );
	m_lblLogin.SetTransparent( FALSE );
	m_lblLogin.SetFontSize( 18, FW_HEAVY );
	m_lblLogin.SetCentered( TRUE );
	m_lblLogin.SetWindowText( Launcher_LoadString( IDS_WON_LOGIN ) );

	// First status line (white, 40px tall).
	int	nH = Launcher_StringHeight( IDS_GERMAN, 0 );

	rc.top    = 20 * nH + 50;
	rc.bottom = 20 * nH + 90;
	m_lblLine2.MoveWindow( rc.left, rc.top, rc.right - rc.left, 40, TRUE );
	m_lblLine2.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblLine2.SetBgColor( RGB( 56, 56, 56 ) );
	m_lblLine2.SetTransparent( FALSE );
	m_lblLine2.SetFontSize( 12, FW_NORMAL );

	// Second status line, stacked under the first (20px tall).
	OffsetRect( &rc, 0, rc.bottom - rc.top );
	rc.bottom = rc.top + 20;
	m_lblLine1.MoveWindow( rc.left, rc.top, rc.right - rc.left, 20, TRUE );
	m_lblLine1.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblLine1.SetBgColor( RGB( 56, 56, 56 ) );
	m_lblLine1.SetTransparent( FALSE );
	m_lblLine1.SetFontSize( 12, FW_NORMAL );

	// Cancel button, bottom-right, one strip cell tall.  The strip is measured
	// twice: once for the top edge, once for the height.
	int	whTop[2];
	int	whBtn[2];

	Launcher_HeaderSize( whTop );
	Launcher_HeaderSize( whBtn );

	rc.right  = rcClient.right - rcClient.left - 10;
	rc.bottom = rcClient.bottom - rcClient.top - 10;
	rc.left   = rcClient.right - rcClient.left - 110;
	rc.top    = rcClient.bottom - rcClient.top - whTop[1] - 10;
	m_btnCancel.MoveWindow( rc.left, rc.top, 100, whBtn[1], TRUE );
	m_btnCancel.SetTransparent( FALSE );
	m_btnCancel.SetHasArrow( 0 );
	m_btnCancel.SetBkColor( RGB( 56, 56, 56 ) );
	m_btnCancel.SetTextColor( RGB( 255, 180, 0 ) );
	SetWindowTextSafe( &m_btnCancel, Launcher_LoadString( IDS_BTN_CANCEL ) );

	m_flStartTime = engineapi.Sys_FloatTime();
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::OnCtlColor (0x4279d0)

HBRUSH CLoginDlg::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH	hbr = CDialog::OnCtlColor( pDC, pWnd, nCtlColor );

	if ( nCtlColor <= CTLCOLOR_EDIT )
	{
		pDC->SetTextColor( RGB( 255, 127, 24 ) );
		pDC->SetBkMode( TRANSPARENT );
		pDC->SetBkColor( 0 );
		return (HBRUSH)m_brush.GetSafeHandle();
	}

	return hbr;
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::RMLPreIdle (0x427a30)
//
// The popup modal loop's per-pass hook: run one engine frame, then drive the
// master-list fetch.

int CLoginDlg::RMLPreIdle()
{
	Eng_Frame( gBackground );

	double	flTime = engineapi.Sys_FloatTime();

	if ( m_bDone )
	{
		// Only flash the banner when the whole login fitted inside a second --
		// after a long wait the user has read enough status text already.
		if ( flTime - m_flStartTime < 1.0 )
		{
			SetStatusLine( "Login successful." );
			Sleep( 100 );
		}
		g_bWonLoginRequired = 0;
		OnOK();
		return 0;
	}

	PollConnect();

	if ( flTime - m_flRequestTime <= 4.0 )
	{
		if ( flTime - s_flLastCountdown > 0.2 )
		{
			s_flLastCountdown = flTime;

			float	flLeft = (float)( 4.0 - ( flTime - m_flRequestTime ) );
			if ( flLeft < 0.0f )
				flLeft = 0.0f;

			char	szLine[256];
			sprintf( szLine, "%.1f s. remaining", flLeft );
			m_lblLine1.SetWindowText( szLine );
		}
		return 0;
	}

	// The host went quiet: two goes at each master, then on to the next one.
	if ( --m_nState > 0 )
	{
		if ( m_nConnectStage == 1 )
		{
			m_pNetGame->ConnectMaster( gFavorites->GetMasterAddr(), gFavorites->GetMasterPort() );
			m_nConnectStage = 1;
			m_flRequestTime = engineapi.Sys_FloatTime();
			SetStatusLine( "Retrying request from %s:%i.",
				gFavorites->GetMasterAddr(), gFavorites->GetMasterPort() );
		}
		return 0;
	}

	if ( gFavorites->NextMasterList() )
	{
		m_flRequestTime = engineapi.Sys_FloatTime();
		m_nConnectStage = 0;
		m_nState        = 2;
	}
	else
	{
		OnOK();		// out of masters -- give up and let the browser open
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::PollConnect (0x427c20)

void CLoginDlg::PollConnect()
{
	if ( m_bDone )
		return;

	if ( !m_nConnectStage )
	{
		m_pNetGame->ConnectMaster( gFavorites->GetMasterAddr(), gFavorites->GetMasterPort() );
		m_nConnectStage = 1;
		m_flRequestTime = engineapi.Sys_FloatTime();
		SetStatusLine( "Requesting server data from %s:%i.",
			gFavorites->GetMasterAddr(), gFavorites->GetMasterPort() );
		return;
	}

	if ( m_nConnectStage == 1 && m_pNetGame->IsDirty() )
	{
		SetStatusLine( "Received server list." );
		Sleep( 200 );
		m_bDone = 1;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::OnCancel (0x427ce0)
//
// Ends the dialog with IDOK so the browser still opens; the latch stops
// RMLPreIdle from re-issuing the request.

void CLoginDlg::OnCancel()
{
	m_bDone = 1;
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::OnOK (0x427cf0)

void CLoginDlg::OnOK()
{
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::SetStatusLine (0x427d00)

void CLoginDlg::SetStatusLine( const char* pszFormat, ... )
{
	va_list	args;

	va_start( args, pszFormat );
	vsprintf( s_szLoginStatus, pszFormat, args );
	va_end( args );

	m_lblLine2.SetWindowText( s_szLoginStatus );
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::OnActivateApp (0x406fe0)

void CLoginDlg::OnActivateApp( BOOL bActive, DWORD /*dwThreadID*/ )
{
	ActiveApp = bActive;
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::OnEraseBkgnd (0x4112e0)

BOOL CLoginDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	CDlgPopupBase::OnPaint();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg::OnPaint (0x4113f0)

void CLoginDlg::OnPaint()
{
	CDlgPopupBase::OnPaint();
}
