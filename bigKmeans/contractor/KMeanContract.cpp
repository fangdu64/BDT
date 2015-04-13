#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/Time.h>
#include <math.h>
#include <algorithm>    // std::copy
#include <GlobalVars.h>
#include <KMeanContract.h>

#include <KMeanWorkItem.h>
#include <KMeanWorkerMgr.h>
#include <limits>       // std::numeric_limits

//========================================================
CKMeanContractL2::CKMeanContractL2(CGlobalVars& gv,int projectID)
	:m_gv(gv), m_projectID(projectID)
{
	m_it=0;
	m_bContinue=true;
	m_workerCnt=m_gv.theWorkerCnt;
	m_memSize=m_gv.theMemSize;
	m_shutdownRequested = false;
	m_remainWorkerCallBackCnt=0;
	m_needNotify=0;
}

CKMeanContractL2::~CKMeanContractL2()
{
	for(size_t i=0;i<m_workerKCnts.size();i++)
	{
		DoubleArrayPtr_T ptr=m_workerKCnts[i];
		if(ptr!=0)
		{
			delete[] ptr;
		}
	}

	for(size_t i=0;i<m_workerKSums.size();i++)
	{
		DoubleArrayPtr_T ptr=m_workerKSums[i];
		if(ptr!=0)
		{
			delete[] ptr;
		}
	}

	cout<<IceUtil::Time::now().toDateTime()<< " ~CKMeanContractL2, projectID="<<m_projectID<<endl; 
}

void CKMeanContractL2::Start()
{
	// will block until KMeanServer notifies all contractors to start work
	RequestToBeContractor();
	if (!m_bContinue)
		return;

	// get data points that this contractor needs
	GetObserverData();
	if (!m_bContinue)
		return;

	while (true)
	{
		cout << IceUtil::Time::now().toDateTime() << " new task " << m_contractPtr->Task
			<< " K " << m_contractPtr->K << endl;

		if (m_contractPtr->Task == iBS::KMeansTaskPPSeeds)
		{
			StartPPSeeds();
		}
		else
		{
			StartKMeans();
		}

		if (!m_bContinue)
			return;

		iBS::KMeansTaskEnum task;
		Ice::Long K = 0;
		m_gv.theKMeanPrx->GetNextTask(
			m_contractPtr->ProjectID,
			m_contractPtr->ContractorIdx,
			task, K);
		if (task == iBS::KMeansTaskNone){
			break;
		}else{
			m_contractPtr->Task = task;
			m_contractPtr->K = K;
		}
	}

	cout << IceUtil::Time::now().toDateTime() << " task done, projectID " << m_projectID << endl;

}

void CKMeanContractL2::StartKMeans()
{
	InitWorkersData();
	if (!m_bContinue)
		return;

	IterateReportKCntsAndSums();
	if (!m_bContinue)
		return;

	ReportKMembers();
}

void CKMeanContractL2::StartPPSeeds()
{
	InitWorkersData_PP();
	if (!m_bContinue)
		return;

	IterateReportSeeds_PP();
	if (!m_bContinue)
		return;
}

void CKMeanContractL2::RequestToBeContractor()
{
	string contractorName =m_gv.theContractorName;
	cout<<IceUtil::Time::now().toDateTime()<< " RequestToBeContractor, ContractorName="<<contractorName<<endl; 
	m_workerCnt=m_gv.theWorkerCnt;
	m_memSize=m_gv.theMemSize;

	int rt =m_gv.theKMeanPrx->RequestToBeContractor(
		m_projectID, contractorName,m_workerCnt,m_memSize,m_contractPtr);

	m_bContinue=(rt==1 && m_contractPtr && m_contractPtr->AcceptedAsContractor);
}

void CKMeanContractL2::GetObserverData()
{
	//determine how many rows per batch
	Ice::Long batchByteSize= 1024*1024; //M
	batchByteSize*= 128; //128M
	
	Ice::Long totalRowCnt=m_contractPtr->FeatureIdxTo-m_contractPtr->FeatureIdxFrom;
	Ice::Long obsCnt=m_contractPtr->ObserverIDs.size();
	Ice::Long totalValueCnt=totalRowCnt*obsCnt;

	Ice::Long rowBytesSize=obsCnt *sizeof(Ice::Double);
	Ice::Long batchRowCnt = batchByteSize/rowBytesSize;
	Ice::Long remainRowCnt=totalRowCnt;
	
	//determine row adjustment method by distance used
	iBS::RowAdjustEnum rowAdjust = iBS::RowAdjustNone;
	if (m_contractPtr->Distance == iBS::KMeansDistCorrelation)
	{
		rowAdjust = iBS::RowAdjustZeroMeanUnitLength;
	}

	m_data.reset(new Ice::Double[totalValueCnt]); //allocate too many mem, 
	if(!m_data.get())
	{
		cout<<IceUtil::Time::now().toDateTime()<< "GetObserverData allocate m_data failed"<<batchRowCnt<<endl;
		m_bContinue=false;
		throw ::std::bad_alloc();
	}

	Ice::Long featureIdxFrom=m_contractPtr->FeatureIdxFrom;
	cout<<IceUtil::Time::now().toDateTime()<< " Get observer data, begin ... batchRowCnt="<<batchRowCnt<<endl;
	int batchIdx=0;
	while (remainRowCnt>0)
	{
        Ice::Long thisBatchCnt=0;
        if(remainRowCnt>batchRowCnt)
		{
            thisBatchCnt=batchRowCnt;
		}
        else
		{
            thisBatchCnt = remainRowCnt;
		}
		DoubleVec values;
		cout<<IceUtil::Time::now().toDateTime()<< "  GetRowMatrix, featureIdxFrom, to="<<featureIdxFrom<<"-"<<featureIdxFrom+thisBatchCnt<<endl;
		
		int rt = m_contractPtr->FcdcReader->GetRowMatrix(
			m_contractPtr->ObserverIDs, featureIdxFrom, featureIdxFrom + thisBatchCnt, rowAdjust, values);
		if(rt!=1)
		{
			m_bContinue=false;
			return;
		}
		Ice::Long fetchedValueCnt=(featureIdxFrom-m_contractPtr->FeatureIdxFrom)*obsCnt;
		std::copy(values.begin(),values.end(),m_data.get()+fetchedValueCnt);
        
        featureIdxFrom+=thisBatchCnt;
        remainRowCnt-=thisBatchCnt;
		cout<<IceUtil::Time::now().toDateTime()<< "  batchIdx="<<batchIdx<<endl;
		batchIdx++;
	}
	
}

void CKMeanContractL2::InitWorkersData()
{
	m_it = 0;
	Ice::Long colCnt=m_contractPtr->ObserverIDs.size();
	Ice::Long rowCnt=m_contractPtr->K;

	//init shared data
	m_localFeatureIdx2ClusterIdx.resize(
		m_contractPtr->FeatureIdxTo-m_contractPtr->FeatureIdxFrom,-1);
	std::fill(m_localFeatureIdx2ClusterIdx.begin(), m_localFeatureIdx2ClusterIdx.end(), -1);

	// delete KCnt and KSums for previous task
	if (!m_workerKCnts.empty())
	{
		for (size_t i = 0; i<m_workerKCnts.size(); i++)
		{
			DoubleArrayPtr_T ptr = m_workerKCnts[i];
			if (ptr != 0)
			{
				delete[] ptr;
			}
			m_workerKCnts[i] = NULL;
		}

		for (size_t i = 0; i<m_workerKSums.size(); i++)
		{
			DoubleArrayPtr_T ptr = m_workerKSums[i];
			if (ptr != 0)
			{
				delete[] ptr;
			}
			m_workerKSums[i] = NULL;
		}
	}
	else
	{
		//init KCnts, KSums per worker
		m_workerKCnts.resize(m_workerCnt, 0);
		m_workerKSums.resize(m_workerCnt, 0);
	}

	// init RAM for KCnts, KSums
	for (int i = 0; i<m_workerCnt; i++)
	{
		m_workerKCnts[i] = new Ice::Double[rowCnt];
		m_workerKSums[i] = new Ice::Double[rowCnt*colCnt];
		if ((!m_workerKCnts[i]) || (!m_workerKSums[i]))
		{
			m_bContinue = false;
			throw ::std::bad_alloc();
		}
	}

	m_workerDistortion.resize(m_workerCnt, 0);
	std::fill(m_workerDistortion.begin(), m_workerDistortion.end(), 0);

	m_workerClusterChangeCnts.resize(m_workerCnt, 0);
	std::fill(m_workerClusterChangeCnts.begin(), m_workerClusterChangeCnts.end(), 0);
	
	
}

void CKMeanContractL2::IterateReportKCntsAndSums()
{
	bool bNeedExit=false;
	m_remainWorkerCallBackCnt =0;
	while(!bNeedExit)
	{
		{
			//entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while(!m_shutdownRequested)
			{
				if(m_remainWorkerCallBackCnt> 0)
				{
					m_needNotify = true;
					m_monitor.wait();
				}
				if(m_remainWorkerCallBackCnt==0)
				{
					//all workers have finished what I requested
					break;
				}
			}
			//if control request shutdown
			if(m_shutdownRequested)
			{

				m_bContinue=false;
				bNeedExit=true;

				cout<<"CKMeanContractL2 m_shutdownRequested..."<<endl; 
			}

			//leaving critical region
			m_needNotify=false;
		}

		//reached here because  I am not waiting more callbacks from workers
		if(!bNeedExit)
		{
			bool needRunOnceAgain = ReportKCntsAndSums();
			if(needRunOnceAgain)
			{
				//creat work items for workers, will change m_remainWorkerCallBackCnt
				AssignUpdateKCntsAndKSumsToWorkers();
				if(!m_bContinue)
					return;
				
			}
			bNeedExit=(needRunOnceAgain==false);
		}
	}
}

void CKMeanContractL2::NotifyWorkerItemDone(const KMeansWorkItemPtr& item)
{
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);

	m_remainWorkerCallBackCnt--;

	if(m_needNotify && m_remainWorkerCallBackCnt==0)
	{
		m_monitor.notify();
	}
}

bool CKMeanContractL2::AssignUpdateKCntsAndKSumsToWorkers()
{
	LongVec rowCntInWorker;
	Ice::Long totalRowCnt= m_contractPtr->FeatureIdxTo-m_contractPtr->FeatureIdxFrom;
	Ice::Long remainingRowCnt=totalRowCnt;

	for (int i=0;i<m_workerCnt;i++)
	{

		Ice::Long cnt=totalRowCnt/m_workerCnt;
		if(cnt<1)
		{
			m_bContinue=false;
			return false;
		}
		
		if(remainingRowCnt<cnt || i==(m_workerCnt-1))
		{
			cnt = remainingRowCnt;
		}

		if(cnt<1)
		{
			m_bContinue=false;
			return false;
		}

		rowCntInWorker.push_back(cnt);
		remainingRowCnt-=cnt;
	}

	m_remainWorkerCallBackCnt=m_workerCnt; //set cnt first
	Ice::Long assignedRowCnt=0;
	for(int i=0;i<m_workerCnt;i++)
	{
		m_workerClusterChangeCnts[i] = 0;
		Ice::Long rowCnt=rowCntInWorker[i];
		int workerIdx = i;
		//global feature idx range for this worker
		::Ice::Long workerFeatureIdxFrom = m_contractPtr->FeatureIdxFrom+assignedRowCnt;
		::Ice::Long workerFeatureIdxTo = workerFeatureIdxFrom+rowCnt;
		bool resetKCntsKSumsFirst = true;
		CKMeanContractL2Ptr kmeanL2Ptr(this);

		KMeansWorkItemPtr wi;
		if (m_contractPtr->Distance == iBS::KMeansDistCorrelation)
		{
			wi = new CCorrelationUpdateKCntsAndKSums(
				kmeanL2Ptr, workerIdx,
				workerFeatureIdxFrom, workerFeatureIdxTo,
				resetKCntsKSumsFirst);
		}
		else
		{
			wi = new CEuclideanUpdateKCntsAndKSums(
				kmeanL2Ptr, workerIdx,
				workerFeatureIdxFrom, workerFeatureIdxTo,
				resetKCntsKSumsFirst);
		}

		m_gv.theKMeanWorkerMgr->AssignItemToWorker(workerIdx,wi);
		
		assignedRowCnt+=rowCnt;
	}

	return true;
}

bool CKMeanContractL2::ReportKCntsAndSums()
{
	bool needRunOnceAgain=true;

	cout<<IceUtil::Time::now().toDateTime()<< " ReportKCntsAndSums, iteration="<<m_it<<endl; 

	std::pair<const Ice::Double*, const Ice::Double*> kcnts(0,0);
	std::pair<const Ice::Double*, const Ice::Double*> ksums(0,0);
	Ice::Long colCnt=m_contractPtr->ObserverIDs.size();
	Ice::Long rowCnt=m_contractPtr->K;
	Ice::Long totalKChangeCnt = 0;
	Ice::Double distortion = 0;
	if(m_it==0)
	{
		//do not need to gather KCnts and KSums from workers
	}
	else
	{
		//when reached here, all workers have already finished their jobs, combine their KCnts and KSums
		//
		Ice::Double *pKCnts=m_workerKCnts[0]; //work0's data as inital values
		Ice::Double	*pKSums=m_workerKSums[0];
		totalKChangeCnt = m_workerClusterChangeCnts[0];
		distortion = m_workerDistortion[0];
		for(int w=1;w<m_workerCnt;w++) //from worker1
		{
			Ice::Double *wKCnts=m_workerKCnts[w];
			Ice::Double	*wKSums=m_workerKSums[w];

			for(int i=0;i<rowCnt;i++)
			{
				pKCnts[i]+=wKCnts[i];
				for(int j=0;j<colCnt;j++)
				{
					pKSums[i*colCnt+j]+=wKSums[i*colCnt+j];
				}
			}
			totalKChangeCnt += m_workerClusterChangeCnts[w];
			distortion += m_workerDistortion[w];
		}

		kcnts.first=pKCnts;
		kcnts.second=pKCnts+rowCnt;

		ksums.first=pKSums;
		ksums.second=pKSums+(colCnt*rowCnt);
	}

	m_gv.theKMeanPrx->ReportKCntsAndSums(
		m_contractPtr->ProjectID,
		m_contractPtr->ContractorIdx,
		kcnts, ksums, totalKChangeCnt, distortion, m_KClusters);

	if(m_KClusters.empty())
	{
		//clustering done
		needRunOnceAgain=false;
	}
	
	cout<<IceUtil::Time::now().toDateTime()<< "Iteration "<<m_it<< " all contractors  join ends"<<endl; 
	m_it++;
	return needRunOnceAgain;
}

void CKMeanContractL2::ReportKMembers()
{
	std::pair<const Ice::Long*, const Ice::Long*> values(0,0);
	values.first=&m_localFeatureIdx2ClusterIdx[0];
	values.second=values.first+(m_contractPtr->FeatureIdxTo-m_contractPtr->FeatureIdxFrom);
	m_gv.theKMeanPrx->ReportKMembers(m_contractPtr->ProjectID,
		m_contractPtr->ContractorIdx,
		m_contractPtr->FeatureIdxFrom,m_contractPtr->FeatureIdxTo,values);
}

////////////////////////////////////////////////////////////////////////////////////////



void CKMeanContractL2::InitWorkersData_PP()
{
	m_it = 0;
	Ice::Long colCnt = m_contractPtr->ObserverIDs.size();
	Ice::Long rowCnt = m_contractPtr->K;

	// init shared data
	m_localFeatureIdx2MinDist.resize(
		m_contractPtr->FeatureIdxTo - m_contractPtr->FeatureIdxFrom,
		std::numeric_limits<Ice::Double>::max());

	std::fill(m_localFeatureIdx2MinDist.begin(), m_localFeatureIdx2MinDist.end(), std::numeric_limits<Ice::Double>::max());

	m_workerDistSum.resize(m_workerCnt, 0);
	std::fill(m_workerDistSum.begin(), m_workerDistSum.end(), 0);

	m_workerFeatureIdxsFrom.resize(m_workerCnt, 0);
	std::fill(m_workerFeatureIdxsFrom.begin(), m_workerFeatureIdxsFrom.end(), 0);

	m_workerFeatureIdxsTo.resize(m_workerCnt, 0);
	std::fill(m_workerFeatureIdxsTo.begin(), m_workerFeatureIdxsTo.end(), 0);
}

void CKMeanContractL2::IterateReportSeeds_PP()
{
	bool bNeedExit = false;
	m_remainWorkerCallBackCnt = 0;
	while (!bNeedExit)
	{
		{
			// entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while (!m_shutdownRequested)
			{
				if (m_remainWorkerCallBackCnt> 0)
				{
					m_needNotify = true;
					m_monitor.wait();
				}
				if (m_remainWorkerCallBackCnt == 0)
				{
					//all workers have finished what I requested
					break;
				}
			}
			//if control request shutdown
			if (m_shutdownRequested)
			{

				m_bContinue = false;
				bNeedExit = true;

				cout << "CKMeanContractL2 m_shutdownRequested..." << endl;
			}

			//leaving critical region
			m_needNotify = false;
		}

		//reached here because  I am not waiting more callbacks from workers
		if (!bNeedExit)
		{
			bool needRunOnceAgain = ReportClusterSeed();
			if (needRunOnceAgain)
			{
				//creat work items for workers, will change m_remainWorkerCallBackCnt
				AssignComputeMinDistanceToWorkers();
				if (!m_bContinue)
					return;
			}
			bNeedExit = (needRunOnceAgain == false);
		}
	}
}

bool CKMeanContractL2::ReportClusterSeed()
{
	bool needRunOnceAgain = true;

	cout << IceUtil::Time::now().toDateTime() << " ReportClusterSeed, iteration=" << m_it << endl;
	Ice::Double distSum = std::numeric_limits<Ice::Double>::quiet_NaN();
	if (m_it == 0)
	{
		//do not need to gather distance sums from workers
	}
	else
	{
		//when reached here, all workers have already finished their jobs, combine their distance sums

		distSum = 0;
		for (int w = 0; w<m_workerCnt; w++) //from worker1
		{
			distSum += m_workerDistSum[w];
		}
	}

	Ice::Double selectSum;
	m_gv.theKMeanPrx->ReportPPDistSum(
		m_contractPtr->ProjectID,
		m_contractPtr->ContractorIdx,
		distSum, selectSum);

	iBS::DoubleVec center;
	Ice::Long seedFeatureIdx = -1;
	if (selectSum == selectSum)
	{
		// if not NaN, select new centroid
		cout << IceUtil::Time::now().toDateTime() 
			<< "Iteration " << m_it << " select seed in this contractor"
			<< "selectSum "<<selectSum<<endl;

		SelectNewSeed_PP(selectSum, seedFeatureIdx, center);
	}
	
	m_gv.theKMeanPrx->ReportNewSeed(
		m_contractPtr->ProjectID,
		m_contractPtr->ContractorIdx,
		seedFeatureIdx, center, m_KClusters);

	if (m_KClusters.empty())
	{
		//clustering done
		needRunOnceAgain = false;
	}

	cout << IceUtil::Time::now().toDateTime() << "Iteration " << m_it << " all contractors  join ends" << endl;
	m_it++;
	return needRunOnceAgain;
}

bool CKMeanContractL2::AssignComputeMinDistanceToWorkers()
{
	LongVec rowCntInWorker;
	Ice::Long totalRowCnt = m_contractPtr->FeatureIdxTo - m_contractPtr->FeatureIdxFrom;
	Ice::Long remainingRowCnt = totalRowCnt;

	for (int i = 0; i<m_workerCnt; i++)
	{

		Ice::Long cnt = totalRowCnt / m_workerCnt;
		if (cnt<1)
		{
			m_bContinue = false;
			return false;
		}

		if (remainingRowCnt<cnt || i == (m_workerCnt - 1))
		{
			cnt = remainingRowCnt;
		}

		if (cnt<1)
		{
			m_bContinue = false;
			return false;
		}

		rowCntInWorker.push_back(cnt);
		remainingRowCnt -= cnt;
	}

	m_remainWorkerCallBackCnt = m_workerCnt; //set cnt first
	Ice::Long assignedRowCnt = 0;
	for (int i = 0; i<m_workerCnt; i++)
	{
		Ice::Long rowCnt = rowCntInWorker[i];
		int workerIdx = i;
		//global feature idx range for this worker
		::Ice::Long workerFeatureIdxFrom = m_contractPtr->FeatureIdxFrom + assignedRowCnt;
		::Ice::Long workerFeatureIdxTo = workerFeatureIdxFrom + rowCnt;
		bool resetKCntsKSumsFirst = false;
		CKMeanContractL2Ptr kmeanL2Ptr(this);

		KMeansWorkItemPtr wi;
		if (m_contractPtr->Seeding == iBS::KMeansSeedingKMeansRandom)
		{
			wi = new CUniformPPSeedComputeMinDistance(
				kmeanL2Ptr, workerIdx,
				workerFeatureIdxFrom, workerFeatureIdxTo);
			
		}
		else if (m_contractPtr->Distance == iBS::KMeansDistCorrelation)
		{
			wi = new CCorrelationPPSeedComputeMinDistance(
				kmeanL2Ptr, workerIdx,
				workerFeatureIdxFrom, workerFeatureIdxTo);
		}
		else
		{
			wi = new CEuclideanPPSeedComputeMinDistance(
				kmeanL2Ptr, workerIdx,
				workerFeatureIdxFrom, workerFeatureIdxTo);
		}

		m_workerFeatureIdxsFrom[workerIdx] = workerFeatureIdxFrom;
		m_workerFeatureIdxsTo[workerIdx] = workerFeatureIdxTo;
		m_gv.theKMeanWorkerMgr->AssignItemToWorker(workerIdx, wi);

		assignedRowCnt += rowCnt;
	}

	return true;
}

bool CKMeanContractL2::SelectNewSeed_PP(
	Ice::Double selectSum, Ice::Long& seedFeatureIdx, iBS::DoubleVec& center)
{
	Ice::Double distSum = selectSum;
	Ice::Double leftDistSum = selectSum;
	int w = 0;
	for (w = 0; w<m_workerCnt-1; w++) //from worker1
	{
		distSum -= m_workerDistSum[w];
		if (distSum <= 0)
			break;
		leftDistSum -= m_workerDistSum[w];
	}
	Ice::Long workerFeatureIdxFrom = m_workerFeatureIdxsFrom[w];
	Ice::Long workerFeatureIdxTo = m_workerFeatureIdxsTo[w];
	Ice::Long dataRowCnt = workerFeatureIdxTo - workerFeatureIdxFrom;
	Ice::Long i;
	for (i = 0; i < dataRowCnt-1; i++)
	{
		Ice::Long rowIdxInContractor = workerFeatureIdxFrom - m_contractPtr->FeatureIdxFrom + i;
		leftDistSum -= m_localFeatureIdx2MinDist[rowIdxInContractor];
		if (leftDistSum <= 0)
			break;
	}
	Ice::Long colCnt = m_contractPtr->ObserverIDs.size();
	Ice::Long seedRowIdxInContractor = workerFeatureIdxFrom - m_contractPtr->FeatureIdxFrom + i;
	seedFeatureIdx = workerFeatureIdxFrom + i;
	//dataRow
	Ice::Double *dr = m_data.get() + (seedRowIdxInContractor*colCnt);
	center.resize(colCnt, 0);
	std::copy(dr, dr + colCnt, center.begin());
	return true;
}