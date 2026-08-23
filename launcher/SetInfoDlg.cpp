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
// Purpose: CSetInfoDlg, the user.scr advanced-options page, with the
//          CInfoDescription model it writes.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// Paging arrow child ids.
#define IDC_ADVOPTS_PREVPAGE	124
#define IDC_ADVOPTS_NEXTPAGE	123

#define ADVOPTS_ROW_H		32		// rows advance 32px

// Every option row is created with the same style; the BS_ bits only mean
// anything to the checkbox.
#define ADVOPTS_CTRL_STYLE	( WS_CHILD | WS_VISIBLE | BS_MULTILINE | BS_CENTER | BS_BOTTOM )

// The paging strip along the top-right, sized off the page width at load time.
static RECT	s_rcPrevPage = { g_nLauncherDefW - 200, 108, g_nLauncherDefW - 183, 125 };
static RECT	s_rcPageInfo = { g_nLauncherDefW - 180, 108, g_nLauncherDefW -  70, 125 };
static RECT	s_rcNextPage = { g_nLauncherDefW -  67, 108, g_nLauncherDefW -  50, 125 };

BEGIN_MESSAGE_MAP( CSetInfoDlg, CDialog )
	//{{AFX_MSG_MAP(CSetInfoDlg)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_COMMAND( IDC_ADVOPTS_NEXTPAGE, OnNextPage )
	ON_COMMAND( IDC_ADVOPTS_PREVPAGE, OnPrevPage )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::CSetInfoDlg (0x462530)

CSetInfoDlg::CSetInfoDlg( CDescription* pDesc, CWnd* pParent )
	: CDlgBase( IDD, pParent )
{
	m_pDesc        = pDesc;
	m_nNumControls = 0;
	m_nPerPage     = 8;
	m_iPage        = 0;
	m_pControls    = NULL;
	m_pSelfWnd   = this;		// the page points the slide at itself
	LoadHeaderBitmap( "head_advoptions", 0 );
	m_pPrevPage    = NULL;
	m_pNextPage    = NULL;
	LoadButtonStrips();
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::~CSetInfoDlg (0x462630)

CSetInfoDlg::~CSetInfoDlg()
{
	DestroyControls();
	delete m_pPrevPage;
	delete m_pNextPage;
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::LoadButtonStrips (0x4626F0)

void CSetInfoDlg::LoadButtonStrips()
{
	m_hHeader      = Launcher_HeaderLoaded();
	Launcher_HeaderSize( m_headerWH );		// {w,h}
	m_headerStride = Launcher_HeaderStride();

	if ( m_hHeader )
	{
		m_btnOK.FreeSkinBitmaps();
		m_btnOK.SetDIBData( CSize( m_headerWH[0], m_headerWH[1] ), BTNSTRIP_DONE, m_hHeader );
		m_btnCancel.FreeSkinBitmaps();
		m_btnCancel.SetDIBData( CSize( m_headerWH[0], m_headerWH[1] ), BTNSTRIP_BACK, m_hHeader );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::DoDataExchange (0x462780)

void CSetInfoDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_SETINFO_PAGE,             m_lblPageInfo );
	DDX_Control( pDX, IDCANCEL,                    m_btnCancel );
	DDX_Control( pDX, IDOK,                        m_btnOK );
	DDX_Control( pDX, IDC_SETINFO_SENSHELP,  m_lblSensHelp );
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::ApplyToDescription (0x4627E0)

void CSetInfoDlg::ApplyToDescription()
{
	if ( !m_pDesc )
		return;

	CString	str;

	for ( OptCtrl* oc = m_pControls; oc; oc = oc->m_pNext )
	{
		CWnd*			pCtrl = oc->m_pControl;
		CScriptObject*	o     = oc->m_pOption;
		float			flValue;

		if ( pCtrl )
		{
			char		szValue[256];
			const char*	psz;

			switch ( o->type )
			{
			case O_BOOL:
				psz = ( (CODBlendCheckBox*)pCtrl )->GetCheck() ? "1" : "0";
				sprintf( szValue, "%s", psz );
				break;
			case O_NUMBER:
			case O_STRING:
				{
					CBorderLessEdit*	ed = (CBorderLessEdit*)pCtrl;
					if ( ed->m_pEdit )
						ed->m_pEdit->GetWindowText( str );
					sprintf( szValue, "%s", (LPCSTR)str );
				}
				break;
			case O_LIST:
				{
					// The row index selects a choice; its cvar text is the one written
					// back, and an unset combo falls back to the parsed default.
					CScriptListItem*	c = o->pListItems;
					int	iSel = ( (CODComboBox*)pCtrl )->GetCurSel();
					if ( iSel == -1 )
						iSel = (int)o->fdefValue;

					for ( int n = 0; c && n < iSel; n++ )
						c = c->pNext;

					if ( c )
						sprintf( szValue, "%s", c->szValue );
					else
						sprintf( szValue, "%s", (LPCSTR)o->defValue );
				}
				break;
			default:
				break;
			}

			Sys_StripQuotesAndPercents( szValue );
			str = szValue;
			o->curValue = str;
			flValue = (float)atof( str );
		}
		else
		{
			o->curValue = o->defValue;
			flValue = (float)atof( o->defValue );
		}

		o->fcurValue = flValue;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::BuildControls (0x462970)

void CSetInfoDlg::BuildControls()
{
	int	wh[2] = { 0, 0 };
	Launcher_HeaderSize( wh );
	int	cw = wh[0];
	int	ch = wh[1];

	DestroyControls();

	int	xCtrl  = cw / 2 + 60;			// the entry column
	int	xCtrlR = xCtrl + cw;
	int	y      = ch + 148;				// clear of the fixed sensitivity row

	m_nNumControls = 0;

	for ( CScriptObject* o = m_pDesc->pObjList; o; o = o->pNext )
	{
		if ( !( m_nNumControls % m_nPerPage ) )
			y = ch + 148;				// each page restarts at the top
		m_nNumControls++;

		RECT	rc;
		rc.left   = xCtrl;
		rc.top    = y;
		rc.right  = xCtrlR;
		rc.bottom = y + ch;

		OptCtrl*	oc = new OptCtrl;
		oc->m_nType    = 0;
		oc->m_pControl = NULL;
		oc->m_pHelp    = NULL;
		oc->m_pOption  = NULL;
		oc->m_pNext    = NULL;
		oc->m_nType    = o->type;

		switch ( o->type )
		{
		case O_BOOL:
			{
				// The checkbox carries the prompt itself and runs the full width, so
				// it gets no separate help label.
				CODBlendCheckBox*	cb = new CODBlendCheckBox;
				rc.right = g_nLauncherDefW - 50;
				cb->Create( o->prompt, ADVOPTS_CTRL_STYLE,
					rc, this, (UINT)-1 );
				::SetWindowLongA( cb->GetSafeHwnd(), GWL_STYLE,
					cb->GetStyle() | BS_OWNERDRAW );
				cb->SetCheck( o->fdefValue != 0.0f );
				cb->MoveWindow( rc.left, rc.top, rc.right - rc.left,
					rc.bottom - rc.top, TRUE );
				oc->m_pControl = cb;
			}
			break;
		case O_NUMBER:
		case O_STRING:
			{
				CBorderLessEdit*	ed = new CBorderLessEdit;
				ed->SetBorderColor( RGB( 56, 56, 56 ) );
				ed->SetActive( 1 );
				ed->SetAutoHScroll();
				ed->Create( ADVOPTS_CTRL_STYLE, &rc, this, (UINT)-1 );
				ed->MoveWindow( rc.left, rc.top, rc.right - rc.left,
					rc.bottom - rc.top, TRUE );
				ed->ShowWindow( SW_RESTORE );
				ed->SetText( o->defValue );
				oc->m_pControl = ed;
			}
			break;
		case O_LIST:
			{
				CODComboBox*	combo = new CODComboBox;
				rc.bottom = rc.top + 15;
				combo->SetDropHeight( 75 );
				combo->Create( ADVOPTS_CTRL_STYLE, &rc, this, (UINT)-1 );
				for ( CScriptListItem* c = o->pListItems; c; c = c->pNext )
					combo->AddString( c->szItemText );
				combo->MoveTo( &rc, 1 );
				combo->SetCurSel( (int)o->fdefValue );
				oc->m_pControl = combo;
			}
			break;
		default:
			break;
		}

		// The help text sits to the right of the entry column and runs to the page
		// margin.  BOOL rows already say it on the glyph.
		::OffsetRect( &rc, xCtrlR - xCtrl + 15, 0 );
		rc.right = g_nLauncherDefW - 50;
		if ( oc->m_nType != O_BOOL )
		{
			oc->m_pHelp = new CODStatic;
			oc->m_pHelp->Create( o->prompt, WS_CHILD | WS_VISIBLE,
				rc, this, (UINT)-1 );
			oc->m_pHelp->MoveWindow( rc.left, rc.top, rc.right - rc.left,
				rc.bottom - rc.top, TRUE );
			oc->m_pHelp->SetWindowText( o->prompt );
			oc->m_pHelp->SetTextColor( RGB( 255, 255, 255 ) );
			oc->m_pHelp->SetFontSize( 14, FW_NORMAL );
			oc->m_pHelp->SetOffsets( 0, 2 );
		}

		oc->m_pOption = o;
		y += ADVOPTS_ROW_H;

		if ( m_pControls )
		{
			OptCtrl*	tail = m_pControls;
			while ( tail->m_pNext )
				tail = tail->m_pNext;
			tail->m_pNext = oc;
		}
		else
			m_pControls = oc;

		oc->m_pNext = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::DestroyControls (0x462DD0)

void CSetInfoDlg::DestroyControls()
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
		default:
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
// CSetInfoDlg::OnOK (0x462EF0)

void CSetInfoDlg::OnOK()
{
	ApplyToDescription();

	CString	str;
	if ( m_pSensEdit->m_pEdit )
		m_pSensEdit->m_pEdit->GetWindowText( str );

	// (sic) 0x462F61 -- with an empty field the binary stores the CString's own
	// data pointer through the float slot instead of reading a value.
	float	flSens = str.GetLength() ? (float)atof( str )
									 : *(const float*)&str;
	g_pServerBrowser->m_playerConfig.sensitivity = flSens;

	if ( m_pDesc )
	{
		m_pDesc->WriteToConfig();

		char	szPath[260];
		sprintf( szPath, "%s\\user.scr", com_gamedir );
		COM_FixSlashes( szPath );
		SetFileAttributes( szPath, FILE_ATTRIBUTE_NORMAL );

		FILE*	fp = fopen( szPath, "wb" );
		if ( fp )
		{
			m_pDesc->WriteToScriptFile( fp );
			fclose( fp );
		}
	}

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::OnInitDialog (0x463010)

BOOL CSetInfoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );

	BuildControls();
	ShowPage( 0 );

	int	wh[2] = { 0, 0 };
	Launcher_HeaderSize( wh );
	int	btnW = ( wh[0] + 2 * 9 * Launcher_StringHeight( IDS_GERMAN, 0 ) ) / 2;
	int	btnH = wh[1];

	m_btnOK.MoveWindow( 50, 140, btnW, btnH, TRUE );
	m_btnOK.SetWindowText( Launcher_LoadString( IDS_BTN_DONE ) );

	int	nCancelPad = Launcher_StringHeight( IDS_ADVANCEDMP_OFFSETS, 0 );

	RECT	rc;
	rc.left   = 50;
	rc.top    = 172;
	rc.right  = 50 + btnW + nCancelPad;
	rc.bottom = 172 + btnH;
	m_btnCancel.MoveWindow( 50, 172, btnW + nCancelPad, btnH, TRUE );
	m_btnCancel.SetWindowText( Launcher_LoadString( IDS_BTN_CANCEL ) );

	// The one fixed row: "sensitivity", which is not a user.scr node.
	int	xCtrl  = wh[0] / 2 + 60;
	int	xCtrlR = xCtrl + wh[0];
	rc.left   = xCtrl;
	rc.top    = 140;
	rc.right  = xCtrlR;
	rc.bottom = 140 + btnH;

	m_pSensEdit = new CBorderLessEdit;
	m_pSensEdit->Create( 0, &rc, this, IDC_SETINFO_SENSHELP );
	m_pSensEdit->SetBorderColor( RGB( 56, 56, 56 ) );
	m_pSensEdit->MoveWindow( rc.left, rc.top, rc.right - rc.left,
		rc.bottom - rc.top, TRUE );
	m_pSensEdit->SetText( "" );

	char	szValue[256];
	sprintf( szValue, "%f", g_pServerBrowser->m_playerConfig.sensitivity );
	m_pSensEdit->SetText( szValue );

	::OffsetRect( &rc, xCtrlR - xCtrl + 15, 0 );
	rc.right = g_nLauncherDefW - 50;
	m_lblSensHelp.ShowWindow( SW_RESTORE );
	m_lblSensHelp.MoveWindow( rc.left, rc.top, rc.right - rc.left,
		rc.bottom - rc.top, TRUE );
	m_lblSensHelp.SetWindowText( Launcher_LoadString( IDS_OPTS_SENSITIVITYHELP ) );
	m_lblSensHelp.SetTextColor( RGB( 255, 255, 255 ) );
	m_lblSensHelp.SetFontSize( 14, FW_NORMAL );
	m_lblSensHelp.SetOffsets( 0, 2 );

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
		"larrowdefault", "larrowpressed", "larrowflyover", IDC_ADVOPTS_PREVPAGE );
	m_pNextPage = SetupArrowButton( &s_rcNextPage, this,
		"rarrowdefault", "rarrowpressed", "rarrowflyover", IDC_ADVOPTS_NEXTPAGE );

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::RMLPreIdle (0x4633D0)
//
// Frame-protocol slot 56.

int CSetInfoDlg::RMLPreIdle()
{
	Launcher_SyncEngineWindow( this );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::OnNextPage (0x4633E0)

void CSetInfoDlg::OnNextPage()
{
	ShowPage( m_iPage + 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::OnPrevPage (0x4633F0)

void CSetInfoDlg::OnPrevPage()
{
	ShowPage( m_iPage - 1 );
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::ShowPage (0x463400)

void CSetInfoDlg::ShowPage( int iPage )
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

	::LockWindowUpdate( GetSafeHwnd() );
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
CInfoDescription::WriteScriptHeader (0x463530)

The settings.scr layout header.
==================
*/
int CInfoDescription::WriteScriptHeader( FILE* fp )
{
	char	szAmPm[4];
	strcpy( szAmPm, "AM" );

	time_t		now;
	time( &now );
	struct tm*	t = localtime( &now );
	if ( t->tm_hour > 12 )
		strcpy( szAmPm, "PM" );
	if ( t->tm_hour > 12 )
		t->tm_hour -= 12;
	if ( !t->tm_hour )
		t->tm_hour = 12;

	fprintf( fp, m_pszHintText );
	fprintf( fp, "// Half-Life User Info Configuration Layout Script (stores last settings chosen, too)\r\n" );
	fprintf( fp, "// File generated:  %.19s %s\r\n", asctime( t ), szAmPm );
	fprintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );
	fprintf( fp, "VERSION %.1f\r\n\r\n", 1.0 );
	return fprintf( fp, "DESCRIPTION INFO_OPTIONS\r\n{\r\n" );
}

/*
==================
CInfoDescription::WriteFileHeader (0x463630)

The game.cfg-style header.
==================
*/
int CInfoDescription::WriteFileHeader( FILE* fp )
{
	char	szAmPm[4];
	strcpy( szAmPm, "AM" );

	time_t		now;
	time( &now );
	struct tm*	t = localtime( &now );
	if ( t->tm_hour > 12 )
		strcpy( szAmPm, "PM" );
	if ( t->tm_hour > 12 )
		t->tm_hour -= 12;
	if ( !t->tm_hour )
		t->tm_hour = 12;

	fprintf( fp, "// Half-Life User Info Configuration Settings\r\n" );
	fprintf( fp, "// DO NOT EDIT, GENERATED BY HALF-LIFE\r\n" );
	fprintf( fp, "// File generated:  %.19s %s\r\n", asctime( t ), szAmPm );
	return fprintf( fp, "//\r\n//\r\n// Cvar\t-\tSetting\r\n\r\n" );
}

/*
==================
CInfoDescription::CInfoDescription (0x463710)
==================
*/
CInfoDescription::CInfoDescription()
{
	m_pszHintText = _strdup(
		"// NOTE:  THIS FILE IS AUTOMATICALLY REGENERATED, \r\n"
		"//DO NOT EDIT THIS HEADER, YOUR COMMENTS WILL BE LOST IF YOU DO\r\n"
		"// User options script\r\n"
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
	m_pszDescriptionType = _strdup( "INFO_OPTIONS" );
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::OnPaint (0x412860)

void CSetInfoDlg::OnPaint()
{
	PaintSkinnedDialog();
}

/////////////////////////////////////////////////////////////////////////////
// CSetInfoDlg::OnEraseBkgnd (0x412870)

BOOL CSetInfoDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	PaintSkinnedDialog();
	return TRUE;
}

