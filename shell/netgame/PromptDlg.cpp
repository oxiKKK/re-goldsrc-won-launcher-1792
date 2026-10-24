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
// Purpose: CPromptDlg, the launcher's skinned message / prompt dialog.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// OK/Cancel use IDOK/IDCANCEL so CDialog routes the skin buttons to OnOK/OnCancel.
// The popup base paints the skinned panel; the text is the slot-54 overdraw.

BEGIN_MESSAGE_MAP( CPromptDlg, CDialog )
	ON_MESSAGE( WM_DISPLAYCHANGE, &CPromptDlg::OnDisplayChange )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
	ON_COMMAND( IDC_PROMPT_DONTASK, OnCheckbox )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::CPromptDlg (0x45A2D0)

CPromptDlg::CPromptDlg( int nStyle, CWnd* pParent )
	: CDlgPopupBase( IDD, pParent ), m_nStyle( nStyle )
{
	int	dims[2];

	m_pPaintWnd  = this;
	m_nTextAlign = DT_CENTER;
	m_promptW    = 320;
	m_promptH    = Launcher_StringHeight( IDS_PROMPT_OFFSET, 0 ) + 160;

	m_msgFont.CreateFontA( -16, 0, 0, 0, FW_BLACK, 0, 0, 0, 0, OUT_TT_PRECIS,
		CLIP_DEFAULT_PRECIS, PROOF_QUALITY, VARIABLE_PITCH, "Arial" );
	m_titleFont.CreateFontA( -18, 0, 0, 0, FW_BLACK, 0, 0, 0, 0, OUT_TT_PRECIS,
		CLIP_DEFAULT_PRECIS, PROOF_QUALITY, VARIABLE_PITCH, "Arial" );

	strcpy( m_szCheckboxText, "" );
	strcpy( m_szTitle, "" );
	m_message  = "";
	m_clrText  = Scheme_GetColor( "PROMPT_TEXT_COLOR" );
	m_clrTitle = Scheme_GetColor( "PROMPT_TITLE_COLOR" );

	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( dims );
	m_headerW = dims[0];
	m_headerH = dims[1];
	m_headerStride = Launcher_HeaderStride();

	// The Spanish build needs a wider button cell.
	if ( Launcher_StringHeight( IDS_SPANISH, 0 ) )
		m_headerW += 30;

	if ( m_headerLoaded )
	{
		m_btnOK.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_OK, m_headerLoaded );
		m_btnCancel.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_BACK, m_headerLoaded );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::~CPromptDlg (0x405460)

CPromptDlg::~CPromptDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::DoDataExchange (0x45A530)

void CPromptDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_PROMPT_DONTASK, m_checkDontAsk );
	DDX_Control( pDX, IDOK,     m_btnOK );
	DDX_Control( pDX, IDCANCEL, m_btnCancel );
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::OnInitDialog (0x45A580)

BOOL CPromptDlg::OnInitDialog()
{
	CDlgPopupBase::OnInitDialog();

	// Size from the prompt fields (+896 width, +892 height), as the binary does.
	int	w = m_promptW;
	int	h = m_promptH;
#ifdef LAUNCHER_FIXES
	Dlg_CenterPopup( this, w, h );
#else
	int	x = ( GetSystemMetrics( SM_CXSCREEN ) - w ) / 2;
	int	y = ( GetSystemMetrics( SM_CYSCREEN ) - h ) / 2;
	MoveWindow( x, y, w, h, TRUE );
#endif

	// Message text is owner-drawn in OnPaint (no static control).

	if ( m_nStyle & 1 )
	{
		// Single OK button, centred along the bottom.
		m_btnCancel.ShowWindow( SW_HIDE );
		m_btnOK.MoveWindow( ( w - 80 ) / 2, h - 40, 80, 26, TRUE );
		SetWindowTextSafe( &m_btnOK, Launcher_LoadString( IDS_BTN_OK ) );
	}
	else
	{
		// OK (left) + Cancel (right).
		m_btnOK.MoveWindow( w / 2 - 90, h - 40, 80, 26, TRUE );
		SetWindowTextSafe( &m_btnOK, Launcher_LoadString( IDS_BTN_OK ) );
		m_btnCancel.MoveWindow( w / 2 + 10, h - 40, 80, 26, TRUE );
		SetWindowTextSafe( &m_btnCancel, Launcher_LoadString( IDS_BTN_CANCEL ) );
	}

	// Opaque faces filled with the prompt panel colour -- without this the
	// buttons composite against the shell skin and read as transparent.
	COLORREF	clrBg = Scheme_GetColor( "PROMPT_BG_COLOR" );
	CODBlendBtn*	pBtns[2] = { &m_btnOK, &m_btnCancel };
	for ( int i = 0; i < 2; i++ )
	{
		pBtns[i]->m_bTransparent = 0;
		pBtns[i]->m_bHasArrow    = 0;
		pBtns[i]->m_clrBg        = clrBg;
		pBtns[i]->m_clrDown      = RGB( 255, 180, 0 );		// 46335
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::SetMessage (0x45A880)

void CPromptDlg::SetMessage( const char* fmt, ... )
{
	char	buffer[1024];
	va_list	ap;

	buffer[0] = 0;
	va_start( ap, fmt );
	_vsnprintf( buffer, sizeof( buffer ) - 1, fmt ? fmt : "", ap );
	va_end( ap );
	buffer[sizeof( buffer ) - 1] = 0;

	m_message = buffer;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::SetPromptSize (0x45A910)

int CPromptDlg::SetPromptSize( int w, int h )
{
	m_promptW = w;
	m_promptH = h;
	return w;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::SetTextAlign (0x45A930)

int CPromptDlg::SetTextAlign( int v )
{
	m_nTextAlign = v;
	return v;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::SetMessageFont (0x45A940)

void CPromptDlg::SetMessageFont( int nHeight, int nWeight )
{
	m_msgFont.DeleteObject();
	m_msgFont.Attach( ::CreateFontA( -nHeight, 0, 0, 0, nWeight, 0, 0, 0, 0,
									 OUT_TT_PRECIS, 0, PROOF_QUALITY, FF_ROMAN, "Arial" ) );
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::OnCheckbox (0x45AAA0)

void CPromptDlg::OnCheckbox()
{
	m_bCheckboxShown = m_checkDontAsk.m_bChecked;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::SetCheckboxShown (0x45AAB0)

int CPromptDlg::SetCheckboxShown( int bShown )
{
	m_bCheckboxShown = bShown;
	return bShown;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::IsCheckboxChecked (0x45AAC0)

int CPromptDlg::IsCheckboxChecked()
{
	return m_bCheckboxShown;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::SetCheckboxText (0x45AAD0)

void CPromptDlg::SetCheckboxText( const char* psz )
{
	memcpy( m_szCheckboxText, psz, strlen( psz ) + 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::OnPaint (0x4113F0)

void CPromptDlg::OnPaint()
{
	CDlgPopupBase::OnPaint();
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::OnEraseBkgnd (0x4112E0)

BOOL CPromptDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	CDlgPopupBase::OnPaint();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::OnActivateApp (0x406FE0)

void CPromptDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::OnDisplayChange (0x453D00)

LRESULT CPromptDlg::OnDisplayChange( WPARAM, LPARAM )
{
	Dlg_CenterWindow( this );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::SetTitle (0x45AB00)

void CPromptDlg::SetTitle( const char* psz )
{
	memcpy( m_szTitle, psz, strlen( psz ) + 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::RMLPreIdle (0x45A8D0)

int CPromptDlg::RMLPreIdle()
{
	if ( gBackground && gbConsoleMode )
	{
		Eng_Frame( 1 );
		return 0;
	}

	Eng_Frame( 0 );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CPromptDlg::DrawPopupContent (0x45A990)
//
// CDlgPopupBase paint slot 54.
// The base already painted the skinned panel; this only lays the text over it.

void CPromptDlg::DrawPopupContent( CDC* pDC, RECT* prc )
{
	pDC->SetTextColor( m_clrText );
	pDC->SetBkMode( TRANSPARENT );
	CFont*	pOld = pDC->SelectObject( &m_msgFont );

	RECT	rc;
	rc.left   = prc->left + 15;
	rc.top    = prc->top + 15;
	rc.right  = prc->right - 15;
	rc.bottom = prc->bottom - 40;
	if ( m_nStyle < 0 )
		rc.top += 40;			// leave room for the title band

	pDC->DrawText( m_message, m_message.GetLength(), &rc, m_nTextAlign | DT_NOPREFIX | DT_WORDBREAK | DT_VCENTER );

	if ( m_nStyle < 0 )
	{
		rc.top    -= 40;
		rc.bottom  = prc->top + 40;
		pDC->SetTextColor( m_clrTitle );
		pDC->SelectObject( &m_titleFont );
		pDC->DrawText( m_szTitle, -1, &rc, m_nTextAlign | DT_NOPREFIX | DT_WORDBREAK | DT_VCENTER );
	}

	pDC->SelectObject( pOld );
}
