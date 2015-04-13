#ifndef __KMeanProject_h__
#define __KMeanProject_h__

#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <KMeanService.h>
#include <KMeanWorkItem.h>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/mersenne_twister.hpp>

using namespace iBS;

class CGlobalVars;

class CKMeanProjectL2;
typedef IceUtil::Handle<CKMeanProjectL2> CKMeanProjectL2Ptr;

class CKMeanProjectL2 : public IceUtil::Shared
{
public:
	CKMeanProjectL2(const KMeanProjectInfoPtr& projectPtr);
	~CKMeanProjectL2();

public:
	void RequestToBeContractor(const RequestToBeContractorPtr& itemPtr);

	::Ice::Int
		LaunchProjectWithCurrentContractors(::Ice::Int projectID, Ice::Long amdTaskID);

	void ReportKCntsAndSums(const ReportKCntsAndSumsPtr& itemPtr);

	void ReportKClusters(const ::iBS::AMD_KMeanServerService_GetKClustersPtr& cb);
	void GetKSeeds(const ::iBS::AMD_KMeanServerService_GetKSeedsPtr& cb);
	
	void ReportKMembers(const ReportKMembersPtr& itemPtr);

	void GetKMembers(
		const ::iBS::AMD_KMeanServerService_GetKMembersPtr& cb,
		::Ice::Long featureIdxFrom,
		::Ice::Long featureIdxTo);
	void GetKCnts(const ::iBS::AMD_KMeanServerService_GetKCntsPtr& cb);

	KMeanProjectInfoPtr GetProjectInfo() { return m_projectPtr; }

	// KMeans++ seeds
	void ReportPPDistSum(const ReportPPDistSumPtr& itemPtr);
	void ReportNewSeed(const ReportNewSeedPtr& itemPtr);
	void GetPPSeedFeatureIdxs(const ::iBS::AMD_KMeanServerService_GetPPSeedFeatureIdxsPtr& cb);

	// pipeline control
	void GetNextTask(const GetNextTaskPtr& itemPtr);

	void GetKMeansResults(const ::iBS::AMD_KMeanServerService_GetKMeansResultsPtr& cb);

private:
	std::string GetTaskName();
	void SwitchToNextTask();
	void InitForNextTask(::iBS::KMeansTaskEnum task, Ice::Long K);
	void SaveCurrentKClustersAsSeedsPool(Ice::Long K, Ice::Double *pKClusters);
	

private:
	KMeanProjectInfoPtr m_projectPtr;
	Ice::Long m_amdTaskID;
	::IceUtil::ScopedArray<Ice::Double>  m_KCnts;
	::IceUtil::ScopedArray<Ice::Double>  m_KSums; //also as KClusters
	::IceUtil::ScopedArray<Ice::Long>  m_featureIdx2ClusterID;
	Ice::Long m_it; //iteration no.

	typedef std::list<RequestToBeContractorPtr> RequestToBeContractorPtrLsit_T;
	RequestToBeContractorPtrLsit_T m_requestToBeContractorItems;

	typedef std::vector<KMeanContractInfoPtr> KMeanContractInfoPtrLsit_T;
	KMeanContractInfoPtrLsit_T m_contractors;

	typedef std::list<ReportKCntsAndSumsPtr> ReportKCntsAndSumsPtrLsit_T;
	ReportKCntsAndSumsPtrLsit_T m_reportKCntsAndSumsItems;

	typedef std::list<ReportPPDistSumPtr> ReportPPDistSumPtrList_T;
	ReportPPDistSumPtrList_T m_reportPPDistSumItems;

	typedef std::list<ReportNewSeedPtr> ReportNewSeedPtrList_T;
	ReportNewSeedPtrList_T m_reportNewSeedItems;

	typedef std::list<GetNextTaskPtr> GetNextTaskPtrPtrList_T;
	GetNextTaskPtrPtrList_T m_getNextTaskItems;

	Ice::Double			m_distSum;

	IceUtil::Mutex		m_mutex;

	Ice::Long			m_KChangeCnt;
	Ice::Double			m_KDistortion;
	Ice::Double			m_KDistortion_k1;
	Ice::Double			m_lastExplained;
	Ice::Long			m_minExplainedReachedCnt;
	boost::uniform_01<boost::mt19937> m_rand01;
	iBS::LongVec		m_reportedSeedFeatureIdxs;

	::IceUtil::ScopedArray<Ice::Double>  m_seedsPool;
	Ice::Long m_seedsPoolSize;
	Ice::Long m_batchTaskIdx;
	Ice::Long m_maxIteration;

	iBS::KMeansResultVec	m_kmeansResults;
};


#endif

