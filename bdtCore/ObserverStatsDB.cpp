#include <bdt/bdtBase.h>
#include <IceUtil/Random.h>
#include <ObserverStatsDB.h>
#include <GlobalVars.h>
#include <algorithm>       // std::numeric_limits
#include <limits>       // std::numeric_limits

CObserverStatsDB::CObserverStatsDB(Ice::CommunicatorPtr communicator, 
							 const std::string& strEnvName,  const std::string& strDBName)
	:
	m_communicator(communicator),
	m_connection(Freeze::createConnection(communicator, strEnvName)),
	m_map(m_connection, strDBName)
{
	Initilize();
}

CObserverStatsDB::~CObserverStatsDB()
{
	
}

void CObserverStatsDB::Initilize()
{
	cout<<"CObserverStatsDB Initilize begin ..."<<endl; 
	//not in service yet, no concurrency issues

	ObserverStatsFreezeMap_T::const_iterator p;
	int obsCnt=0;
	for(p=m_map.begin(); p!=m_map.end(); ++p)
	{
		obsCnt++;
	}
	cout<<"TotalObserver="<<obsCnt<<endl;
	cout<<"CObserverStatsDB Initilize End"<<endl;
}

void CObserverStatsDB::SetObserverStatsInfoDefault(::iBS::ObserverStatsInfo& osi)
{
	//osi.ObserverID;
    osi.Version=0;
    osi.Cnt=0;
    osi.Max=0;
    osi.Min=0;
    osi.Sum=0;
	osi.Version=0;
	osi.UpdateDT=IceUtil::Time::now().toMilliSeconds();
    //osi.StatsNames;
    //osi.StatsValues;
}

::Ice::Int CObserverStatsDB::GetObserverStatsInfo(
	Ice::Int obserserID, ::iBS::ObserverStatsInfo& osi)
{
	IceUtil::Mutex::Lock lock(m_observerMutex);

	if(obserserID>iBS::SpecialFeatureObserverRAMOnlyMaxID)
	{
		ObserverStatsFreezeMap_T::const_iterator p
			= m_map.find(obserserID);
		if(p !=m_map.end())
		{
			osi=p->second;
		}
		else
		{
			return 0;
		}

		return 1;
	}
	else
	{
		return 0;
	}
}

::Ice::Int 
CObserverStatsDB::GetObserversStats(const ::iBS::IntVec& observerIDs,
                                           ::iBS::ObserverStatsInfoVec& observerStats)
{
	IceUtil::Mutex::Lock lock(m_observerMutex);

	for(size_t i=0; i<observerIDs.size();i++)
	{
		int obserserID=observerIDs[i];
		if(obserserID>iBS::SpecialFeatureObserverRAMOnlyMaxID)
		{
			ObserverStatsFreezeMap_T::const_iterator p
				= m_map.find(obserserID);
			if(p !=m_map.end())
			{
				observerStats.push_back(p->second);
			}
			else
			{
				return 0;
			}

			
		}
		else
		{
			return 0;
		}
	}
	return 1;
}

::Ice::Int
CObserverStatsDB::RemoveObserversStats(const ::iBS::IntVec& observerIDs)
{
	IceUtil::Mutex::Lock lock(m_observerMutex);

	for (size_t i = 0; i<observerIDs.size(); i++)
	{
		int obserserID = observerIDs[i];
		ObserverStatsFreezeMap_T::iterator p
			= m_map.find(obserserID);
		if (p != m_map.end())
		{
			m_map.erase(p);
		}
	}
	return 1;
}

::Ice::Int CObserverStatsDB::SetObserverStatsInfos(const ::iBS::ObserverStatsInfoVec& osis)
{
	IceUtil::Mutex::Lock lock(m_observerMutex);

	::Freeze::TransactionHolder tx(m_connection); //exception safe
	::iBS::ObserverStatsInfoVec::const_iterator it;

	for (it = osis.begin(); it != osis.end(); ++it)
	{
		iBS::ObserverStatsInfo& foi = const_cast<iBS::ObserverStatsInfo&> (*it);
		setObserverStatsInfo(foi.ObserverID, foi);
	}

	tx.commit();
	return 1;
}

::Ice::Int CObserverStatsDB::SetObserverStatsInfo(
	Ice::Int obserserID, const ::iBS::ObserverStatsInfo& osi)
{
	IceUtil::Mutex::Lock lock(m_observerMutex);

	if(obserserID>iBS::SpecialFeatureObserverRAMOnlyMaxID)
	{
		::Freeze::TransactionHolder tx(m_connection); //exception safe
		ObserverStatsFreezeMap_T::iterator p
			= m_map.find(obserserID);
		if(p ==m_map.end())
		{
			m_map.insert(std::make_pair(obserserID, osi));
		}
		else
		{
			p.set(osi);
		}
		tx.commit();
		return 1;
	}
	else
	{
		return 0;
	}
}

::Ice::Int CObserverStatsDB::setObserverStatsInfo(
	Ice::Int obserserID, const ::iBS::ObserverStatsInfo& osi)
{
	if (obserserID>iBS::SpecialFeatureObserverRAMOnlyMaxID)
	{
		ObserverStatsFreezeMap_T::iterator p
			= m_map.find(obserserID);
		if (p == m_map.end())
		{
			m_map.insert(std::make_pair(obserserID, osi));
		}
		else
		{
			p.set(osi);
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

//////////////////////////////////////////////////////////////////////////
//CObserverStatsHelper


bool CObserverStatsHelper::BasicJobProcessNextBatch(
		CObserverStatsBasicJob& job, const Ice::Double* values, 
		const Ice::Long valueCnt)
{
	if(job.m_processedValueCnt==0)
	{
		Ice::Double val=values[job.m_colIdx];
		
		if(job.m_handleNaN&&val!=val)
		{
			//A NAN will test unequal to any value
			job.m_min=std::numeric_limits< ::Ice::Double >::max();
			job.m_max=std::numeric_limits< ::Ice::Double >::min();
			job.m_sum=0;
			job.m_cnt=0;
		}
		else
		{
			job.m_min=val;
			job.m_max=job.m_min;
			job.m_sum=0;
			job.m_cnt=0;
		}
	}
	//valueCnt=rowCnt*colCnt

	for(int i=job.m_colIdx;i<valueCnt;i+=job.m_colCnt)
	{
		Ice::Double val=values[i];
		job.m_processedValueCnt++;
		if(job.m_handleNaN&&val!=val)
		{
			continue;
		}

		if(job.m_min>val)
		{
			job.m_min=val;
		}
		if(job.m_max<val)
		{
			job.m_max=val;
		}
		job.m_sum+=val;
		job.m_cnt++;
	}
	
	return true;
}

bool CObserverStatsHelper::BasicJobToStats(
	CObserverStatsBasicJob& job, ::iBS::ObserverStatsInfo& osi)
{
	osi.Cnt=job.m_cnt;
	osi.Max=job.m_max;
	osi.Min=job.m_min;
	osi.Sum=job.m_sum;
	return true;
}