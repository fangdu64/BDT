#ifndef __BigMatrixFacetImpl_h__
#define __BigMatrixFacetImpl_h__

#include <FCDCentralService.h>
#include <FCDCentralServiceImpl.h>

class CBigMatrixServiceImpl;
typedef IceUtil::Handle<CBigMatrixServiceImpl> CBigMatrixServiceImplPtr;
class CGlobalVars;

class CBigMatrixServiceImpl : virtual public iBS::BigMatrixService,
							 virtual public CFcdcReadServiceImpl
{
public:
	CBigMatrixServiceImpl(CGlobalVars& globalVars,
		const ::iBS::FeatureObserverSimpleInfoVec& fois)
		:CFcdcReadServiceImpl(globalVars), m_fois(fois), m_rowAdjust(iBS::RowAdjustNone)
	{
	}

	virtual ~CBigMatrixServiceImpl(){}

public:
	virtual ::Ice::Int SetOutputSamples(const ::iBS::IntVec&,
		const Ice::Current&);

	virtual ::Ice::Int SetRowAdjust(::iBS::RowAdjustEnum,
		const Ice::Current&);

	virtual void GetRowMatrix_async(const ::iBS::AMD_FcdcReadService_GetRowMatrixPtr&,
										const ::iBS::IntVec&,
										::Ice::Long,
										::Ice::Long,
										const IceUtil::Optional< ::iBS::RowAdjustEnum>&,
										const Ice::Current&);
	virtual void SampleRowMatrix_async(const ::iBS::AMD_FcdcReadService_SampleRowMatrixPtr&,
		const ::iBS::IntVec&,
		const ::iBS::LongVec&,
		const IceUtil::Optional< ::iBS::RowAdjustEnum>&,
		const Ice::Current&);
  
	virtual void RecalculateObserverStats_async(const ::iBS::AMD_BigMatrixService_RecalculateObserverStatsPtr&,
		::Ice::Long,
		const Ice::Current&);

public:
	static std::string GetServantName(int gid);

private:
	::iBS::FeatureObserverSimpleInfoVec m_fois;
	::iBS::RowAdjustEnum m_rowAdjust;
	::iBS::IntVec		 m_outputSampleIDs;
	
};
#endif
