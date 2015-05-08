#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/ScopedArray.h>
#include <GlobalVars.h>
#include <RUVFacetImpl.h>
#include <FeatureDomainDB.h>
#include <FeatureObserverDB.h>
#include <FeatureValueRAM.h>
#include <FeatureValueWorker.h>
#include <RUVFacetWorkItem.h>
#include <FeatureValueWorkerMgr.h>
#include <ObserverStatsDB.h>


std::string CFcdcRUVServiceImpl::GetServantName(int facetID)
{
	ostringstream os;
	os<<"RUVService_Facet"<<facetID;
    return os.str();
}

::Ice::Int
CFcdcRUVServiceImpl::GetFeatureObservers(const ::iBS::IntVec& observerIDs,
::iBS::FeatureObserverInfoVec& observerInfos,
const Ice::Current& current)
{
	::iBS::IntVec theObserverIDs;
	if (observerIDs.empty())
	{
		theObserverIDs = m_ruvBuilder.GetOutputSamples();
		if (theObserverIDs.empty())
		{
			theObserverIDs = m_ruvBuilder.m_RUVInfo.SampleIDs;
		}
	}
	else
	{
		theObserverIDs = observerIDs;
	}

	return m_gv.theObserversDB.GetFeatureObservers(theObserverIDs, observerInfos);
}

void
CFcdcRUVServiceImpl::SetActiveK_async(const ::iBS::AMD_FcdcRUVService_SetActiveKPtr& cb,
                                          ::Ice::Int k,
										  ::Ice::Int extW,
                                          const Ice::Current& current)
{

	iBS::FeatureObserverSimpleInfoPtr wts_foi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_ruvBuilder.m_RUVInfo.ObserverIDforWts);
	if (!wts_foi && k == 0 && extW == 0)
	{
		cb->ice_response(1);
		return;
	}
	if(!wts_foi)
	{
		::iBS::ArgumentException ex;
		ex.reason = "RUVs model not ready";
		cb->ice_exception(ex);
		return;
	}

	FeatureValueWorkItemPtr wi = new ::RUVs::CSetActiveK(cb, k, extW,m_ruvBuilder);
	m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(wts_foi,wi);

}

::Ice::Int
CFcdcRUVServiceImpl::SetOutputScale(::iBS::RUVOutputScaleEnum scale,
const Ice::Current& current)
{
	return m_ruvBuilder.SetOutputScale(scale);
}

::Ice::Int
CFcdcRUVServiceImpl::SetOutputMode(::iBS::RUVOutputModeEnum mode,
const Ice::Current& current)
{
	return m_ruvBuilder.SetOutputMode(mode);
}

::Ice::Int
CFcdcRUVServiceImpl::SetOutputSamples(const ::iBS::IntVec& sampleIDs,
const Ice::Current& current)
{
	return m_ruvBuilder.SetOutputSamples(sampleIDs);
}

::Ice::Int
CFcdcRUVServiceImpl::SetWtVectorIdxs(const ::iBS::IntVec& vecIdxs,
const Ice::Current& current)
{
	return m_ruvBuilder.SetWtVectorIdxs(vecIdxs);
}

::Ice::Int
CFcdcRUVServiceImpl::ExcludeSamplesForGroupMean(const ::iBS::IntVec& excludeSampleIDs,
const Ice::Current& current)
{
	//not implemented
	return m_ruvBuilder.SetExcludedSamplesForGroupMean(excludeSampleIDs);
}


::Ice::Int
CFcdcRUVServiceImpl::SetOutputWorkerNum(::Ice::Int workerNum,
const Ice::Current& current)
{
	return m_ruvBuilder.SetOutputWorkerNum(workerNum);
}

::Ice::Int
CFcdcRUVServiceImpl::SetCtrlQuantileValues(::Ice::Double quantile,
const ::iBS::DoubleVec& qvalues, ::Ice::Double fraction,
const Ice::Current& current)
{
	return m_ruvBuilder.SetCtrlQuantileValues(quantile, qvalues, fraction);
}

::Ice::Int
CFcdcRUVServiceImpl::GetConditionIdxs(const ::iBS::IntVec& observerIDs,
                                        ::iBS::IntVec& conditionIdxs,
                                        const Ice::Current& current)
{
	::iBS::IntVec theObserverIDs;
	if(observerIDs.empty())
	{
		theObserverIDs=m_ruvBuilder.m_RUVInfo.RawCountObserverIDs;
	}
	else
	{
		theObserverIDs=observerIDs;
	}

	return m_ruvBuilder.GetConditionIdxs(theObserverIDs,conditionIdxs);
}

::Ice::Int
CFcdcRUVServiceImpl::GetConditionInfos(::iBS::ConditionInfoVec& conditions,
                                         const Ice::Current& current)
{
	iBS::FeatureObserverInfoVec fois;
	
	if(m_ruvBuilder.m_RUVInfo.SampleIDs.size()
		>=m_ruvBuilder.m_RUVInfo.RawCountObserverIDs.size())
	{
		//unfiltered has full infomation
		CGlobalVars::get()->theObserversDB.GetFeatureObservers(
			m_ruvBuilder.m_RUVInfo.SampleIDs,fois);
	}
	else
	{
		CGlobalVars::get()->theObserversDB.GetFeatureObservers(
			m_ruvBuilder.m_RUVInfo.RawCountObserverIDs,fois);
	}

	
	int p=(int)m_ruvBuilder.m_RUVInfo.ConditionObserverIDs.size();
	conditions.reserve(p);
	for(int i=0;i<p;i++)
	{
		::iBS::ConditionInfo ci;
		ci.ConditionIdx=i;
		if(m_ruvBuilder.m_conditionSampleIdxs.size()>i)
		{
			int sampleIdx=m_ruvBuilder.m_conditionSampleIdxs[i][0];
			ci.Name=fois[sampleIdx].ObserverName;
			ci.ObserverCnt=(int)m_ruvBuilder.m_conditionSampleIdxs[i].size();
		}
		conditions.push_back(ci);
	}
    return 1;
}

::Ice::Int
CFcdcRUVServiceImpl :: GetSamplesInGroups(const ::iBS::IntVec& sampleIDs,
::iBS::IntVecVec& groupSampleIDs,
const Ice::Current& current)
{
	::iBS::IntVec observerIDs;
	if (sampleIDs.empty())
	{
		groupSampleIDs = m_ruvBuilder.m_RUVInfo.ReplicateSampleIDs;
		return 1;
	}
	else
	{
		observerIDs = sampleIDs;
	}

	//m_ruvBuilder.m_RUVInfo.ReplicateSampleIDs
	int p = (int)m_ruvBuilder.m_RUVInfo.ReplicateSampleIDs.size();
	for (int g = 0; g < p; g++)
	{
		const iBS::IntVec& gSamples = m_ruvBuilder.m_RUVInfo.ReplicateSampleIDs[g];
		iBS::IntVec mSamples;
		for (int i = 0; i < gSamples.size(); i++)
		{
			if (std::find(observerIDs.begin(), observerIDs.end(), gSamples[i]) != observerIDs.end()) {
				mSamples.push_back(gSamples[i]);
			}
		}
		if (!mSamples.empty())
		{
			groupSampleIDs.push_back(mSamples);
		}
	}

	return 1;
}

::Ice::Int
CFcdcRUVServiceImpl::GetG(::iBS::DoubleVec& values,
const Ice::Current& current)
{
	return m_ruvBuilder.GetG(values);
}

::Ice::Int
CFcdcRUVServiceImpl::GetWt(::iBS::DoubleVec& values,
const Ice::Current& current)
{
	return m_ruvBuilder.GetWt(values);
}

::Ice::Int
CFcdcRUVServiceImpl::GetEigenVals(::iBS::DoubleVec& values,
const Ice::Current& current)
{
	return m_ruvBuilder.GetEigenVals(values);
}

::Ice::Int
CFcdcRUVServiceImpl::SelectKByEigenVals(::Ice::Double minFraction,
::Ice::Int& k,
::iBS::DoubleVec& fractions,
const Ice::Current& current)
{
	return m_ruvBuilder.SelectKByEigenVals(minFraction, k, fractions);
}

void
::CFcdcRUVServiceImpl::DecomposeVariance_async(const ::iBS::AMD_FcdcRUVService_DecomposeVariancePtr& cb,
const ::iBS::IntVec& ks,
const ::iBS::IntVec& extWs,
const ::iBS::IntVecVec& wtVecIdxs,
::Ice::Long featureIdxFrom,
::Ice::Long featureIdxTo,
::Ice::Int threadCnt,
::Ice::Long ramMb,
const ::std::string& outfile,
const Ice::Current& current)
{
	if (ks.empty())
	{
		::iBS::ArgumentException ex;
		ex.reason = "ks empty";
		cb->ice_exception(ex);
		return;
	}

	iBS::IntVec knowFactors = extWs;
	if (knowFactors.size() < ks.size())
	{
		knowFactors.resize(ks.size(), 0);
	}

	iBS::IntVec& observerIDs = m_ruvBuilder.m_RUVInfo.RawCountObserverIDs;

	size_t observerCnt = observerIDs.size();
	if (observerCnt == 0)
	{
		::iBS::ArgumentException ex;
		ex.reason = "empty observer";
		cb->ice_exception(ex);
		return;
	}

	iBS::FeatureObserverSimpleInfoPtr foi
		= CGlobalVars::get()->theObserversDB.GetFeatureObserver(observerIDs[0]);
	if (!foi)
	{
		::iBS::ArgumentException ex;
		ex.reason = "invalid observer ID";
		cb->ice_exception(ex);
		return;
	}

	FeatureValueWorkItemPtr wi = new ::RUVs::CDecomposeVariance(
		cb, m_ruvBuilder, ks, knowFactors, wtVecIdxs, featureIdxFrom, featureIdxTo, threadCnt, ramMb, outfile);
	m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi, wi);
}


void
CFcdcRUVServiceImpl::RebuildRUVModel_async(const ::iBS::AMD_FcdcRUVService_RebuildRUVModelPtr& cb,
												::Ice::Int threadCnt,
                                                ::Ice::Long ramMb,
                                                const Ice::Current& current)
{

	if(threadCnt<1 || ramMb<128)
	{
		::iBS::ArgumentException ex;
		ex.reason = "at least 1 thread, 128M RAM";
		cb->ice_exception(ex);
		return;
	}

	iBS::IntVec& observerIDs=m_ruvBuilder.m_RUVInfo.RawCountObserverIDs;

	size_t observerCnt=observerIDs.size();
	if(observerCnt==0)
	{
		::iBS::ArgumentException ex;
		ex.reason = "empty observer";
		cb->ice_exception(ex);
		return;
	}

	iBS::FeatureObserverSimpleInfoPtr foi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(observerIDs[0]);
	if(!foi)
	{
		::iBS::ArgumentException ex;
		ex.reason = "invalid observer ID";
		cb->ice_exception(ex);
		return;
	}

	Ice::Long taskID = CGlobalVars::get()->theFeatureValueWorkerMgr->RegisterAMDTask(
		"RebuildRUVsModel", 0);
	cb->ice_response(1, taskID);

	FeatureValueWorkItemPtr wi = new ::RUVs::CRebuildRUVsModel(m_ruvBuilder,threadCnt,ramMb,taskID);
	m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);
	
}

void
CFcdcRUVServiceImpl::GetDoublesColumnVector_async(const ::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr& cb,
                                                        ::Ice::Int observerID,
                                                        ::Ice::Long featureIdxFrom,
                                                        ::Ice::Long featureIdxTo,
                                                        const Ice::Current& current)
{
    ::iBS::ArgumentException ex;
	ex.reason = "not implemented";
	cb->ice_exception(ex);
	return;
}


void
CFcdcRUVServiceImpl::GetDoublesRowMatrix_async(const ::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr& cb,
                                                     ::Ice::Int observerGroupID,
                                                     ::Ice::Long featureIdxFrom,
                                                     ::Ice::Long featureIdxTo,
                                                     const Ice::Current& current)
{
	::iBS::ArgumentException ex;
	ex.reason = "not implemented";
	cb->ice_exception(ex);
	return;
}

void
CFcdcRUVServiceImpl::GetRowMatrix_async(const ::iBS::AMD_FcdcReadService_GetRowMatrixPtr& cb,
	const ::iBS::IntVec& observerIDs,
	::Ice::Long featureIdxFrom,
	::Ice::Long featureIdxTo,
	const IceUtil::Optional< ::iBS::RowAdjustEnum>& rowAdjust,
	const Ice::Current& current)
{
	::iBS::IntVec theObserverIDs;
	if (observerIDs.empty())
	{
		theObserverIDs = m_ruvBuilder.m_RUVInfo.RawCountObserverIDs;
	}
	else
	{
		theObserverIDs = observerIDs;
	}

	iBS::FeatureObserverSimpleInfoVec fois;
	std::string exReason;
	bool rqstValid = rvGetJoinedRowMatrix(theObserverIDs, featureIdxFrom, featureIdxTo, fois, exReason);
	if (!rqstValid)
	{
		::iBS::ArgumentException ex;
		ex.reason = exReason;
		cb->ice_exception(ex);
		return;
	}

	::iBS::RowAdjustEnum rowadj = rowAdjust ? rowAdjust.get() : iBS::RowAdjustNone;
	FeatureValueWorkItemPtr wi = new ::RUVs::CGetRowMatrix(
		fois, cb, theObserverIDs, featureIdxFrom, featureIdxTo, m_ruvBuilder, rowadj);

	m_gv.theFeatureValueWorkerMgr->AssignItemToWorkerByTime(wi);
}

void
CFcdcRUVServiceImpl::SampleRowMatrix_async(const ::iBS::AMD_FcdcReadService_SampleRowMatrixPtr& cb,
const ::iBS::IntVec& observerIDs,
const ::iBS::LongVec& featureIdxs,
const IceUtil::Optional< ::iBS::RowAdjustEnum>& rowAdjust,
const Ice::Current& current)
{
	::iBS::IntVec theObserverIDs;
	if (observerIDs.empty())
	{
		theObserverIDs = m_ruvBuilder.m_RUVInfo.RawCountObserverIDs;
	}
	else
	{
		theObserverIDs = observerIDs;
	}

	iBS::FeatureObserverSimpleInfoVec fois;
	Ice::Long rowCnt = featureIdxs.size();
	std::string exReason;
	bool rqstValid = rvSampleJoinedRowMatrix(theObserverIDs, rowCnt, fois, exReason);
	if (!rqstValid)
	{
		::iBS::ArgumentException ex;
		ex.reason = exReason;
		cb->ice_exception(ex);
		return;
	}

	::iBS::RowAdjustEnum rowadj = rowAdjust ? rowAdjust.get() : iBS::RowAdjustNone;
	FeatureValueWorkItemPtr wi = new ::RUVs::CSampleRowMatrix(
		fois, cb, theObserverIDs, featureIdxs, m_ruvBuilder, rowadj);

	m_gv.theFeatureValueWorkerMgr->AssignItemToWorkerByTime(wi);
}
