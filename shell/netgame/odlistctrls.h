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
// Purpose: declares CODRuleListCtrl and CODPlayerListCtrl, plus the row
//          preamble the binary inlines into every DrawRow override.
//
// $NoKeywords: $
//=============================================================================

#ifndef ODLISTCTRLS_H
#define ODLISTCTRLS_H
#ifdef _WIN32
#pragma once
#endif

#include "ODListCtrl.h"
#include "ODMenu.h"

/////////////////////////////////////////////////////////////////////////////
// CODRuleListCtrl window
//
// The server-details rule list: key on the left, value on the right.

class CODRuleListCtrl : public CODListCtrl
{
public:
	CODRuleListCtrl();
	virtual ~CODRuleListCtrl();

	virtual void	DrawRow( CDC* pDC, int iRow );

protected:
	DECLARE_MESSAGE_MAP()
};

class CODPlayerListCtrl : public CODListCtrl
{
public:
	virtual void	DrawRow( CDC* pDC, int iRow );
};

#endif // ODLISTCTRLS_H
