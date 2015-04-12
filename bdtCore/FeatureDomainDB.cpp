#include <bdt/bdtBase.h>
#include <FeatureDomainDB.h>
#include <algorithm>


CFeatureDomainDB::CFeatureDomainDB(Ice::CommunicatorPtr communicator, 
							 const std::string& strEnvName,  const std::string& strDBName)
	: m_communicator(communicator),
	m_connection(Freeze::createConnection(communicator, strEnvName)),
	m_map(m_connection, strDBName)
{
	Initilize();
}

CFeatureDomainDB::~CFeatureDomainDB()
{

}

void CFeatureDomainDB::Initilize()
{
	cout<<"CFeatureDomainDB Initilize begin ..."<<endl; 
	//not in service yet, no concurrency issues
	m_domainIDMax=100;
	FeatureDomainFreezeMap_T::const_iterator p;
	int domainCnt =0;
	for(p=m_map.begin(); p!=m_map.end(); ++p)
	{
		domainCnt++;
		const iBS::FeatureDomainInfo& di=p->second;
		if(di.DomainID>m_domainIDMax)
			m_domainIDMax=di.DomainID;
	}
	cout<<"TotalDomain="<<domainCnt<<" Max Domain ID="<<m_domainIDMax<<endl; 
	cout<<"CFeatureDomainDB Initilize End"<<endl;
}

::Ice::Int
CFeatureDomainDB::RqstNewFeatureDomainID(::Ice::Int& domainID)
{
	//ensure thread safty
	IceUtil::Mutex::Lock lock(m_domainMutex);

	//transaction begin
	::Freeze::TransactionHolder tx(m_connection);
	domainID=++m_domainIDMax;

	::iBS::FeatureDomainInfo fdi;
	fdi.DomainID =domainID;
    fdi.DomainName="untitled";
    fdi.DomainSize=0;
	fdi.DomainType=::iBS::FeatureDomainUnknown;
	fdi.Status=::iBS::NodeStatusIDOnly;
	fdi.CreateDT=IceUtil::Time::now().toMilliSeconds();
	fdi.UpdateDT=fdi.CreateDT;
	m_map.insert(std::make_pair(fdi.DomainID, fdi));

	tx.commit();
	//transaction end

    return 1;
}

::Ice::Int 
CFeatureDomainDB::GetFeatureDomain(int domainID, ::iBS::FeatureDomainInfo& domainInfo)
{
	//ensure thread safty
	IceUtil::Mutex::Lock lock(m_domainMutex);

	FeatureDomainFreezeMap_T::const_iterator p
			= m_map.find(domainID);
	if(p !=m_map.end())
	{
		domainInfo = p->second;
		return 1;
	}
	else
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal domain id";
		throw ex;
	}
}

::Ice::Int
CFeatureDomainDB::GetFeatureDomains(const ::iBS::IntVec& domainIDs,
                                           ::iBS::FeatureDomainInfoVec& domainInfos)
{
	//ensure thread safty
	IceUtil::Mutex::Lock lock(m_domainMutex);

	if(domainIDs.empty())
	{
		//out up all 
		FeatureDomainFreezeMap_T::const_iterator p;
		for(p=m_map.begin(); p!=m_map.end(); ++p)
		{
			domainInfos.push_back(p->second);
		}

		//sort data, using default comparison (operator <): i.e., by DomainID
		if(!domainInfos.empty())
		{
			std::sort(domainInfos.begin(),domainInfos.end());
		}
		return 1;
	}
	
	for (size_t i = 0; i < domainIDs.size(); i++)
	{
		FeatureDomainFreezeMap_T::const_iterator p
			= m_map.find(domainIDs[i]);
		if(p !=m_map.end())
		{
			domainInfos.push_back(p->second);
		}
		else
		{
			::iBS::ArgumentException ex;
			ex.reason = "illegal domain id";
			throw ex;
		}
	}

    return 1;
}

::Ice::Int
CFeatureDomainDB::SetFeatureDomains(const ::iBS::FeatureDomainInfoVec& domainInfos)
{
	//ensure thread safty
	IceUtil::Mutex::Lock lock(m_domainMutex);

    ::Freeze::TransactionHolder tx(m_connection);
	::iBS::FeatureDomainInfoVec::const_iterator it;
	for (it =domainInfos.begin(); it!=domainInfos.end(); ++it)
	{
		iBS::FeatureDomainInfo& fdi=const_cast<iBS::FeatureDomainInfo&> (*it);

		FeatureDomainFreezeMap_T::iterator p
			= m_map.find(fdi.DomainID);
		if(p ==m_map.end())
		{
			::iBS::ArgumentException ex;
			ex.reason = "domain not exist, should request it first";
			throw ex;
		}
		else
		{
			//assume the caller to set the status
			//fdi.Status=::iBS::NodeStatusUploaded;
			fdi.UpdateDT=IceUtil::Time::now().toMilliSeconds();
			
			p.set(fdi);
		}
	}

	tx.commit();
    return 1;
}
