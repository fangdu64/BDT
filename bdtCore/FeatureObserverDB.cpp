#include <bdt/bdtBase.h>
#include <IceUtil/Random.h>
#include <FeatureObserverDB.h>
#include <GlobalVars.h>


CFeatureObserverDB::CFeatureObserverDB(Ice::CommunicatorPtr communicator, 
							 const std::string& strEnvName,  const std::string& strDBName)
	:theMaxAllowedObserverID(1000*1000),
	theMaxThreadRandomIdx(256),
	m_communicator(communicator),
	m_connection(Freeze::createConnection(communicator, strEnvName)),
	m_map(m_connection, strDBName)
{
	Initilize();
}

CFeatureObserverDB::~CFeatureObserverDB()
{
	//release in RAM observers
	for(int i=0;i<m_ramObserverIDMax;i++)
	{
		::iBS::FeatureObserverInfo* ptr=m_ramObserverInfos[i];
		if(ptr!=0)
		{
			delete ptr;
		}
	}
}

void CFeatureObserverDB::Initilize()
{
	cout<<"CFeatureObserverDB Initilize begin ..."<<endl; 
	//not in service yet, no concurrency issues
	
	//init RAM observers
	m_ramObserverInfos.resize(iBS::SpecialFeatureObserverRAMOnlyMaxID+1,0);
	m_ramObserverIDMax = iBS::SpecialFeatureObserverRAMOnlyMinID;

	m_observerIDMax=iBS::SpecialFeatureObserverMaxID;

	FeatureObserverFreezeMap_T::const_iterator p;
	int featureCnt=0;
	for(p=m_map.begin(); p!=m_map.end(); ++p)
	{
		featureCnt++;
		const iBS::FeatureObserverInfo& di=p->second;
		if(di.ObserverID>m_observerIDMax)
			m_observerIDMax=di.ObserverID;
	}
	cout<<"TotalObserver="<<featureCnt<<", Max ObserverID="<<m_observerIDMax<<endl;
	cout<<"CFeatureObserverDB Initilize End"<<endl;
}

void CFeatureObserverDB::SetFeatureObserverInfoDefault(::iBS::FeatureObserverInfo& foi)
{
	foi.ObserverID = 0;
    foi.ObserverName="untitled";
	foi.ContextName="";
    foi.DomainID =0;
	foi.ValueType = iBS::FeatureValueDouble;
	foi.StorePolicy=iBS::FeatureValueStorePolicyBinaryFilesSingleObserver;
    foi.Status = ::iBS::NodeStatusIDOnly;
	foi.GetPolicy = ::iBS::FeatureValueGetPolicyGetForOneTimeRead;
	foi.SetPolicy = ::iBS::FeatureValueSetPolicyNoRAMImmediatelyToStore;
	foi.ThreadRandomIdx = 0;
    foi.ObserverGroupID=-1;
	foi.ObserverGroupSize=1;
    foi.IdxInObserverGroup=0;
	foi.CreateDT=IceUtil::Time::now().toMilliSeconds();
	foi.UpdateDT=foi.CreateDT;
	foi.ParentObserverID=0;
	foi.MapbackObserverID=0;
    foi.Version=0;
	foi.StoreLocation = ::iBS::FeatureValueStoreLocationDefault;

}

::Ice::Int
CFeatureObserverDB::RqstNewFeatureObserverID(::Ice::Int& observerID, bool inRAM)
{
	//ensure thread safty
	IceUtil::Mutex::Lock lock(m_observerMutex);
	
	if(inRAM)
	{
		observerID=++m_ramObserverIDMax;
		if(observerID>=iBS::SpecialFeatureObserverRAMOnlyMaxID)
		{
			return 0;
		}

		::iBS::FeatureObserverInfo *ptr=new ::iBS::FeatureObserverInfo ();
		if(!ptr){
			return 0;
		}
		::iBS::FeatureObserverInfo& foi=*ptr;

		SetFeatureObserverInfoDefault(foi);
		foi.ObserverID = observerID;
		foi.StorePolicy=iBS::FeatureValueStorePolicyInRAMNoSave;
		foi.GetPolicy = iBS::FeatureValueGetPolicyGetFromRAM;
		foi.SetPolicy = iBS::FeatureValueSetPolicyInRAMNoSave;
		
		foi.ThreadRandomIdx = IceUtilInternal::random(theMaxThreadRandomIdx);
		m_ramObserverInfos[observerID]=ptr;
	}
	else
	{
		::Freeze::TransactionHolder tx(m_connection); //exception safe
		observerID=++m_observerIDMax;

		::iBS::FeatureObserverInfo foi;
		SetFeatureObserverInfoDefault(foi);
		foi.ObserverID = observerID;
		foi.ThreadRandomIdx = IceUtilInternal::random(theMaxThreadRandomIdx);
		m_map.insert(std::make_pair(foi.ObserverID, foi));
		tx.commit();
	}
    return 1;
}

::Ice::Int
CFeatureObserverDB::RqstNewFeatureObserversInGroup(::Ice::Int groupSize, ::iBS::IntVec& observerIDs, bool inRAM)
{
	//ensure thread safty
    IceUtil::Mutex::Lock lock(m_observerMutex);

	if(groupSize<1)
	{
		return 0;
	}

	//request a new feature group id
	int groupID=0;

	if(inRAM)
	{
		int threadRandomIdx = IceUtilInternal::random(theMaxThreadRandomIdx);
		for(int i=0; i<groupSize;i++)
		{
			int observerID=++m_ramObserverIDMax;
			if(observerID>=iBS::SpecialFeatureObserverRAMOnlyMaxID)
			{
				return 0;
			}

			::iBS::FeatureObserverInfo *ptr=new ::iBS::FeatureObserverInfo ();
			::iBS::FeatureObserverInfo& foi=*ptr;
			SetFeatureObserverInfoDefault(foi);
			foi.ObserverID = observerID;
			if(i==0)
			{
				//ObserverGroupID = first ObservereID inGroup
				groupID = observerID;
			}
			foi.ObserverGroupID = groupID;
			foi.ObserverGroupSize = groupSize;
			foi.IdxInObserverGroup = i;
			foi.ThreadRandomIdx = threadRandomIdx; //observer in group will live in the same thread
			foi.StorePolicy=::iBS::FeatureValueStorePolicyInRAMNoSave;
			foi.GetPolicy = iBS::FeatureValueGetPolicyGetFromRAM;
			foi.SetPolicy = iBS::FeatureValueSetPolicyInRAMNoSave;

			m_ramObserverInfos[observerID]=ptr;

			observerIDs.push_back(observerID);
		}
		return 1;
	}
	else
	{
		::Freeze::TransactionHolder tx(m_connection); //exception safe
		//request a new feature group id
		int groupID=0;
		//each shares the same thred idx
		int threadRandomIdx = IceUtilInternal::random(theMaxThreadRandomIdx);
		for(int i=0; i<groupSize;i++)
		{
			int observerID=++m_observerIDMax;
		
			::iBS::FeatureObserverInfo foi;
			SetFeatureObserverInfoDefault(foi);
			foi.ObserverID = observerID;
			if(i==0)
			{
				//ObserverGroupID = first ObservereID inGroup
				groupID = observerID;
			}
			foi.ObserverGroupID = groupID;
			foi.ObserverGroupSize = groupSize;
			foi.IdxInObserverGroup = i;
			foi.ThreadRandomIdx = threadRandomIdx; //observer in group will live in the same thread
			foi.StorePolicy=::iBS::FeatureValueStorePolicyBinaryFilesObserverGroup;
			m_map.insert(std::make_pair(foi.ObserverID, foi));
			observerIDs.push_back(observerID);
		}
		tx.commit();
		return 1;
	}
    
}


::Ice::Int CFeatureObserverDB::GetFeatureObservers(const ::iBS::IntVec& observerIDs, iBS::FeatureObserverSimpleInfoVec& fois)
{
	//public function, ensure thread safty
	IceUtil::Mutex::Lock lock(m_observerMutex);

	for (size_t i = 0; i < observerIDs.size(); i++)
	{
		fois.push_back(getFeatureObserver(observerIDs[i]));
	}
	return 1;
}

iBS::FeatureObserverSimpleInfoPtr CFeatureObserverDB::GetFeatureObserver(int observerID)
{
	//public function, ensure thread safty
	IceUtil::Mutex::Lock lock(m_observerMutex);

	return getFeatureObserver(observerID);
}

iBS::FeatureObserverSimpleInfoPtr 
	CFeatureObserverDB::getFeatureObserver(int observerID)
{
	//private function, should already lock should already be acquired by the caller

	if(observerID>iBS::SpecialFeatureObserverRAMOnlyMaxID)
	{
		FeatureObserverFreezeMap_T::const_iterator p
				= m_map.find(observerID);
		if(p !=m_map.end())
		{
			::iBS::FeatureObserverSimpleInfoPtr foiPtr
					=new ::iBS::FeatureObserverSimpleInfo();
			const ::iBS::FeatureObserverInfo& foi =p->second;

			foiPtr->ObserverID=foi.ObserverID;
			foiPtr->DomainID=foi.DomainID;
			foiPtr->DomainSize=foi.DomainSize;
			foiPtr->ValueType=foi.ValueType;
			foiPtr->StorePolicy=foi.StorePolicy;
			foiPtr->GetPolicy=foi.GetPolicy;
			foiPtr->SetPolicy=foi.SetPolicy;
			foiPtr->Status=foi.Status;
			foiPtr->ThreadRandomIdx=foi.ThreadRandomIdx;
			foiPtr->ObserverGroupID=foi.ObserverGroupID;
			foiPtr->ObserverGroupSize=foi.ObserverGroupSize;
			foiPtr->IdxInObserverGroup=foi.IdxInObserverGroup;
			foiPtr->StoreLocation = foi.StoreLocation;
			foiPtr->SpecifiedPathPrefix = foi.SpecifiedPathPrefix;

			//overwrite GetPolicy
			Ice::Long maxAllowedValueCntInRAM=CGlobalVars::get()->theMaxAllowedValueCntInRAMPerObserver;
			Ice::Long foiStoreDomainSize = foiPtr->ObserverGroupSize*foiPtr->DomainSize;
			if(maxAllowedValueCntInRAM>0
			&& (foiStoreDomainSize>maxAllowedValueCntInRAM))
			{
				//exceed the upper limit, not to load in RAM
				foiPtr->GetPolicy=iBS::FeatureValueGetPolicyGetForOneTimeRead;
			}
			

			return foiPtr;
		}
		else
		{
			return 0;
		
		}
	}
	else if(m_ramObserverInfos[observerID])
	{
		//RAM observer

		::iBS::FeatureObserverSimpleInfoPtr foiPtr
				=new ::iBS::FeatureObserverSimpleInfo();
		const ::iBS::FeatureObserverInfo& foi =*m_ramObserverInfos[observerID];

		foiPtr->ObserverID=foi.ObserverID;
		foiPtr->DomainID=foi.DomainID;
		foiPtr->DomainSize=foi.DomainSize;
		foiPtr->ValueType=foi.ValueType;
		foiPtr->StorePolicy=foi.StorePolicy;
		foiPtr->GetPolicy=foi.GetPolicy;
		foiPtr->SetPolicy=foi.SetPolicy;
		foiPtr->Status=foi.Status;
		foiPtr->ThreadRandomIdx=foi.ThreadRandomIdx;
		foiPtr->ObserverGroupID=foi.ObserverGroupID;
		foiPtr->ObserverGroupSize=foi.ObserverGroupSize;
		foiPtr->IdxInObserverGroup=foi.IdxInObserverGroup;
		foiPtr->StoreLocation = foi.StoreLocation;
		foiPtr->SpecifiedPathPrefix = foi.SpecifiedPathPrefix;

		return foiPtr;

	}
	else
	{
		return 0;
	}
}

iBS::FeatureObserverSimpleInfoPtr 
	CFeatureObserverDB::getRAMFeatureObserverGroup(int observerGroupID)
{
	int observerID = observerGroupID; //groupID is the first observerID in group
	
	if(observerID>=iBS::SpecialFeatureObserverRAMOnlyMaxID)
		return 0;

	if(m_ramObserverInfos[observerID])
	{
		::iBS::FeatureObserverSimpleInfoPtr foiPtr
				=new ::iBS::FeatureObserverSimpleInfo();
		const ::iBS::FeatureObserverInfo& foi =*m_ramObserverInfos[observerID];

		foiPtr->ObserverID=foi.ObserverID;
		foiPtr->DomainID=foi.DomainID;
		//adjust domain size 
		foiPtr->DomainSize=foi.DomainSize*foi.ObserverGroupSize;

		foiPtr->ValueType=foi.ValueType;
		foiPtr->StorePolicy=foi.StorePolicy;
		foiPtr->GetPolicy=foi.GetPolicy;
		foiPtr->SetPolicy=foi.SetPolicy;
		foiPtr->Status=foi.Status;
		foiPtr->ThreadRandomIdx=foi.ThreadRandomIdx;
		foiPtr->ObserverGroupID=foi.ObserverGroupID;
		foiPtr->ObserverGroupSize=foi.ObserverGroupSize;//still can use ObserverGroupSize to determine rowBytes size
		foiPtr->IdxInObserverGroup=foi.IdxInObserverGroup;
		foiPtr->StoreLocation = foi.StoreLocation;
		foiPtr->SpecifiedPathPrefix = foi.SpecifiedPathPrefix;

		return foiPtr;
	}
	else
	{
		return 0;
		
	}
}

iBS::FeatureObserverSimpleInfoPtr 
	CFeatureObserverDB::GetFeatureObserverGroup(int observerGroupID)
{
	IceUtil::Mutex::Lock lock(m_observerMutex);
	return getFeatureObserverGroup(observerGroupID);
}

iBS::FeatureObserverSimpleInfoPtr 
	CFeatureObserverDB::getFeatureObserverGroup(int observerGroupID)
{
	
	int observerID = observerGroupID; //groupID is the first observerID in group
	
	if(observerID<iBS::SpecialFeatureObserverRAMOnlyMaxID)
	{
		return getRAMFeatureObserverGroup(observerGroupID);
	}

	FeatureObserverFreezeMap_T::const_iterator p
			= m_map.find(observerID);
	if(p !=m_map.end())
	{
		::iBS::FeatureObserverSimpleInfoPtr foiPtr
				=new ::iBS::FeatureObserverSimpleInfo();
		const ::iBS::FeatureObserverInfo& foi =p->second;

		foiPtr->ObserverID=foi.ObserverID;
		foiPtr->DomainID=foi.DomainID;
		//adjust domain size 
		foiPtr->DomainSize=foi.DomainSize*foi.ObserverGroupSize;

		foiPtr->ValueType=foi.ValueType;
		foiPtr->StorePolicy=foi.StorePolicy;
		foiPtr->GetPolicy=foi.GetPolicy;
		foiPtr->SetPolicy=foi.SetPolicy;
		foiPtr->Status=foi.Status;
		foiPtr->ThreadRandomIdx=foi.ThreadRandomIdx;
		foiPtr->ObserverGroupID=foi.ObserverGroupID;
		foiPtr->ObserverGroupSize=foi.ObserverGroupSize;//still can use ObserverGroupSize to determine rowBytes size
		foiPtr->IdxInObserverGroup=foi.IdxInObserverGroup;
		foiPtr->StoreLocation = foi.StoreLocation;
		foiPtr->SpecifiedPathPrefix = foi.SpecifiedPathPrefix;

		//overwrite GetPolicy
		Ice::Long maxAllowedValueCntInRAM=CGlobalVars::get()->theMaxAllowedValueCntInRAMPerObserver;
		Ice::Long foiStoreDomainSize = foiPtr->ObserverGroupSize*foiPtr->DomainSize;
		if(maxAllowedValueCntInRAM>0
		&& (foiStoreDomainSize>maxAllowedValueCntInRAM))
		{
			//exceed the upper limit, not to load in RAM
			foiPtr->GetPolicy=iBS::FeatureValueGetPolicyGetForOneTimeRead;
		}

		return foiPtr;
	}
	else
	{
		return 0;
		
	}
}


::Ice::Int
CFeatureObserverDB::RemoveFeatureObservers(const ::iBS::IntVec& observerIDs)
{
	IceUtil::Mutex::Lock lock(m_observerMutex);
	bool removed = false;
	for (size_t i = 0; i < observerIDs.size(); i++)
	{
		int observerID = observerIDs[i];
		FeatureObserverFreezeMap_T::iterator p
			= m_map.find(observerID);
		if (p != m_map.end())
		{
			m_map.erase(p);
			removed = true;
		}
	}
	if (removed)
	{
		FeatureObserverFreezeMap_T::const_iterator p;
		m_observerIDMax = iBS::SpecialFeatureObserverMaxID;
		for (p = m_map.begin(); p != m_map.end(); ++p)
		{
			const iBS::FeatureObserverInfo& di = p->second;
			if (di.ObserverID>m_observerIDMax)
				m_observerIDMax = di.ObserverID;
		}
	}

	return 1;
}

::Ice::Int
CFeatureObserverDB::GetFeatureObservers(const ::iBS::IntVec& observerIDs,
                                           ::iBS::FeatureObserverInfoVec& observerInfos)
{
	IceUtil::Mutex::Lock lock(m_observerMutex);

	if(observerIDs.empty())
	{
		//RAM obsrevers
		for(int obsIdx=iBS::SpecialFeatureObserverRAMOnlyMinID;
			obsIdx<m_ramObserverIDMax;obsIdx++)
		{
			if(m_ramObserverInfos[obsIdx])
			{
				observerInfos.push_back(*m_ramObserverInfos[obsIdx]);
			}
		}

		//Saved  obsrevers
		FeatureObserverFreezeMap_T::const_iterator p;
		for(p=m_map.begin(); p!=m_map.end(); ++p)
		{
			observerInfos.push_back(p->second);
		}

		//sort data, using default comparison (operator <): i.e., by observerID
		if(!observerInfos.empty())
		{
			std::sort(observerInfos.begin(),observerInfos.end());
		}

		return 1;
	}
	
	for (size_t i = 0; i < observerIDs.size(); i++)
	{
		::iBS::FeatureObserverInfo foi;
		getFeatureObserver(observerIDs[i],foi);
		observerInfos.push_back(foi);
	}

    return 1;
}

::Ice::Int CFeatureObserverDB::getFeatureObserver(
	Ice::Int obserserID, ::iBS::FeatureObserverInfo& observerInfo)
{
	if(obserserID>iBS::SpecialFeatureObserverRAMOnlyMaxID)
	{
		FeatureObserverFreezeMap_T::const_iterator p
			= m_map.find(obserserID);
		if(p !=m_map.end())
		{
			observerInfo=p->second;
			return 1;
		}
		else
		{
			::iBS::ArgumentException ex;
			ex.reason = "illegal observer id";
			throw ex;
		}
	}
	else if(m_ramObserverInfos[obserserID])
	{
		observerInfo=*m_ramObserverInfos[obserserID];
		return 1;
	}
	else
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal observer id";
		throw ex;
	}
}

::Ice::Int CFeatureObserverDB::setFeatureObserver(
	Ice::Int obserserID, const ::iBS::FeatureObserverInfo& observerInfo)
{
	if(obserserID>iBS::SpecialFeatureObserverRAMOnlyMaxID)
	{
		
		FeatureObserverFreezeMap_T::iterator p
			= m_map.find(obserserID);
		if(p ==m_map.end())
		{
			::iBS::ArgumentException ex;
			ex.reason = "observer not exist, should request it first";
			throw ex;
		}
		else
		{
			//assume the caller to set the status
			//fdi.Status=::iBS::NodeStatusUploaded;
			iBS::FeatureObserverInfo& foi=const_cast<iBS::FeatureObserverInfo&> (observerInfo);
			foi.UpdateDT=IceUtil::Time::now().toMilliSeconds();
			
			p.set(foi);
		}
		
		return 1;
	}
	else if(m_ramObserverInfos[obserserID])
	{
		iBS::FeatureObserverInfo& foi = *m_ramObserverInfos[obserserID];
		foi=observerInfo;
		foi.StorePolicy=::iBS::FeatureValueStorePolicyInRAMNoSave;
		foi.GetPolicy = iBS::FeatureValueGetPolicyGetFromRAM;
		foi.SetPolicy = iBS::FeatureValueSetPolicyInRAMNoSave;
		return 1;
	}
	else
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal observer id";
		throw ex;
	}
}

::Ice::Int
CFeatureObserverDB::SetFeatureObservers(const ::iBS::FeatureObserverInfoVec& observerInfos)
{
	IceUtil::Mutex::Lock lock(m_observerMutex);

    ::Freeze::TransactionHolder tx(m_connection); //exception safe
	::iBS::FeatureObserverInfoVec::const_iterator it;

	for (it =observerInfos.begin(); it!=observerInfos.end(); ++it)
	{
		iBS::FeatureObserverInfo& foi=const_cast<iBS::FeatureObserverInfo&> (*it);
		setFeatureObserver(foi.ObserverID,foi);
	}

	tx.commit();
    return 1;
}

bool CFeatureObserverDB::IsObserverConfiguredToLoadAllToContinuesRAM(int observerID)
{
	//thread safty ensured by sub functions

	iBS::FeatureObserverSimpleInfoPtr foi=GetFeatureObserver(observerID);
	if(!foi)
	{
		return false;
	}

	if(foi->StorePolicy==iBS::FeatureValueStorePolicyInRAMNoSave)
		return true;


	Ice::Long domainSize=foi->DomainSize;
	
	::iBS::FeatureValueGetPolicyEnum getPolicy=foi->GetPolicy;

	if(foi->ObserverGroupSize>1)
	{
		int groupID =foi->ObserverGroupID;
		iBS::FeatureObserverSimpleInfoPtr goi= GetFeatureObserverGroup(groupID);
		domainSize=goi->DomainSize;
		getPolicy=goi->GetPolicy;
	}


	if(domainSize<=CGlobalVars::get()->theMaxFeatureValueCntDouble
		&& getPolicy==iBS::FeatureValueGetPolicyGetFromRAM)
	{
		return true;
	}

	return false;
}