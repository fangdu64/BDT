#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/ScopedArray.h>
#include <GlobalVars.h>
#include <FCDCentralServiceImpl.h>
#include <FeatureDomainDB.h>
#include <FeatureObserverDB.h>
#include <ObserverStatsDB.h>
#include <FeatureValueRAM.h>
#include <FeatureValueWorker.h>
#include <FeatureValueWorkItem.h>
#include <FeatureValueWorkerMgr.h>
#include <FeatureValueStoreMgr.h>
#include <ObserverStatsDB.h>
#include <ObserverIndexDB.h>

//=================================================================
//read serverice
//=================================================================
::Ice::Int
CFcdcReadServiceImpl::GetFeatureDomains(const ::iBS::IntVec& domainIDs,
                                           ::iBS::FeatureDomainInfoVec& domainInfos,
                                           const Ice::Current& current)
{

	return m_gv.theDomainsDB.GetFeatureDomains(domainIDs, domainInfos);
}


::Ice::Int
CFcdcReadServiceImpl::GetFeatureObservers(const ::iBS::IntVec& observerIDs,
                                             ::iBS::FeatureObserverInfoVec& observerInfos,
                                             const Ice::Current& current)
{
	return m_gv.theObserversDB.GetFeatureObservers(observerIDs, observerInfos);
}

bool
CFcdcReadServiceImpl::rvGetDoublesColumnVector(const ::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr& cb,
                                                        ::Ice::Int observerID,
                                                        ::Ice::Long featureIdxFrom,
                                                        ::Ice::Long featureIdxTo,
														iBS::FeatureObserverSimpleInfoPtr& foi) const
{
	bool rt=false;
    //[IdxFrom, IdxTo)
    Ice::Long featureCnt = featureIdxTo - featureIdxFrom;
	if(featureCnt<=0)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal feature range";
		cb->ice_exception(ex);
		return rt;
	}

	if(observerID < (::Ice::Int)::iBS::SpecialFeatureObserverTestMaxID)
	{
		//special observers
		if(observerID==(::Ice::Int)::iBS::SpecialFeatureObserverTestDoubles)
		{
		#if (defined(_MSC_VER) && (_MSC_VER >= 1600))
			std::pair<const Ice::Double*, const Ice::Double*> ret(static_cast<const Ice::Double*>(nullptr), static_cast<const Ice::Double*>(nullptr));
		#else
			std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);
		#endif
			IceUtil::ScopedArray<Ice::Double> doubleVals(new Ice::Double[featureCnt]);
			for(Ice::Long i=0; i<featureCnt; ++i)
			{
				doubleVals[i]=(double)i+featureIdxFrom;
			}
			//do not need to consider ENDIAN, as the ICE runtime has already handled that
			//i.e., the array will always changed to little-endian (if needed)

			ret.first = doubleVals.get();
			ret.second = ret.first + featureCnt;
			cb->ice_response(1, ret);
			return rt;
		}
		else if(observerID==(::Ice::Int)::iBS::SpecialFeatureObserverTestFloats)
		{
			::iBS::ArgumentException ex;
			ex.reason = "special feature observer not implemented";
			cb->ice_exception(ex);
			return rt;
		}
		else if(observerID==(::Ice::Int)::iBS::SpecialFeatureObserverTestInt32)
		{
			::iBS::ArgumentException ex;
			ex.reason = "special feature observer not implemented";
			cb->ice_exception(ex);
			return rt;
		}
		else if(observerID==(::Ice::Int)::iBS::SpecialFeatureObserverTestInt64)
		{
			::iBS::ArgumentException ex;
			ex.reason = "special feature observer not implemented";
			cb->ice_exception(ex);
			return rt;
		}
		else
		{
			::iBS::ArgumentException ex;
			ex.reason = "illegal feature observer ID";
			cb->ice_exception(ex);
			return rt;
		}
	}

	//get feature value by (observerID, feature idx range)

	foi = m_gv.theObserversDB.GetFeatureObserver(observerID);
	if(!foi)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal feature observer ID";
		cb->ice_exception(ex);
		return rt;
	}

	if(featureIdxTo<0 || featureIdxFrom<0 || featureIdxTo<featureIdxFrom
		||foi->DomainSize<featureIdxTo)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal feature idx range";
		cb->ice_exception(ex);
		return rt;
	}

	return true;
}

void
CFcdcReadServiceImpl::GetDoublesColumnVector_async(const ::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr& cb,
                                                        ::Ice::Int observerID,
                                                        ::Ice::Long featureIdxFrom,
                                                        ::Ice::Long featureIdxTo,
                                                        const Ice::Current& current)
{
	iBS::FeatureObserverSimpleInfoPtr foi;
	bool rqstValid=rvGetDoublesColumnVector(cb, observerID, featureIdxFrom, featureIdxTo, foi);
	if(!rqstValid)
	{
		//exception or response already made
		return;
	}

	if(foi->GetPolicy==iBS::FeatureValueGetPolicyGetFromRAM
	  ||foi->GetPolicy==iBS::FeatureValueGetPolicyGetForOneTimeRead)
	{
		FeatureValueWorkItemPtr wi= new ::Original::CGetDoublesColumnVector(
			foi,cb,observerID,featureIdxFrom,featureIdxTo);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);
		return;
	}
	else
	{
		::iBS::ArgumentException ex;
		ex.reason = "GetPolicy not implemented";
		cb->ice_exception(ex);
		return;
	}

}

void
CFcdcReadServiceImpl::GetIntsColumnVector_async(const ::iBS::AMD_FcdcReadService_GetIntsColumnVectorPtr& cb,
                                                     ::Ice::Int observerID,
                                                     ::Ice::Long featureIdxFrom,
                                                     ::Ice::Long featureIdxTo,
                                                     const Ice::Current& current)
{
    ::iBS::ArgumentException ex;
	ex.reason = "method not implemented";
	cb->ice_exception(ex);
}

bool
CFcdcReadServiceImpl::rvGetDoublesRowMatrix(const ::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr& cb,
                                                     ::Ice::Int observerGroupID,
                                                     ::Ice::Long featureIdxFrom,
                                                     ::Ice::Long featureIdxTo,
													 iBS::FeatureObserverSimpleInfoPtr& foi) const
{
	bool rt=false;
    Ice::Long featureCnt = featureIdxTo - featureIdxFrom;
	if(featureCnt<=0)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal feature range";
		cb->ice_exception(ex);
		return rt;
	}

	int observerID = observerGroupID;
    foi = m_gv.theObserversDB.GetFeatureObserver(observerID);

	if(!foi || foi->ObserverGroupSize<1)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal observer group id";
		cb->ice_exception(ex);
		return rt;
	}

	int observerCnt=foi->ObserverGroupSize;
	Ice::Long dataSize=observerCnt*(featureIdxTo-featureIdxFrom)*sizeof(Ice::Double);
	if(dataSize>m_gv.theIceMessageSizeMax)
	{
		::iBS::ArgumentException ex;
		ex.reason = "too much data required in one batch";
		cb->ice_exception(ex);
		return rt;
	}

	if(featureIdxTo<0 || featureIdxFrom<0 || featureIdxTo<featureIdxFrom
		||foi->DomainSize<featureIdxTo)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal feature idx range";
		cb->ice_exception(ex);
		return rt;
	}

	return true;
}

void
CFcdcReadServiceImpl::GetDoublesRowMatrix_async(const ::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr& cb,
                                                     ::Ice::Int observerGroupID,
                                                     ::Ice::Long featureIdxFrom,
                                                     ::Ice::Long featureIdxTo,
                                                     const Ice::Current& current)
{
    iBS::FeatureObserverSimpleInfoPtr foi;
	bool rqstValid=rvGetDoublesRowMatrix(cb, observerGroupID, featureIdxFrom, featureIdxTo, foi);
	if(!rqstValid)
	{
		//exception or response already made
		return;
	}

	//valid observer id
	if(foi->GetPolicy==iBS::FeatureValueGetPolicyGetFromRAM
		||foi->GetPolicy==iBS::FeatureValueGetPolicyGetForOneTimeRead)
	{
		FeatureValueWorkItemPtr wi= new ::Original::CGetDoublesRowMatrix(
			foi,cb,observerGroupID, featureIdxFrom, featureIdxTo);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);
		return;
	}
	else
	{
		::iBS::ArgumentException ex;
		ex.reason = "GetPolicy not implemented";
		cb->ice_exception(ex);
		return;
	}
}

::Ice::Int
CFcdcReadServiceImpl::GetObserverStats(::Ice::Int observerID,
                                          ::iBS::ObserverStatsInfo& stats,
                                          const Ice::Current& current)
{
	return m_gv.theObserverStatsDB->GetObserverStatsInfo(observerID, stats);
}

::Ice::Int
CFcdcReadServiceImpl::GetObserversStats(const ::iBS::IntVec& observerIDs,
                                           ::iBS::ObserverStatsInfoVec& observerStats,
                                           const Ice::Current& current)
{
    return m_gv.theObserverStatsDB->GetObserversStats(observerIDs, observerStats);
}

bool
CFcdcReadServiceImpl::rvGetJoinedRowMatrix(
	const ::iBS::IntVec& observerIDs,
	::Ice::Long featureIdxFrom,
	::Ice::Long featureIdxTo, iBS::FeatureObserverSimpleInfoVec& fois,
	std::string& reason) const
{
    bool rt=false;

	size_t observerCnt=observerIDs.size();
	if(observerCnt==0)
	{
		reason = "empty observer";
		return rt;
	}
	
	m_gv.theObserversDB.GetFeatureObservers(observerIDs,fois);
	Ice::Long domainSize=0;
	for(size_t i=0;i<observerCnt;i++)
	{
		iBS::FeatureObserverSimpleInfoPtr foi = fois[i];
		if(!foi){
			reason = "illegal observer ID";
			return rt;
		}
		if(false&&foi->SetPolicy!=iBS::FeatureValueSetPolicyDoNothing)
		{
			//should be read only
			reason = "observer should already be in read only mode";
			return rt;
		}
		if(domainSize==0)
		{
			domainSize=foi->DomainSize;
		}
		if(domainSize!=foi->DomainSize)
		{
			reason = "observers not with same domain size";
			return rt;
		}
	}
	
	if(featureIdxTo<0 || featureIdxFrom<0 || featureIdxTo<featureIdxFrom
		||domainSize<featureIdxTo)
	{
		reason = "illegal feature idx range";
		return rt;
	}

	Ice::Long dataSize=observerCnt*(featureIdxTo-featureIdxFrom)*sizeof(Ice::Double);
	if(dataSize>m_gv.theIceMessageSizeMax)
	{
		reason = "too much data required in one batch";
		return rt;
	}
	
	return true;
}

void
CFcdcReadServiceImpl::GetRowMatrix_async(const ::iBS::AMD_FcdcReadService_GetRowMatrixPtr& cb,
	const ::iBS::IntVec& observerIDs,
	::Ice::Long featureIdxFrom,
	::Ice::Long featureIdxTo,
	const IceUtil::Optional< ::iBS::RowAdjustEnum>& rowAdjust,
	const Ice::Current& current)
{
	iBS::FeatureObserverSimpleInfoVec fois;
	std::string exReason;
	bool rqstValid = rvGetJoinedRowMatrix(observerIDs, featureIdxFrom, featureIdxTo, fois, exReason);
	if (!rqstValid)
	{
		::iBS::ArgumentException ex;
		ex.reason = exReason;
		cb->ice_exception(ex);
		return;
	}

	::iBS::RowAdjustEnum rowadj = rowAdjust ? rowAdjust.get() : iBS::RowAdjustNone;
	

	FeatureValueWorkItemPtr wi = new ::Original::CGetRowMatrix(
		fois, cb, observerIDs, featureIdxFrom, featureIdxTo, rowadj);

	m_gv.theFeatureValueWorkerMgr->AssignItemToWorkerByTime(wi);
}

void
CFcdcReadServiceImpl::SampleRowMatrix_async(const ::iBS::AMD_FcdcReadService_SampleRowMatrixPtr& cb,
	const ::iBS::IntVec& observerIDs,
	const ::iBS::LongVec& featureIdxs,
	const IceUtil::Optional< ::iBS::RowAdjustEnum>& rowAdjust,
	const Ice::Current& current)
{
	iBS::FeatureObserverSimpleInfoVec fois;
	Ice::Long rowCnt = featureIdxs.size();
	std::string exReason;
	bool rqstValid = rvSampleJoinedRowMatrix(observerIDs, rowCnt, fois, exReason);
	if (!rqstValid)
	{
		::iBS::ArgumentException ex;
		ex.reason = exReason;
		cb->ice_exception(ex);
		return;
	}

	::iBS::RowAdjustEnum rowadj = rowAdjust ? rowAdjust.get() : iBS::RowAdjustNone;

	FeatureValueWorkItemPtr wi = new ::Original::CSampleRowMatrix(
		fois, cb, observerIDs, featureIdxs, rowadj);
	m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(fois[0], wi);
}

bool CFcdcReadServiceImpl::rvSampleJoinedRowMatrix(const ::iBS::IntVec& observerIDs,
								Ice::Long rowCnt,
								iBS::FeatureObserverSimpleInfoVec& fois,
								std::string& reason) const
{
	bool rt=false;
	size_t observerCnt=observerIDs.size();
	if(observerCnt==0)
	{
		::iBS::ArgumentException ex;
		reason = "empty observer";
		return rt;
	}
	
	m_gv.theObserversDB.GetFeatureObservers(observerIDs,fois);
	Ice::Long domainSize=0;
	for(size_t i=0;i<observerCnt;i++)
	{
		iBS::FeatureObserverSimpleInfoPtr foi = fois[i];
		if(!foi){
			::iBS::ArgumentException ex;
			reason = "illegal observer ID";
			return rt;
		}
		if(false && (foi->SetPolicy!=iBS::FeatureValueSetPolicyDoNothing))
		{
			//should be read only
			reason = "observer should already be in read only mode";
			return rt;
		}
		if(domainSize==0)
		{
			domainSize=foi->DomainSize;
		}
		if(domainSize!=foi->DomainSize)
		{
			reason = "observers not with same domain size";
			return rt;
		}
	}
	
	Ice::Long dataSize=observerCnt*rowCnt*sizeof(Ice::Double);
	if(dataSize>m_gv.theIceMessageSizeMax)
	{
		reason = "too much data required in one batch";
		return rt;
	}

	return true;
}


::Ice::Int
CFcdcReadServiceImpl::GetObserverIndex(::Ice::Int observerID,
                                          ::iBS::ObserverIndexInfo& oii,
                                          const Ice::Current& current)
{
	return m_gv.theObserverIndexDB->GetObserverIndexInfo(observerID, oii);
}

void
CFcdcReadServiceImpl::GetFeatureIdxsByIntKeys_async(
		const ::iBS::AMD_FcdcReadService_GetFeatureIdxsByIntKeysPtr& cb,
        ::Ice::Int observerID,
        const ::iBS::IntVec& keys,
        ::Ice::Long maxFeatureCnt,
        const Ice::Current& current) const
{
	iBS::FeatureObserverSimpleInfoPtr foi 
		= m_gv.theObserversDB.GetFeatureObserver(observerID);

	if(!foi){
		::iBS::ArgumentException ex;
		ex.reason = "illegal observer ID";
		cb->ice_exception(ex);
		return;
	}

    if(keys.empty()){
		::iBS::ArgumentException ex;
		ex.reason = "empty keys";
		cb->ice_exception(ex);
		return;
	}

	::iBS::ObserverIndexInfo oii;
	int rt=m_gv.theObserverIndexDB->GetObserverIndexInfo(observerID, oii);
	if(rt==0){
		::iBS::ArgumentException ex;
		ex.reason = "index for this observer not ready";
		cb->ice_exception(ex);
		return;
	}

	iBS::IntVec keyIdxs;
	bool rb=CObserverIndexHelper::GetIntKeyIdxsByKeyValues(keys,oii,keyIdxs);
	if(!rb){
		::iBS::ArgumentException ex;
		ex.reason = "invalid keys";
		cb->ice_exception(ex);
		return;
	}

	int indexFileObserverID = oii.IndexObserverID;
	iBS::FeatureObserverSimpleInfoPtr idxfoi
		=m_gv.theObserversDB.GetFeatureObserver(indexFileObserverID);
	if(!idxfoi){
		::iBS::ArgumentException ex;
		ex.reason = "index for this observer not ready";
		cb->ice_exception(ex);
		return;
	}

	FeatureValueWorkItemPtr wi= new CGetFeatureIdxsByIntKeys(
		idxfoi,cb,observerID,maxFeatureCnt,oii,keyIdxs);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(idxfoi,wi);
	

}

void
CFcdcReadServiceImpl::GetFeatureCntsByIntKeys_async(
		const ::iBS::AMD_FcdcReadService_GetFeatureCntsByIntKeysPtr& cb,
        ::Ice::Int observerID,
        const ::iBS::IntVec& keys,
        const Ice::Current& current) const
{
	iBS::FeatureObserverSimpleInfoPtr foi 
		= m_gv.theObserversDB.GetFeatureObserver(observerID);

	if(!foi){
		::iBS::ArgumentException ex;
			ex.reason = "illegal observer ID";
			cb->ice_exception(ex);
			return;
	}

	if(keys.empty()){
		::iBS::ArgumentException ex;
		ex.reason = "empty keys";
		cb->ice_exception(ex);
		return;
	}

	::iBS::ObserverIndexInfo oii;
	int rt=m_gv.theObserverIndexDB->GetObserverIndexInfo(observerID, oii);
	if(rt==0){
		::iBS::ArgumentException ex;
		ex.reason = "index for this observer not ready";
		cb->ice_exception(ex);
		return;
	}

	iBS::IntVec keyIdxs;
	bool rb=CObserverIndexHelper::GetIntKeyIdxsByKeyValues(keys,oii,keyIdxs);
	if(!rb){
		::iBS::ArgumentException ex;
		ex.reason = "invalid keys";
		cb->ice_exception(ex);
		return;
	}

	iBS::LongVec cnts;
	for(int i=0;i<keyIdxs.size();i++)
	{
		cnts.push_back(oii.KeyIdx2RowCnt[keyIdxs[i]]);
	}

	std::pair<const Ice::Long*, const Ice::Long*> values(
		&cnts[0], &cnts[0]+cnts.size());

	cb->ice_response(1,values);
}

::Ice::Int
CFcdcReadServiceImpl::GetAMDTaskInfo(::Ice::Long taskID,
::iBS::AMDTaskInfo& task,
const Ice::Current& current)
{
	return m_gv.theFeatureValueWorkerMgr->GetAMDTask(taskID, task);
}

::Ice::Int
CFcdcReadServiceImpl::GetFeatureValueStoreDir(::std::string& rootDir,
const Ice::Current& current)
{
	rootDir = m_gv.theFeatureValueStoreMgr->GetRootDir();
	return 1;
}

::Ice::Int
CFcdcReadServiceImpl::GetFeatureValuePathPrefix(::Ice::Int observerID,
::std::string& pathPrefix,
const Ice::Current& current)
{
	iBS::FeatureObserverSimpleInfoPtr foi
		= m_gv.theObserversDB.GetFeatureObserver(observerID);

	if (!foi){
		::iBS::ArgumentException ex;
		ex.reason = "illegal observer ID";
		throw ex;
	}
	pathPrefix = m_gv.theFeatureValueStoreMgr->GetStoreFilePathPrefix(foi);

	return 1;
}

//=================================================================
//Write service
//=================================================================

::Ice::Int
CFcdcReadWriteServiceImpl::SetFeatureDomains(const ::iBS::FeatureDomainInfoVec& domainInfos,
                                           const Ice::Current& current)
{
	return m_gv.theDomainsDB.SetFeatureDomains(domainInfos);
}

::Ice::Int
CFcdcReadWriteServiceImpl::SetFeatureObservers(const ::iBS::FeatureObserverInfoVec& observerInfos,
                                             const Ice::Current& current)
{
	return m_gv.theObserversDB.SetFeatureObservers(observerInfos);
}

void
CFcdcReadWriteServiceImpl::SetDoublesColumnVector_async(
		const ::iBS::AMD_FcdcReadWriteService_SetDoublesColumnVectorPtr& cb,
		::Ice::Int observerID,
		::Ice::Long featureIdxFrom,
		::Ice::Long featureIdxTo,
		const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values,
		const Ice::Current& current)
{
    iBS::FeatureObserverSimpleInfoPtr foi 
		= m_gv.theObserversDB.GetFeatureObserver(observerID);

	if(!foi){
		::iBS::ArgumentException ex;
			ex.reason = "illegal feature observer ID";
			cb->ice_exception(ex);
			return;
	}

	// not allowed for an observer in group
	if(foi->ObserverGroupSize>1){
		::iBS::ArgumentException ex;
			ex.reason = "method not implemented yet";
			cb->ice_exception(ex);
			return;
	}

	if(featureIdxTo<0 || featureIdxFrom<0 || featureIdxTo<featureIdxFrom
		||foi->DomainSize<featureIdxTo)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal feature idx range";
		cb->ice_exception(ex);
		return;
	}

	if((values.second-values.first)!=(featureIdxTo-featureIdxFrom))
	{
		::iBS::ArgumentException ex;
			ex.reason = "values are not the same size as the indexs";
			cb->ice_exception(ex);
			return;
	}

	if(foi->SetPolicy==iBS::FeatureValueSetPolicyDoNothing)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal action, not allowed to set";
		cb->ice_exception(ex);
		return;
	}
	if(foi->ObserverGroupID>0 || foi->StorePolicy==iBS::FeatureValueStorePolicyBinaryFilesObserverGroup)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal action, not allowed to set individual observer";
		cb->ice_exception(ex);
		return;
	}
	else if(foi->SetPolicy==iBS::FeatureValueSetPolicyInRAMAndSaveToStore)
	{
		FeatureValueWorkItemPtr wi= new CSetRAMDoublesColumnVector(
			foi,cb,observerID,featureIdxFrom,featureIdxTo,values);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);
		return;
	}
	else if(foi->SetPolicy==iBS::FeatureValueSetPolicyNoRAMImmediatelyToStore)
	{
		FeatureValueWorkItemPtr wi= new CSetToStoreDoublesColumnVector(
			foi,cb,observerID,featureIdxFrom,featureIdxTo,values);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);
		return;
	}
	else 
	{
		::iBS::ArgumentException ex;
		ex.reason = "SetPolicy not implemented";
		cb->ice_exception(ex);
		return;
	}
}

void
CFcdcReadWriteServiceImpl::SetIntsColumnVector_async(const ::iBS::AMD_FcdcReadWriteService_SetIntsColumnVectorPtr& cb,
                                                     ::Ice::Int observerID,
                                                     ::Ice::Long featureIdxFrom,
                                                     ::Ice::Long featureIdxTo,
                                                     const ::std::pair<const ::Ice::Int*, const ::Ice::Int*>& values,
                                                     const Ice::Current& current)
{
    iBS::FeatureObserverSimpleInfoPtr foi 
		= m_gv.theObserversDB.GetFeatureObserver(observerID);

	if(!foi){
		::iBS::ArgumentException ex;
			ex.reason = "illegal feature observer ID";
			cb->ice_exception(ex);
			return;
	}

	// not allowed for an observer in group
	if(foi->ObserverGroupSize>1){
		::iBS::ArgumentException ex;
			ex.reason = "method not implemented yet";
			cb->ice_exception(ex);
			return;
	}

	if(featureIdxTo<0 || featureIdxFrom<0 || featureIdxTo<featureIdxFrom
		||foi->DomainSize<featureIdxTo)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal feature idx range";
		cb->ice_exception(ex);
		return;
	}

	if((values.second-values.first)!=(featureIdxTo-featureIdxFrom))
	{
		::iBS::ArgumentException ex;
			ex.reason = "values are not the same size as the indexs";
			cb->ice_exception(ex);
			return;
	}

	if(foi->SetPolicy==iBS::FeatureValueSetPolicyDoNothing)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal action, not allowed to set";
		cb->ice_exception(ex);
		return;
	}
	if(foi->ObserverGroupID>0 || foi->StorePolicy==iBS::FeatureValueStorePolicyBinaryFilesObserverGroup)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal action, not allowed to set individual observer";
		cb->ice_exception(ex);
		return;
	}
	else if(foi->SetPolicy==iBS::FeatureValueSetPolicyInRAMAndSaveToStore)
	{
		FeatureValueWorkItemPtr wi= new CSetRAMIntsColumnVector(
			foi,cb,observerID,featureIdxFrom,featureIdxTo,values);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);
		return;
	}
	else if(foi->SetPolicy==iBS::FeatureValueSetPolicyNoRAMImmediatelyToStore)
	{
		FeatureValueWorkItemPtr wi= new CSetToStoreIntsColumnVector(
			foi,cb,observerID,featureIdxFrom,featureIdxTo,values);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);
		return;
	}
	else 
	{
		::iBS::ArgumentException ex;
		ex.reason = "SetPolicy not implemented";
		cb->ice_exception(ex);
		return;
	}
}

void
CFcdcReadWriteServiceImpl::SetBytesColumnVector_async(const ::iBS::AMD_FcdcReadWriteService_SetBytesColumnVectorPtr& cb,
                                                      ::Ice::Int observerID,
                                                      ::Ice::Long featureIdxFrom,
                                                      ::Ice::Long featureIdxTo,
                                                      const ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*>& bytes,
                                                      ::iBS::ByteArrayContentEnum content,
                                                      ::iBS::ByteArrayEndianEnum endian,
                                                      const Ice::Current& current)
{
    iBS::FeatureObserverSimpleInfoPtr foi 
		= m_gv.theObserversDB.GetFeatureObserver(observerID);

	if(!foi){
		::iBS::ArgumentException ex;
			ex.reason = "illegal feature observer ID";
			cb->ice_exception(ex);
			return;
	}

	// not allowed for an observer in group
	if(foi->ObserverGroupSize>1){
		::iBS::ArgumentException ex;
			ex.reason = "method not implemented yet";
			cb->ice_exception(ex);
			return;
	}

	if(featureIdxTo<0 || featureIdxFrom<0 || featureIdxTo<featureIdxFrom)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal feature idx range";
		cb->ice_exception(ex);
		return;
	}

	if(foi->SetPolicy==iBS::FeatureValueSetPolicyDoNothing)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal action, not allowed to set";
		cb->ice_exception(ex);
		return;
	}
	if(foi->ObserverGroupID>0 || foi->StorePolicy==iBS::FeatureValueStorePolicyBinaryFilesObserverGroup)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal action, not allowed to set individual observer";
		cb->ice_exception(ex);
		return;
	}
	else if(foi->SetPolicy==iBS::FeatureValueSetPolicyInRAMAndSaveToStore)
	{
		FeatureValueWorkItemPtr wi= new CSetRAMBytesColumnVector(
			foi,cb,observerID,featureIdxFrom,featureIdxTo,bytes,content,endian);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);
		return;
	}
	else if(foi->SetPolicy==iBS::FeatureValueSetPolicyNoRAMImmediatelyToStore)
	{
		FeatureValueWorkItemPtr wi= new CSetToStoreBytesColumnVector(
			foi,cb,observerID,featureIdxFrom,featureIdxTo,bytes,content,endian);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);
		return;
	}
	else 
	{
		::iBS::ArgumentException ex;
		ex.reason = "SetPolicy not implemented";
		cb->ice_exception(ex);
		return;
	}
}

void
CFcdcReadWriteServiceImpl::SetDoublesRowMatrix_async(const ::iBS::AMD_FcdcReadWriteService_SetDoublesRowMatrixPtr& cb,
                                                     ::Ice::Int observerGroupID,
                                                     ::Ice::Long featureIdxFrom,
                                                     ::Ice::Long featureIdxTo,
                                                     const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values,
                                                     const Ice::Current& current)
{
    int observerID = observerGroupID;
    iBS::FeatureObserverSimpleInfoPtr foi 
		= m_gv.theObserversDB.GetFeatureObserver(observerID);

	iBS::FeatureObserverSimpleInfoPtr goi 
		= m_gv.theObserversDB.GetFeatureObserverGroup(observerGroupID);

	::Ice::Long g_featureIdxFrom = featureIdxFrom * goi->ObserverGroupSize;
	::Ice::Long g_featureIdxTo = featureIdxTo * goi->ObserverGroupSize;


	if(!foi){
		::iBS::ArgumentException ex;
			ex.reason = "illegal feature observer ID";
			cb->ice_exception(ex);
			return;
	}

	if(foi->ObserverGroupID!=observerGroupID){
		::iBS::ArgumentException ex;
			ex.reason = "observer not in a group";
			cb->ice_exception(ex);
			return;
	}

	if((values.second-values.first)!=(g_featureIdxTo-g_featureIdxFrom))
	{
		::iBS::ArgumentException ex;
			ex.reason = "values are not the same size as the indexs";
			cb->ice_exception(ex);
			return;
	}



	if(featureIdxTo<0 || featureIdxFrom<0 || featureIdxTo<featureIdxFrom
		||foi->DomainSize<featureIdxTo)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal feature idx range";
		cb->ice_exception(ex);
		return;
	}

	if(foi->SetPolicy==iBS::FeatureValueSetPolicyDoNothing)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal action, not allowed to set";
		cb->ice_exception(ex);
		return;
	}
	else //if(foi->SetPolicy==iBS::FeatureValueSetPolicyInRAMAndSaveToStore)
	{
		//just use the group observer to set values, therefore can treated as a normal observer
		
		FeatureValueWorkItemPtr wi= new CSetRAMDoublesRowMatrix(
			goi,cb,observerID,g_featureIdxFrom,g_featureIdxTo,values);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(goi,wi);
		return;
	}
	/*else 
	{
		::iBS::ArgumentException ex;
		ex.reason = "SetPolicy not implemented";
		cb->ice_exception(ex);
		return;
	}*/
}


//=================================================================
//Admin service
//=================================================================

void
CFcdcAdminServiceImpl::Shutdown(const Ice::Current& current)
{
	if(CGlobalVars::get()&&CGlobalVars::get()->theFeatureValueWorkerMgr)
	{
		CGlobalVars::get()->theFeatureValueWorkerMgr->RequestShutdownAllWorkers();
	}
	 current.adapter->getCommunicator()->shutdown();
}

::Ice::Int
CFcdcAdminServiceImpl::RqstNewFeatureDomainID(::Ice::Int& domainID,
                                                const Ice::Current& current)
{
	return m_gv.theDomainsDB.RqstNewFeatureDomainID(domainID);
}

::Ice::Int
CFcdcAdminServiceImpl::RqstNewFeatureObserverID(bool inRAMNoSave, ::Ice::Int& observerID,
                                                  const Ice::Current& current)
{
	return m_gv.theObserversDB.RqstNewFeatureObserverID(observerID,inRAMNoSave);
}

::Ice::Int
CFcdcAdminServiceImpl::RqstNewFeatureObserversInGroup( ::Ice::Int groupSize, bool inRAMNoSave,::iBS::IntVec& observerIDs,
                                                        const Ice::Current& current)
{
	return m_gv.theObserversDB.RqstNewFeatureObserversInGroup(groupSize, observerIDs,inRAMNoSave);
}

::Ice::Int
CFcdcAdminServiceImpl::AttachBigMatrix(::Ice::Int colCnt,
	::Ice::Long rowCnt,
	const ::iBS::StringVec& colNames,
	const ::std::string& storePathPrefix,
	::iBS::IntVec& OIDs,
	const Ice::Current& current)
{
	// create observer group info
	
	CGlobalVars::get()->theObserversDB.RqstNewFeatureObserversInGroup(
		(int)colCnt, OIDs, false);

	iBS::FeatureObserverInfoVec out_FOIs;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(OIDs, out_FOIs);
	::Ice::Long domainSize = rowCnt;
	for (int i = 0; i < colCnt; i++)
	{
		iBS::FeatureObserverInfo& foi = out_FOIs[i];
		foi.ObserverName = colNames[i];
		foi.DomainSize = domainSize;
		foi.SetPolicy = iBS::FeatureValueSetPolicyDoNothing;	//readonly outside, interannly writalbe
		foi.GetPolicy = iBS::FeatureValueGetPolicyGetForOneTimeRead;
		foi.StoreLocation = iBS::FeatureValueStoreLocationSpecified;
		foi.SpecifiedPathPrefix = storePathPrefix;
	}
	CGlobalVars::get()->theObserversDB.SetFeatureObservers(out_FOIs);

	return 1;
}

::Ice::Int
CFcdcAdminServiceImpl::AttachBigVector(::Ice::Long rowCnt,
const ::std::string& colName,
const ::std::string& storePathPrefix,
::Ice::Int& OID,
const Ice::Current& current)
{
	CGlobalVars::get()->theObserversDB.RqstNewFeatureObserverID(OID);
	::iBS::IntVec OIDs;
	OIDs.push_back(OID);
	iBS::FeatureObserverInfoVec out_FOIs;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(OIDs, out_FOIs);
	::Ice::Long domainSize = rowCnt;
	for (int i = 0; i < 1; i++)
	{
		iBS::FeatureObserverInfo& foi = out_FOIs[i];
		foi.ObserverName = colName;
		foi.DomainSize = domainSize;
		foi.SetPolicy = iBS::FeatureValueSetPolicyDoNothing;	//readonly outside, interannly writalbe
		foi.GetPolicy = iBS::FeatureValueGetPolicyGetForOneTimeRead;
		foi.StoreLocation = iBS::FeatureValueStoreLocationSpecified;
		foi.SpecifiedPathPrefix = storePathPrefix;
	}
	CGlobalVars::get()->theObserversDB.SetFeatureObservers(out_FOIs);

	return 1;
}

void
CFcdcAdminServiceImpl::ForceLoadInRAM_async(const ::iBS::AMD_FcdcAdminService_ForceLoadInRAMPtr& cb,
                                                const ::iBS::IntVec& observerIDs,
                                                const Ice::Current& current) const
{
	cout<<IceUtil::Time::now().toDateTime()<<" ForceLoadInRAM ..."<<endl; 
	for(size_t i=0;i<observerIDs.size();i++)
	{
		int observerID = observerIDs[i];
		iBS::FeatureObserverSimpleInfoPtr foi 
			= m_gv.theObserversDB.GetFeatureObserver(observerID);
		if(!foi)
		{
			 ostringstream os;
			 os<<" illegal feature observer ID="<<observerID;

			::iBS::ArgumentException ex;
			ex.reason = os.str();
			cb->ice_exception(ex);
			return;
		}

		FeatureValueWorkItemPtr wi= new CForceLoadInRAM(
			foi,observerID,0,1);
		//use different threads to load data
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);
	}
	cb->ice_response(1);
}

void
CFcdcAdminServiceImpl::ForceLeaveRAM_async(const ::iBS::AMD_FcdcAdminService_ForceLeaveRAMPtr& cb,
                                               const ::iBS::IntVec& observerIDs,
                                               const Ice::Current& current) const
{
   ::Ice::Int r = 0;
    cb->ice_response(r);
}

void
CFcdcAdminServiceImpl::RecalculateObserverStats_async(
		const ::iBS::AMD_FcdcAdminService_RecalculateObserverStatsPtr& cb,
		const ::iBS::IntVec& observerIDs,
        const Ice::Current& current)
{
	for (size_t i = 0; i<observerIDs.size(); i++)
	{
		int observerID = observerIDs[i];
		iBS::FeatureObserverSimpleInfoPtr foi
			= m_gv.theObserversDB.GetFeatureObserver(observerID);
		if (!foi)
		{
			ostringstream os;
			os << " illegal feature observer ID=" << observerID;

			::iBS::ArgumentException ex;
			ex.reason = os.str();
			cb->ice_exception(ex);
			return;
		}

		FeatureValueWorkItemPtr wi = new CRecalculateObserverStats(foi);
		//use different threads
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi, wi);
	}
	cb->ice_response(1);
   return;
}


void
CFcdcAdminServiceImpl::RecalculateObserverIndex_async(
		const ::iBS::AMD_FcdcAdminService_RecalculateObserverIndexPtr& cb,
        ::Ice::Int observerID,
		bool saveIndexFile,
        const Ice::Current& current)
{
    iBS::FeatureObserverSimpleInfoPtr foi 
		= m_gv.theObserversDB.GetFeatureObserver(observerID);

	if(!foi){
		::iBS::ArgumentException ex;
		ex.reason = "illegal observer ID";
		cb->ice_exception(ex);
		return;
	}

    FeatureValueWorkItemPtr wi= new CRecalculateObserverIndexIntValueIntKey(foi,cb,saveIndexFile);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);

   return;
}

void
CFcdcAdminServiceImpl::RemoveFeatureObservers_async(const ::iBS::AMD_FcdcAdminService_RemoveFeatureObserversPtr& cb,
const ::iBS::IntVec& observerIDs,
bool removeDataFile,
const Ice::Current& current)
{
	for (size_t i = 0; i < observerIDs.size(); i++)
	{
		int observerID = observerIDs[i];
		iBS::FeatureObserverSimpleInfoPtr foi
			= m_gv.theObserversDB.GetFeatureObserver(observerID);
		if (!foi)
		{
			ostringstream os;
			os << " illegal feature observer ID=" << observerID;

			::iBS::ArgumentException ex;
			ex.reason = os.str();
			cb->ice_exception(ex);
			return;
		}
	}

	if (removeDataFile)
	{
		iBS::FeatureObserverSimpleInfoVec fois;
		m_gv.theObserversDB.GetFeatureObservers(observerIDs, fois);
		m_gv.theObserversDB.RemoveFeatureObservers(observerIDs);
		m_gv.theObserverStatsDB->RemoveObserversStats(observerIDs);
		FeatureValueWorkItemPtr wi = new ::Original::CRemoveFeatureValues(
			fois, observerIDs, cb);

		m_gv.theFeatureValueWorkerMgr->AssignItemToWorkerByTime(wi);
	}
	else
	{
		m_gv.theObserversDB.RemoveFeatureObservers(observerIDs);
		m_gv.theObserverStatsDB->RemoveObserversStats(observerIDs);
		cb->ice_response(1);
	}
	
}

::Ice::Int
CFcdcAdminServiceImpl::SetObserverStats(const ::iBS::ObserverStatsInfoVec& osis,
const Ice::Current& current)
{
	for (size_t i = 0; i < osis.size(); i++)
	{
		int observerID = osis[i].ObserverID;
		iBS::FeatureObserverSimpleInfoPtr foi
			= m_gv.theObserversDB.GetFeatureObserver(observerID);
		if (!foi)
		{
			ostringstream os;
			os << " illegal feature observer ID=" << observerID;

			::iBS::ArgumentException ex;
			ex.reason = os.str();
			throw ex;
		}
	}

	CGlobalVars::get()->theObserverStatsDB->SetObserverStatsInfos(osis);
	return 1;
}
