
#ifndef __FeatureObserverDB_h__
#define __FeatureObserverDB_h__

#include <IceUtil/Mutex.h>
#include <Ice/Ice.h>
#include <Freeze/Freeze.h>
#include <FCDCentralService.h>
#include <FCDCentralFreeze.h>
#include <FeatureObserverFreezeMap.h>

typedef iBS::CFeatureObserverFreezeMap FeatureObserverFreezeMap_T;

class CFeatureObserverDB
{
public:
	CFeatureObserverDB(Ice::CommunicatorPtr communicator, const std::string& strEnvName, 
		const std::string& strDBName);

	~CFeatureObserverDB();

public:
	::Ice::Int RqstNewFeatureObserverID(::Ice::Int& observerID, bool inRAM=false);

	::Ice::Int RqstNewFeatureObserversInGroup(::Ice::Int groupSize, ::iBS::IntVec& observerIDs, bool inRAM);



	::Ice::Int GetFeatureObservers(const ::iBS::IntVec& observerIDs, iBS::FeatureObserverSimpleInfoVec& fois);

	iBS::FeatureObserverSimpleInfoPtr GetFeatureObserver(int observerID);

	iBS::FeatureObserverSimpleInfoPtr GetFeatureObserverGroup(int observerGroupID);

	::Ice::Int GetFeatureObservers(const ::iBS::IntVec& observerIDs,
                                           ::iBS::FeatureObserverInfoVec& observerInfos);

	::Ice::Int SetFeatureObservers(const ::iBS::FeatureObserverInfoVec& observerInfos);

	bool IsObserverConfiguredToLoadAllToContinuesRAM(int observerID);

	::Ice::Int RemoveFeatureObservers(const ::iBS::IntVec& observerIDs);
	
private:

	void Initilize();
	void SetFeatureObserverInfoDefault(::iBS::FeatureObserverInfo& foi);

	iBS::FeatureObserverSimpleInfoPtr getRAMFeatureObserverGroup(int observerGroupID);

	::Ice::Int getFeatureObserver(Ice::Int obserserID, ::iBS::FeatureObserverInfo& observerInfo);

	::Ice::Int setFeatureObserver(Ice::Int obserserID, const ::iBS::FeatureObserverInfo& observerInfo);

	iBS::FeatureObserverSimpleInfoPtr getFeatureObserver(int observerID);

	iBS::FeatureObserverSimpleInfoPtr getFeatureObserverGroup(int observerGroupID);

public:
	const int theMaxAllowedObserverID;
	const int theMaxThreadRandomIdx;

private:
	Ice::CommunicatorPtr		m_communicator;
	Freeze::ConnectionPtr		m_connection;
	FeatureObserverFreezeMap_T	m_map;
	int							m_observerIDMax; //current max observer ID
	int							m_ramObserverIDMax;//current max RAM observer ID
	IceUtil::Mutex				m_observerMutex;

	typedef ::std::vector< iBS::FeatureObserverInfo* > FeatureObserverInfoPtrList_T;

	FeatureObserverInfoPtrList_T m_ramObserverInfos;
	
};

#endif
