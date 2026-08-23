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
// Purpose: CToken, the launcher's text tokenizer.
//
// $NoKeywords: $
//=============================================================================

#include "precompiled.h"

/*
==================
CToken::CToken (0x466300)
==================
*/
CToken::CToken( char* pData )
{
	m_pData = pData;
	m_bSemiColonMode = FALSE;
	m_bQuoteMode = FALSE;
	m_bSkipComments = FALSE;
}

/*
==================
CToken::~CToken (0x466350)
==================
*/
CToken::~CToken()
{
	m_bSemiColonMode = FALSE;
	m_pData = NULL;
}

/*
==================
CToken::ParseNextToken (0x466370)

Parse the next token out of the data stream; the token is placed in 'token'.
==================
*/
void CToken::ParseNextToken()
{
	int		c;
	int		len;
	BOOL	inColon;

	if ( !m_pData )
		return;

	// Skip comment lines.
	if ( m_bSkipComments )
	{
		while ( 1 )
		{
			// Skip white space at start of line.
			while ( m_pData &&
				( *m_pData == '\r' ||
				  *m_pData == '\n' ||
				  *m_pData == ' '  ||
				  *m_pData == '\t' ) )
				m_pData++;

			if ( *m_pData == '/' && *( m_pData + 1 ) == '/' )
				GetRemainder();		// Skip to end of line
			else
				break;
		}
	}

	// Special handling
	if ( m_bSemiColonMode )
	{
		ParseNextSemiColonToken();
		return;
	}

	if ( m_bQuoteMode )
	{
		ParseNextQuoteToken();
		return;
	}

	inColon = FALSE;
	len = 0;

	// The return token
	token[0] = 0;

	if ( !m_pData )
		return;

	// skip whitespace.  Only space characters are white space
	while ( ( c = *m_pData ) == ' ' )
	{
		if ( c == 0 )
			return;		// end of stream
		m_pData++;
	}

	// Skip cr/lf
	if ( ( c = *m_pData ) == '\r' )
		m_pData++;

	if ( ( c = *m_pData ) == '\n' )
		m_pData++;

	if ( *m_pData == ':' )	// If we parse a leading colon, entire remainder of
		inColon = TRUE;		//  string is next token.

	// parse a regular word
	do
	{
		// Don't copy cr/lf pairs, just skip them.
		if ( c != '\r' && c != '\n' )
		{
			token[len] = c;
			len++;
		}
		m_pData++;
		c = *m_pData;

		// If we are reading from colon to end of string,
		//  only break at end of string
		if ( inColon && !c )
			break;

		// If we get to end of line, stop parsing token
		if ( c == '\n' )
			break;

	} while ( ( ( c != 0 ) && ( c != ' ' ) ) || inColon );

	token[len] = 0;
}

/*
==================
CToken::SetData (0x4664C0)
==================
*/
void CToken::SetData( char* pData )
{
	m_pData = pData;
}

/*
==================
CToken::GetRemainder (0x4664D0)
==================
*/
void CToken::GetRemainder()
{
	int		c;
	int		len;

	token[0] = 0;
	len = 0;

	if ( !m_pData )
		return;

	// skip whitespace.  Only space characters are white space
	while ( ( c = *m_pData ) == ' ' )
	{
		if ( c == 0 )
			return;		// end of stream
		m_pData++;
	}

	// If only thing left is cr/lf, nothing is left
	c = *m_pData;
	if ( ( c == '\r' ) ||
		 ( c == '\n' ) )
		return;

	// parse rest of line
	do
	{
		// Don't copy cr/lf pair
		if ( c != '\r' && c != '\n' )
		{
			token[len] = c;
			len++;
		}
		m_pData++;
		c = *m_pData;

		if ( !c )
			break;

		// If we get to end of line, stop parsing token
		if ( c == '\n' )
			break;

	} while ( c != 0 );

	token[len] = 0;
}

/*
==================
CToken::SetSemicolonMode
==================
*/
void CToken::SetSemicolonMode( BOOL bMode )
{
	m_bSemiColonMode = bMode;
}

/*
==================
CToken::ParseNextSemiColonToken (0x466540)
==================
*/
void CToken::ParseNextSemiColonToken()
{
	int		c;
	int		len;

	len = 0;

	// The return token
	token[0] = 0;

	if ( !m_pData )
		return;

	// skip whitespace.  Only ';' are white space
	while ( ( c = *m_pData ) == ';' )
	{
		if ( c == 0 )
			return;		// end of stream
		m_pData++;
	}

	// parse a regular word
	do
	{
		token[len++] = c;
		m_pData++;
		c = *m_pData;
	} while ( c && ( c != ';' ) );

	token[len] = 0;
}

/*
==================
CToken::SetQuoteMode (0x4665A0)

Treat everything inside double quotes as one token, omitting the quotes.
==================
*/
void CToken::SetQuoteMode( BOOL bMode )
{
	m_bQuoteMode = bMode;
}

/*
==================
CToken::ParseNextQuoteToken (0x4665B0)
==================
*/
void CToken::ParseNextQuoteToken()
{
	int		c;
	int		len;

	len = 0;

	// The return token
	token[0] = 0;

	if ( !m_pData )
		return;

	// skip whitespace, including tabs and cr/lf
	while ( 1 )
	{
		c = *m_pData;

		if ( c == 0 )
			return;		// end of stream

		if ( c != ' ' && c != '\t' && c != '\r' && c != '\n' )
			break;

		m_pData++;
	}

	// parse a regular word
	while ( c != '"' )
	{
		// Don't copy cr/lf pairs, just skip them.
		if ( c != '\r' )
		{
			token[len] = c;
			len++;
		}
		m_pData++;
		c = *m_pData;

		// End of line or end of stream stops the token
		if ( !c || c == '\n' || c == '\t' || c == ' ' )
		{
			token[len] = 0;
			return;
		}
	}

	// A double quote: read until we get the closing double quote.
	m_pData++;
	c = *m_pData;

	// !!!HACK to allow """ in parsing the key file
	if ( c == '"' && *( m_pData + 1 ) == '"' )
	{
		token[len++] = c;
		m_pData++;
		c = *m_pData;
	}

	while ( c != '"' )
	{
		if ( !c )	// Parsing error
		{
			token[len] = 0;
			return;
		}

		token[len] = c;
		len++;
		m_pData++;
		c = *m_pData;
	}

	m_pData++;

	token[len] = 0;

	// Skip cr/lf or tabs at end of line.
	while ( 1 )
	{
		if ( ( c = *m_pData ) == '\r' )
		{
			m_pData++;
			continue;
		}

		if ( ( c = *m_pData ) == '\n' )
		{
			m_pData++;
			continue;
		}

		if ( ( c = *m_pData ) == '\t' )
		{
			m_pData++;
			continue;
		}

		break;
	}
}

/*
==================
CToken::SetCommentMode (0x4666E0)
==================
*/
void CToken::SetCommentMode( BOOL bMode )
{
	m_bSkipComments = bMode;
}

/*
==================
CToken::GetData (0x4666F0)
==================
*/
char* CToken::GetData()
{
	return m_pData;
}
