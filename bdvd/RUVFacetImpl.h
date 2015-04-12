#ifndef __RUVFacetImpl_h__
#define __RUVFacetImpl_h__

#include <FCDCentralService.h>
#include <FCDCentralServiceImpl.h>
#include <RUVBuilder.h>

class CFcdcRUVServiceImpl;
typedef IceUtil::Handle<CFcdcRUVServiceImpl> CFcdcRUVServiceImplPtr;
class CGlobalVars;

class CFcdcRUVServiceImpl : virtual public iBS::FcdcRUVService,
							 virtual public CFcdcReadServiceImpl
{
public:
	CFcdcRUVServiceImpl(CGlobalVars& globalVars, 
			const ::iBS::RUVFacetInfo& RUVInfo, 
			const ::iBS::FeatureObserverSimpleInfoVec& fois,
			const ::iBS::ObserverStatsInfoVec& osis)
		:CFcdcReadServiceImpl(globalVars),m_ruvBuilder(RUVInfo,fois,osis)
	{
	}

	virtual ~CFcdcRUVServiceImpl(){}

public:

    virtual void SetActiveK_async(const ::iBS::AMD_FcdcRUVService_SetActiveKPtr&,
                                  ::Ice::Int,
								  ::Ice::Int,
                                  const Ice::Current&);

	virtual ::Ice::Int SetOutputMode(::iBS::RUVOutputModeEnum,
		const Ice::Current&);

	virtual ::Ice::Int SetOutputSamples(const ::iBS::IntVec&,
		const Ice::Current&);

	virtual ::Ice::Int ExcludeSamplesForGroupMean(const ::iBS::IntVec&,
		const Ice::Current&);

	virtual ::Ice::Int SetOutputScale(::iBS::RUVOutputScaleEnum,
		const Ice::Current&);

	virtual ::Ice::Int SetOutputWorkerNum(::Ice::Int,
		const Ice::Current&);

	virtual ::Ice::Int SetCtrlQuantileValues(::Ice::Double,
		const ::iBS::DoubleVec&,
		::Ice::Double,
		const Ice::Current&);

    virtual void RebuildRUVModel_async(const ::iBS::AMD_FcdcRUVService_RebuildRUVModelPtr&,
									::Ice::Int,
                                    ::Ice::Long,
                                        const Ice::Current&);

	virtual ::Ice::Int GetConditionIdxs(const ::iBS::IntVec&,
                                    ::iBS::IntVec&,
                                    const Ice::Current&);

	virtual ::Ice::Int GetConditionInfos(::iBS::ConditionInfoVec&,
                                         const Ice::Current&);

	virtual ::Ice::Int GetSamplesInGroups(const ::iBS::IntVec&,
		::iBS::IntVecVec&,
		const Ice::Current&);

	virtual ::Ice::Int GetG(
		::iBS::DoubleVec&,
		const Ice::Current&);

	virtual ::Ice::Int GetWt(::iBS::DoubleVec&,
		const Ice::Current&);

	virtual ::Ice::Int GetEigenVals(::iBS::DoubleVec&,
		const Ice::Current&);

	virtual ::Ice::Int SelectKByEigenVals(::Ice::Double,
		::Ice::Int&,
		::iBS::DoubleVec&,
		const Ice::Current&);

	virtual void DecomposeVariance_async(const ::iBS::AMD_FcdcRUVService_DecomposeVariancePtr&,
		const ::iBS::IntVec&,
		const ::iBS::IntVec&,
		const ::iBS::IntVecVec&,
		::Ice::Long,
		::Ice::Long,
		::Ice::Int,
		::Ice::Long,
		const ::std::string&,
		const Ice::Current&);

	virtual ::Ice::Int SetWtVectorIdxs(const ::iBS::IntVec&,
		const Ice::Current&);

	virtual ::Ice::Int GetFeatureObservers(const ::iBS::IntVec&,
		::iBS::FeatureObserverInfoVec&,
		const Ice::Current&);

    virtual void GetDoublesColumnVector_async(const ::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr&,
                                              ::Ice::Int,
                                              ::Ice::Long,
                                              ::Ice::Long,
                                              const Ice::Current&);

    virtual void GetDoublesRowMatrix_async(const ::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr&,
                                           ::Ice::Int,
                                           ::Ice::Long,
                                           ::Ice::Long,
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
  
public:
	static std::string GetServantName(int facetID);

private:
	CRUVBuilder m_ruvBuilder;
};
#endif
