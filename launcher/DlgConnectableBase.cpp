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
// Purpose: CDlgConnectableBase, the base for pages that can join a server.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgConnectableBase::CDlgConnectableBase (0x40AFA0)

CDlgConnectableBase::CDlgConnectableBase( UINT nIDTemplate, CWnd* pParent )
	: CDlgBase( nIDTemplate, pParent )
{
}

/////////////////////////////////////////////////////////////////////////////
// CDlgConnectableBase::RefreshAfterGameDirChange (0x40AFF0)
//
// the binary re-derives
// GetParent(m_hWnd) for every step instead of caching it (sic).

void CDlgConnectableBase::RefreshAfterGameDirChange()
{
	InitButtonStrips();
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );

	if ( !CWnd::FromHandle( ::GetParent( m_hWnd ) ) )
		return;

	CMultiSelectDlg*	pHub =
		dynamic_cast<CMultiSelectDlg*>( CWnd::FromHandle( ::GetParent( m_hWnd ) ) );
	if ( pHub )
	{
		pHub->InitMembers();
	}
	else
	{
		CHLMainDlg*	pMenu =
			dynamic_cast<CHLMainDlg*>( CWnd::FromHandle( ::GetParent( m_hWnd ) ) );
		if ( pMenu )
			pMenu->RefreshDialogSkin();
	}

	::InvalidateRect( CWnd::FromHandle( ::GetParent( m_hWnd ) )->m_hWnd, NULL, TRUE );
	::UpdateWindow( CWnd::FromHandle( ::GetParent( m_hWnd ) )->m_hWnd );

	CWnd*	pGrand = CWnd::FromHandle(
		::GetParent( CWnd::FromHandle( ::GetParent( m_hWnd ) )->m_hWnd ) );
	if ( !pGrand )
		return;

	CHLMainDlg*	pGrandMenu = dynamic_cast<CHLMainDlg*>( pGrand );
	if ( pGrandMenu )
		pGrandMenu->RefreshDialogSkin();
	::InvalidateRect( pGrand->m_hWnd, NULL, TRUE );
	::UpdateWindow( pGrand->m_hWnd );
}

/////////////////////////////////////////////////////////////////////////////
// CDlgConnectableBase::VerifyServerBeforeConnect (0x40B140)
//
// re-query the record when
// it reads full or its ping is older than 5 s, then re-test both.

int CDlgConnectableBase::VerifyServerBeforeConnect( CNetGameDlg* pTarget, CServerInfo* pInfo )
{
	if ( CheckParm( "-nocheck", NULL ) )
		return 1;

	double	flNow = engineapi.Sys_FloatTime();

	int	bFull = ( pInfo->m_nMaxPlayers > 0 && pInfo->m_nCurrentPlayers >= pInfo->m_nMaxPlayers );
	if ( !bFull && flNow - pInfo->GetPingTime() <= 5.0 )
		return 1;

	RefreshCriteria_t	criteria;
	memset( &criteria, 0, sizeof( criteria ) );
	criteria.m_nMaxOutstanding = 1;
	criteria.m_nMaxRetries     = ( pTarget->m_nRetries <= 3 ) ? pTarget->m_nRetries : 3;
	criteria.m_dStateTimeout   = pTarget->m_dTimeout;
	criteria.m_nPhaseMask      = 2;
	criteria.m_flOverallTimeout = 3.0;
	criteria.m_bReportErrors   = 1;

	// Query this record alone: unlink it for the duration of the pass.
	CServerInfo*	pSavedNext = pInfo->m_pNext;
	pInfo->m_pNext = NULL;
	pInfo->SetPingTime( -1.0 );

	CRefreshDlg	refresh( &criteria, pInfo, NULL );
	refresh.DoModal();

	pInfo->m_pNext = pSavedNext;

	if ( pInfo->GetPingTime() < 0.0 )
	{
		Launcher_ShowMessageByIdEx( NULL, IDS_MAIN_NOMEMMAPCONNECT, (LPCSTR)pInfo->m_strName );
		::InvalidateRect( m_hWnd, NULL, TRUE );
		::UpdateWindow( m_hWnd );
		return 0;
	}

	if ( pInfo->m_nMaxPlayers > 0 && pInfo->m_nCurrentPlayers >= pInfo->m_nMaxPlayers )
	{
		Launcher_ShowMessageByIdEx( NULL, IDS_CONN_FULL, (LPCSTR)pInfo->m_strName );
		::InvalidateRect( m_hWnd, NULL, TRUE );
		::UpdateWindow( m_hWnd );
		return 0;
	}

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CDlgConnectableBase::CheckModVersion (0x40B510)
//
// warn when the server runs a
// different build of the mod than the one installed.  Suppressible via the
// "Mod Version Prompt" profile flag; returns 0 only when the user cancels.

int CDlgConnectableBase::CheckModVersion( CServerInfo* pInfo, mod_t* pMod )
{
	if ( !pMod->GetKeyInt( "version" ) )
		return 1;

	if ( pMod->GetKeyInt( "version" ) == pInfo->m_nVersion )
		return 1;

	char	szMsg[1024];
	Launcher_LoadStringInto( szMsg, 0x22A, pInfo->m_nVersion, (LPCSTR)pInfo->m_strDir,
							 pMod->GetKeyInt( "version" ) );

	// The flag is read then written straight back, so a missing key gets created.
	int	bPrompt = Launcher_GetProfileInt( "Settings", "Mod Version Prompt", 1 );
	Launcher_WriteProfileInt( "Settings", "Mod Version Prompt", bPrompt );
	if ( !bPrompt )
		return 1;

	CPromptDlg	prompt( (int)0x80000002, NULL );
	prompt.SetMessage( szMsg );
	prompt.SetCheckboxShown( 0 );
	prompt.SetPromptSize( 400, 250 );
	prompt.SetTextAlign( DT_LEFT );
	prompt.SetMessageFont( 12, 400 );
	prompt.SetCheckboxText( Launcher_LoadString( 0x21F ) );

	char	szTitle[256];
	Launcher_LoadStringInto( szTitle, 0xE6, (LPCSTR)pInfo->m_strName );
	prompt.SetTitle( szTitle );

	if ( prompt.DoModal() != IDOK )
	{
		::InvalidateRect( m_hWnd, NULL, TRUE );
		::UpdateWindow( m_hWnd );
		return 0;
	}

	if ( prompt.IsCheckboxChecked() == 1 )
		Launcher_WriteProfileInt( "Settings", "Mod Version Prompt", 0 );

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// CDlgConnectableBase::ConnectToSelectedServer (0x40B860)

void CDlgConnectableBase::ConnectToSelectedServer( CNetGameDlg* pTarget, CServerInfo* pInfo )
{
	if ( !VerifyServerBeforeConnect( pTarget, pInfo ) )
		return;

	if ( _strcmpi( (LPCSTR)pInfo->m_strDir, "VALVE" ) )
	{
		mod_t*	pMod = ModList_FindByGamedir( &g_pModList, (LPCSTR)pInfo->m_strDir );
		if ( !pMod )
		{
			// A server-side-only mod needs nothing installed locally; anything else
			// the player does not have is a dead end.
			if ( !pInfo->m_bSvSide || pInfo->m_bClDll )
			{
				Launcher_ShowMessageByIdEx( NULL, IDS_CONNECT_NEEDMOD, (LPCSTR)pInfo->m_strDir );
				goto repaint;
			}
		}
		else if ( !CheckModVersion( pInfo, pMod ) )
		{
			return;
		}

		if ( pMod != g_pCurrentMod )
		{
			Launcher_SavePlayerInfo();
			Sys_SetCmdLineParm( "-game", (LPCSTR)pInfo->m_strDir );
			COM_ResetGameDirectories();
			COM_AddGameDirectory( 0, COM_GetBaseDir(), (LPCSTR)pInfo->m_strDir );
			g_pCurrentMod = pMod;

			Launcher_OnGameDirChanged();
			RefreshAfterGameDirChange();
		}
	}
	else if ( g_pValveMod != g_pCurrentMod )
	{
		Launcher_SavePlayerInfo();
		Sys_StripCmdLineParm( "-game" );
		COM_ResetGameDirectories();
		g_pCurrentMod = g_pValveMod;

		Launcher_OnGameDirChanged();
		RefreshAfterGameDirChange();
	}

	// The record has to still be in the target's list, and unfiltered.
	CServerInfo*	p;
	for ( p = pTarget ? pTarget->m_pServerListHead : NULL; p; p = p->m_pNext )
	{
		if ( p == pInfo && !p->GetFiltered() )
			break;
	}
	if ( !p )
		goto repaint;

	for ( ; ; )
	{
		resumeOnSwitch = 0;
		gBackground    = 0;
		if ( engineapi.Cbuf_AddText )
			engineapi.Cbuf_AddText( "disconnect\n" );

		if ( Launcher_ConnectAndLaunch( pTarget, p ) )
			break;

		VID_HideEngineWindow();
		::InvalidateRect( m_hWnd, NULL, TRUE );
		Relayout();

		// A bad password is worth another go; anything else ends the attempt.
		if ( !Launcher_HandleConnectFailure() )
		{
			OnConnectAbort();
			return;
		}
	}

	// Joined: fold the launcher shell away behind the game.
	ShowWindow( SW_HIDE );
	CWnd::FromHandle( ::GetParent( m_hWnd ) )->ShowWindow( SW_HIDE );
	::ShowWindow( gLauncherWnd, SW_HIDE );
	return;

repaint:
	::InvalidateRect( m_hWnd, NULL, TRUE );
	::UpdateWindow( m_hWnd );
}
