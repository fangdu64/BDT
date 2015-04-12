#ifndef __ObserverIndexDB_h__
#define __ObserverIndexDB_h__

#include <IceUtil/Mutex.h>
#include <Ice/Ice.h>
#include <Freeze/Freeze.h>
#include <FCDCentralService.h>
#include <FCDCentralFreeze.h>
#include <ObserverIndexFreezeMap.h>

typedef iBS::CObserverIndexFreezeMap ObserverIndexFreezeMap_T;

class CObserverIndexDB
{
public:
	CObserverIndexDB(Ice::CommunicatorPtr communicator, const std::string& strEnvName, 
		const std::string& strDBName);

	~CObserverIndexDB();

public:
	::Ice::Int GetObserverIndexInfo(Ice::Int obserserID, ::iBS::ObserverIndexInfo& oii);
	::Ice::Int GetObserverIndexObserverID(Ice::Int obserserID);

	::Ice::Int SetObserverIndexInfo(Ice::Int obserserID, const ::iBS::ObserverIndexInfo& oii);

	void SetObserverIndexInfoBlank(::iBS::ObserverIndexInfo& oii);
	
private:
	void Initilize();
private:
	Ice::CommunicatorPtr		m_communicator;
	Freeze::ConnectionPtr		m_connection;
	ObserverIndexFreezeMap_T	m_map;
	IceUtil::Mutex				m_mutex;
};

class CObserverIndexIntValueIntKeyJob
{
public:
	CObserverIndexIntValueIntKeyJob(::iBS::ObserverIndexInfo& oii)
		:m_oii(oii)
	{
		m_round=0;
	}

public:
	::iBS::ObserverIndexInfo& m_oii;
	//as if values are from grouped 
	int m_colCnt;
	int m_colIdx;
	IceUtil::Time m_jobStartTime;
	bool m_handleNaN;
	int m_round;

	typedef std::map<int, int> IntValue2KeyIdx_T;
	IntValue2KeyIdx_T m_intVal2KeyIdx;

	typedef std::vector<Ice::Long> KeyIdxFeatureIdxCnt_T;
	KeyIdxFeatureIdxCnt_T m_keyIdxFeatureIdxCnt;

	::IceUtil::ScopedArray<Ice::Double>  m_featureIdxs;
};

class CObserverIndexHelper
{
public:
	CObserverIndexHelper()
	{
	}

	static bool IntValueIntKeyProcessNextBatch(
			CObserverIndexIntValueIntKeyJob& job, 
			Ice::Long featureIdxFrom,
			const Ice::Double* values,
			const Ice::Long valueCnt);

	static bool SyncKeyIdx2RowIdxListStartIdx(::iBS::ObserverIndexInfo& oii);

	static bool GetIntKeyIdxsByKeyValues(
			const ::iBS::IntVec& keys, 
			const ::iBS::ObserverIndexInfo& oii,
			::iBS::IntVec& keyIdxs);

private:
	static bool firstRound_IntValueIntKeyProcessNextBatch(
			CObserverIndexIntValueIntKeyJob& job, 
			Ice::Long featureIdxFrom,
			const Ice::Double* values,
			const Ice::Long valueCnt);

	static bool secondRound_IntValueIntKeyProcessNextBatch(
			CObserverIndexIntValueIntKeyJob& job, 
			Ice::Long featureIdxFrom,
			const Ice::Double* values,
			const Ice::Long valueCnt);
};


#endif

