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
// Purpose: CAdvancedMPDlg, the advanced multiplayer options page, with
//          CServerDescription and the CD-key checksum.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Paging arrow child ids.
#define IDC_ADVMP_PREVPAGE	124
#define IDC_ADVMP_NEXTPAGE	123

static RECT	s_rcPrevPage = { g_nLauncherDefW - 200, 108, g_nLauncherDefW - 183, 125 };
static RECT	s_rcPageInfo = { g_nLauncherDefW - 180, 108, g_nLauncherDefW -  70, 125 };
static RECT	s_rcNextPage = { g_nLauncherDefW -  67, 108, g_nLauncherDefW -  50, 125 };

BEGIN_MESSAGE_MAP( CAdvancedMPDlg, CDialog )
	//{{AFX_MSG_MAP(CAdvancedMPDlg)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_COMMAND( IDC_ADVMP_NEXTPAGE, OnNextPage )
	ON_COMMAND( IDC_ADVMP_PREVPAGE, OnPrevPage )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define ADVMP_ROW_H		32		// rows advance 32px

// Every option row is created with the same style; BS_MULTILINE only means
// anything to the checkbox.
#define ADVMP_CTRL_STYLE	( WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_MULTILINE )

/*
==================
CDKey_Checksum (0x401000)
==================
*/
BOOL CDKey_Checksum( const char* key )
{
	int		sum = 3;
	int		i;

	for ( i = 0; i < 12; ++i )
		sum += ( 2 * sum ) ^ ( key[i] - '0' );

	return ( sum % 10 ) == ( key[12] - '0' );
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::CAdvancedMPDlg (0x401050)

CAdvancedMPDlg::CAdvancedMPDlg( CServerDescription* pDesc, int nContext, CWnd* pParent )
	: CDlgBase( IDD, pParent )
{
	m_pDesc        = pDesc;
	m_nContext     = nContext;
	m_pControls    = NULL;
	m_nNumControls = 0;
	m_nPerPage     = 8;
	m_iPage        = 0;
	m_pSelfWnd   = this;		// the page points the slide at itself
	LoadHeaderBitmap( "head_creategame", 0 );
	m_pPrevPage = NULL;
	m_pNextPage = NULL;
	LoadButtonStrips();
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::~CAdvancedMPDlg (0x401150)

CAdvancedMPDlg::~CAdvancedMPDlg()
{
	DestroyControls();
	delete m_pPrevPage;
	delete m_pNextPage;
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::LoadButtonStrips (0x401200)

void CAdvancedMPDlg::LoadButtonStrips()
{
	m_hHeader = Launcher_HeaderLoaded();
	Launcher_HeaderSize( m_headerWH );		// {w,h}
	m_headerStride = Launcher_HeaderStride();

	if ( m_hHeader )
	{
		m_btnOK.m_bTwoBitmap = 1;
		m_btnOK.SetDIBData( CSize( m_headerWH[0], m_headerWH[1] ), BTNSTRIP_DONE, m_hHeader );
		m_btnCancel.m_bTwoBitmap = 1;
		m_btnCancel.SetDIBData( CSize( m_headerWH[0], m_headerWH[1] ), BTNSTRIP_BACK, m_hHeader );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::DoDataExchange (0x401290)

void CAdvancedMPDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDCANCEL,    m_btnCancel );
	DDX_Control( pDX, IDC_ADVANCEDMP_PAGE, m_lblPageInfo );
	DDX_Control( pDX, IDOK,    m_btnOK );
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::ApplyToDescription (0x4012E0)

void CAdvancedMPDlg::ApplyToDescription()
{
	if ( !m_pDesc )
		return;

	CString	str;
	for ( OptCtrl* oc = m_pControls; oc; oc = oc->m_pNext )
	{
		CScriptObject*	o = oc->m_pOption;

		if ( !oc->m_pControl )
		{
			o->curValue   = o->defValue;
			o->fcurValue = (float)atof( o->defValue );
			continue;
		}

		char	szValue[256];
		switch ( o->type )
		{
		case O_BOOL:
			sprintf( szValue, "%s",
				( (CODBlendCheckBox*)oc->m_pControl )->GetCheck() ? "1" : "0" );
			break;
		case O_NUMBER:
		case O_STRING:
			{
				CBorderLessEdit*	ed = (CBorderLessEdit*)oc->m_pControl;
				if ( ed->m_pEdit )
					ed->m_pEdit->GetWindowText( str );
				sprintf( szValue, "%s", (LPCSTR)str );
			}
			break;
		case O_LIST:
			{
				// The combo holds a row index; walk the choices to the cvar text.
				int	iSel = ( (CODComboBox*)oc->m_pControl )->GetCurSel();
				if ( iSel == -1 )
					iSel = (int)o->fdefValue;

				CScriptListItem*	c = o->pListItems;
				for ( int i = 0; i < iSel && c; i++ )
					c = c->pNext;

				sprintf( szValue, "%s", c ? c->szValue : (LPCSTR)o->defValue );
			}
			break;
		default:
			continue;
		}

		Sys_StripQuotesAndPercents( szValue );
		str = szValue;
		o->curValue   = str;
		o->fcurValue = (float)atof( str );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::OnOK (0x401470)

void CAdvancedMPDlg::OnOK()
{
	ApplyToDescription();

	if ( m_pDesc )
	{
		char	szPath[260];
		FILE*	fp;

		sprintf( szPath, "%s\\game.cfg", com_gamedir );
		if ( ( fp = fopen( szPath, "wb" ) ) != NULL )
		{
			m_pDesc->WriteToFile( fp );
			fclose( fp );
		}

		sprintf( szPath, "%s\\settings.scr", com_gamedir );
		COM_FixSlashes( szPath );
		SetFileAttributes( szPath, FILE_ATTRIBUTE_NORMAL );
		if ( ( fp = fopen( szPath, "wb" ) ) != NULL )
		{
			m_pDesc->WriteToScriptFile( fp );
			fclose( fp );
		}
	}

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::BuildControls (0x401540)

void CAdvancedMPDlg::BuildControls()
{
	DestroyControls();

	int	wh[2] = { 0, 0 };
	Launcher_HeaderSize( wh );
	int	cw = wh[0];
	int	ch = wh[1];

	int	xCtrl  = cw / 2 + 60;			// the entry column
	int	xCtrlR = xCtrl + cw;
	int	y      = 140;

	m_nNumControls = 0;
	OptCtrl**	pp = &m_pControls;

	for ( CScriptObject* o = m_pDesc ? m_pDesc->pObjList : NULL; o; o = o->pNext )
	{
		if ( !( m_nNumControls % m_nPerPage ) )
			y = 140;					// each page restarts at the top

		OptCtrl*	oc = new OptCtrl;
		oc->m_nType    = o->type;
		oc->m_pOption  = o;
		oc->m_pControl = NULL;
		oc->m_pHelp    = NULL;
		oc->m_pNext    = NULL;

		CRect	rc( xCtrl, y, xCtrlR, y + ch );
		m_nNumControls++;

		switch ( o->type )
		{
		case O_BOOL:
			{
				// The checkbox carries the prompt itself and runs the full width, so
				// it gets no separate help label.
				CODBlendCheckBox*	cb = new CODBlendCheckBox;
				rc.right = g_nLauncherDefW - 50;
				cb->Create( o->prompt, ADVMP_CTRL_STYLE, rc, this, (UINT)-1 );
				cb->ModifyStyle( 0, BS_OWNERDRAW );
				cb->SetCheck( o->fdefValue != 0.0f );
				cb->MoveWindow( rc.left, rc.top, rc.Width(), rc.Height(), TRUE );
				oc->m_pControl = cb;
			}
			break;
		case O_LIST:
			{
				CODComboBox*	combo = new CODComboBox;
				rc.bottom = rc.top + 15;
				combo->SetDropHeight( 75 );
				combo->Create( ADVMP_CTRL_STYLE, &rc, this, (UINT)-1 );
				for ( CScriptListItem* c = o->pListItems; c; c = c->pNext )
					combo->AddString( c->szItemText );
				combo->MoveTo( &rc, 1 );
				combo->SetCurSel( (int)o->fdefValue );
				oc->m_pControl = combo;
			}
			break;
		case O_NUMBER:
		case O_STRING:
			{
				CBorderLessEdit*	ed = new CBorderLessEdit;
				ed->SetBorderColor( RGB( 56, 56, 56 ) );
				ed->SetActive( 1 );
				ed->SetAutoHScroll();
				ed->Create( ADVMP_CTRL_STYLE, &rc, this, (UINT)-1 );
				ed->MoveWindow( rc.left, rc.top, rc.Width(), rc.Height(), TRUE );
				ed->ShowWindow( SW_RESTORE );
				ed->SetText( o->defValue );
				oc->m_pControl = ed;
			}
			break;
		}

		// The help text sits to the right of the entry column and runs to the page
		// margin.  BOOL rows already say it on the glyph.
		rc.OffsetRect( xCtrlR - xCtrl + 15, 0 );
		rc.right   = g_nLauncherDefW - 50;
		rc.bottom += 8;
		if ( o->type != O_BOOL )
		{
			oc->m_pHelp = new CODStatic;
			oc->m_pHelp->Create( o->prompt, WS_CHILD | WS_VISIBLE, rc, this, (UINT)-1 );
			oc->m_pHelp->MoveWindow( rc.left, rc.top, rc.Width(), rc.Height(), TRUE );
			oc->m_pHelp->SetWindowText( o->prompt );
			oc->m_pHelp->SetTextColor( RGB( 255, 255, 255 ) );
			oc->m_pHelp->SetFontSize( 14, FW_NORMAL );
			oc->m_pHelp->SetOffsets( 0, 2 );
		}

		*pp = oc;
		pp  = &oc->m_pNext;
		y  += ADVMP_ROW_H;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::DestroyControls (0x4019A0)

void CAdvancedMPDlg::DestroyControls()
{
	OptCtrl*	oc = m_pControls;
	while ( oc )
	{
		OptCtrl*	pNext = oc->m_pNext;

		switch ( oc->m_nType )
		{
		case O_BOOL:
		case O_NUMBER:
		case O_STRING:
			delete oc->m_pControl;
			oc->m_pControl = NULL;
			break;
		case O_LIST:
			// (sic) the drop list is left to leak; only the slot is cleared.
			oc->m_pControl = NULL;
			break;
		}

		delete oc->m_pHelp;
		oc->m_pHelp = NULL;
		delete oc;
		oc = pNext;
	}
	m_pControls = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::OnInitDialog (0x401AC0)

BOOL CAdvancedMPDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );
	BuildControls();
	ShowPage( 0 );

	int	wh[2] = { 0, 0 };
	Launcher_HeaderSize( wh );
	int	nPad = 9 * Launcher_StringHeight( IDS_GERMAN, 0 );
	int	btnW = ( wh[0] + 2 * nPad ) / 2;
	int	btnH = wh[1];

	m_btnOK.MoveWindow( 50, 140, btnW, btnH, TRUE );
	SetWindowTextSafe( &m_btnOK, Launcher_LoadString( IDS_BTN_DONE ) );
	m_btnCancel.MoveWindow( 50, 172,
		Launcher_StringHeight( IDS_ADVANCEDMP_OFFSETS, 0 ) + btnW, btnH, TRUE );
	SetWindowTextSafe( &m_btnCancel, Launcher_LoadString( IDS_BTN_CANCEL ) );

	// "Page n of m" between the two paging arrows.
	SetupLabel( &m_lblPageInfo, &s_rcPageInfo, IDS_EMPTY );

	int		nPages = (int)ceil( (double)m_nNumControls / (double)m_nPerPage );
	char	szInfo[256];
	sprintf( szInfo, Launcher_LoadString( IDS_ADVANCEDSVR_PAGE ), m_iPage + 1, nPages );
	m_lblPageInfo.ShowWindow( SW_RESTORE );
	m_lblPageInfo.SetWindowText( szInfo );
	::InvalidateRect( m_lblPageInfo.GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( m_lblPageInfo.GetSafeHwnd() );

	m_pPrevPage = SetupArrowButton( &s_rcPrevPage, this,
		"larrowdefault", "larrowpressed", "larrowflyover", IDC_ADVMP_PREVPAGE );
	m_pNextPage = SetupArrowButton( &s_rcNextPage, this,
		"rarrowdefault", "rarrowpressed", "rarrowflyover", IDC_ADVMP_NEXTPAGE );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::RMLPreIdle (0x401C80)
//
// Frame-protocol slot 56.

int CAdvancedMPDlg::RMLPreIdle()
{
	// (sic) the slot pumped here is m_nContext, reinterpreted as a sheet
	// pointer -- the binary reads this+34Ch, not this+348h.
	if ( m_nContext )
		( (CNetGameDlg*)m_nContext )->Pump();

	Launcher_SyncEngineWindow( this );

	if ( Eng_Frame( gBackground ) && !gBackground )
		return 1;

	if ( Launcher_AppOwnsForeground() )
	{
		ShowWindow( SW_SHOWNORMAL );
		::ShowWindow( mainwindow, SW_HIDE );
	}

	ClipCursor( NULL );

	if ( Launcher_GetRestartFlag() )
		OnCancel();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::OnNextPage (0x401D10)

void CAdvancedMPDlg::OnNextPage()
{
	ShowPage( m_iPage + 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::OnPrevPage (0x401D20)

void CAdvancedMPDlg::OnPrevPage()
{
	ShowPage( m_iPage - 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::ShowPage (0x401D30)

void CAdvancedMPDlg::ShowPage( int iPage )
{
	int	nPages = (int)ceil( (double)m_nNumControls / (double)m_nPerPage );
	if ( iPage < 0 || iPage >= nPages )
		return;

	m_iPage = iPage;

	char	szInfo[256];
	sprintf( szInfo, Launcher_LoadString( IDS_ADVANCEDSVR_PAGE ), iPage + 1, nPages );
	m_lblPageInfo.SetWindowText( szInfo );
	::InvalidateRect( m_lblPageInfo.GetSafeHwnd(), NULL, TRUE );
	::UpdateWindow( m_lblPageInfo.GetSafeHwnd() );

	::LockWindowUpdate( m_hWnd );
	int	i = 0;
	for ( OptCtrl* oc = m_pControls; oc; oc = oc->m_pNext, i++ )
	{
		int	nShow = ( m_iPage == i / m_nPerPage ) ? SW_RESTORE : SW_HIDE;

		switch ( oc->m_nType )
		{
		case O_BOOL:
		case O_NUMBER:
		case O_STRING:
		case O_LIST:
			if ( oc->m_pControl )
				oc->m_pControl->ShowWindow( nShow );
			break;
		}

		if ( oc->m_pHelp )
			oc->m_pHelp->ShowWindow( nShow );
	}
	::LockWindowUpdate( NULL );
}

/*
==================
CServerDescription::WriteScriptHeader (0x401E60)

The settings.scr header.
==================
*/
int CServerDescription::WriteScriptHeader( FILE* fp )
{
	char	szAmPm[8];
	time_t	now;

	strcpy( szAmPm, "AM" );
	time( &now );

	struct tm*	pt = localtime( &now );
	if ( pt->tm_hour > 12 )
		strcpy( szAmPm, "PM" );
	if ( pt->tm_hour > 12 )
		pt->tm_hour -= 12;
	if ( !pt->tm_hour )
		pt->tm_hour = 12;

	fprintf( fp, m_pszHintText );		// (sic) passed as the format string
	fprintf( fp, "// Half-Life Server Configuration Layout Script (stores last settings chosen, too)\r\n" );
	fprintf( fp, "// File generated:  %.19s %s\r\n", asctime( pt ), szAmPm );
	fprintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );
	fprintf( fp, "VERSION %.1f\r\n\r\n", 1.0 );
	return fprintf( fp, "DESCRIPTION SERVER_OPTIONS\r\n{\r\n" );
}

/*
==================
CServerDescription::WriteFileHeader (0x401F60)

The game.cfg header.
==================
*/
int CServerDescription::WriteFileHeader( FILE* fp )
{
	char	szAmPm[8];
	time_t	now;

	strcpy( szAmPm, "AM" );
	time( &now );

	struct tm*	pt = localtime( &now );
	if ( pt->tm_hour > 12 )
		strcpy( szAmPm, "PM" );
	if ( pt->tm_hour > 12 )
		pt->tm_hour -= 12;
	if ( !pt->tm_hour )
		pt->tm_hour = 12;

	fprintf( fp, "// Half-Life Server Configuration Settings\r\n" );
	fprintf( fp, "// DO NOT EDIT, GENERATED BY HALF-LIFE\r\n" );
	fprintf( fp, "// File generated:  %.19s %s\r\n", asctime( pt ), szAmPm );
	return fprintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );
}

/*
==================
CServerDescription::CServerDescription (0x402040)
==================
*/
CServerDescription::CServerDescription()
{
	m_pszHintText = _strdup(
		"// NOTE:  THIS FILE IS AUTOMATICALLY REGENERATED, \r\n"
		"//DO NOT EDIT THIS HEADER, YOUR COMMENTS WILL BE LOST IF YOU DO\r\n"
		"// Multiplayer options script\r\n"
		"//\r\n"
		"// Format:\r\n"
		"//  Version [float]\r\n"
		"//  Options description followed by \r\n"
		"//  Options defaults\r\n"
		"//\r\n"
		"// Option description syntax:\r\n"
		"//\r\n"
		"//  \"cvar\" { \"Prompt\" { type [ type info ] } { default } }\r\n"
		"//\r\n"
		"//  type = \r\n"
		"//   BOOL   (a yes/no toggle)\r\n"
		"//   STRING\r\n"
		"//   NUMBER\r\n"
		"//   LIST\r\n"
		"//\r\n"
		"// type info:\r\n"
		"// BOOL                 no type info\r\n"
		"// NUMBER       min max range, use -1 -1 for no limits\r\n"
		"// STRING       no type info\r\n"
		"// LIST          delimited list of options value pairs\r\n"
		"//\r\n"
		"//\r\n"
		"// default depends on type\r\n"
		"// BOOL is \"0\" or \"1\"\r\n"
		"// NUMBER is \"value\"\r\n"
		"// STRING is \"value\"\r\n"
		"// LIST is \"index\", where index \"0\" is the first element of the list\r\n"
		"\r\n"
		"\r\n" );
	m_pszDescriptionType = _strdup( "SERVER_OPTIONS" );
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::OnPaint (0x412860)

void CAdvancedMPDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CAdvancedMPDlg::OnEraseBkgnd (0x412870)

BOOL CAdvancedMPDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}
