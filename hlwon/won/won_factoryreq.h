#ifndef WON_FACTORYREQ_H
#define WON_FACTORYREQ_H
#ifdef _WIN32
#pragma once
#endif

#include "TitanRequest.h"

// FactoryRequest -- the per-host request object LaunchChatServer builds for each
// TitanFactoryServer candidate.  vftable 0x4B089C; the derived constructor only
// stores that vftable over TitanRequest's, and the destructor (0x43ACA0) is empty
// and ICF-folded with ~DirRequest, so there is nothing else to model.
class FactoryRequest : public TitanRequest
{
public:
	FactoryRequest( const std::string& sAddr, int nPort )
		: TitanRequest( sAddr, nPort )
	{
	}

	virtual ~FactoryRequest()
	{
	}
};

#endif // WON_FACTORYREQ_H
