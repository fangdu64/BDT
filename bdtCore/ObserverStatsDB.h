
#ifndef __ObserverStatsDB_h__
#define __ObserverStatsDB_h__

#include <IceUtil/Mutex.h>
#include <Ice/Ice.h>
#include <IceUtil/Time.h>
#include <Freeze/Freeze.h>
#include <FCDCentralService.h>
#include <FCDCentralFreeze.h>
#include <ObserverStatsFreezeMap.h>


typedef iBS::CObserverStatsFreezeMap ObserverStatsFreezeMap_T;

class CObserverStatsDB
{
public:
	CObserverStatsDB(Ice::CommunicatorPtr communicator, const std::string& strEnvName, 
		const std::string& strDBName);

	~CObserverStatsDB();

public:
	::Ice::Int GetObserverStatsInfo(Ice::Int obserserID, ::iBS::ObserverStatsInfo& osi);

	::Ice::Int SetObserverStatsInfo(Ice::Int obserserID, const ::iBS::ObserverStatsInfo& osi);
	::Ice::Int SetObserverStatsInfos(const ::iBS::ObserverStatsInfoVec& osis);

	::Ice::Int GetObserversStats(const ::iBS::IntVec& observerIDs,
                                           ::iBS::ObserverStatsInfoVec& observerStats);
	::Ice::Int RemoveObserversStats(const ::iBS::IntVec& observerIDs);
	
private:
	::Ice::Int setObserverStatsInfo(Ice::Int obserserID, const ::iBS::ObserverStatsInfo& osi);
private:
	void Initilize();
	void SetObserverStatsInfoDefault(::iBS::ObserverStatsInfo& osi);

private:
	Ice::CommunicatorPtr		m_communicator;
	Freeze::ConnectionPtr		m_connection;
	ObserverStatsFreezeMap_T	m_map;
	IceUtil::Mutex				m_observerMutex;
};

struct CObserverStatsBasicJob
{
	Ice::Long m_totalValueCnt;
	Ice::Long m_processedValueCnt;
	IceUtil::Time m_jobStartTime;
	double m_min;
	double m_max;
	double m_cnt;
	double m_sum;
	bool m_handleNaN;
	//as if values are from grouped 
	int m_colCnt;
	int m_colIdx;
};

class CObserverStatsHelper
{
public:
	CObserverStatsHelper()
	{
	}
	static bool BasicJobProcessNextBatch(CObserverStatsBasicJob& job, const Ice::Double* values, 
			const Ice::Long valueCnt);

	static bool BasicJobToStats(CObserverStatsBasicJob& job, ::iBS::ObserverStatsInfo& osi);

};

#endif
