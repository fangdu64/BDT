#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <FeatureDomainDB.h>
#include <FeatureObserverDB.h>
#include <FeatureValueRAM.h>
#include <GlobalVars.h>
#include <algorithm>    // std::copy
#include <FeatureValueStoreMgr.h>

CFeatureValueRAMDouble::CFeatureValueRAMDouble(CGlobalVars& globalVars)
	:m_gv(globalVars)
{
	m_doubleArrays.resize(m_gv.theObserversDB.theMaxAllowedObserverID,0);
}

CFeatureValueRAMDouble::~CFeatureValueRAMDouble()
{
	for(int i=0;i<m_gv.theObserversDB.theMaxAllowedObserverID;i++)
	{
		DoubleArrayPtr_T ptr=m_doubleArrays[i];
		if(ptr!=0)
		{
			delete[] ptr;
		}
	}
}

void CFeatureValueRAMDouble::SetFeatureValuesInRAM(
		iBS::FeatureObserverSimpleInfoPtr& foi,
		::Ice::Int observerID,
		::Ice::Long featureIdxFrom,
		::Ice::Long featureIdxTo,
		const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values)
{

	//calling from worker  thread, concurrency already handled

	if(m_doubleArrays[observerID]==0)
	{
		m_doubleArrays[observerID] = m_gv.theFeatureValueStoreMgr->LoadFeatureValuesFromStore(foi);
	}

	//do not need to consider ENDIAN, as the ICE runtime already handled that
	//i.e., the array is already in the same endianess as the machine

	std::copy(values.first, values.second, m_doubleArrays[observerID]+featureIdxFrom);
}


void CFeatureValueRAMDouble::UpdateFeatureValuesIfLoaded(
		iBS::FeatureObserverSimpleInfoPtr& foi,
		::Ice::Int observerID,
		::Ice::Long featureIdxFrom,
		::Ice::Long featureIdxTo,
		const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values)
{

	//calling from worker  thread, concurrency already handled

	if(m_doubleArrays[observerID]==0)
	{
		return;
	}
	else
	{
		//do not need to consider ENDIAN, as the ICE runtime already handled that
		//i.e., the array is already in the same endianess as the machine
		std::copy(values.first, values.second, m_doubleArrays[observerID]+featureIdxFrom);
	}

}

void
CFeatureValueRAMDouble::GetFeatureValuesLoadAllToRAM(
	iBS::FeatureObserverSimpleInfoPtr& foi,
    ::Ice::Int observerID,
    ::Ice::Long featureIdxFrom,
    ::Ice::Long featureIdxTo,
	::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values)
{
	//not consider thread-safe yet
	if(m_doubleArrays[observerID]==0)
	{
		if(foi->StorePolicy==iBS::FeatureValueStorePolicyInRAMNoSave)
		{
			values.first = 0;
			values.second = 0;
			return;
		}

		//load data
		m_doubleArrays[observerID] = m_gv.theFeatureValueStoreMgr->LoadFeatureValuesFromStore(foi);

	}

	//do not need to consider ENDIAN, as the ICE runtime has already handled that
	//i.e., the array will always changed to little-endian (if needed)

	values.first = m_doubleArrays[observerID]+featureIdxFrom;
	values.second = m_doubleArrays[observerID] + featureIdxTo;
	

}


::Ice::Double* 
CFeatureValueRAMDouble::GetValuesInRAM(::Ice::Int observerID)
{
	return  m_doubleArrays[observerID];
}

::Ice::Double* 
CFeatureValueRAMDouble::GetValuesInRAM(const iBS::FeatureObserverSimpleInfoPtr& foi)
{
	::Ice::Int observerID=foi->ObserverID;
	if(foi->ObserverGroupSize>1)
	{
		observerID=foi->ObserverGroupID;
	}
	return  m_doubleArrays[observerID];
}


