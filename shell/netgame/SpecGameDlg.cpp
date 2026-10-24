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
// Purpose: CSpecGameDlg, the spectate variant of the Internet Games page
//          (proxies only, one extra strip button).
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg::CSpecGameDlg (0x4652d0)

CSpecGameDlg::CSpecGameDlg( int nMode, CWnd* pParent )
	: CServerBrowserDlg( nMode, pParent )
{
	SetupJoinButtonStrip();
}

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg::~CSpecGameDlg (0x465350)

CSpecGameDlg::~CSpecGameDlg()
{
}

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg::SetupJoinButtonStrip (0x465360)
//
// Join wears a different strip cell here, and the page opens with the
// spectate mode already selected.

void CSpecGameDlg::SetupJoinButtonStrip()
{
	int	wh[2];

	m_headerLoaded = Launcher_HeaderLoaded();
	Launcher_HeaderSize( wh );
	m_headerW      = wh[0];
	m_headerH      = wh[1];
	m_headerStride = Launcher_HeaderStride();

	if ( m_headerLoaded )
	{
		m_btnJoin.FreeSkinBitmaps();
		m_btnJoin.SetDIBData( CSize( m_headerW, m_headerH ), BTNSTRIP_SPECTATE_JOIN,
			m_headerLoaded );
	}

	m_bSelSpectate = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg::HasCreateGameButton (0x4653e0)

BOOL CSpecGameDlg::HasCreateGameButton()
{
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg::Relayout (0x4653f0)
//
// The base moves everything, then Join's cell is re-sliced over it.

void CSpecGameDlg::Relayout()
{
	CServerBrowserDlg::Relayout();
	SetupJoinButtonStrip();
}

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg::ApplyFilter (0x465410)
//
// Spectator proxies only.

void CSpecGameDlg::ApplyFilter( netfilter_t* pFilter )
{
	pFilter->m_bFilterProxies = 1;
	pFilter->m_bProxiesOnly   = 1;
	pFilter->m_bHideProxies   = 0;
	CServerBrowserDlg::ApplyFilter( pFilter );
}

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg::OnInitDialog (0x465440)

BOOL CSpecGameDlg::OnInitDialog()
{
	BOOL	bRet = CServerBrowserDlg::OnInitDialog();

	ApplyFilter( &m_filter );
	return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg::GetSettingsSection (0x465470)

const char* CSpecGameDlg::GetSettingsSection()
{
	return "SpectatorSettings";
}

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg::LoadFilter (0x465480)
//
// The proxy flags are forced after the load, so a stored filter cannot turn
// spectating off.

void CSpecGameDlg::LoadFilter( netfilter_t* pFilter )
{
	CServerBrowserDlg::LoadFilter( pFilter );
	pFilter->m_bHideProxies   = 0;
	pFilter->m_bFilterProxies = 1;
	pFilter->m_bProxiesOnly   = 1;
}

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg::GetHeaderBitmap (0x4654b0)

const char* CSpecGameDlg::GetHeaderBitmap()
{
	return "head_specgames";
}

/////////////////////////////////////////////////////////////////////////////
// CSpecGameDlg::GetJoinCaptionId (0x4654c0)

UINT CSpecGameDlg::GetJoinCaptionId()
{
	return IDS_BTN_SPECTATEGAME;
}
