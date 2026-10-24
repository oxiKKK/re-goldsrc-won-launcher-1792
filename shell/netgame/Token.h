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
// Purpose: declares CToken, the launcher's text tokenizer.
//
// $NoKeywords: $
//=============================================================================

#ifndef TOKEN_H
#define TOKEN_H
#ifdef _WIN32
#pragma once
#endif

#include <windows.h>

// One token at a time out of a text buffer.
class CToken
{
public:
	CToken( char* pData );
	virtual ~CToken();

	void	ParseNextToken();
	void	SetData( char* pData );
	void	GetRemainder();
	void	SetSemicolonMode( BOOL bMode );
	void	SetQuoteMode( BOOL bMode );
	void	ParseNextQuoteToken();
	void	SetCommentMode( BOOL bMode );
	char*	GetData();

	// +0     vftable  (virtual ~CToken)
	char	token[1024];		// +4     the current token text

protected:
	BOOL	m_bSkipComments;	// +1028  strip // comments + leading whitespace
	BOOL	m_bQuoteMode;		// +1032  parse double-quoted strings
	char*	m_pData;			// +1036  current position in the text
	BOOL	m_bSemiColonMode;	// +1040  split on ';' instead of whitespace

	void	ParseNextSemiColonToken();
};

#endif // TOKEN_H
