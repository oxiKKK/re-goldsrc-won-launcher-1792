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
// Purpose: the intro logo dialog (CLogoDlg, IDD 202) and the master-list fetch
//          object.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// One-shot: the Sierra logo plays only on the first CLogoDlg of the process
static int	g_bPlaySierraLogo = 1;

// Command ids the dialog posts to itself to drive the intro.  0x76 kicks off
// the AVI sequence (posted from OnInitDialog); 0x69 arms the run.
#define IDC_LOGO_START		105
#define IDC_LOGO_PLAY		118

/////////////////////////////////////////////////////////////////////////////
// WM_SYSTIMER (0x118)
//
// : undocumented internal timer message, not in <winuser.h>.
// The modal loop treats it (like WM_SYSKEYDOWN) as a late-show trigger.

#ifndef WM_SYSTIMER
#define WM_SYSTIMER		0x0118
#endif

BEGIN_MESSAGE_MAP( CLogoDlg, CDialog )
	//{{AFX_MSG_MAP(CLogoDlg)
	ON_WM_ERASEBKGND()
	ON_COMMAND( IDC_LOGO_START, OnLogoStart )
	ON_COMMAND( IDC_LOGO_PLAY, PlayIntroSequence )
	ON_WM_PAINT()
	ON_WM_ACTIVATEAPP()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
/*
==================
Launcher_FormatMCIError (0x427D30)
==================
*/
static const char* Launcher_FormatMCIError( MCIERROR mcierr )
{
	static char	s_szText[512];
	char		mciText[256];

	memset( mciText, 0, sizeof( mciText ) );
	if ( !mciGetErrorStringA( mcierr, mciText, sizeof( mciText ) ) )
		return "Unknown";
	sprintf( s_szText, "%i: %s", (int)mcierr, mciText );
	return s_szText;
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::RunModalLoop (0x427DA0)
//
// The dialog's own copy of the frame-protocol loop.  Unlike CDlgBase's it never
// blocks in GetMessage and never calls VGui_Frame: the intro has to keep ticking
// RMLPreIdle so the DIB pair advances while the AVI plays.

int CLogoDlg::RunModalLoop( DWORD dwFlags )
{
	BOOL	bIdle = TRUE;			// MFC idle protocol latch
	LONG	lIdleCount = 0;
	BOOL	bShowIdle = ( dwFlags & MLF_SHOWONIDLE ) && !( GetStyle() & WS_VISIBLE );
	HWND	hWndParent = ::GetParent( m_hWnd );
#if defined(_MSC_VER) && (_MSC_VER < 1300)
	MSG*	pMsg = &( AfxGetThread()->m_msgCur );		// VC6: m_msgCur lives on CWinThread
#else
	MSG*	pMsg = &( AfxGetThreadState()->m_msgCur );
#endif
	int		nFrame;

	m_nFlags |= ( WF_MODALLOOP | WF_CONTINUEMODAL );

	RMLSetup();

	for ( ;; )
	{
		// frame burst until the dialog reports idle (or quits)
		do
		{
			nFrame = RMLPreIdle();
			if ( nFrame < 0 )
				goto ExitModal;
		} while ( nFrame > 0 );

		// the stock MFC idle protocol, once per idle stretch
		if ( bIdle )
		{
			while ( !::PeekMessage( pMsg, NULL, NULL, NULL, PM_NOREMOVE ) )
			{
				if ( bShowIdle )
				{
					ShowWindow( SW_SHOWNORMAL );
					UpdateWindow();
					bShowIdle = FALSE;
				}
				RMLIdle();
				if ( !( dwFlags & MLF_NOIDLEMSG ) && hWndParent != NULL && lIdleCount == 0 )
					::SendMessage( hWndParent, WM_ENTERIDLE, MSGF_DIALOGBOX, (LPARAM)m_hWnd );
				if ( ( dwFlags & MLF_NOKICKIDLE ) ||
					 !SendMessage( WM_KICKIDLE, MSGF_DIALOGBOX, lIdleCount++ ) )
				{
					// no more idle work wanted
					bIdle = FALSE;
					break;
				}
			}
		}
		RMLPrePump();

		do
		{
			// an empty queue drops straight through to the next frame tick
			if ( !::PeekMessage( pMsg, NULL, NULL, NULL, PM_NOREMOVE ) )
				continue;

			if ( !AfxGetThread()->PumpMessage() )
			{
				// WM_QUIT -- forward it and bail
				AfxPostQuitMessage( 0 );
				return -1;
			}
			RMLPump();

			// late show: certain messages force the window up
			if ( bShowIdle && ( pMsg->message == WM_SYSTIMER ||
								pMsg->message == WM_SYSKEYDOWN ) )
			{
				ShowWindow( SW_SHOWNORMAL );
				UpdateWindow();
				bShowIdle = FALSE;
			}

			if ( !ContinueModal() )
				goto ExitModal;

			if ( AfxGetThread()->IsIdleMessage( pMsg ) )
			{
				bIdle = TRUE;
				lIdleCount = 0;
			}
		} while ( ::PeekMessage( pMsg, NULL, NULL, NULL, PM_NOREMOVE ) );

		RMLPostPump();
	}

ExitModal:
	m_nFlags &= ~( WF_MODALLOOP | WF_CONTINUEMODAL );
	return m_nModalResult;
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::DoModal (0x427F90)

#if defined(_MSC_VER) && (_MSC_VER < 1300)
int CLogoDlg::DoModal()			// VC6: CDialog::DoModal returns int
#else
INT_PTR CLogoDlg::DoModal()
#endif
{
	// resolve the dialog template exactly as CDialog does
	LPCDLGTEMPLATE lpDialogTemplate = m_lpDialogTemplate;
	HGLOBAL hDialogTemplate = m_hDialogTemplate;
	HINSTANCE hInst = AfxGetResourceHandle();
	if ( m_lpszTemplateName != NULL )
	{
		hInst = AfxGetResourceHandle();
		HRSRC hResource = ::FindResource( hInst, m_lpszTemplateName, RT_DIALOG );
		hDialogTemplate = ::LoadResource( hInst, hResource );
	}
	if ( hDialogTemplate != NULL )
		lpDialogTemplate = (LPCDLGTEMPLATE)::LockResource( hDialogTemplate );
	if ( lpDialogTemplate == NULL )
		return -1;

	// disable the parent for the modal stretch
	HWND hWndParent = PreModal();
	BOOL bEnableParent = FALSE;
	if ( hWndParent != NULL && ::IsWindowEnabled( hWndParent ) )
	{
		::EnableWindow( hWndParent, FALSE );
		bEnableParent = TRUE;
	}

	TRY
	{
		if ( CreateDlgIndirect( lpDialogTemplate,
				CWnd::FromHandle( hWndParent ), hInst ) )
		{
			if ( m_nFlags & WF_CONTINUEMODAL )
			{
				DWORD dwFlags = MLF_SHOWONIDLE;
				if ( GetStyle() & DS_NOIDLEMSG )
					dwFlags |= MLF_NOIDLEMSG;
				// (sic) the result is dropped -- EndDialog already stored it
				RunModalLoop( dwFlags );
			}

			// hide before destruction so the teardown never paints
			if ( m_hWnd != NULL )
				SetWindowPos( NULL, 0, 0, 0, 0, SWP_HIDEWINDOW |
					SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER );
		}
	}
	END_TRY

	if ( bEnableParent )
		::EnableWindow( hWndParent, TRUE );
	if ( hWndParent != NULL && ::GetActiveWindow() == m_hWnd )
		::SetActiveWindow( hWndParent );

	DestroyWindow();
	PostModal();
	return m_nModalResult;
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::CLogoDlg (0x428130)

CLogoDlg::CLogoDlg( CWnd* pParent )
	: CDialog( IDD_LOGO, pParent )
{
	m_bDibLogo = 0;
	m_hDib1 = NULL;
	m_hDib2 = NULL;
	m_flDelay = 2.0f;
	m_flLastTime = 0.0f;
	m_nActiveDib = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::~CLogoDlg (0x428190)

CLogoDlg::~CLogoDlg()
{
	if ( m_hDib1 )
	{
		GlobalFree( m_hDib1 );
		m_hDib1 = NULL;
	}
	if ( m_hDib2 )
	{
		GlobalFree( m_hDib2 );
		m_hDib2 = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::RMLPreIdle (0x4281D0)

int CLogoDlg::RMLPreIdle()
{
	if ( !m_bDibLogo )
		return 0;

	float now = (float)engineapi.Sys_FloatTime();
	if ( now - m_flLastTime < m_flDelay )
		return 0;

	if ( m_nActiveDib == 1 )
	{
		m_flLastTime = now;
		m_nActiveDib = 2;
		::InvalidateRect( GetSafeHwnd(), NULL, TRUE );
		::UpdateWindow( GetSafeHwnd() );
		return 0;
	}

	FlushInputAndClose();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::OnInitDialog (0x428240)

BOOL CLogoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	Dlg_CenterWindow( this );
	::PostMessageA( GetSafeHwnd(), WM_COMMAND, IDC_LOGO_PLAY, 0 );
	m_flLastTime = (float)engineapi.Sys_FloatTime();
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::OnEraseBkgnd (0x428280)

BOOL CLogoDlg::OnEraseBkgnd( CDC* /*pDC*/ )
{
	CPaintDC	dc( this );
	RECT		rc;

	::GetClientRect( GetSafeHwnd(), &rc );

	CDC		mem;
	if ( !mem.CreateCompatibleDC( &dc ) )
		return FALSE;

	if ( m_bDibLogo && m_hDib1 && m_hDib2 )
	{
		DIB_BlitDib( dc.GetSafeHdc(), &rc,
			( m_nActiveDib == 1 ) ? m_hDib1 : m_hDib2, &rc );
	}
	else
	{
		dc.PatBlt( 0, 0, rc.right - rc.left, rc.bottom - rc.top, BLACKNESS );
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::OnLogoStart (0x4283E0)

void CLogoDlg::OnLogoStart()
{
	if ( !m_bDibLogo )
		OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::FlushInputAndClose (0x4283F0)

void CLogoDlg::FlushInputAndClose()
{
	MSG		msg;

	Sleep( 50 );
	while ( ::PeekMessage( &msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_NOREMOVE ) )
	{
		if ( !::GetMessage( &msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST ) )
			::PostQuitMessage( 0 );
	}
	while ( ::PeekMessage( &msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_NOREMOVE ) )
	{
		if ( !::GetMessage( &msg, NULL, WM_KEYFIRST, WM_KEYLAST ) )
			::PostQuitMessage( 0 );
	}

	OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::OnPaint (0x4284C0)

void CLogoDlg::OnPaint()
{
	CPaintDC	dc( this );
	RECT		rc;

	::GetClientRect( GetSafeHwnd(), &rc );

	CDC		mem;
	if ( !mem.CreateCompatibleDC( &dc ) )
		return;

	CBitmap		bmp;
	bmp.Attach( ::CreateCompatibleBitmap( dc.GetSafeHdc(),
		rc.right - rc.left, rc.bottom - rc.top ) );
	CBitmap*	pOld = mem.SelectObject( &bmp );

	if ( m_bDibLogo && m_hDib1 && m_hDib2 )
	{
		DIB_BlitDib( mem.GetSafeHdc(), &rc,
			( m_nActiveDib == 1 ) ? m_hDib1 : m_hDib2, &rc );
	}
	else
	{
		CBrush	black( (COLORREF)0 );
		mem.FillRect( &rc, &black );
	}

	dc.BitBlt( rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
		&mem, 0, 0, SRCCOPY );
	mem.SelectObject( pOld );
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::PlayIntroSequence (0x4286F0)

void CLogoDlg::PlayIntroSequence()
{
	int	wh[2];

	if ( m_bDibLogo )
		return;

	if ( gEngineModeWindowed )
	{
		if ( lpDD )
			lpDD->Release();
		lpDD = NULL;
	}
	else
	{
		if ( !DDraw_IsModeAvailable() )
		{
			Launcher_ShowMessageById( 0, IDS_DDRAW_REQUIRED );
			FlushInputAndClose();
		}
		DDraw_Init( 1, 0 );
		if ( !lpDD )
		{
			Launcher_ShowMessageById( 0, IDS_DDRAW_REQUIRED );
			FlushInputAndClose();
		}
		DDraw_SetDisplayMode( -1 );
	}

	wh[0] = 640;
	wh[1] = 480;
	if ( g_bPlaySierraLogo )
	{
		Launcher_PlayAVI( this, "sierra", wh );
		g_bPlaySierraLogo = 0;
	}

	// black the client between the two clips
	{
		CClientDC	dc( this );
		RECT		rc;
		::GetClientRect( GetSafeHwnd(), &rc );
		CBrush		black( (COLORREF)0 );
		dc.FillRect( &rc, &black );
		UpdateWindow();
	}

	wh[0] = 640;
	wh[1] = 480;
	Launcher_PlayAVI( this, "valve", wh );

	::ShowWindow( gLauncherWnd, SW_SHOW );
	::SetActiveWindow( gLauncherWnd );
	FlushInputAndClose();
	UpdateWindow();
}

/*
==================
Launcher_PlayAVI (0x4288A0)
==================
*/
void Launcher_PlayAVI( CWnd* pWnd, const char* pszName, int* pWH )
{
	char	alias[64];
	char	avi[128];
	char	cmd[1024];
	char	ret[1024];
	RECT	rc;
	MCIERROR	err;

	if ( !pszName )
		return;

	::GetWindowRect( pWnd->GetSafeHwnd(), &rc );
	rc.top = ( 480 - pWH[1] ) / 2;
	rc.bottom = rc.top + pWH[1];
	rc.left = ( 640 - pWH[0] ) / 2;
	rc.right = rc.left + pWH[0];

	sprintf( alias, "sierravideo" );
	sprintf( avi, "media\\%s.avi", pszName );

	sprintf( cmd, "open %s type AVIVideo alias %s parent %d style child wait",
		COM_FindPath( avi ), alias, (int)(INT_PTR)pWnd->GetSafeHwnd() );
	err = mciSendStringA( cmd, ret, sizeof( ret ), NULL );
	if ( err )
	{
		Launcher_ShowMessageByIdEx( 0, IDS_MCI_OPENFAIL, Launcher_FormatMCIError( err ) );
		return;
	}

	if ( !mciGetDeviceIDA( alias ) )
	{
		Launcher_ShowMessageById( 0, IDS_MCI_GETIDFAIL );
		return;
	}

	sprintf( cmd, "window %s handle %d state restore wait", alias,
		(int)(INT_PTR)pWnd->GetSafeHwnd() );
	err = mciSendStringA( cmd, ret, sizeof( ret ), NULL );
	if ( err )
	{
		Launcher_ShowMessageByIdEx( 0, IDS_MCI_WINDOWFAIL, Launcher_FormatMCIError( err ) );
		return;
	}

	sprintf( cmd, "put %s destination at %i %i %i %i wait", alias,
		rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top );
	err = mciSendStringA( cmd, ret, sizeof( ret ), NULL );
	if ( err )
	{
		Launcher_ShowMessageByIdEx( 0, IDS_MCI_PUTFAIL, Launcher_FormatMCIError( err ) );
		return;
	}

	sprintf( cmd, "seek %s to start wait", alias );
	err = mciSendStringA( cmd, ret, sizeof( ret ), NULL );
	if ( err )
	{
		Launcher_ShowMessageByIdEx( 0, IDS_MCI_SEEKFAIL, Launcher_FormatMCIError( err ) );
		return;
	}

	sprintf( cmd, "break %s on 27 wait", alias );	// ESC skips
	err = mciSendStringA( cmd, ret, sizeof( ret ), NULL );
	if ( err )
		Launcher_ShowMessageByIdEx( 0, IDS_MCI_BREAKFAIL, Launcher_FormatMCIError( err ) );

	sprintf( cmd, "play %s wait", alias );
	err = mciSendStringA( cmd, ret, sizeof( ret ), NULL );
	if ( err )
	{
		Launcher_ShowMessageByIdEx( 0, IDS_MCI_PLAYFAIL, Launcher_FormatMCIError( err ) );
	}
	else
	{
		sprintf( cmd, "stop %s wait", alias );
		err = mciSendStringA( cmd, ret, sizeof( ret ), NULL );
		if ( err )
			Launcher_ShowMessageByIdEx( 0, IDS_MCI_STOPFAIL, Launcher_FormatMCIError( err ) );
	}

	sprintf( cmd, "close %s wait", alias );
	err = mciSendStringA( cmd, ret, sizeof( ret ), NULL );
	if ( err )
		Launcher_ShowMessageByIdEx( 0, IDS_MCI_CLOSEFAIL, Launcher_FormatMCIError( err ) );
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::OnChar (0x428BD0)
//
// the intro swallows keystrokes.

void CLogoDlg::OnChar( UINT /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/ )
{
}

/////////////////////////////////////////////////////////////////////////////
// CLogoDlg::OnActivateApp (0x406FE0)

void CLogoDlg::OnActivateApp( BOOL bActive, DWORD dwThreadID )
{
	ActiveApp = bActive;
	CDialog::OnActivateApp( bActive, dwThreadID );
}

/*
==================
MasterFetch_Init (0x428BE0)
==================
*/
void MasterFetch_Init( masterfetch_t* p, CNetGameDlg* pOwner,
					   void ( *pfnDone )( void* pOwner, int nResult ),
					   void ( *pfnStatus )( void* pOwner, const char* pszFormat, ... ) )
{
	p->m_pfnDone    = pfnDone;
	p->m_pOwner     = pOwner;
	p->m_flLastSend = 0.0;
	p->m_pfnStatus  = pfnStatus;
	p->m_bBusy      = 0;
	p->m_pSocket    = NULL;

	// The binary's Init stops at the six fields above and leaves the host and
	// port as heap fill.  MasterFetch_Service's progress chatter formats the
	// host with %s on its very first pass, before any successful Connect has
	// written it, so the fill is what gets printed -- and an allocator that
	// hands back a 1024-byte run with no NUL in it (the debug CRT's 0xCD) runs
	// the status text off the end of its buffer.
	p->m_szHost[0]  = 0;
	p->m_nPort      = 0;
}

/*
==================
MasterFetch_CloseSocket (0x428C10)
==================
*/
void MasterFetch_CloseSocket( masterfetch_t* p )
{
	if ( p && p->m_pSocket )
	{
		delete p->m_pSocket;
		p->m_pSocket = NULL;
	}
}

/*
==================
MasterFetch_Request (0x428C30)
==================
*/
void MasterFetch_Request( masterfetch_t* p, const char* pszHost, unsigned int nPort,
						  const char* pszFilter )
{
	if ( !p )
		return;

	p->m_bBusy      = 1;
	p->m_flLastSend = engineapi.Sys_FloatTime();

	MasterFetch_CloseSocket( p );
	if ( !p->m_pSocket )
	{
		ServerBrowser_CreateMasterSocket( p->m_pOwner, &p->m_pSocket );
		if ( p->m_pSocket )
		{
			// Two retries at 50 ms before the host is written off.
			for ( int nTries = 2; !p->m_pSocket->Connect( pszHost, nPort ); --nTries )
			{
				Sleep( 50 );
				if ( nTries - 1 < 0 )
				{
					MasterFetch_CloseSocket( p );
					return;
				}
			}
		}
	}

	strcpy( p->m_szHost, pszHost );
	p->m_nPort = (int)nPort;

	// The binary walks straight into the socket from here; a CreateMasterSocket
	// that came back empty would take it through a null this.
	if ( !p->m_pSocket )
		return;

	p->m_pSocket->SetListDone( 0 );
	p->m_pSocket->Reset();
	if ( pszFilter )
		p->m_pSocket->SetFilter( pszFilter );
	p->m_pSocket->RequestServerBatch( 0 );
	p->m_pSocket->BeginFetch();
}

/*
==================
MasterFetch_Start (0x428D20)

Rewind the favourites' master-host walk and fire the first request; an empty
host list reports failure through the done callback straight away.
==================
*/
void MasterFetch_Start( masterfetch_t* p, const char* pszFilter )
{
	p->m_pOwner->SetDirty( 0 );
	gFavorites->BeginMasterList();

	if ( !gFavorites->NextMasterList() )
	{
		p->m_bBusy = 0;
		p->m_pfnDone( p->m_pOwner, 1 );			// no master hosts configured
		return;
	}

	MasterFetch_Request( p, gFavorites->GetMasterAddr(), gFavorites->GetMasterPort(),
						 pszFilter );
}

/*
==================
MasterFetch_Service (0x428D80)

Drive one master host at a time: resend once on a 4 s stall, roll onto the next
host when a host goes quiet with nothing to show, and report completion through
the done callback.
==================
*/
void MasterFetch_Service( masterfetch_t* p )
{
	static double	s_flLastReport;

	if ( !p->m_bBusy )
		return;

	double	flNow    = engineapi.Sys_FloatTime();
	double	flLastReq = p->m_flLastSend;
	int		bRollOver = 1;

	if ( p->m_pSocket )
	{
		flLastReq = p->m_pSocket->m_flLastSend;
		if ( flNow - flLastReq <= 4.0 )
		{
			bRollOver = 0;
		}
		else
		{
			// The host has gone quiet: spend one retry resending before giving up
			// on it.
			p->m_pSocket->DecTries();
			if ( p->m_pSocket->HasTries() )
			{
				p->m_pSocket->FlushSend();
				return;
			}

			// Retries gone.  Anything already parsed counts as a finished list;
			// an empty one moves us to the next master host.
			if ( p->m_pSocket->m_nServers )
			{
				p->m_pfnStatus( p->m_pOwner, "Finished receiving list from %s:%i, transferring data...\n",
								p->m_szHost, p->m_nPort );
				p->m_bBusy = 0;
				p->m_pfnDone( p->m_pOwner, 0 );
				return;
			}
		}
	}

	if ( bRollOver )
	{
		if ( !gFavorites->NextMasterList() )
		{
			p->m_bBusy = 0;
			p->m_pfnDone( p->m_pOwner, 2 );		// out of hosts
			return;
		}
		flLastReq = flNow;
		MasterFetch_Request( p, gFavorites->GetMasterAddr(), gFavorites->GetMasterPort(), NULL );
	}

	if ( p->m_pSocket && p->m_pSocket->m_dwListDone )
	{
		p->m_pfnStatus( p->m_pOwner, "Finished receiving list from %s:%i, transferring data...\n",
						p->m_szHost, p->m_nPort );
		p->m_bBusy = 0;
		p->m_pfnDone( p->m_pOwner, 0 );
		return;
	}

	// Progress chatter, throttled to ~10 Hz.
	if ( s_flLastReport == 0.0 || flNow - s_flLastReport > 0.1 )
	{
		s_flLastReport = flNow;
		double	flLeft = 4.0 - ( flNow - flLastReq );
		if ( p->m_pSocket && p->m_pSocket->m_nServers )
		{
			p->m_pfnStatus( p->m_pOwner, "Receiving list from %s:%i, %i received, %.1f remaining\n",
							p->m_szHost, p->m_nPort, p->m_pSocket->m_nServers, flLeft );
		}
		else
		{
			p->m_pfnStatus( p->m_pOwner, "Requesting list from %s:%i, %.1f remaining\n",
							p->m_szHost, p->m_nPort, flLeft );
		}
	}
}
