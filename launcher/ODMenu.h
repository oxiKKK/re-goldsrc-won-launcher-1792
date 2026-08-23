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
// Purpose: declares CODMenu, the skinned owner-draw popup menu.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODMENU_H
#define ODMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include "ODComboBox.h"

class CODMenu : public CMenu
{
public:
	CODMenu( const char* pszFace, int nSize, int nWeight );
	virtual ~CODMenu();

	virtual void	DrawItem( LPDRAWITEMSTRUCT lpDIS );
	virtual void	MeasureItem( LPMEASUREITEMSTRUCT lpMIS );

	COLORREF	m_clrText;		// +8   item text (0xFFFFFF)
	COLORREF	m_clrBg;		// +12  item background (0)
	COLORREF	m_clrSelText;	// +16  selected item text (0x18B6FF)
	COLORREF	m_clrSelBg;		// +20  selected item fill (0x183850)
	CFont		m_font;			// +24  item font
};

/////////////////////////////////////////////////////////////////////////////
// CODPingComboBox window
//
// vftable 0x4B1DEC -- the server-browser's ping filter drop; each row paints
// in its own latency colour.

class CODPingComboBox : public CODComboBox
{
public:
	CODPingComboBox();

	virtual void	DrawRow( CDC* pDC, int iRow );		// vtbl+188

	CBrush		m_brRow;		// +196  row fill brush
	COLORREF	m_clrRowBk;		// +204  row colour
	COLORREF	m_clrRow;		// +208  row text colour

protected:
	DECLARE_MESSAGE_MAP()
};

#endif // ODMENU_H
