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
// Purpose: CDlgPopupBase, the base for the skinned popup dialogs.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/////////////////////////////////////////////////////////////////////////////
// WM_SYSTIMER (0x118)
//
// : undocumented internal timer message, not in <winuser.h>.

#ifndef WM_SYSTIMER
#define WM_SYSTIMER		0x0118
#endif
/////////////////////////////////////////////////////////////////////////////
// CDlgPopupBase::RunModalLoop (0x40BA50)

int CDlgPopupBase::RunModalLoop( DWORD dwFlags )
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
		// frame burst until the popup reports idle (or quits)
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
					bIdle = FALSE;
					break;
				}
			}
		}
		RMLPrePump();

		// a progress popup keeps ticking instead of blocking on the queue
		while ( m_bModalProgressPopup && !::PeekMessage( pMsg, NULL, NULL, NULL, PM_NOREMOVE ) )
		{
			RMLPostPump();
			do
			{
				nFrame = RMLPreIdle();
				if ( nFrame < 0 )
					goto ExitModal;
			} while ( nFrame > 0 );
		}

		// pump the queue (blocks here when idle and the pump flag is off)
		do
		{
			if ( !AfxGetThread()->PumpMessage() )
			{
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
// CDlgPopupBase::DoModal (0x40BC40)

#if defined(_MSC_VER) && (_MSC_VER < 1300)
int CDlgPopupBase::DoModal()		// VC6: CDialog::DoModal returns int
#else
INT_PTR CDlgPopupBase::DoModal()
#endif
{
	LPCDLGTEMPLATE lpDialogTemplate = m_lpDialogTemplate;
	HGLOBAL hDialogTemplate = m_hDialogTemplate;
	HINSTANCE hInst = AfxGetResourceHandle();
	if ( m_lpszTemplateName != NULL )
	{
		hInst = AfxFindResourceHandle( m_lpszTemplateName, RT_DIALOG );
		HRSRC hResource = ::FindResource( hInst, m_lpszTemplateName, RT_DIALOG );
		hDialogTemplate = ::LoadResource( hInst, hResource );
	}
	if ( hDialogTemplate != NULL )
		lpDialogTemplate = (LPCDLGTEMPLATE)::LockResource( hDialogTemplate );
	if ( lpDialogTemplate == NULL )
		return -1;

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
				m_nModalResult = RunModalLoop( dwFlags );
			}

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
// CDlgPopupBase::CDlgPopupBase (0x40BDF0)

CDlgPopupBase::CDlgPopupBase( UINT nIDTemplate, CWnd* pParent )
	: CDialog( nIDTemplate, pParent )
{
	m_pPaintWnd = NULL;
	m_bModalProgressPopup = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CDlgPopupBase::~CDlgPopupBase (0x40BE40)

CDlgPopupBase::~CDlgPopupBase()
{
}

/////////////////////////////////////////////////////////////////////////////
// CDlgPopupBase::OnPaint (0x40BE50)

void CDlgPopupBase::OnPaint()
{
	if ( !m_pPaintWnd )
		return;

	CPaintDC	dc( m_pPaintWnd );

	CRect	rc;
	GetClientRect( &rc );

	CDC		dcMem;
	if ( !dcMem.CreateCompatibleDC( &dc ) )
		return;

	CBitmap	bmMem;
	bmMem.CreateCompatibleBitmap( &dc, rc.Width(), rc.Height() );
	CBitmap*	pbmOld = dcMem.SelectObject( &bmMem );

	dcMem.FillSolidRect( &rc, RGB( 56, 56, 56 ) );

	DrawPopupContent( &dcMem, &rc );

	dc.BitBlt( rc.left, rc.top, rc.Width(), rc.Height(), &dcMem, 0, 0, SRCCOPY );

	dcMem.SelectObject( pbmOld );
}

/////////////////////////////////////////////////////////////////////////////
// CDlgPopupBase::PreTranslateMessage (0x40A4A0)
//
// ENTER fires the focused button instead of the dialog's default one.  Byte
// identical to CDlgBase's, so the linker folded the two.

BOOL CDlgPopupBase::PreTranslateMessage( MSG* pMsg )
{
#ifdef LAUNCHER_FIXES
	if ( pMsg->message == WM_MOUSEWHEEL && Dlg_RouteMouseWheel( pMsg ) )
		return TRUE;
#endif

	if ( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN )
	{
		CWnd*	pFocus = CWnd::FromHandlePermanent( ::GetFocus() );
		CButton*	pBtn = (CButton*)AfxDynamicDownCast( RUNTIME_CLASS( CButton ), pFocus );
		if ( !pBtn )
			return CWnd::PreTranslateMessage( pMsg );
		::SendMessageA( m_hWnd, WM_COMMAND, pBtn->GetDlgCtrlID(), (LPARAM)pBtn->m_hWnd );
		return TRUE;
	}
	return CDialog::PreTranslateMessage( pMsg );
}

/////////////////////////////////////////////////////////////////////////////
// CDlgPopupBase::OnEngineFrame (0x45B4E0)

int CDlgPopupBase::OnEngineFrame()
{
	return Eng_Frame( gBackground );
}
