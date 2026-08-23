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
// Purpose: CHLModSocket, the mod-list query socket.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/////////////////////////////////////////////////////////////////////////////
// CHLModSocket::CHLModSocket (0x41AA20)

CHLModSocket::CHLModSocket( mod_t** ppModListOut, const char* pszServer, short nPort )
{
	m_ppModListOut = ppModListOut;
	if ( ppModListOut )
		*ppModListOut = NULL;

	m_bDone        = FALSE;
	m_pModListHead = NULL;
	m_nBytesRead   = 0;

	strcpy( m_szServer, pszServer );
	m_nPort = nPort;

	m_pMsg = new CMessageBuffer( 0x800 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLModSocket::~CHLModSocket (0x41AB10)
//
// The assembled mod_t chain belongs to the caller once it has been published.

CHLModSocket::~CHLModSocket()
{
	m_pMsg->SZ_Clear();
	if ( m_pMsg )
	{
		delete m_pMsg;
		m_pMsg = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLModSocket::StartList (0x41AB70)

BOOL CHLModSocket::StartList()
{
	int		nTries = 2;

	if ( !Create( 0, SOCK_DGRAM,
			FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE, NULL ) )
		return FALSE;

	while ( !Connect( m_szServer, (UINT)(unsigned short)m_nPort ) )
	{
		Sleep( 50 );
		if ( --nTries < 0 )
			return FALSE;
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CHLModSocket::OnReceive (0x41ABD0)
//
// CAsyncSocket slot 7.

void CHLModSocket::OnReceive( int nErrorCode )
{
	int		cmd;

	if ( nErrorCode )
	{
		CAsyncSocket::OnReceive( nErrorCode );
		m_nBytesRead = 0;
		return;
	}

	m_nBytesRead = Receive( m_pMsg->GetData(), m_pMsg->GetMaxSize(), 0 );
	if ( m_nBytesRead <= 0 )
		return;

	m_pMsg->SetCurSize( m_nBytesRead );
	m_pMsg->MSG_BeginReading();

	if ( m_pMsg->MSG_ReadLong() != -1 )		// connectionless header
	{
		m_nBytesRead = 0;
		return;
	}

	cmd = m_pMsg->MSG_ReadByte();
	if ( cmd == 'o' )
	{
		ParseModList();
		CAsyncSocket::OnReceive( 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////
// CHLModSocket::SendModListRequest (0x41AC80)

void CHLModSocket::SendModListRequest()
{
	m_bDone = FALSE;
	SendListRequest( 0 );
}

/////////////////////////////////////////////////////////////////////////////
// CHLModSocket::ParseModList (0x41AC90)

void CHLModSocket::ParseModList()
{
	mod_t*		pMod;
	const char*	pszGame;
	const char*	pszDir;
	char*		pszVal;
	char		szKey[256];
	long		lContinuation;
	int			nModCount;
	int			i;

	m_pMsg->MSG_ReadByte();						// spacer / command echo
	lContinuation = m_pMsg->MSG_ReadLong();
	nModCount     = m_pMsg->MSG_ReadShort();

	for ( i = 0; i < nModCount; i++ )
	{
		pMod = (new mod_t)->Init();

		for ( ;; )
		{
			// The reader hands back a shared static, so the key has to be
			// copied out before the value is read over it.
			strcpy( szKey, m_pMsg->MSG_ReadString() );
			if ( !szKey[0] )
				break;
			pszVal = m_pMsg->MSG_ReadString();
			pMod->SetKey( szKey, pszVal );
		}

		pszGame = pMod->GetKey( "game" );
		pszDir  = pMod->GetKey( "gamedir" );
		if ( pszGame && *pszGame && pszDir && *pszDir )
		{
			pMod->next     = m_pModListHead;
			m_pModListHead = pMod;
		}
		else
		{
			pMod->FreeKeys();
			delete pMod;
		}
	}

	if ( lContinuation )
	{
		SendListRequest( lContinuation );	// pull the next chunk
		return;
	}

	m_bDone = TRUE;
	if ( m_ppModListOut )
		*m_ppModListOut = m_pModListHead;
}

/////////////////////////////////////////////////////////////////////////////
// CHLModSocket::SendListRequest (0x41AE10)

void CHLModSocket::SendListRequest( long lArg )
{
	m_pMsg->SZ_Clear();
	m_pMsg->MSG_WriteChar( 'n' );
	m_pMsg->MSG_WriteLong( lArg );
	AsyncSelect( FD_READ );
	Send( m_pMsg->GetData(), m_pMsg->GetCurSize(), 0 );
	m_pMsg->SZ_Clear();
}

/////////////////////////////////////////////////////////////////////////////
// CHLModSocket::SendInstallNotify (0x41AE70)
//
// The same frame as the list request under the 'p' opcode; CModDlg::OnInstall
// sends it on a throwaway socket.

void CHLModSocket::SendInstallNotify( long lArg )
{
	m_pMsg->SZ_Clear();
	m_pMsg->MSG_WriteChar( 'p' );
	m_pMsg->MSG_WriteLong( lArg );
	AsyncSelect( FD_READ );
	Send( m_pMsg->GetData(), m_pMsg->GetCurSize(), 0 );
	m_pMsg->SZ_Clear();
}
