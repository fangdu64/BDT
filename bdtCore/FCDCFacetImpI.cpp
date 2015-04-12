#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/ScopedArray.h>
#include <GlobalVars.h>
#include <FCDCFacetImpI.h>
#include <FeatureDomainDB.h>
#include <FeatureObserverDB.h>
#include <FeatureValueRAM.h>
#include <FeatureValueWorker.h>
#include <FCDCFacetWorkItem.h>
#include <FeatureValueWorkerMgr.h>
#include <ObserverStatsDB.h>


void
CFCDCFacetDivideByColumnSumImpl::GetDoublesColumnVector_async(const ::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr& cb,
                                                        ::Ice::Int observerID,
                                                        ::Ice::Long featureIdxFrom,
                                                        ::Ice::Long featureIdxTo,
                                                        const Ice::Current& current) const
{
    iBS::FeatureObserverSimpleInfoPtr foi;
	bool rqstValid=rvGetDoublesColumnVector(cb, observerID, featureIdxFrom, featureIdxTo, foi);
	if(!rqstValid)
	{
		//exception or response already made
		return;
	}

	//make sure observer stats is ready
	::iBS::ObserverStatsInfo stats;
	int rt= m_gv.theObserverStatsDB->GetObserverStatsInfo(observerID, stats);
	if(rt!=1)
	{
		::iBS::ArgumentException ex;
		ex.reason = "observer's stats not ready";
		cb->ice_exception(ex);
		return;
	}

	if(stats.Sum==0)
	{
		::iBS::ArgumentException ex;
		ex.reason = "observer's column sum = 0";
		cb->ice_exception(ex);
		return;
	}
   

	//valid observer id

	if(foi->GetPolicy==iBS::FeatureValueGetPolicyGetFromRAM
		||foi->GetPolicy==iBS::FeatureValueGetPolicyGetForOneTimeRead
		||foi->GetPolicy==iBS::FeatureValueGetPolicyAuto)
	{
		FeatureValueWorkItemPtr wi= new DivideByColumnSum::CGetDoublesColumnVector(
			foi,cb,observerID,featureIdxFrom,featureIdxTo,stats.Sum);
		m_gv.theFeatureValueWorkerMgr->AssignItemToWorker(foi,wi);
		return;
	}
	else
	{
		::iBS::ArgumentException ex;
		ex.reason = "GetPolicy N/A";
		cb->ice_exception(ex);
		return;
	}

}


void
CFCDCFacetDivideByColumnSumImpl::GetDoublesRowMatrix_async(const ::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr& cb,
                                                     ::Ice::Int observerGroupID,
                                                     ::Ice::Long featureIdxFrom,
                                                     ::Ice::Long featureIdxTo,
                                                     const Ice::Current& current) const
{
	
	iBS::FeatureObserverSimpleInfoPtr foi;
	bool rqstValid=rvGetDoublesRowMatrix(cb, observerGroupID, featureIdxFrom, featureIdxTo, foi);
	if(!rqstValid)
	{
		//exception or response already made
		return;
	}

	iBS::IntVec observerIDs;
	for(int i=0;i<foi->ObserverGroupSize;i++)
	{
		observerIDs.push_back(foi->ObserverGroupID+i);
	}

	//make sure observer stats is ready
	::iBS::ObserverStatsInfoVec observerStats;
	int rt= m_gv.theObserverStatsDB->GetObserversStats(observerIDs, observerStats);
	if(rt!=1)
	{
		 ostringstream os;
		 os<<" stats not ready, observer ID="<<observerIDs[observerStats.size()];
		::iBS::ArgumentException ex;
		ex.reason = os.str();
		cb->ice_exception(ex);
		return;
	}

	iBS::DoubleVec columnSums;
	for(size_t i=0;i<observerIDs.size();i++)
	{
		if(observerStats[i].Sum==0)
		{
			ostringstream os;
			os<<"observer's column sum = 0, obsrverID="<<observerIDs[i];
			::iBS::ArgumentException ex;
			ex.reason =  os.str();
			cb->ice_exception(ex);
			return;
		}
		columnSums.push_back(observerStats[i].Sum);
	}

	//valid observer id
	if(foi->GetPolicy==iBS::FeatureValueGetPolicyGetFromRAM
		||foi->GetPolicy==iBS::FeatureValueGetPolicyGetForOneTimeRead)
	{
		FeatureValueWorkItemPtr wi= new DivideByColumnSum::CGetDoublesRowMatrix(
			foi,cb,observerGroupID, featureIdxFrom, featureIdxTo, columnSums);
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
