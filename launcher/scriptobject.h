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
// Purpose: declares CScriptObject and CDescription, the settings.scr model.
//
// $NoKeywords: $
//=============================================================================

#ifndef SCRIPTOBJECT_H
#define SCRIPTOBJECT_H
#ifdef _WIN32
#pragma once
#endif

#include <afxwin.h>
#include <stdio.h>

enum objtype_t
{
	O_BADTYPE,
	O_BOOL,
	O_NUMBER,
	O_LIST,
	O_STRING
};

// The type-name table at 0x4D1990: four 36-byte records, walked with add esi,24h.
typedef struct
{
	objtype_t	type;
	char		szDescription[32];
} objtypedesc_t;

// One LIST choice.  sizeof 0x184.  settings.scr writes the pair as
// "label" "value", and the drop list paints the node pointer straight as a
// string, so the item text -- not the cvar value -- is the one at +0.
class CScriptListItem
{
public:
	CScriptListItem( const CString& strItem, const CString& strValue );

	char				szItemText[128];	// +0    shown in the drop list
	char				szValue[256];		// +128  written to the cvar
	CScriptListItem*	pNext;				// +384
};

// One parsed option (cvar) node.  sizeof 0x30.
class CScriptObject
{
public:
	CScriptObject();
	~CScriptObject();

	objtype_t			type;			// +0
	CString				cvarname;		// +4
	CString				prompt;			// +8
	CScriptListItem*	pListItems;		// +12  LIST choices (NULL unless O_LIST)
	float				fMin;			// +16  NUMBER min (-1 = none)
	float				fMax;			// +20  NUMBER max (-1 = none)
	CString				defValue;		// +24
	float				fdefValue;		// +28
	CString				curValue;		// +32
	float				fcurValue;		// +36
	int					bSetInfo;		// +40
	CScriptObject*		pNext;			// +44

	void		AddItem( CScriptListItem* pItem );
	objtype_t	GetType( char* pszType );
	BOOL		ReadFromBuffer( char** pBuffer );
	void		WriteToScriptFile( FILE* fp );
	void		WriteToFile( FILE* fp );
	void		WriteToConfig();
};

// Abstract description base.  The vftable at 0x4B37EC has exactly two slots,
// both _purecall -- so the destructor is not virtual.
class CDescription
{
public:
	CDescription();
	~CDescription();

	virtual int	WriteScriptHeader( FILE* fp ) = 0;	// slot 0 (settings.scr header)
	virtual int	WriteFileHeader( FILE* fp ) = 0;	// slot 1 (game.cfg header)

	BOOL	InitFromFile( const char* pszFile );
	BOOL	ReadFromBuffer( char** pBuffer );
	void	AddObject( CScriptObject* pObj );
	int		WriteToFile( FILE* fp );				// game.cfg
	int		WriteToScriptFile( FILE* fp );			// settings.scr
	void	WriteToConfig();						// push cur values -> token profile
	void	TransferCurrentValues( const char* pszName );	// pull cur values <- config

	CScriptObject*	pObjList;				// +4   head of the option list
	char*			m_pszHintText;			// +8   banner comment (strdup)
	char*			m_pszDescriptionType;	// +12  "SERVER_OPTIONS" (strdup)
};

// Concrete SERVER_OPTIONS model.
class CServerDescription : public CDescription
{
public:
	CServerDescription();						// 0x402040

	virtual int	WriteScriptHeader( FILE* fp );	// 0x401E60
	virtual int	WriteFileHeader( FILE* fp );	// 0x401F60
};

#endif // SCRIPTOBJECT_H
