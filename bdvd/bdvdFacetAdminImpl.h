#ifndef __BdvdFacetAdminServiceImpl_h__
#define __BdvdFacetAdminServiceImpl_h__

#include <FCDCentralService.h>

class CBdvdFacetAdminServiceImpl;
typedef IceUtil::Handle<CBdvdFacetAdminServiceImpl> CBdvdFacetAdminServiceImplPtr;

class CBdvdFacetAdminServiceImpl : virtual public iBS::BdvdFacetAdminService
{
public:
	virtual ~CBdvdFacetAdminServiceImpl(){}

    virtual ::Ice::Int RqstNewRUVFacet(::iBS::RUVFacetInfo&,
                                        const Ice::Current&);

	virtual ::Ice::Int RemoveRUVFacet(::Ice::Int,
		const Ice::Current&);

    virtual ::Ice::Int SetRUVFacetInfo(const ::iBS::RUVFacetInfo&,
                                        const Ice::Current&);

    virtual ::Ice::Int GetRUVFacetInfo(::Ice::Int,
                                        ::iBS::RUVFacetInfo&,
                                        const Ice::Current&);

	virtual ::iBS::FcdcRUVServicePrx GetRUVFacet(::Ice::Int,
                                                        const Ice::Current&);
};

#endif
