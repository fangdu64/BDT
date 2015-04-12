#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <FeatureValueWorker.h>
#include <BigMatrixFacetWorkItem.h>
#include <algorithm>    // std::copy
#include <limits>
#include <GlobalVars.h>
#include <FeatureValueRAM.h>
#include <FeatureValueStoreMgr.h>
#include <FeatureObserverDB.h>
#include <ObserverStatsDB.h>
#include <CommonHelper.h>
#include <bdtUtil/RowAdjustHelper.h>
#include <FeatureValueWorkerMgr.h>

///////////////////////////////////////////////////////////////////////////////////////

BigMatrix::CGetRowMatrix::~CGetRowMatrix()
{

}

void
BigMatrix::CGetRowMatrix::DoWork()
{
	Ice::Long colCnt = m_observerIDs.size();
	Ice::Long rowCnt = m_featureIdxTo - m_featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	::IceUtil::ScopedArray<Ice::Double>  retValues(new ::Ice::Double[totalValueCnt]);
	if (!retValues.get())
	{
		::iBS::ArgumentException ex;
		ex.reason = "no mem available";
		m_cb->ice_exception(ex);
		return;
	}

	getRetValues(retValues.get());

	CRowAdjustHelper::Adjust(retValues.get(), rowCnt, colCnt, m_rowAdjust);

	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);
	ret.first = retValues.get();
	ret.second = retValues.get() + totalValueCnt;
	m_cb->ice_response(1, ret);
}

///////////////////////////////////////////////////////////////////////////////////////

BigMatrix::CSampleRowMatrix::~CSampleRowMatrix()
{

}

void
BigMatrix::CSampleRowMatrix::DoWork()
{
	Ice::Long colCnt = m_observerIDs.size();
	Ice::Long rowCnt = m_featureIdxs.size();
	Ice::Long totalValueCnt = rowCnt*colCnt;

	::IceUtil::ScopedArray<Ice::Double>  retValues(new ::Ice::Double[totalValueCnt]);
	if (!retValues.get())
	{
		::iBS::ArgumentException ex;
		ex.reason = "no mem";
		m_cb->ice_exception(ex);
		return;
	}

	getRetValues(retValues.get());

	CRowAdjustHelper::Adjust(retValues.get(), rowCnt, colCnt, m_rowAdjust);

	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);
	ret.first = retValues.get();
	ret.second = retValues.get() + totalValueCnt;
	m_cb->ice_response(1, ret);
}

///////////////////////////////////////////////////////////////////////////////////////

BigMatrix::CRecalculateObserverStats::~CRecalculateObserverStats()
{

}

bool BigMatrix::CRecalculateObserverStats::GetY(
	::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Double*  Y)
{
	if (!Y)
	{
		//should already allocated 
		return false;
	}

	Ice::Long colCnt = m_fois.size();
	Ice::Long rowCnt = featureIdxTo - featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	::iBS::AMD_FcdcReadService_GetRowMatrixPtr nullcb;
	::Original::CGetRowMatrix wi(
		m_fois, nullcb, m_observerIDs, featureIdxFrom, featureIdxTo);
	wi.getRetValues(Y);

	return true;
}

void
BigMatrix::CRecalculateObserverStats::DoWork()
{
	Ice::Long colCnt = m_observerIDs.size();
	Ice::Long ramMb = m_ramMb;
	ramMb /= sizeof(Ice::Double);

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::IceUtil::ScopedArray<Ice::Double>  Y(new ::Ice::Double[batchValueCnt]);
	if (!Y.get()){
		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_taskID, 0, iBS::AMDTaskStatusFailure);
		return;
	}
	Ice::Long TotalRowCnt = m_fois[0]->DomainSize;
	::Ice::Long batchRowCnt = batchValueCnt / colCnt;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;
	CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_taskID, 0, batchCnt);
	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = 0;
	::Ice::Long featureIdxTo = 0;

	::iBS::ObserverStatsInfoVec osis(colCnt);
	for (int i = 0; i < colCnt; i++)
	{
		::iBS::ObserverStatsInfo& osi = osis[i];
		osi.ObserverID = m_observerIDs[i];
		osi.UpdateDT = IceUtil::Time::now().toMilliSeconds();
		osi.Version = 0;
		osi.Cnt = 0;
		osi.Min = std::numeric_limits<Ice::Double>::max();
		osi.Max = std::numeric_limits<Ice::Double>::min();
		osi.Sum = 0;
	}

	while (remainCnt>0)
	{
		if (remainCnt>batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;
		
		std::cout << IceUtil::Time::now().toDateTime() << " RecalculateObserverStats batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		GetY(featureIdxFrom, featureIdxTo, Y.get());

		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			for (Ice::Long j = 0; j<colCnt; j++)
			{
				Ice::Double val = Y[i*colCnt + j];
				::iBS::ObserverStatsInfo& osi = osis[j];
				if (osi.Min>val)
				{
					osi.Min = val;
				}
				if (osi.Max<val)
				{
					osi.Max = val;
				}
				osi.Sum += val;
				osi.Cnt++;
			}
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;

		std::cout << IceUtil::Time::now().toDateTime() << " RecalculateObserverStats batch " << batchIdx << "/" << batchCnt << " end" << endl;
		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_taskID, 1);
	}

	CGlobalVars::get()->theObserverStatsDB->SetObserverStatsInfos(osis);

	std::cout << IceUtil::Time::now().toDateTime() << " RecalculateObserverStats [done] "<<endl;
	CGlobalVars::get()->theFeatureValueWorkerMgr->SetAMDTaskDone(m_taskID);
}