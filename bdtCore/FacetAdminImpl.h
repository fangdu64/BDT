#ifndef __FacetAdminServiceImpl_h__
#define __FacetAdminServiceImpl_h__

#include <FCDCentralService.h>

class CFcdcFacetAdminServiceImpl;
typedef IceUtil::Handle<CFcdcFacetAdminServiceImpl> CFcdcFacetAdminServiceImplPtr;

class CFcdcFacetAdminServiceImpl : virtual public iBS::FcdcFacetAdminService
{
public:
	virtual ~CFcdcFacetAdminServiceImpl(){}

	virtual ::iBS::BigMatrixServicePrx GetBigMatrixFacet(::Ice::Int,
		const Ice::Current&);
};

#endif
