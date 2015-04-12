#include <bdt/bdtBase.h>
#include <IceUtil/Random.h>
#include <ObserverIndexDB.h>
#include <GlobalVars.h>
#include <algorithm>       // std::numeric_limits
#include <limits>       // std::numeric_limits

CObserverIndexDB::CObserverIndexDB(Ice::CommunicatorPtr communicator, 
							 const std::string& strEnvName,  const std::string& strDBName)
	:m_communicator(communicator),
	m_connection(Freeze::createConnection(communicator, strEnvName)),
	m_map(m_connection, strDBName)
{
	Initilize();
}

CObserverIndexDB::~CObserverIndexDB()
{
	
}

void CObserverIndexDB::Initilize()
{
	cout<<"CObserverIndexDB Initilize begin ..."<<endl; 
	//not in service yet, no concurrency issues

	ObserverIndexFreezeMap_T::const_iterator p;
	int obsCnt=0;
	for(p=m_map.begin(); p!=m_map.end(); ++p)
	{
		obsCnt++;
	}
	cout<<"TotalObserverIndex="<<obsCnt<<endl;
	cout<<"CObserverIndexDB Initilize End"<<endl;
}

void CObserverIndexDB::SetObserverIndexInfoBlank(::iBS::ObserverIndexInfo& oii)
{
	oii.IndexID=0;
	oii.IndexType=iBS::InvertIndexIntValueIntKey;
	oii.ObserverID=0;
	oii.IndexObserverID=0;
    oii.Version=0;
	oii.KeyCnt=0;
	oii.TotalRowCnt=0;
	oii.IntKeys.clear();
	oii.MakeIndexFile=false;
	oii.KeyIdx2RowCnt.clear();
	oii.KeyIdx2RowIdxListStartIdx.clear();
	oii.KeyNames.clear();
	oii.UpdateDT=IceUtil::Time::now().toMilliSeconds();
}

::Ice::Int CObserverIndexDB::GetObserverIndexInfo(
	Ice::Int obserserID, ::iBS::ObserverIndexInfo& oii)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	
	ObserverIndexFreezeMap_T::const_iterator p
			= m_map.find(obserserID);
	if(p !=m_map.end())
	{
		oii=p->second;
	}
	else
	{
		oii.IndexType=iBS::InvertIndexUnknown;
		return 0;
	}

	return 1;
}

::Ice::Int CObserverIndexDB::GetObserverIndexObserverID(
	Ice::Int obserserID)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	ObserverIndexFreezeMap_T::const_iterator p
			= m_map.find(obserserID);
	if(p !=m_map.end())
	{
		return p->second.IndexObserverID;
	}
	else
	{
		return 0;
	}

	return 1;
}


::Ice::Int CObserverIndexDB::SetObserverIndexInfo(
	Ice::Int obserserID, const ::iBS::ObserverIndexInfo& oii)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	::Freeze::TransactionHolder tx(m_connection); //exception safe
	ObserverIndexFreezeMap_T::iterator p
		= m_map.find(obserserID);
	if(p ==m_map.end())
	{
		m_map.insert(std::make_pair(obserserID, oii));
	}
	else
	{
		p.set(oii);
	}
	tx.commit();
	return 1;
}


//////////////////////////////////////////////////////////////////////////
//CObserverIndexHelper

bool CObserverIndexHelper::IntValueIntKeyProcessNextBatch(CObserverIndexIntValueIntKeyJob& job, 
			Ice::Long featureIdxFrom,
			const Ice::Double* values,
			const Ice::Long valueCnt)
{
	if(job.m_round==0)
	{
		return firstRound_IntValueIntKeyProcessNextBatch(job,featureIdxFrom,values,valueCnt);
	}
	else
	{
		return secondRound_IntValueIntKeyProcessNextBatch(job,featureIdxFrom,values,valueCnt);
	}
}

bool CObserverIndexHelper::firstRound_IntValueIntKeyProcessNextBatch(
			CObserverIndexIntValueIntKeyJob& job, 
			Ice::Long featureIdxFrom,
			const Ice::Double* values,
			const Ice::Long valueCnt)
{
	for(int i=job.m_colIdx;i<valueCnt;i+=job.m_colCnt)
	{
		if(job.m_handleNaN&&values[i]!=values[i])
		{
			continue;
		}

		int val=(int)values[i];

		CObserverIndexIntValueIntKeyJob::IntValue2KeyIdx_T::iterator 
			p = job.m_intVal2KeyIdx.find(val);
		int keyIdx=0;
		if(p==job.m_intVal2KeyIdx.end())
		{
			//new key
			keyIdx=(int)job.m_intVal2KeyIdx.size();
			job.m_intVal2KeyIdx.insert(std::pair<int,int>(val,keyIdx));
		}
		else
		{
			keyIdx=p->second;
		}

		if(job.m_oii.KeyCnt<=keyIdx)
		{
			//new key
			job.m_oii.IntKeys.push_back(val);
			job.m_oii.KeyIdx2RowCnt.push_back(1);
			job.m_oii.KeyCnt++;

		}
		else
		{
			//existing key
			job.m_oii.KeyIdx2RowCnt[keyIdx]++;
		}
		job.m_oii.TotalRowCnt++;

	}
	
	return true;
}


bool CObserverIndexHelper::secondRound_IntValueIntKeyProcessNextBatch(
			CObserverIndexIntValueIntKeyJob& job, 
			Ice::Long featureIdxFrom,
			const Ice::Double* values,
			const Ice::Long valueCnt)
{
	if(job.m_keyIdxFeatureIdxCnt.empty())
	{
		job.m_keyIdxFeatureIdxCnt.resize(job.m_oii.KeyCnt,0);
		job.m_featureIdxs.reset(new Ice::Double[job.m_oii.TotalRowCnt]);
		if(!job.m_featureIdxs.get())
		{
			return false;
		}
	}

	Ice::Long featureIdxOffset=-1;
	for(int i=job.m_colIdx;i<valueCnt;i+=job.m_colCnt)
	{
		featureIdxOffset++;
		if(job.m_handleNaN&&values[i]!=values[i])
		{
			continue;
		}

		int val=(int)values[i];

		CObserverIndexIntValueIntKeyJob::IntValue2KeyIdx_T::iterator 
			p = job.m_intVal2KeyIdx.find(val);

		if(p==job.m_intVal2KeyIdx.end()){
			return false;
		}


		int keyIdx=p->second;
		Ice::Long fidx=featureIdxFrom+featureIdxOffset; //featureIdxFrom in unrolled inde

		Ice::Long address=job.m_oii.KeyIdx2RowIdxListStartIdx[keyIdx]+job.m_keyIdxFeatureIdxCnt[keyIdx];
		job.m_featureIdxs.get()[address]=(Ice::Double)fidx;
		job.m_keyIdxFeatureIdxCnt[keyIdx]++;
	}
	
	return true;
}

bool CObserverIndexHelper::SyncKeyIdx2RowIdxListStartIdx(::iBS::ObserverIndexInfo& oii)
{
	oii.KeyIdx2RowIdxListStartIdx.resize(oii.KeyCnt);
	Ice::Long featureIdxCnt=0;
	for(int i=0;i<oii.KeyCnt;i++)
	{
		oii.KeyIdx2RowIdxListStartIdx[i]=featureIdxCnt;
		featureIdxCnt+=oii.KeyIdx2RowCnt[i];
	}
	return true;
}

bool CObserverIndexHelper::GetIntKeyIdxsByKeyValues(
			const ::iBS::IntVec& keys, 
			const ::iBS::ObserverIndexInfo& oii,
			::iBS::IntVec& keyIdxs)
{
	std::map<int,int> keyVal2Idx;
	for(int i=0;i<oii.KeyCnt;i++)
	{
		int key=oii.IntKeys[i];
		keyVal2Idx.insert(std::pair<int,int>(key,i));
	}
	
	for(int i=0;i<keys.size();i++)
	{
		std::map<int,int>::iterator p= keyVal2Idx.find(keys[i]);
		if(p!=keyVal2Idx.end())
		{
			keyIdxs.push_back(p->second);
		}
		else
		{
			return false;
		}
	}
	return true;
}
