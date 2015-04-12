#ifndef __BDVDGlobalVars_h__
#define __BDVDGlobalVars_h__

#include <Ice/Ice.h>
class CBdvdGlobalVars;
class CRUVFacetDB;

namespace iBSInternal
{
	namespace bdvdGlobalVars
	{
		 extern CBdvdGlobalVars *_ptrGV;
	}
}


class CBdvdGlobalVars
{
public:
	CBdvdGlobalVars()
	{
		theRUVFacetDB = 0;
		iBSInternal::bdvdGlobalVars::_ptrGV = this;
	}

	static CBdvdGlobalVars* get();

public:
	CRUVFacetDB* theRUVFacetDB;
};

#endif
