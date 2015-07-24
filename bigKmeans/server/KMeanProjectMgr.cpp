#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <KMeanProjectMgr.h>
#include <GlobalVars.h>

CKMeanProjectMgr::CKMeanProjectMgr(CGlobalVars& gv)
	:m_gv(gv)
{
	m_currProjectMaxID=0;
}


CKMeanProjectMgr::~CKMeanProjectMgr()
{
}

void CKMeanProjectMgr::Initilize()
{
	cout<<"CKMeanProjectMgr Initilize begin ..."<<endl; 
	
	
	cout<<"CKMeanProjectMgr Initilize End"<<endl;
}


void CKMeanProjectMgr::UnInitilize()
{
	cout<<"CKMeanProjectMgr UnInitilize begin ..."<<endl; 
	
	cout<<"CKMeanProjectMgr UnInitilize End"<<endl;
}

::Ice::Int CKMeanProjectMgr::CreateProjectAndWaitForContractors(
			const ::iBS::KMeanProjectInfoPtr& rqstProjectInfo,
            ::iBS::KMeanProjectInfoPtr& retProjectInfo)
{
	int ObserverCnt=(int)rqstProjectInfo->ObserverIDs.size();
	if(ObserverCnt==0)
	{
		return 0;
	}
	
	//lock
	{
		IceUtil::Mutex::Lock lock(m_mutex);
		retProjectInfo =new iBS::KMeanProjectInfo();
		retProjectInfo->ProjectID = ++m_currProjectMaxID;
		retProjectInfo->ProjectName = rqstProjectInfo->ProjectName;
		retProjectInfo->Task = rqstProjectInfo->Task;
		retProjectInfo->Seeding = rqstProjectInfo->Seeding;
		retProjectInfo->K = rqstProjectInfo->K;
		retProjectInfo->BatchTasks = rqstProjectInfo->BatchTasks;
		retProjectInfo->BatchKs = rqstProjectInfo->BatchKs;
		retProjectInfo->Distance = rqstProjectInfo->Distance;
		retProjectInfo->MaxIteration = rqstProjectInfo->MaxIteration;
		retProjectInfo->MinChangeCnt = rqstProjectInfo->MinChangeCnt;
		retProjectInfo->MinExplainedChanged = rqstProjectInfo->MinExplainedChanged;
		retProjectInfo->FcdcReader = rqstProjectInfo->FcdcReader;
		retProjectInfo->ObserverIDs = rqstProjectInfo->ObserverIDs;
		retProjectInfo->FeatureIdxFrom = rqstProjectInfo->FeatureIdxFrom;
		retProjectInfo->FeatureIdxTo = rqstProjectInfo->FeatureIdxTo;

		retProjectInfo->FcdcForKMeans = rqstProjectInfo->FcdcForKMeans;
		retProjectInfo->GIDForClusterSeeds = rqstProjectInfo->GIDForClusterSeeds;
		retProjectInfo->SeedsFeatureIdxFrom = rqstProjectInfo->SeedsFeatureIdxFrom;
		retProjectInfo->GIDForKClusters = rqstProjectInfo->GIDForKClusters;
		retProjectInfo->OIDForFeature2ClusterIdx = rqstProjectInfo->OIDForFeature2ClusterIdx;
		
		retProjectInfo->ObserverCnt = (Ice::Int) rqstProjectInfo->ObserverIDs.size();
		retProjectInfo->TotalRowCnt= rqstProjectInfo->FeatureIdxTo-rqstProjectInfo->FeatureIdxFrom;
		retProjectInfo->ProjectStatus = iBS::KMeanProjectStatusWaitingForInitialContractors;
		retProjectInfo->ContractorCnt = 0;

		retProjectInfo->ExpectedContractorCnt = rqstProjectInfo->ExpectedContractorCnt;

		retProjectInfo->WaitForContractorsTaskId =
			m_gv.theAMDTaskMgr.RegisterAMDTask("WaitForContractors",
			retProjectInfo->ExpectedContractorCnt);

		string taskName = "Run KMeans";
		if (retProjectInfo->Task == iBS::KMeansTaskPPSeeds)
		{
			taskName = " KMeans++ Seeds";
		}

		retProjectInfo->RunKmeansTaskId =
			m_gv.theAMDTaskMgr.RegisterAMDTask(
			taskName, 0);

		CKMeanProjectL2Ptr kmeanl2Ptr = new CKMeanProjectL2(retProjectInfo);
		m_l2Projects.insert(std::pair<int,CKMeanProjectL2Ptr>(retProjectInfo->ProjectID,kmeanl2Ptr));

	}

	return 1;
}


CKMeanProjectL2Ptr CKMeanProjectMgr::GetKMeanProjectL2ByProjectID(int projectID)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	CKMeanProjectL2Ptr kmeanl2Ptr;
	KMeanProjectL2PtrMap_T::iterator it = m_l2Projects.find(projectID);
	if(it!=m_l2Projects.end())
	{
		kmeanl2Ptr=it->second;
	}
	return kmeanl2Ptr;
}

CKMeanProjectL2Ptr CKMeanProjectMgr::GetActiveProject()
{
	IceUtil::Mutex::Lock lock(m_mutex);
	CKMeanProjectL2Ptr kmeanl2Ptr;
	KMeanProjectL2PtrMap_T::iterator it = m_l2Projects.find(this->m_currProjectMaxID);
	if (it != m_l2Projects.end())
	{
		kmeanl2Ptr = it->second;
	}
	return kmeanl2Ptr;
}


::Ice::Int CKMeanProjectMgr::DestroyProject(int projectID)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	CKMeanProjectL2Ptr kmeanl2Ptr;
	KMeanProjectL2PtrMap_T::iterator it = m_l2Projects.find(projectID);
	if(it!=m_l2Projects.end())
	{
		m_l2Projects.erase(it);
		return 1;
	}
	return 0;
	
}

