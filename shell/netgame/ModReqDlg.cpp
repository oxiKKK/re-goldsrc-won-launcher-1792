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
// Purpose: CModReqDlg, the mod-list request popup.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The custom-master path gives up on a quiet host after this long.
#define MODREQ_TIMEOUT		3.0

// Entries at 0x4AFFA8, base map 0x4B4398 = CDialog.
BEGIN_MESSAGE_MAP( CModReqDlg, CDialog )
	//{{AFX_MSG_MAP(CModReqDlg)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_BN_CLICKED( IDC_MODREQ_CANCEL, OnCancel )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModReqDlg::CModReqDlg (0x42fd00)

CModReqDlg::CModReqDlg( BOOL bMode, mod_t** ppModList, CWnd* pParent )
	: CDlgPopupBase( IDD_MODREQ, pParent )
{
	m_ppModList        = ppModList;
	m_pPaintWnd        = this;
	m_pHLModSocket     = NULL;
	m_pModInfoSocket   = NULL;
	m_bUseCustomMaster = bMode;

	// m_flStartTime is left alone: only a query that actually went out sets it,
	// and only that path reads it back.
	SetupButtons();
}

/////////////////////////////////////////////////////////////////////////////
// CModReqDlg::SetupButtons (0x42fdc0)

void CModReqDlg::SetupButtons()
{
	int	wh[2];

	m_hStripBmp = Launcher_HeaderLoaded();
	Launcher_HeaderSize( wh );
	m_nStripWidth  = wh[0];
	m_nStripHeight = wh[1];
	m_nStripStride = Launcher_HeaderStride();

	if ( m_hStripBmp )
		m_btnCancel.SetDIBData( CSize( m_nStripWidth, m_nStripHeight ), BTNSTRIP_BACK, m_hStripBmp );
}

/////////////////////////////////////////////////////////////////////////////
// CModReqDlg::~CModReqDlg (0x42fe20)

CModReqDlg::~CModReqDlg()
{
	if ( m_pHLModSocket )
		delete m_pHLModSocket;
	m_pHLModSocket = NULL;

	if ( m_pModInfoSocket )
		delete m_pModInfoSocket;
	m_pModInfoSocket = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CModReqDlg::RMLPreIdle (0x42fec0)
//
// The popup modal loop's per-pass hook.  The custom-master path has no
// completion callback, so it is polled and timed out here; the Half-Life
// master path closes itself when its socket stops pumping.

int CModReqDlg::RMLPreIdle()
{
	if ( m_bUseCustomMaster )
	{
		if ( engineapi.Sys_FloatTime() - m_flStartTime > MODREQ_TIMEOUT )
			OnOK();
		if ( m_pHLModSocket && m_pHLModSocket->m_bDone )
			OnOK();
	}
	else
	{
		if ( m_pModInfoSocket && !m_pModInfoSocket->Pump() )
			OnOK();
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CModReqDlg::DoDataExchange (0x42ff30)

void CModReqDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_MODREQ_CANCEL, m_btnCancel );
	DDX_Control( pDX, IDC_MODREQ_STATUS, m_stStatus );
}

/////////////////////////////////////////////////////////////////////////////
// CModReqDlg::StartCustomQuery (0x42ff70)
//
// Walk the custom-game masters until one accepts a list request.

void CModReqDlg::StartCustomQuery()
{
	AFX_MANAGE_STATE( AfxGetModuleState() );

	gFavorites->BeginModList();
	while ( gFavorites->NextModList() )
	{
		m_pHLModSocket = new CHLModSocket( m_ppModList,
			gFavorites->GetModAddr(),
			gFavorites->GetModPort() );

		if ( m_pHLModSocket && m_pHLModSocket->StartList() )
		{
			m_pHLModSocket->SendModListRequest();
			m_flStartTime = engineapi.Sys_FloatTime();
			return;
		}

		if ( m_pHLModSocket )
		{
			delete m_pHLModSocket;
			m_pHLModSocket = NULL;
		}
	}

	Launcher_ErrorMessageBox( 0, "Unable to communicate with custom game master server(s)" );
}

/////////////////////////////////////////////////////////////////////////////
// CModReqDlg::StartHLMasterQuery (0x430090)

void CModReqDlg::StartHLMasterQuery()
{
	AFX_MANAGE_STATE( AfxGetModuleState() );

	gFavorites->BeginMasterList();
	if ( gFavorites->NextMasterList() )
	{
		m_pModInfoSocket = new CModInfoSocket( *m_ppModList );
		if ( m_pModInfoSocket->Create( 0, SOCK_STREAM, FD_READ | FD_WRITE | FD_OOB
				| FD_ACCEPT | FD_CONNECT | FD_CLOSE, NULL ) )
		{
			if ( m_pModInfoSocket->Connect( gFavorites->GetMasterAddr(),
					gFavorites->GetMasterPort() ) )
			{
				m_pModInfoSocket->StartList();
			}
			else
			{
				delete m_pModInfoSocket;
				m_pModInfoSocket = NULL;
			}
		}
		else
		{
			delete m_pModInfoSocket;
			m_pModInfoSocket = NULL;
		}
	}
	else
	{
		Launcher_ErrorMessageBox( 0,
			"Unable to talk to Half-Life master server to get additional custom game information." );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModReqDlg::OnInitDialog (0x4301c0)
//
// A fixed 300x200 popup, centred on the screen: a caption band across the top
// and Cancel bottom-right.

BOOL CModReqDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	MoveWindow( 0, 0, 300, 200, FALSE );

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

	int	wh[2];

	Launcher_HeaderSize( wh );

	int		cyCell = wh[1];
	RECT	rc;
	GetClientRect( &rc );

	m_stStatus.MoveWindow( 10, 10, rc.right - rc.left - 20, 40, TRUE );
	m_stStatus.SetTextColor( RGB( 240, 180, 56 ) );
	m_stStatus.SetBgColor( RGB( 56, 56, 56 ) );
	m_stStatus.SetFontSize( 16, FW_HEAVY );
	m_stStatus.SetTransparent( FALSE );
	m_stStatus.SetCentered( TRUE );
	m_stStatus.SetWindowText( Launcher_LoadString( IDS_MODREQ_TITLE ) );

	m_btnCancel.MoveWindow( rc.right - rc.left - 110,
		rc.bottom - rc.top - cyCell - 10, 100, cyCell, TRUE );
	m_btnCancel.SetTransparent( FALSE );
	m_btnCancel.SetHasArrow( 0 );
	m_btnCancel.SetBkColor( RGB( 56, 56, 56 ) );
	m_btnCancel.SetTextColor( RGB( 255, 180, 0 ) );

	if ( m_bUseCustomMaster )
		StartCustomQuery();
	else
		StartHLMasterQuery();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CModReqDlg::OnEraseBkgnd (0x4112e0)

BOOL CModReqDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	CDlgPopupBase::OnPaint();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CModReqDlg::OnPaint (0x4113f0)

void CModReqDlg::OnPaint()
{
	CDlgPopupBase::OnPaint();
}
