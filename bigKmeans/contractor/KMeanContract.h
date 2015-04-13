#ifndef __KMeanContract_h__
#define __KMeanContract_h__

#include <KMeanCommonDefine.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <KMeanService.h>

using namespace iBS;

class CGlobalVars;

class CKMeanContractL2 : public IceUtil::Shared
{
	friend class CEuclideanUpdateKCntsAndKSums;
	friend class CCorrelationUpdateKCntsAndKSums;
	friend class CEuclideanPPSeedComputeMinDistance;
	friend class CCorrelationPPSeedComputeMinDistance;
	friend class CUniformPPSeedComputeMinDistance;
public:
	CKMeanContractL2(CGlobalVars& gv, int projectID);
	~CKMeanContractL2();

public:
	void Start();
	//for worker to callback
	
	void NotifyWorkerItemDone(const KMeansWorkItemPtr& item);
private:
	void StartKMeans();
	void RequestToBeContractor();
	void GetObserverData();
	void NormalizeObserverData();
	void InitWorkersData();
	
	void IterateReportKCntsAndSums();
	bool AssignUpdateKCntsAndKSumsToWorkers();
	bool ReportKCntsAndSums();
	void ReportKMembers();

	// KMeans++ seeds
	void StartPPSeeds();
	void InitWorkersData_PP();
	void IterateReportSeeds_PP();
	bool ReportClusterSeed();
	bool AssignComputeMinDistanceToWorkers();
	bool SelectNewSeed_PP(Ice::Double selectSum, Ice::Long& seedFeatureIdx, iBS::DoubleVec& center);
	
private:
	CGlobalVars&	m_gv;
	bool m_bContinue;
	int m_projectID;
	int m_workerCnt;
	int m_memSize;
	KMeanContractInfoPtr m_contractPtr;
	iBS::ObserverStatsInfoVec m_observerStats;

	::IceUtil::ScopedArray<Ice::Double>  m_data; //shared for all worker threads
	
	typedef ::Ice::Double*	  DoubleArrayPtr_T;
	typedef ::std::vector<DoubleArrayPtr_T> DoubleArrayPtrList_T;

	DoubleArrayPtrList_T m_workerKCnts;
	DoubleArrayPtrList_T m_workerKSums;
	iBS::LongVec		 m_localFeatureIdx2ClusterIdx;
	iBS::LongVec		 m_workerClusterChangeCnts;
	iBS::DoubleVec			m_workerDistortion;
	iBS::DoubleVec  m_KClusters;
	Ice::Long m_it;

	int  m_remainWorkerCallBackCnt;
	bool m_needNotify;
	bool m_shutdownRequested;
	IceUtil::Monitor<IceUtil::Mutex>	m_monitor; //for worker thread to notify back

	//KMeans++ Seeds
	iBS::DoubleVec		m_localFeatureIdx2MinDist;
	iBS::DoubleVec		m_workerDistSum;
	iBS::LongVec		m_workerFeatureIdxsFrom;
	iBS::LongVec		m_workerFeatureIdxsTo;
};


#endif

