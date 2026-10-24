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
// Purpose: the Readme sub-dialog (CReadmeDlg, IDD 0xBE = 190).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/////////////////////////////////////////////////////////////////////////////
// CReadmeDlg::CReadmeDlg (0x45ADB0)

CReadmeDlg::CReadmeDlg( CWnd* pParent )
	: CDlgBase( IDD_README, pParent )
{
	int	dims[2];

	m_pRich = NULL;				// the binary leaves it uninitialised

	m_pSelfWnd = this;		// gates the slide transition
	LoadHeaderBitmap( "head_readme", 0 );
	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
		m_btnDone.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DONE, m_headerLoaded );
}

BEGIN_MESSAGE_MAP( CReadmeDlg, CDialog )
	//{{AFX_MSG_MAP(CReadmeDlg)
	ON_MESSAGE( WM_DISPLAYCHANGE, &CReadmeDlg::OnDisplayChange )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReadmeDlg::DoDataExchange (0x45AEB0)

void CReadmeDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDOK, m_btnDone );
	DDX_Control( pDX, IDC_README_STATIC, m_frame );
}

/////////////////////////////////////////////////////////////////////////////
// CReadmeDlg::~CReadmeDlg (0x45AEF0)

CReadmeDlg::~CReadmeDlg()
{
	delete m_pRich;
}

/////////////////////////////////////////////////////////////////////////////
// CReadmeDlg::OnInitDialog (0x45AF70)

BOOL CReadmeDlg::OnInitDialog()
{
	RECT	rc;
	FILE*	f;
	long	len;
	char*	raw;
	char*	norm;
	char*	s;
	char*	d;
	int		dims[2];
	int		w, h, right, bottom;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	m_pRich = new CODEdit();

	m_frame.GetWindowRect( &rc );
	ScreenToClient( &rc );

	// CODEdit builds its rich-edit child in ON_WM_CREATE, off this rect.
	m_pRich->Create( NULL, "", 0, rc, this, 0 );
	m_pRich->ShowWindow( SW_RESTORE );

	f = fopen( "readme.txt", "rt" );
	if ( !f )
	{
		m_pRich->SetText( Launcher_LoadString( IDS_README_NOFILE ) );
	}
	else
	{
		fseek( f, 0, SEEK_END );
		len = ftell( f );
		fseek( f, 0, SEEK_SET );

		raw  = new char[len + 1];
		norm = new char[2 * len + 1];
		if ( raw && norm )
		{
			memset( raw, 0, len + 1 );
			memset( norm, 0, 2 * len + 1 );
			fread( raw, len, 1, f );
			raw[len] = 0;

			// Collapse CR/LF runs to single LFs.
			for ( s = raw, d = norm; *s; s++ )
				*d++ = ( *s == '\r' || *s == '\n' ) ? '\n' : *s;
			*d = 0;

			m_pRich->SetText( norm );
			delete[] raw;
			delete[] norm;
			fclose( f );
		}
		else
		{
			Launcher_ShowMessageByIdEx( 0, IDS_README_NOMEM, len + 1 );
			PostQuitMessage( 0 );
			return TRUE;
		}
	}

	m_pRich->Finalize();

	Launcher_HeaderSize( dims );
	w      = dims[0];
	h      = dims[1];
	right  = g_nLauncherDefW - 50;
	bottom = g_nLauncherDefH - 50;

	m_btnDone.MoveWindow( 50, 140, w, h, TRUE );
	SetWindowTextSafe( &m_btnDone, Launcher_LoadString( IDS_BTN_DONE ) );
	m_pRich->MoveWindow( w + 70, 140, right - ( w + 70 ), bottom - 140, TRUE );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CReadmeDlg::OnPaint (0x412860)

void CReadmeDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CReadmeDlg::OnEraseBkgnd (0x412870)

BOOL CReadmeDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CReadmeDlg::OnActivateApp (0x406FE0)

void CReadmeDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CReadmeDlg::OnDisplayChange (0x453D00)

LRESULT CReadmeDlg::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}
