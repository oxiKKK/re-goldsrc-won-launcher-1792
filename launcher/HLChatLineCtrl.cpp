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
// Purpose: CHLChatLineCtrl, the chat input line the internet-games page
//          hosts.  Submit parses the leading '/' commands itself.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

// The typed line, assembled here and handed to the transport.
static char	s_szLine[512];			// 0x4E1F3C

BEGIN_MESSAGE_MAP( CHLChatLineCtrl, CBorderLessEdit )
	//{{AFX_MSG_MAP(CHLChatLineCtrl)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHLChatLineCtrl::CHLChatLineCtrl (0x415130)

CHLChatLineCtrl::CHLChatLineCtrl( CWnd* pOwner )
{
	m_pOwner    = (CServerBrowserDlg*)pOwner;
	m_clrChatBk = RGB( 63, 63, 63 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLChatLineCtrl::~CHLChatLineCtrl (0x415180)

CHLChatLineCtrl::~CHLChatLineCtrl()
{
}

/////////////////////////////////////////////////////////////////////////////
// CHLChatLineCtrl::Submit (0x4151A0)
//
// Run the leading-'/' commands ourselves and otherwise hand the text to the
// chat transport.

void CHLChatLineCtrl::Submit( const char* pszWhisperTarget )
{
	CServerBrowserDlg*	pPage = m_pOwner;
	CNetGameDlg*		pSheet;
	const char*			pszNick;

	if ( !pPage )
		return;

	pSheet = pPage->GetBrowserEngine();
	if ( !pSheet || !pSheet->m_pSelfIdentity )
		return;

	pSheet->GetCurrentRoom();

	if ( !m_pEdit )
		return;

	memset( s_szLine, 0, sizeof( s_szLine ) );
	m_pEdit->GetWindowText( s_szLine, sizeof( s_szLine ) );
	if ( !strlen( s_szLine ) )
		return;

	strcat( s_szLine, "\r\n" );

	if ( s_szLine[0] == '/' && strlen( s_szLine ) > 1 )
	{
		CToken	tok( &s_szLine[1] );

		tok.ParseNextToken();
		if ( !_strnicmp( tok.token, "rooms", 5 ) )
		{
			pSheet->ListRooms();
			m_pEdit->SetWindowText( "" );
			::InvalidateRect( m_pEdit->m_hWnd, NULL, TRUE );
			::UpdateWindow( m_pEdit->m_hWnd );
			return;
		}
		if ( !_strnicmp( tok.token, "find", 4 ) )
		{
			tok.ParseNextToken();
			if ( strlen( tok.token ) )
				pSheet->FindPlayer( tok.token );
			m_pEdit->SetWindowText( "" );
			::InvalidateRect( m_pEdit->m_hWnd, NULL, TRUE );
			::UpdateWindow( m_pEdit->m_hWnd );
			return;
		}
		if ( !_strnicmp( tok.token, "auth", 4 ) )
		{
			pSheet->ChatPrintf( "Requesting certificate" );
			pSheet->ChatPrintf( pSheet->Authenticate( 1 ) ? "Success" : "Failed" );
			m_pEdit->SetWindowText( "" );
			::InvalidateRect( m_pEdit->m_hWnd, NULL, TRUE );
			::UpdateWindow( m_pEdit->m_hWnd );
			return;
		}
		if ( !_strnicmp( tok.token, "join", 4 ) )
		{
			tok.GetRemainder();
			if ( strlen( tok.token ) )
			{
				pSheet->ChatPrintf( Launcher_LoadString( IDS_CHAT_JOIN ), tok.token );
				pSheet->JoinRoomByName( tok.token );
			}
			m_pEdit->SetWindowText( "" );
			::InvalidateRect( m_pEdit->m_hWnd, NULL, TRUE );
			::UpdateWindow( m_pEdit->m_hWnd );
			return;
		}
	}

	// Plain text: send it and clear the input.  There is deliberately no local
	// echo here -- the room server broadcasts the line back to every member,
	// including the sender, and OnChatText is what puts it on screen.  Echoing
	// as well would print it twice.
	if ( strlen( s_szLine ) )
	{
		pSheet->SendChatText( s_szLine, (int)strlen( s_szLine ) );
		m_pEdit->SetWindowText( "" );
		::InvalidateRect( m_pEdit->m_hWnd, NULL, TRUE );
		::UpdateWindow( m_pEdit->m_hWnd );
		return;
	}

	// NOTE(ox): 0x4151a0's else arm.  s_szLine always carries the "\r\n" appended
	// above, so this is unreachable in practice; kept because the binary has it.
	if ( pszWhisperTarget )
		pPage->GetChatText()->SetWhisperTarget( pszWhisperTarget );

	if ( pPage->GetChatText() )
	{
		pszNick = NET_CleanServerName( pSheet->m_pSelfIdentity->GetPlayerName() );
		ChatWnd_Printf( pPage->GetChatText(), pszNick, s_szLine );
	}

	m_pEdit->SetWindowText( "" );
	::InvalidateRect( m_pEdit->m_hWnd, NULL, TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// CHLChatLineCtrl::OnChar (0x415520)

void CHLChatLineCtrl::OnChar( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	if ( !m_pOwner->GetBrowserEngine() )
		return;

	if ( nChar == VK_RETURN )
	{
		Submit( NULL );
		return;
	}

	CBorderLessEdit::OnChar( nChar, nRepCnt, nFlags );
}

/////////////////////////////////////////////////////////////////////////////
// CHLChatLineCtrl::ClearEditSelection (0x415560)

void CHLChatLineCtrl::ClearEditSelection()
{
	m_pEdit->SendMessage( WM_CLEAR, 0, 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLChatLineCtrl::PreTranslateMessage (0x415580)

BOOL CHLChatLineCtrl::PreTranslateMessage( MSG* pMsg )
{
	if ( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN )
	{
		Submit( NULL );
		return TRUE;
	}
	return CWnd::PreTranslateMessage( pMsg );
}
