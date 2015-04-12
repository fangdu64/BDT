#include <bdt/bdtBase.h>
#include <Ice/Ice.h>
#include <Freeze/Freeze.h>
#include <GlobalVars.h>
#include <bdvdGlobalVars.h>
#include <FCDCentralServiceImpl.h>
#include <FacetAdminImpl.h>
#include <bdvdFacetAdminImpl.h>
#include <FCDCFacetImpI.h>
#include <ProxyCentralServiceImpl.h>
#include <FeatureDomainDB.h>
#include <FeatureObserverDB.h>
#include <ObserverStatsDB.h>
#include <ObserverIndexDB.h>
#include <RUVDB.h>
#include <FeatureValueRAM.h>
#include <FeatureValueWorkerMgr.h>
#include <FeatureValueStoreMgr.h>
#include <ComputeServiceImpl.h>

class CBdvdServer : public Ice::Application
{
public:
	CBdvdServer()
	{
	}

	virtual int run(int argc, char* argv[]);
	virtual void interruptCallback(int);
};

int main(int argc, char* argv[])
{
	CBdvdServer app;
	
	//FCDCentralServer.config is the default properties
	//values can be overwrite by commond line, e.g., --Ice.Config=FCDCentralServer1.config
	return app.main(argc, argv, "FCDCentralServer.config");
}

void
CBdvdServer::interruptCallback(int)
{
	if(CGlobalVars::get()&&CGlobalVars::get()->theFeatureValueWorkerMgr)
	{
		CGlobalVars::get()->theFeatureValueWorkerMgr->RequestShutdownAllWorkers();
	}
    communicator()->shutdown();
}

int CBdvdServer::run(int argc, char* argv[])
{

	 callbackOnInterrupt();

	Ice::PropertiesPtr properties = communicator()->getProperties();

	Ice::StringSeq options=properties->getCommandLineOptions();

	std::string strEnvName = "FCDCentralDBDefault";
	std::string strDBName = properties->getProperty("FeatureDomainDBName");
	
	//init feature domains
	CFeatureDomainDB domainsDb(communicator(), strEnvName,strDBName);

	//init feature observers
	strDBName = properties->getProperty("FeatureObserverDBName");
	CFeatureObserverDB observersDb(communicator(), strEnvName,strDBName);
	
	strDBName = properties->getProperty("ObserverStatsDBName");
	CObserverStatsDB observerStatsDb(communicator(), strEnvName,strDBName);

	strDBName = properties->getProperty("ObserverIndexDBName");
	CObserverIndexDB observerIndexDb(communicator(), strEnvName,strDBName);

	strDBName = properties->getProperty("RUVFacetDBName");
	CRUVFacetDB ruvFacetDb(communicator(), strEnvName, strDBName);

	CGlobalVars gVars(communicator(),domainsDb,
				observersDb, NULL, NULL, NULL);

	CBdvdGlobalVars gBdvdVars;
	
	
	gVars.theIceMessageSizeMax=properties->getPropertyAsIntWithDefault("Ice.MessageSizeMax",16384);//in kilobytes
	gVars.theIceMessageSizeMax=gVars.theIceMessageSizeMax*1024-10240;//leave some space for protocal

	gVars.theMaxAllowedValueCntInRAMPerObserver
		=properties->getPropertyAsIntWithDefault("FeatureValueRAM.MaxAllowedValueCntPerObserver",0);//in kilobytes
	gVars.theMaxAllowedValueCntInRAMPerObserver*=1024;

	gVars.theObserverStatsDB = &observerStatsDb;
	gVars.theObserverIndexDB = &observerIndexDb;

	//big RAM for feature values, allocate on demand
	CFeatureValueRAMDouble featureValueRAMDouble(gVars);
	gVars.theFeatureValueRAMDobule = &featureValueRAMDouble;

	gBdvdVars.theRUVFacetDB = &ruvFacetDb;
	

	//workder threads to process feature value get/set
	//use observer's thread radomn idx to ensure  an observer be processed always in one thread
	
	int workerThreadNum=properties->getPropertyAsIntWithDefault("FeatureValueWorker.Size",4);

	CFeatureValueWorkerMgr featureValueWorkerMgr(gVars, workerThreadNum);
	gVars.theFeatureValueWorkerMgr=&featureValueWorkerMgr;

	CFeatureValueStoreMgr featureValueStoreMgr(
		gVars.theMaxFeatureValueFileSize,properties->getProperty("FeatureValueStore.RootDir"));
	gVars.theFeatureValueStoreMgr = &featureValueStoreMgr;

	//start worker threads
	featureValueWorkerMgr.Initilize();

	Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("AdptFCDCentralServer");

	Ice::Identity id;
	CFcdcAdminServiceImplPtr fcdcService = new CFcdcAdminServiceImpl(gVars);
	id.name="FCDCentralService";
	adapter->add(fcdcService, id);

	FCDCFacetDivideByColumnSumImplPtr fcdcService_divideByColumnSum
		=new CFCDCFacetDivideByColumnSumImpl(gVars);
	adapter->addFacet(fcdcService_divideByColumnSum, id, iBS::FcdcFacetNameDivideByColumnSum);

	CProxyCentralServiceImplPtr proxyCentralService= new CProxyCentralServiceImpl(gVars);
	id.name="ProxyCentralService";
	adapter->add(proxyCentralService, id);

	CFcdcFacetAdminServiceImplPtr facetAdminService= new CFcdcFacetAdminServiceImpl();
	id.name="FcdcFacetAdminService";
	adapter->add(facetAdminService, id);

	CBdvdFacetAdminServiceImplPtr bdvdFacetAdminService = new CBdvdFacetAdminServiceImpl();
	id.name = "BdvdFacetAdminService";
	adapter->add(bdvdFacetAdminService, id);

	CComputeServiceImplPtr computeService = new CComputeServiceImpl();
	id.name = "ComputeService";
	adapter->add(computeService, id);

	adapter->activate();

	cout<<"Service Activated"<<endl;
	communicator()->waitForShutdown();

	//join worker threads
	featureValueWorkerMgr.UnInitilize();

	return EXIT_SUCCESS;
}



