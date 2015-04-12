#ifndef __GlobalVars_h__
#define __GlobalVars_h__

#include <Ice/Ice.h>
class CFeatureObserverDB;
class CFeatureDomainDB;
class CFeatureValueRAMDouble;
class CFeatureValueWorkerMgr;
class CFeatureValueStoreMgr;
class CGlobalVars;
class CObserverStatsDB;
class CObserverIndexDB;

namespace iBSInternal
{
	namespace GlobalVars
	{
		 extern CGlobalVars *_ptrGV;
	}
}

class CGlobalVars
{
public:
	CGlobalVars(
		Ice::CommunicatorPtr communicator, 
		CFeatureDomainDB& domainsDB, 
		CFeatureObserverDB& observersDB,
		CFeatureValueRAMDouble* pFeatureValueRAMDobule,
		CFeatureValueWorkerMgr* pFeatureValueWorkerMgr,
		CFeatureValueStoreMgr* pFeatureValueStoreMgr)
		:theCommunicator(communicator),
		theDomainsDB(domainsDB), theObserversDB(observersDB),
		theFeatureValueRAMDobule(pFeatureValueRAMDobule),
		theFeatureValueWorkerMgr(pFeatureValueWorkerMgr),
		theFeatureValueStoreMgr(pFeatureValueStoreMgr)
	{
		theObserverStatsDB=0;
		theObserverIndexDB=0;
		theMaxFeatureValueFileSize=(((Ice::Long)1024)*1024*256*4);
		theMaxFeatureValueCntDouble=theMaxFeatureValueFileSize/sizeof(Ice::Double);
		theIceMessageSizeMax=0;
		iBSInternal::GlobalVars::_ptrGV = this;
		theMaxAllowedValueCntInRAMPerObserver=0;
	}

	static CGlobalVars* get();

	int GetUniformInt(int valFrom, int valTo);

public:
	Ice::CommunicatorPtr theCommunicator;
	CFeatureDomainDB&	theDomainsDB;
	CFeatureObserverDB& theObserversDB;
	CFeatureValueRAMDouble* theFeatureValueRAMDobule;
	CFeatureValueWorkerMgr* theFeatureValueWorkerMgr;
	CFeatureValueStoreMgr* theFeatureValueStoreMgr;
	CObserverStatsDB* theObserverStatsDB;
	CObserverIndexDB* theObserverIndexDB;
	
	Ice::Long theMaxFeatureValueCntDouble; //1G double value cnt
	Ice::Long theMaxFeatureValueFileSize; //1G in Byte
	Ice::Long theIceMessageSizeMax; //in bytes

	Ice::Long theMaxAllowedValueCntInRAMPerObserver; //0-no limit
};

#endif
