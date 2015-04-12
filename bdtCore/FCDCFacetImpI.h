#ifndef __FCDFacetImpl_h__
#define __FCDFacetImpl_h__

#include <FCDCentralService.h>
#include <FCDCentralServiceImpl.h>

class CFCDCFacetDivideByColumnSumImpl;
typedef IceUtil::Handle<CFCDCFacetDivideByColumnSumImpl> FCDCFacetDivideByColumnSumImplPtr;
class CGlobalVars;

class CFCDCFacetDivideByColumnSumImpl : virtual public CFcdcReadServiceImpl
{
public:
	CFCDCFacetDivideByColumnSumImpl(CGlobalVars& globalVars)
		:CFcdcReadServiceImpl(globalVars)
	{
	}

	virtual ~CFCDCFacetDivideByColumnSumImpl(){}

public:

    virtual void GetDoublesColumnVector_async(const ::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr&,
                                              ::Ice::Int,
                                              ::Ice::Long,
                                              ::Ice::Long,
                                              const Ice::Current&) const;

    virtual void GetDoublesRowMatrix_async(const ::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr&,
                                           ::Ice::Int,
                                           ::Ice::Long,
                                           ::Ice::Long,
                                           const Ice::Current&) const;
};


#endif
