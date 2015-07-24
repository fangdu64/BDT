#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/ScopedArray.h>
#include <GlobalVars.h>
#include <KMeanServerImpl.h>
#include <KMeanProjectMgr.h>


::iBS::KMeanServerServicePrx
CKMeanServerAdminServiceImpl::GetKMeansSeverProxy(const Ice::Current& current)
{
	Ice::Identity id;
	id.name = "KMeanServerService";

	return iBS::KMeanServerServicePrx::uncheckedCast(
		current.adapter->createProxy(id));
}

::Ice::Int
CKMeanServerAdminServiceImpl::GetBlankProject(::iBS::KMeanProjectInfoPtr& retProjectInfo,
                                               const Ice::Current& current)
{
	retProjectInfo =new iBS::KMeanProjectInfo();
	retProjectInfo->ProjectID =0;
	retProjectInfo->ProjectName ="untitled";
	retProjectInfo->Task = iBS::KMeansTaskRunKMeans;
	retProjectInfo->Seeding = iBS::KMeansSeedingKMeansPlusPlus;
	retProjectInfo->FeatureIdxFrom = 0;
	retProjectInfo->FeatureIdxTo = 0;
	retProjectInfo->GIDForClusterSeeds = 0;
	retProjectInfo->SeedsFeatureIdxFrom = 0;
	retProjectInfo->GIDForKClusters =0;
	retProjectInfo->OIDForFeature2ClusterIdx = 0;
	retProjectInfo->K =0;
	retProjectInfo->Distance = iBS::KMeansDistEuclidean;
	retProjectInfo->MaxIteration =100;
	retProjectInfo->MinChangeCnt = 1;
	retProjectInfo->MinExplainedChanged = 0.0;

	retProjectInfo->ObserverCnt = 0;
	retProjectInfo->TotalRowCnt= 0;
	retProjectInfo->ProjectStatus = iBS::KMeanProjectStatusUnknown;
	retProjectInfo->ContractorCnt = 0;
	retProjectInfo->ExpectedContractorCnt = 1;
	retProjectInfo->WaitForContractorsTaskId = 0;
	retProjectInfo->RunKmeansTaskId = 0;
    return 1;
}

::Ice::Int
CKMeanServerAdminServiceImpl::CreateProjectAndWaitForContractors(
		const ::iBS::KMeanProjectInfoPtr& rqstProjectInfo,
		::iBS::KMeanProjectInfoPtr& retProjectInfo,
		const Ice::Current& current)
{
    return m_gv.theKMeanMgr->CreateProjectAndWaitForContractors(
		rqstProjectInfo, retProjectInfo);
}

::Ice::Int
CKMeanServerAdminServiceImpl::LaunchProjectWithCurrentContractors(
		::Ice::Int projectID,
		::Ice::Long& taskID,
		const Ice::Current& current)
{
    CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	taskID = 0;
	if(kmeanPtr)
	{
		string taskName = "Run KMeans";
		if (kmeanPtr->GetProjectInfo()->Task == iBS::KMeansTaskPPSeeds)
		{
			taskName= " KMeans++ Seeds";
		}

		taskID = kmeanPtr->GetProjectInfo()->RunKmeansTaskId;

		return kmeanPtr->LaunchProjectWithCurrentContractors(projectID);
	}
	else
	{
		::iBS::ArgumentException ex;
		ex.reason = " invalid projectID";
		throw ex;
	}
    
}

::Ice::Int
CKMeanServerAdminServiceImpl::GetAMDTaskInfo(::Ice::Long taskID,
::iBS::AMDTaskInfo& task,
const Ice::Current& current)
{
	return m_gv.theAMDTaskMgr.GetAMDTask(taskID, task);
}

::Ice::Int
CKMeanServerAdminServiceImpl::DestroyProject(::Ice::Int projectID,
                                              const Ice::Current& current)
{
	::Ice::Int rt= m_gv.theKMeanMgr->DestroyProject(projectID);
	if(rt==1)
	{
		return 1;
	}
	else
	{
		::iBS::ArgumentException ex;
		ex.reason = " invalid projectID";
		throw ex;
	}
}

::Ice::Int
CKMeanServerAdminServiceImpl::GetActiveProject(::iBS::KMeanProjectInfoPtr& retProjectInfo,
const Ice::Current& current)
{
	CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetActiveProject();
	if (kmeanPtr)
	{
		retProjectInfo = kmeanPtr->GetProjectInfo();
		return 1;
	}
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////

::Ice::Int
CKMeanServerServiceImpl::ReportStatus(::Ice::Int projectID,
                                           ::Ice::Int contractorIdx,
                                           const ::std::string& contractorName,
                                           const ::std::string& strStatus,
                                           ::iBS::KMeanSvrMsgEnum& svrMsg,
                                           const Ice::Current& current)
{
    return 0;
}

void
CKMeanServerServiceImpl::RequestToBeContractor_async(
		const ::iBS::AMD_KMeanServerService_RequestToBeContractorPtr& cb,
        ::Ice::Int projectID,
        const ::std::string& contractorName,
        ::Ice::Int workerCnt,
        ::Ice::Int ramSize,
        const Ice::Current& current)
{
    CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if(!kmeanPtr)
	{
		iBS::KMeanContractInfoPtr contractInfoPtr= new iBS::KMeanContractInfo();
		contractInfoPtr->AcceptedAsContractor=false;
		cb->ice_response(0,contractInfoPtr);
	}
	else
	{
		RequestToBeContractorPtr item = new CRequestToBeContractor(
			cb,projectID,contractorName,workerCnt,ramSize);

		kmeanPtr->RequestToBeContractor(item);

	}
}

void
CKMeanServerServiceImpl::ReportKCntsAndSums_async(
		const ::iBS::AMD_KMeanServerService_ReportKCntsAndSumsPtr& cb,
        ::Ice::Int projectID,
        ::Ice::Int contractorIdx,
        const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& KCnts,
        const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& KSums,
		::Ice::Long KChangeCnt,
		::Ice::Double distortion,
        const Ice::Current& current)
{
    CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if(!kmeanPtr)
	{
		cb->ice_exception();
	}
	else
	{
		ReportKCntsAndSumsPtr item = new CReportKCntsAndSums(
			cb, projectID, contractorIdx, KCnts, KSums, KChangeCnt, distortion);

		kmeanPtr->ReportKCntsAndSums(item);
	}
}

void
CKMeanServerServiceImpl::GetKClusters_async(const ::iBS::AMD_KMeanServerService_GetKClustersPtr& cb,
::Ice::Int projectID,
const Ice::Current& current)
{
	CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if (!kmeanPtr)
	{
		cb->ice_exception();
	}
	else
	{
		kmeanPtr->ReportKClusters(cb);
	}
}

void
CKMeanServerServiceImpl::ReportKMembers_async(const ::iBS::AMD_KMeanServerService_ReportKMembersPtr& cb,
                                                     ::Ice::Int projectID,
                                                     ::Ice::Int contractorIdx,
                                                     ::Ice::Long featureIdxFrom,
                                                     ::Ice::Long featureIdxTo,
                                                     const ::std::pair<const ::Ice::Long*, const ::Ice::Long*>& featureIdx2ClusterIdx,
                                                     const Ice::Current& current)
{
     CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if(!kmeanPtr)
	{
		cb->ice_exception();
	}
	else
	{
		ReportKMembersPtr item = new CReportKMembers(
			cb,projectID,contractorIdx,
			featureIdxFrom,featureIdxTo,
			featureIdx2ClusterIdx);

		kmeanPtr->ReportKMembers(item);
	}
}

void
CKMeanServerServiceImpl::GetKMembers_async(const ::iBS::AMD_KMeanServerService_GetKMembersPtr& cb,
    ::Ice::Int projectID,
    ::Ice::Long featureIdxFrom,
    ::Ice::Long featureIdxTo,
    const Ice::Current& current)
{
    CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if(!kmeanPtr)
	{
		cb->ice_exception();
	}
	else
	{
		
		kmeanPtr->GetKMembers(cb,featureIdxFrom,featureIdxTo);
	}
}

void
CKMeanServerServiceImpl::GetKCnts_async(const ::iBS::AMD_KMeanServerService_GetKCntsPtr& cb,
    ::Ice::Int projectID,
    const Ice::Current& current)
{
     CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if(!kmeanPtr)
	{
		cb->ice_exception();
	}
	else
	{
		kmeanPtr->GetKCnts(cb);
	}
}

void
CKMeanServerServiceImpl::ReportPPDistSum_async(
const ::iBS::AMD_KMeanServerService_ReportPPDistSumPtr& cb,
::Ice::Int projectID,
::Ice::Int contractorIdx,
::Ice::Double distSum,
const Ice::Current& current)
{
	CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if (!kmeanPtr)
	{
		cb->ice_exception();
	}
	else
	{
		ReportPPDistSumPtr item = new CReportPPDistSum(
			cb, projectID, contractorIdx, distSum);

		kmeanPtr->ReportPPDistSum(item);
	}
}

void
CKMeanServerServiceImpl::ReportNewSeed_async(
const ::iBS::AMD_KMeanServerService_ReportNewSeedPtr& cb,
::Ice::Int projectID,
::Ice::Int contractorIdx,
::Ice::Long seedFeatureIdx,
const ::iBS::DoubleVec& center,
const Ice::Current& current)
{
	CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if (!kmeanPtr)
	{
		cb->ice_exception();
	}
	else
	{
		ReportNewSeedPtr item = new CReportNewSeed(
			cb, projectID, contractorIdx, seedFeatureIdx, center);

		kmeanPtr->ReportNewSeed(item);
	}
}

void
CKMeanServerServiceImpl::GetPPSeedFeatureIdxs_async(const ::iBS::AMD_KMeanServerService_GetPPSeedFeatureIdxsPtr& cb,
::Ice::Int projectID,
const Ice::Current& current)
{
	CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if (!kmeanPtr)
	{
		cb->ice_exception();
	}
	else
	{
		kmeanPtr->GetPPSeedFeatureIdxs(cb);
	}
}

void
CKMeanServerServiceImpl::GetNextTask_async(const ::iBS::AMD_KMeanServerService_GetNextTaskPtr& cb,
::Ice::Int projectID,
::Ice::Int contractorIdx,
const Ice::Current& current)
{
	CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if (!kmeanPtr)
	{
		cb->ice_exception();
	}
	else
	{
		GetNextTaskPtr item = new CGetNextTask(
			cb, projectID, contractorIdx);

		kmeanPtr->GetNextTask(item);
	}
}

void
CKMeanServerServiceImpl::GetKMeansResults_async(
const ::iBS::AMD_KMeanServerService_GetKMeansResultsPtr& cb,
::Ice::Int projectID,
const Ice::Current& current)
{
	CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if (!kmeanPtr)
	{
		cb->ice_exception();
	}
	else
	{
		kmeanPtr->GetKMeansResults(cb);
	}
}

void
CKMeanServerServiceImpl::GetKSeeds_async(const ::iBS::AMD_KMeanServerService_GetKSeedsPtr& cb,
::Ice::Int projectID,
const Ice::Current& current)
{
	CKMeanProjectL2Ptr kmeanPtr = m_gv.theKMeanMgr->GetKMeanProjectL2ByProjectID(projectID);
	if (!kmeanPtr)
	{
		cb->ice_exception();
	}
	else
	{
		kmeanPtr->GetKSeeds(cb);
	}
}
