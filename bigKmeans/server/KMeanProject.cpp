#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/Time.h>
#include <GlobalVars.h>
#include <algorithm>    // std::copy
#include <KMeanProject.h>
#include <KMeanProjectMgr.h>
#include <math.h>
#include <bdtUtil/RowAdjustHelper.h>
#include <limits>       // std::numeric_limits
#include <ctime>
#include <iomanip>		// std::setfill, std::setw
CKMeanProjectL2::CKMeanProjectL2(const KMeanProjectInfoPtr& projectPtr)
:m_projectPtr(projectPtr),
m_rand01(boost::random::mt19937(std::time(0)))
{
	m_seedsPoolSize = 0;
	m_amdTaskID = 0;
	m_batchTaskIdx = 0;
	m_KDistortion = 0;
	m_KDistortion_k1 = 0;
	m_maxIteration = m_projectPtr->MaxIteration;
	InitForNextTask(m_projectPtr->Task, m_projectPtr->K);
}

CKMeanProjectL2::~CKMeanProjectL2()
{
	cout<<IceUtil::Time::now().toDateTime()<< " ~CKMeanProjectL2, projectID="<<m_projectPtr->ProjectID<<endl; 
}

void CKMeanProjectL2::RequestToBeContractor(const RequestToBeContractorPtr& itemPtr)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	cout<<IceUtil::Time::now().toDateTime()<< " RequestToBeContractor, ContractorName="<<itemPtr->m_contractorName<<endl; 

	m_requestToBeContractorItems.push_back(itemPtr);
}

::Ice::Int
CKMeanProjectL2::LaunchProjectWithCurrentContractors(::Ice::Int projectID, Ice::Long amdTaskID)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	m_amdTaskID = amdTaskID;
	if(m_projectPtr->ProjectStatus
		!=iBS::KMeanProjectStatusWaitingForInitialContractors)
	{
		return 0;
	}

	int contractorCnt =(int) m_requestToBeContractorItems.size();
	if(contractorCnt==0)
	{
		cout<<IceUtil::Time::now().toDateTime()<< " LaunchProjectWithCurrentContractors, Rqsted ContractorCnt="<<0<<endl; 
		return 0;
	}
	
	cout<<IceUtil::Time::now().toDateTime()<< " LaunchProjectWithCurrentContractors, ContractorCnt="<<contractorCnt<<endl; 
	m_projectPtr->ContractorCnt = contractorCnt;

	//determine process row counts for each contractor, assume mem are always enough

	double threadSum=0;
	for (RequestToBeContractorPtrLsit_T::const_iterator it = m_requestToBeContractorItems.begin(); 
		it !=m_requestToBeContractorItems.end(); it++)
	{
		threadSum+= (*it)->m_workerCnt;
	}

	LongVec rowCntInContractor;
	Ice::Long totalRowCnt= m_projectPtr->TotalRowCnt;
	Ice::Long remainingRowCnt=totalRowCnt;
	int i=0;
	for (RequestToBeContractorPtrLsit_T::const_iterator it = m_requestToBeContractorItems.begin(); 
		it !=m_requestToBeContractorItems.end(); it++)
	{
		i++;
		double rowCnt=((double)(*it)->m_workerCnt)* totalRowCnt/threadSum;

		Ice::Long cnt=static_cast<int>(rowCnt);

		if(remainingRowCnt<cnt || i==contractorCnt)
		{
			cnt = remainingRowCnt;
		}

		rowCntInContractor.push_back(cnt);
		remainingRowCnt-=cnt;
	}

	Ice::Long assignedRowCnt=0;
	for(size_t i=0;i<rowCntInContractor.size();i++)
	{
		RequestToBeContractorPtr wi = m_requestToBeContractorItems.front();
		iBS::KMeanContractInfoPtr contractInfoPtr= new iBS::KMeanContractInfo();
		Ice::Long rowCnt=rowCntInContractor[i];
		contractInfoPtr->FcdcReader = m_projectPtr->FcdcReader;
		contractInfoPtr->ObserverIDs=m_projectPtr->ObserverIDs;
		contractInfoPtr->FeatureIdxFrom=m_projectPtr->FeatureIdxFrom+assignedRowCnt;
		contractInfoPtr->FeatureIdxTo=contractInfoPtr->FeatureIdxFrom+rowCnt;
		contractInfoPtr->K=m_projectPtr->K;
		contractInfoPtr->Distance = m_projectPtr->Distance;
		contractInfoPtr->AcceptedAsContractor = true;
		contractInfoPtr->ProjectID = m_projectPtr->ProjectID;
		contractInfoPtr->ProjectName = m_projectPtr->ProjectName;
		contractInfoPtr->Task = m_projectPtr->Task;
		contractInfoPtr->Seeding = m_projectPtr->Seeding;
		contractInfoPtr->ContractorName = wi->m_contractorName;
		contractInfoPtr->ContractorIdx = (Ice::Int)i;
		contractInfoPtr->ContractorCnt = contractorCnt;
		contractInfoPtr->ContractorLastFinishedStage  =0;

		m_contractors.push_back(contractInfoPtr);
		wi->m_cb->ice_response(1,contractInfoPtr);
		m_requestToBeContractorItems.pop_front();

		cout<<IceUtil::Time::now().toDateTime()<< " Contractor"<<i<<"="<<contractInfoPtr->ContractorName
			<<" Assigned RowCnt="<<rowCnt<<endl; 
		assignedRowCnt+=rowCnt;
	}

	m_projectPtr->ProjectStatus = iBS::KMeanProjectStatusRunning;
	m_it=0;

	Ice::Long subTotalCnt = m_projectPtr->MaxIteration;
	if (m_projectPtr->Task == iBS::KMeansTaskPPSeeds)
	{
		subTotalCnt = m_projectPtr->K;
	}

	CGlobalVars::get()->theAMDTaskMgr.InitAMDSubTask(m_amdTaskID, GetTaskName(), subTotalCnt);

	return 1;

}

void CKMeanProjectL2::ReportKClusters(const ::iBS::AMD_KMeanServerService_GetKClustersPtr& cb)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	Ice::Long colCnt = m_projectPtr->ObserverCnt;
	Ice::Long rowCnt = m_projectPtr->K;
	Ice::Double *KClusters = m_KSums.get();
	std::pair<const Ice::Double*, const Ice::Double*> retValues(
		KClusters, KClusters + (colCnt*rowCnt));
	cb->ice_response(1, retValues);
}

void CKMeanProjectL2::GetKSeeds(const ::iBS::AMD_KMeanServerService_GetKSeedsPtr& cb)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	Ice::Long colCnt = m_projectPtr->ObserverCnt;
	Ice::Long rowCnt = m_seedsPoolSize;
	Ice::Long valueCnt = rowCnt*colCnt;
	Ice::Double *KSeeds = m_seedsPool.get();
	std::pair<const Ice::Double*, const Ice::Double*> retValues(
		KSeeds, KSeeds + (colCnt*rowCnt));
	cb->ice_response(1, retValues);
}

void CKMeanProjectL2::ReportKCntsAndSums(const ReportKCntsAndSumsPtr& itemPtr)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	cout<<IceUtil::Time::now().toDateTime()<< " ReportKCntsAndSums, ContractorIdx="<<itemPtr->m_contractorIdx<<endl; 

	Ice::Long colCnt=m_projectPtr->ObserverCnt;
	Ice::Long rowCnt=m_projectPtr->K;
	if(itemPtr->m_KCnts.first==itemPtr->m_KCnts.second)
	{
		//empty data meaning to get updated clusters only
		Ice::Double *KClusters=m_KSums.get();
		std::pair<const Ice::Double*, const Ice::Double*> retValues(
			 KClusters, KClusters+(colCnt*rowCnt));

		itemPtr->m_cb->ice_response(1,retValues);
		return ;
	}

	
	//process KCnts and KSums using callers' threads
	if(m_reportKCntsAndSumsItems.empty())
	{
		//first contractor, replace KCnts and KSums
		std::copy(itemPtr->m_KCnts.first, itemPtr->m_KCnts.second, m_KCnts.get());
		std::copy(itemPtr->m_KSums.first, itemPtr->m_KSums.second, m_KSums.get());
		m_KChangeCnt = itemPtr->m_KChangeCnt;
		m_KDistortion = itemPtr->m_distortion;
	}
	else
	{
		m_KChangeCnt += itemPtr->m_KChangeCnt;
		m_KDistortion += itemPtr->m_distortion;
		for(int i=0;i<rowCnt;i++)
		{
			m_KCnts[i]+=itemPtr->m_KCnts.first[i];
			for(int j=0;j<colCnt;j++)
			{
				m_KSums[i*colCnt+j]+=itemPtr->m_KSums.first[i*colCnt+j];
			}
		}
	}
	//KCnts and KSums will become dangling pointers, should not use them later
	itemPtr->m_KCnts.first=itemPtr->m_KCnts.second=0;
	itemPtr->m_KSums.first=itemPtr->m_KSums.second=0;
	
	m_reportKCntsAndSumsItems.push_back(itemPtr);
	
	size_t contractorCnt = m_projectPtr->ContractorCnt;
	if(m_reportKCntsAndSumsItems.size()==contractorCnt)
	{
		m_it++;
		cout<<IceUtil::Time::now().toDateTime()<< " Iteration "<<m_it<< " all contractors return back, join begins..."<<endl; 

		//all contractors are back
		Ice::Double *KClusters=m_KSums.get();

		for(int i=0;i<rowCnt;i++)
		{
			for(int j=0;j<colCnt;j++)
			{
				if(m_KCnts[i]>0)
				{
					KClusters[i*colCnt+j]=m_KSums[i*colCnt+j]/m_KCnts[i];
				}
				else
				{
					//empty cluster
					KClusters[i*colCnt+j]=0.0;
				}
			}
		}

		if (m_projectPtr->Distance == iBS::KMeansDistCorrelation)
		{
			//normalizing those points to zero mean, unit standard deviation
			CRowAdjustHelper::Adjust(KClusters, rowCnt, colCnt, iBS::RowAdjustZeroMeanUnitLength);
		}

		std::pair<const Ice::Double*, const Ice::Double*> retValues(
			 KClusters, KClusters+(colCnt*rowCnt));

		iBS::KMeansResult& kret = m_kmeansResults[m_kmeansResults.size() - 1];
		kret.ChangedCnts.push_back(m_KChangeCnt);
		kret.Distorsions.push_back(m_KDistortion);

		Ice::Double explainedChanged = 1.0;
		if (m_KDistortion_k1 > 0)
		{
			Ice::Double explained = 1.0 - m_KDistortion / m_KDistortion_k1;
			explainedChanged = explained - m_lastExplained;
			m_lastExplained = explained;
			if (explainedChanged < m_projectPtr->MinExplainedChanged)
			{
				m_minExplainedReachedCnt++;
			}
			else
			{
				m_minExplainedReachedCnt = 0;
			}
			
			kret.Explaineds.push_back(explained);
		}
		else
		{
			kret.Explaineds.push_back(0);
		}

		

		bool needExit = false;
		if (m_it >= m_projectPtr->MaxIteration 
			|| m_KChangeCnt<m_projectPtr->MinChangeCnt
			|| m_minExplainedReachedCnt>=3)
		{
			kret.WallTimeSeconds = IceUtil::Time::now().toSecondsDouble() - kret.WallTimeSeconds;
			retValues.first=0;
			retValues.second=0;
			if (m_projectPtr->K == 1)
			{
				m_KDistortion_k1 = m_KDistortion;
			}

			cout<<IceUtil::Time::now().toDateTime()<< " termination reached!"<<endl;
			ostringstream os;
			os << GetTaskName()<<" (changed " << m_KChangeCnt;
			if (m_KDistortion_k1 > 0)
			{
				Ice::Double explained = 1.0 - m_KDistortion / m_KDistortion_k1;
				os.precision(4);
				os<< ", explained " << explained;
			}
			os << ")";

			CGlobalVars::get()->theAMDTaskMgr.ChangeTaskName(m_amdTaskID, os.str());

			//to see if there are task pending
			SwitchToNextTask();
			if (m_projectPtr->Task == iBS::KMeansTaskNone)
			{
				CGlobalVars::get()->theAMDTaskMgr.SetAMDTaskDone(m_amdTaskID);
			}

			needExit = true;
		}
		while(!m_reportKCntsAndSumsItems.empty())
		{
			ReportKCntsAndSumsPtr ci = m_reportKCntsAndSumsItems.front();
			ci->m_cb->ice_response(1,retValues);
			m_reportKCntsAndSumsItems.pop_front();
		}
		if (!needExit)
		{
			ostringstream os;
			os << GetTaskName() << " (changed " << m_KChangeCnt;
			if (m_KDistortion_k1 > 0)
			{
				Ice::Double explained = 1.0 - m_KDistortion / m_KDistortion_k1;
				os.precision(4);
				os << ", explained " << explained;
			}
			os << ")";
			CGlobalVars::get()->theAMDTaskMgr.ChangeTaskName(m_amdTaskID, os.str());
			CGlobalVars::get()->theAMDTaskMgr.UpdateAMDTaskProgress(m_amdTaskID, 1);
		}
		
		cout<<IceUtil::Time::now().toDateTime()<< " Iteration "<<m_it<< " all contractors  join ends"<<endl; 
	}
}


void CKMeanProjectL2::ReportKMembers(const ReportKMembersPtr& itemPtr)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	Ice::Long localFeatureIdxFrom=itemPtr->m_featureIdxFrom-m_projectPtr->FeatureIdxFrom;
	std::copy(itemPtr->m_featureIdx2ClusterIdx.first,itemPtr->m_featureIdx2ClusterIdx.second,
		m_featureIdx2ClusterID.get()+localFeatureIdxFrom);

	itemPtr->m_cb->ice_response(1);
}

void
CKMeanProjectL2::GetKMembers(
	const ::iBS::AMD_KMeanServerService_GetKMembersPtr& cb,
    ::Ice::Long featureIdxFrom,
    ::Ice::Long featureIdxTo)
{
    IceUtil::Mutex::Lock lock(m_mutex);
	Ice::Long localFeatureIdxFrom=featureIdxFrom-m_projectPtr->FeatureIdxFrom;
	Ice::Long localFeatureIdxTo=localFeatureIdxFrom+(featureIdxTo-featureIdxFrom);

	std::pair<const Ice::Long*, const Ice::Long*> retValues(
			 m_featureIdx2ClusterID.get()+localFeatureIdxFrom, m_featureIdx2ClusterID.get()+localFeatureIdxTo);

	cb->ice_response(1,retValues);
}

void
CKMeanProjectL2::GetKCnts(const ::iBS::AMD_KMeanServerService_GetKCntsPtr& cb)
{
    IceUtil::Mutex::Lock lock(m_mutex);
	std::pair<const Ice::Double*, const Ice::Double*> retValues(
		m_KCnts.get(), m_KCnts.get()+m_projectPtr->K);

	cb->ice_response(1,retValues);
}



///////////////////////////////////////////////////////////////////////////////////////
void CKMeanProjectL2::ReportPPDistSum(const ReportPPDistSumPtr& itemPtr)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	cout << IceUtil::Time::now().toDateTime() << " ReportPPDistSum, ContractorIdx=" << itemPtr->m_contractorIdx << endl;

	if (itemPtr->m_distSum != itemPtr->m_distSum)
	{
		//empty data meaning to get the first cluster center
		Ice::Double selectSum = std::numeric_limits<Ice::Double>::quiet_NaN();

		itemPtr->m_cb->ice_response(1, selectSum);
		return;
	}

	//process sum of distance using callers' threads
	if (m_reportPPDistSumItems.empty())
	{
		m_distSum = itemPtr->m_distSum;
	}
	else
	{
		m_distSum += itemPtr->m_distSum;
	}

	m_reportPPDistSumItems.push_back(itemPtr);

	size_t contractorCnt = m_projectPtr->ContractorCnt;
	if (m_reportPPDistSumItems.size() == contractorCnt)
	{
		//all contractors are back
		Ice::Double selectSum = m_distSum*m_rand01();
		Ice::Double selectSum_0 = selectSum;
		Ice::Double leftDistSum = selectSum;
		iBS::DoubleVec contractorDistSums(contractorCnt,0);
		int w = 0;
		for (ReportPPDistSumPtrList_T::iterator it = m_reportPPDistSumItems.begin(); 
			it != m_reportPPDistSumItems.end(); ++it)
		{
			contractorDistSums[w] = (*it)->m_distSum;
			w++;
		}
		
		for (w = 0; w<contractorCnt - 1; w++)
		{
			selectSum -= contractorDistSums[w];
			if (selectSum <= 0)
				break;
			leftDistSum -= contractorDistSums[w];
		}
		int selected = w;
		w = 0;
		while (!m_reportPPDistSumItems.empty())
		{
			ReportPPDistSumPtr ci = m_reportPPDistSumItems.front();
			if (w == selected)
			{
				cout << IceUtil::Time::now().toDateTime() << " ReportPPDistSum " << m_it 
					<< " selected contractor "<< ci->m_contractorIdx
					<< " totalSum " << m_distSum
					<< " selectSum " << selectSum_0
					<< " leftSum " << leftDistSum << endl;
				ci->m_cb->ice_response(1, leftDistSum);
			}
			else
			{
				ci->m_cb->ice_response(1, std::numeric_limits<Ice::Double>::quiet_NaN());
			}
			
			m_reportPPDistSumItems.pop_front();
			w++;
		}
	}
}

void CKMeanProjectL2::ReportNewSeed(const ReportNewSeedPtr& itemPtr)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	cout << IceUtil::Time::now().toDateTime() << " ReportNewSeed, ContractorIdx=" << itemPtr->m_contractorIdx << endl;

	Ice::Long colCnt = m_projectPtr->ObserverCnt;
	Ice::Long rowCnt = m_projectPtr->K;

	if (itemPtr->m_seedFeatureIdx >= 0)
	{
		//report new seed
		Ice::Long k = (Ice::Long)m_reportedSeedFeatureIdxs.size();
		m_reportedSeedFeatureIdxs.push_back(itemPtr->m_seedFeatureIdx);
		for (int j = 0; j<colCnt; j++)
		{
			m_KSums[k*colCnt + j] += itemPtr->m_center[j];
		}
		cout << IceUtil::Time::now().toDateTime() << " update seed, ContractorIdx=" << itemPtr->m_contractorIdx << endl;
	}

	m_reportNewSeedItems.push_back(itemPtr);

	size_t contractorCnt = m_projectPtr->ContractorCnt;
	if (m_reportNewSeedItems.size() == contractorCnt)
	{
		m_it++;
		cout << IceUtil::Time::now().toDateTime() << " ReportNewSeed Iteration " << m_it << " all contractors return back, join begins..." << endl;

		//all contractors are back
		Ice::Double *KClusters = m_KSums.get();
		std::pair<const Ice::Double*, const Ice::Double*> retValues(
			KClusters, KClusters + (colCnt*rowCnt));

		bool needExit = false;
		if (m_it >= m_projectPtr->MaxIteration)
		{
			SaveCurrentKClustersAsSeedsPool(m_projectPtr->K, KClusters);

			retValues.first = 0;
			retValues.second = 0;
			cout << IceUtil::Time::now().toDateTime() << " max iteration reached!" << endl;
			ostringstream os;
			os << GetTaskName()<<", "<<m_reportedSeedFeatureIdxs.size() << " seeds reported";
			CGlobalVars::get()->theAMDTaskMgr.ChangeTaskName(m_amdTaskID, os.str());
			
			//to see if there are task pending
			SwitchToNextTask();
			if (m_projectPtr->Task == iBS::KMeansTaskNone)
			{
				CGlobalVars::get()->theAMDTaskMgr.SetAMDTaskDone(m_amdTaskID);
			}
			
			needExit = true;
		}
		while (!m_reportNewSeedItems.empty())
		{
			ReportNewSeedPtr ci = m_reportNewSeedItems.front();
			ci->m_cb->ice_response(1, retValues);
			m_reportNewSeedItems.pop_front();
		}
		if (!needExit)
		{
			ostringstream os;
			os << "newly reported seed featureidx " << m_reportedSeedFeatureIdxs[m_reportedSeedFeatureIdxs.size()-1];
			CGlobalVars::get()->theAMDTaskMgr.ChangeTaskName(m_amdTaskID, os.str());
			CGlobalVars::get()->theAMDTaskMgr.UpdateAMDTaskProgress(m_amdTaskID, 1);
		}

		cout << IceUtil::Time::now().toDateTime() << " Iteration " << m_it << " all contractors  join ends" << endl;
	}
}

void
CKMeanProjectL2::GetPPSeedFeatureIdxs(const ::iBS::AMD_KMeanServerService_GetPPSeedFeatureIdxsPtr& cb)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	cb->ice_response(1,m_reportedSeedFeatureIdxs);
}

void
CKMeanProjectL2::GetKMeansResults(const ::iBS::AMD_KMeanServerService_GetKMeansResultsPtr& cb)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	cb->ice_response(1, m_kmeansResults);
}


void CKMeanProjectL2::SaveCurrentKClustersAsSeedsPool(
	Ice::Long K, Ice::Double *pKClusters)
{
	Ice::Long colCnt = m_projectPtr->ObserverCnt;
	Ice::Long rowCnt = K;
	m_seedsPoolSize = K;
	Ice::Long valueCnt = rowCnt*colCnt;
	m_seedsPool.reset(new ::Ice::Double[valueCnt]);
	std::copy(pKClusters, pKClusters + valueCnt, m_seedsPool.get());
	if (m_projectPtr->Distance == iBS::KMeansDistCorrelation)
	{
		//normalizing those points to zero mean, unit standard deviation
		CRowAdjustHelper::Adjust(m_seedsPool.get(), rowCnt, colCnt, iBS::RowAdjustZeroMeanUnitLength);
	}
}

std::string CKMeanProjectL2::GetTaskName()
{
	ostringstream os;
	if (m_projectPtr->Task == iBS::KMeansTaskPPSeeds)
	{
		os << "KMeans++ seeding K "<<m_projectPtr->K;
	}
	else if (m_projectPtr->Task == iBS::KMeansTaskRunKMeans)
	{
		os << "KMeans K " << m_projectPtr->K;
	}
	else
	{
		os << "KMeans None" << endl;
	}
	return os.str();
}
void CKMeanProjectL2::SwitchToNextTask()
{
	if (m_batchTaskIdx < (Ice::Long)m_projectPtr->BatchTasks.size())
	{
		m_projectPtr->K = m_projectPtr->BatchKs[m_batchTaskIdx];
		m_projectPtr->Task = m_projectPtr->BatchTasks[m_batchTaskIdx];
		m_batchTaskIdx++;
		InitForNextTask(m_projectPtr->Task, m_projectPtr->K);
	}
	else
	{
		m_projectPtr->Task = iBS::KMeansTaskNone;
	}
	
}

void CKMeanProjectL2::GetNextTask(const GetNextTaskPtr& itemPtr)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	cout << IceUtil::Time::now().toDateTime() << " GetNextTask, ContractorIdx=" << itemPtr->m_contractorIdx << endl;

	m_getNextTaskItems.push_back(itemPtr);

	size_t contractorCnt = m_projectPtr->ContractorCnt;
	if (m_getNextTaskItems.size() == contractorCnt)
	{
		while (!m_getNextTaskItems.empty())
		{
			GetNextTaskPtr ci = m_getNextTaskItems.front();
			ci->m_cb->ice_response(1, m_projectPtr->Task, m_projectPtr->K);
			m_getNextTaskItems.pop_front();
		}
	}
}

void CKMeanProjectL2::InitForNextTask(::iBS::KMeansTaskEnum task, Ice::Long K)
{
	m_it = 0;
	m_KChangeCnt = 0;
	m_KDistortion = 0;
	m_lastExplained = 0;
	m_minExplainedReachedCnt = 0;
	m_projectPtr->K = K;
	m_projectPtr->Task = task;
	m_projectPtr->MaxIteration = m_maxIteration;

	Ice::Long colCnt = m_projectPtr->ObserverCnt;
	Ice::Long rowCnt = m_projectPtr->K;

	m_KCnts.reset(new ::Ice::Double[rowCnt]);
	m_KSums.reset(new ::Ice::Double[rowCnt*colCnt]);

	//determine row adjustment method by distance used
	iBS::RowAdjustEnum rowAdjust = iBS::RowAdjustNone;
	if (m_projectPtr->Distance == iBS::KMeansDistCorrelation)
	{
		rowAdjust = iBS::RowAdjustZeroMeanUnitLength;
	}

	if (m_projectPtr->Task == iBS::KMeansTaskPPSeeds)
	{
		m_projectPtr->MaxIteration = m_projectPtr->K;
		DoubleVec values;
		m_reportedSeedFeatureIdxs.clear();
		std::fill(m_KSums.get(), m_KSums.get() + (rowCnt*colCnt), 0);

		//select a random point as initial seed
		Ice::Long featureIdx = m_projectPtr->FeatureIdxFrom
			+ (Ice::Long)((m_projectPtr->FeatureIdxTo - m_projectPtr->FeatureIdxFrom)*m_rand01());

		m_reportedSeedFeatureIdxs.push_back(featureIdx);
		//m_projectPtr->FeatureIdxFrom
		m_projectPtr->FcdcReader->GetRowMatrix(
			m_projectPtr->ObserverIDs, featureIdx, featureIdx + 1, rowAdjust, values);
		std::copy(values.begin(), values.end(), m_KSums.get());
	}
	else if (m_projectPtr->Task == iBS::KMeansTaskRunKMeans && m_projectPtr->K<=m_seedsPoolSize)
	{
		std::copy(m_seedsPool.get(), m_seedsPool.get() + (rowCnt*colCnt), m_KSums.get());
	}
	else
	{
		DoubleVec values;
		//read initial KClusters
		Ice::Long featureIdxFrom = m_projectPtr->SeedsFeatureIdxFrom;
		Ice::Long featureIdxTo = featureIdxFrom + m_projectPtr->K;
		iBS::IntVec observerIDs(colCnt, 0);
		for (int i = 0; i < colCnt; i++)
		{
			observerIDs[i] = m_projectPtr->GIDForClusterSeeds + i;
		}
		m_projectPtr->FcdcForKMeans->GetRowMatrix(
			observerIDs, featureIdxFrom, featureIdxTo, rowAdjust, values);

		std::copy(values.begin(), values.end(), m_KSums.get());
		SaveCurrentKClustersAsSeedsPool(m_projectPtr->K, m_KSums.get());
	}

	if (m_projectPtr->Task == iBS::KMeansTaskRunKMeans)
	{
		if (m_featureIdx2ClusterID.get())
		{
			std::fill(m_featureIdx2ClusterID.get(), m_featureIdx2ClusterID.get() + m_projectPtr->TotalRowCnt, -1);
		}
		else
		{
			m_featureIdx2ClusterID.reset(new Ice::Long[m_projectPtr->TotalRowCnt]);
			std::fill(m_featureIdx2ClusterID.get(), m_featureIdx2ClusterID.get() + m_projectPtr->TotalRowCnt, -1);
		}

		iBS::KMeansResult result;
		result.ColCnt = m_projectPtr->ObserverCnt;
		result.RowCnt = m_projectPtr->TotalRowCnt;
		result.Distance = m_projectPtr->Distance;
		result.K = m_projectPtr->K;
		result.MinExplainedChanged = m_projectPtr->MinExplainedChanged;
		result.Seeding = m_projectPtr->Seeding;
		result.Task = m_projectPtr->Task;
		result.WallTimeSeconds = IceUtil::Time::now().toSecondsDouble();
		m_kmeansResults.push_back(result);
	}

	if (m_amdTaskID>0)
	{
		Ice::Long subTotalCnt = m_projectPtr->MaxIteration;
		CGlobalVars::get()->theAMDTaskMgr.InitAMDSubTask(m_amdTaskID, GetTaskName(), subTotalCnt);
	}
	
}