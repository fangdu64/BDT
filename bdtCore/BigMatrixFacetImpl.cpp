#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/ScopedArray.h>
#include <GlobalVars.h>
#include <BigMatrixFacetImpl.h>
#include <FeatureDomainDB.h>
#include <FeatureObserverDB.h>
#include <FeatureValueRAM.h>
#include <FeatureValueWorker.h>
#include <BigMatrixFacetWorkItem.h>
#include <FeatureValueWorkerMgr.h>
#include <ObserverStatsDB.h>


std::string CBigMatrixServiceImpl::GetServantName(int gid)
{
	ostringstream os;
	os << "BigMatrix_Facet" << gid;
    return os.str();
}

::Ice::Int
CBigMatrixServiceImpl::SetOutputSamples(const ::iBS::IntVec& sampleIDs,
const Ice::Current& current)
{
	m_outputSampleIDs = sampleIDs;
	return 1;
}

::Ice::Int
CBigMatrixServiceImpl::SetRowAdjust(::iBS::RowAdjustEnum rowAdjust,
const Ice::Current& current)
{
	m_rowAdjust = rowAdjust;
	return 1;
}

void
CBigMatrixServiceImpl::GetRowMatrix_async(const ::iBS::AMD_FcdcReadService_GetRowMatrixPtr& cb,
	const ::iBS::IntVec& observerIDs,
	::Ice::Long featureIdxFrom,
	::Ice::Long featureIdxTo,
	const IceUtil::Optional< ::iBS::RowAdjustEnum>& rowAdjust,
	const Ice::Current& current)
{
	::iBS::IntVec theObserverIDs;
	if (observerIDs.empty())
	{
		
		int colCnt = (int)m_fois.size();
		theObserverIDs.resize(colCnt, 0);
		for (int i = 0; i < colCnt; i++)
		{
			theObserverIDs[i] = m_fois[i]->ObserverID;
		}
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

	::iBS::RowAdjustEnum rowadj = rowAdjust ? rowAdjust.get() : m_rowAdjust;
	FeatureValueWorkItemPtr wi = new ::BigMatrix::CGetRowMatrix(
		fois, cb, theObserverIDs, featureIdxFrom, featureIdxTo, rowadj);

	m_gv.theFeatureValueWorkerMgr->AssignItemToWorkerByTime(wi);
}

void
CBigMatrixServiceImpl::SampleRowMatrix_async(const ::iBS::AMD_FcdcReadService_SampleRowMatrixPtr& cb,
const ::iBS::IntVec& observerIDs,
const ::iBS::LongVec& featureIdxs,
const IceUtil::Optional< ::iBS::RowAdjustEnum>& rowAdjust,
const Ice::Current& current)
{
	::iBS::IntVec theObserverIDs;
	if (observerIDs.empty())
	{
		int colCnt = (int)m_fois.size();
		theObserverIDs.resize(colCnt, 0);
		for (int i = 0; i < colCnt; i++)
		{
			theObserverIDs[i] = m_fois[i]->ObserverID;
		}
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

	::iBS::RowAdjustEnum rowadj = rowAdjust ? rowAdjust.get() : m_rowAdjust;
	FeatureValueWorkItemPtr wi = new ::BigMatrix::CSampleRowMatrix(
		fois, cb, theObserverIDs, featureIdxs, rowadj);

	m_gv.theFeatureValueWorkerMgr->AssignItemToWorkerByTime(wi);
}

void
CBigMatrixServiceImpl::RecalculateObserverStats_async(
const ::iBS::AMD_BigMatrixService_RecalculateObserverStatsPtr& cb,
::Ice::Long ramMb,
const Ice::Current& current)
{
	::iBS::IntVec theObserverIDs;
	int colCnt = (int)m_fois.size();
	theObserverIDs.resize(colCnt, 0);
	for (int i = 0; i < colCnt; i++)
	{
		theObserverIDs[i] = m_fois[i]->ObserverID;
	}
	::Ice::Long featureIdxFrom = 0;
	::Ice::Long featureIdxTo = m_fois[0]->DomainSize;
	iBS::FeatureObserverSimpleInfoVec fois;
	std::string exReason;
	bool rqstValid = rvGetJoinedRowMatrix(theObserverIDs, featureIdxFrom, 1, fois, exReason);
	if (!rqstValid)
	{
		::iBS::ArgumentException ex;
		ex.reason = exReason;
		cb->ice_exception(ex);
		return;
	}

	Ice::Long taskID = CGlobalVars::get()->theFeatureValueWorkerMgr->RegisterAMDTask(
		"RecalculateObserverStats", 100);
	cb->ice_response(1, taskID);

	FeatureValueWorkItemPtr wi = new ::BigMatrix::CRecalculateObserverStats(
		fois, theObserverIDs, ramMb, taskID);

	m_gv.theFeatureValueWorkerMgr->AssignItemToWorkerByTime(wi);
}