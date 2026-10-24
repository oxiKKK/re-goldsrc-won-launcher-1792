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
// Purpose: the gore / violence-lock dialog (CGoreDlg, IDD_GORE = 232).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"


BEGIN_MESSAGE_MAP( CGoreDlg, CDialog )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED( IDC_GORE_CHECKBOX, OnGoreCheck )	// cmd 30
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGoreDlg::CGoreDlg (0x413E90)

CGoreDlg::CGoreDlg( CWnd* pParent )
	: CDlgBase( IDD_GORE, pParent )
{
	m_pSelfWnd = this;		// +204 -- gates the slide transition
	LoadHeaderBitmap( "head_gore", 0 );
	SetupButtonStrip();

	m_userToken = Launcher_GetProfileString( "Settings", "User Token 2", "" );
}

/////////////////////////////////////////////////////////////////////////////
// CGoreDlg::~CGoreDlg (0x413F80)

CGoreDlg::~CGoreDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CGoreDlg::SetupButtonStrip (0x414010)

void CGoreDlg::SetupButtonStrip()
{
	int	dims[2];

	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnDone.SetTransparent( 1 );
		m_btnDone.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_DONE, m_headerLoaded );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGoreDlg::DoDataExchange (0x414080)

void CGoreDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_GORE_CHECKBOX, m_checkGore );	// 30
	DDX_Control( pDX, IDC_GORE_HELP,     m_help );		// 1209
	DDX_Control( pDX, IDOK,              m_btnDone );	// 1
}

/////////////////////////////////////////////////////////////////////////////
// CGoreDlg::OnOK (0x4140D0)

void CGoreDlg::OnOK()
{
	if ( engineapi.Cbuf_AddText )
	{
		if ( m_checkGore.m_bChecked )
		{
			engineapi.Cbuf_AddText( "violence_hblood 0\n" );
			engineapi.Cbuf_AddText( "violence_hgibs 0\n" );
			engineapi.Cbuf_AddText( "violence_ablood 0\n" );
			engineapi.Cbuf_AddText( "violence_agibs 0\n" );
		}
		else
		{
			engineapi.Cbuf_AddText( "violence_hblood 1\n" );
			engineapi.Cbuf_AddText( "violence_hgibs 1\n" );
			engineapi.Cbuf_AddText( "violence_ablood 1\n" );
			engineapi.Cbuf_AddText( "violence_agibs 1\n" );
		}
	}

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CGoreDlg::OnInitDialog (0x414150)

BOOL CGoreDlg::OnInitDialog()
{
	int	dims[2];
	int	w, h, ctlX;

	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	Launcher_HeaderSize( dims );
	w = dims[0];
	h = dims[1];

	// The Done button (left column) and the check box (to its right).
	m_btnDone.MoveWindow( 50, 140, w, h, TRUE );
	SetWindowTextSafe( &m_btnDone, Launcher_LoadString( IDS_BTN_DONE ) );

	ctlX = w + 60;
	m_checkGore.MoveWindow( ctlX, 140, g_nLauncherDefW - 50 - ctlX, h, TRUE );
	m_checkGore.ModifyStyle( 0, BS_OWNERDRAW );
	SetWindowTextSafe( &m_checkGore, Launcher_LoadString( IDS_GORE_CHECKBOX ) );

	// Checked == the violence lock is currently engaged (a password exists).
	m_checkGore.m_bChecked = !m_userToken.IsEmpty();
	m_checkGore.InvalidateRect( NULL, TRUE );

	// The help label spans three button heights below the check box.
	m_help.MoveWindow( ctlX, 172, g_nLauncherDefW - 50 - ctlX, 3 * h, TRUE );
	m_help.SetWindowText( Launcher_LoadString( IDS_GORE_HELP ) );
	m_help.SetTransparent( 1 );
	m_help.SetTextColor( Scheme_GetColor( "HELP_COLOR" ) );
	m_help.SetFontSize( 11, FW_NORMAL );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGoreDlg::RMLPreIdle (0x4142D0)
//
// CDlgBase frame slot 56

int CGoreDlg::RMLPreIdle()
{
	Launcher_SyncEngineWindow( this );
	if ( Eng_Frame( 0 ) && !gBackground )
		return 1;

	if ( Launcher_AppOwnsForeground() )
	{
		ShowWindow( SW_SHOWNORMAL );			// raise the launcher dialog
		::ShowWindow( mainwindow, SW_HIDE );	// hide the engine window
	}

	// IN_HideMouse (0x40E460) runs here in the binary; it is an empty stub.
	ClipCursor( NULL );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CGoreDlg::OnGoreCheck (0x414330)

void CGoreDlg::OnGoreCheck()
{
	CString	strFirst, strSecond;

	if ( m_checkGore.m_bChecked )
	{
		// User wants to ENGAGE the lock; only meaningful when none is set yet.
		if ( m_userToken.IsEmpty() )
		{
			CInputDlg	dlgPw( this );
			dlgPw.SetPasswordMode( 1 );
			dlgPw.SetPrompt( Launcher_LoadString( IDS_GORE_PWPROMPT1 ) );

			while ( dlgPw.DoModal() == IDOK )
			{
				strFirst = dlgPw.m_strInput;

				dlgPw.SetPrompt( Launcher_LoadString( IDS_GORE_PWPROMPT2 ) );
				if ( dlgPw.DoModal() != IDOK )
					break;
				strSecond = dlgPw.m_strInput;

				if ( strcmp( strFirst, strSecond ) == 0 )	// binary: _mbscmp
				{
					// Match -- hash and store the lock.
					MD5Context_t	ctx;
					unsigned char	digest[16];
					MD5Init( &ctx );
					MD5Update( &ctx, (const unsigned char*)(LPCTSTR)strFirst, strFirst.GetLength() );
					MD5Final( digest, &ctx );

					Launcher_WriteProfileString( "Settings", "User Token 2",
						Launcher_BinToHex( digest, 16 ) );
					m_userToken = Launcher_BinToHex( digest, 16 );
					return;	// stays checked
				}

				// Mismatch -- notify, then loop back to the first prompt.
				CPromptDlg	dlgErr( 2, this );
				dlgErr.SetMessage( Launcher_LoadString( IDS_GORE_PWMISMATCHED ) );
				dlgErr.DoModal();
				dlgPw.SetPrompt( Launcher_LoadString( IDS_GORE_PWPROMPT1 ) );
			}

			// Cancelled at some point -- revert to unchecked.
			m_checkGore.m_bChecked = FALSE;
			m_checkGore.InvalidateRect( NULL, TRUE );
		}
	}
	else if ( !m_userToken.IsEmpty() )
	{
		// User wants to RELEASE the lock; require the password.
		CInputDlg	dlgPw( this );
		dlgPw.SetPasswordMode( 1 );
		dlgPw.SetPrompt( Launcher_LoadString( IDS_GORE_PWPROMPT1 ) );

		if ( dlgPw.DoModal() == IDOK )
		{
			CString			strEntered;
			MD5Context_t	ctx;
			unsigned char	digest[16];
			MD5Init( &ctx );
			MD5Update( &ctx, (const unsigned char*)(LPCTSTR)dlgPw.m_strInput, dlgPw.m_strInput.GetLength() );
			MD5Final( digest, &ctx );
			strEntered = Launcher_BinToHex( digest, 16 );

			if ( strcmp( strEntered, m_userToken ) != 0 )	// binary: _mbscmp
			{
				// Wrong password -- keep the lock engaged.
				CPromptDlg	dlgErr( 1, this );
				dlgErr.SetMessage( Launcher_LoadString( IDS_GORE_BADPW ) );
				dlgErr.DoModal();
				m_checkGore.m_bChecked = TRUE;
				m_checkGore.InvalidateRect( NULL, TRUE );
			}
			else
			{
				// Correct -- clear the lock.
				Launcher_WriteProfileString( "Settings", "User Token 2", "" );
				m_userToken = "";
			}
		}
		else
		{
			// Cancelled -- stay locked.
			m_checkGore.m_bChecked = TRUE;
			m_checkGore.InvalidateRect( NULL, TRUE );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CGoreDlg::OnPaint (0x412860)

void CGoreDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CGoreDlg::OnEraseBkgnd (0x412870)

BOOL CGoreDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}
