#ifndef __FeatureDomainDB_h__
#define __FeatureDomainDB_h__

#include <IceUtil/Mutex.h>
#include <Ice/Ice.h>
#include <Freeze/Freeze.h>
#include <FCDCentralService.h>
#include <FCDCentralFreeze.h>
#include <FeatureDomainFreezeMap.h>


typedef iBS::CFeatureDomainFreezeMap FeatureDomainFreezeMap_T;


class CFeatureDomainDB
{
public:
	CFeatureDomainDB(Ice::CommunicatorPtr communicator, const std::string& strEnvName, 
		const std::string& strDBName);

	~CFeatureDomainDB();

public:
	::Ice::Int RqstNewFeatureDomainID(::Ice::Int& domainID);

	::Ice::Int GetFeatureDomain(int domainID, ::iBS::FeatureDomainInfo& domainInfo);
	::Ice::Int GetFeatureDomains(const ::iBS::IntVec& domainIDs,
                                           ::iBS::FeatureDomainInfoVec& domainInfos);
	::Ice::Int SetFeatureDomains(const ::iBS::FeatureDomainInfoVec& domainInfos);
private:

	void Initilize();
private:
	Ice::CommunicatorPtr		m_communicator;
	Freeze::ConnectionPtr		m_connection;
	FeatureDomainFreezeMap_T	m_map;
	int							m_domainIDMax; //current max domainID
	IceUtil::Mutex				m_domainMutex;
};

#endif
