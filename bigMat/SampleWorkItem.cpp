#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <FeatureValueWorker.h>
#include <SampleWorkItem.h>
#include <algorithm>    // std::copy
#include <GlobalVars.h>
#include <FeatureValueRAM.h>
#include <FeatureValueWorkerMgr.h>
#include <FeatureValueStoreMgr.h>
#include <FeatureObserverDB.h>
#include <ObserverStatsDB.h>
#include <CommonHelper.h>
#include <BamToBinCountCreator.h>
void
CCreateBamToBinSample::DoWork()
{
	Ice::Long totalValueCnt = m_foi->DomainSize;
	::IceUtil::ScopedArray<Ice::Double>  binValues(new ::Ice::Double[totalValueCnt]);
	if (!binValues.get()){
		cout << "CCreateBamToBinSample mem failed, " << m_sample.SampleName << endl;

		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_taskID, 0, iBS::AMDTaskStatusFailure);
		return;
	}

	std::fill(binValues.get(), binValues.get() + totalValueCnt, 0);

	CBamToBinCountCreator bc(m_sample, m_binMap);
	bc.ReadBinValues(binValues.get());

	//all in RAM
	CObserverStatsBasicJob job;
	job.m_totalValueCnt = totalValueCnt;
	job.m_processedValueCnt = 0;
	job.m_handleNaN = false;

	job.m_colCnt = m_foi->ObserverGroupSize;
	job.m_colIdx = m_foi->IdxInObserverGroup;
	
	//already handled gapped data
	CObserverStatsHelper::BasicJobProcessNextBatch(job, binValues.get(), job.m_totalValueCnt);

	::iBS::ObserverStatsInfo osi;
	osi.ObserverID = m_foi->ObserverID;
	osi.UpdateDT = IceUtil::Time::now().toMilliSeconds();
	osi.Version = 0;
	CObserverStatsHelper::BasicJobToStats(job, osi);
	CGlobalVars::get()->theObserverStatsDB->SetObserverStatsInfo(m_foi->ObserverID, osi);


	//save binValues to store
	std::pair<const Ice::Double*, const Ice::Double*> values(
		binValues.get(), binValues.get() + totalValueCnt);

	CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
		m_foi, 0, totalValueCnt, values);

	CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_taskID, 1);
}

void
CCreateBamToBinSample::CancelWork()
{

}
