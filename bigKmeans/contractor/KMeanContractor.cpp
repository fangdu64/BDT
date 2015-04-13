#include <bdt/bdtBase.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <GlobalVars.h>
#include <KMeanContractorImpl.h>
#include <KMeanWorkerMgr.h>

using namespace std;
using namespace iBS;

class CKMeanContractor : public Ice::Application
{
public:
	CKMeanContractor()
	{
	}

	virtual int run(int argc, char* argv[]);
	virtual void interruptCallback(int);

private:
	std::string m_adminProxyStrNoHost;
};

int main(int argc, char* argv[])
{
	CKMeanContractor app;
	return app.main(argc, argv, "KMeansContractor.config");
}

void
CKMeanContractor::interruptCallback(int)
{
	if(CGlobalVars::get()&&CGlobalVars::get()->thePCPrx)
	{
		CGlobalVars::get()->thePCPrx->UnRegister(CGlobalVars::get()->theContractorName);
	}

	if(CGlobalVars::get()&&CGlobalVars::get()->theKMeanWorkerMgr)
	{
		CGlobalVars::get()->theKMeanWorkerMgr->RequestShutdownAllWorkers();
	}
    communicator()->shutdown();
}

int CKMeanContractor::run(int argc, char* argv[])
{
	callbackOnInterrupt();

	Ice::PropertiesPtr properties = communicator()->getProperties();
	ProxyCentralServicePrx pcPrx = ProxyCentralServicePrx::checkedCast(
		communicator()->propertyToProxy("ProxyCentralService.Proxy"));
    if(!pcPrx)
    {
        cerr << argv[0] << ": invalid ProxyCentralService.Proxy" << endl;
        return EXIT_FAILURE;
    }

	CGlobalVars gVars(communicator(), pcPrx);

	int workerThreadNum=properties->getPropertyAsIntWithDefault("KMeansContractor.WorkerCnt",4);
	gVars.theWorkerCnt=workerThreadNum;

	int workerRAMSize = properties->getPropertyAsIntWithDefault("KMeansContractor.RAMSize", 40000);
	gVars.theMemSize = workerRAMSize;

	gVars.theContractorName=properties->getPropertyWithDefault("KMeansContractor.Name","KMeanContractor_untitled");

	CKMeanWorkerMgr kmeanWorkerMgr(gVars, workerThreadNum);
	gVars.theKMeanWorkerMgr=&kmeanWorkerMgr;

	//start worker threads
	kmeanWorkerMgr.Initilize();

	Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("AdptKMeanContractor");

	Ice::Identity id;
	CKMeanContractorAdminServiceImplPtr contractorAdminService = new CKMeanContractorAdminServiceImpl(gVars);
	id.name="KMeanContractorAdminService";
	adapter->add(contractorAdminService, id);

	iBS::KMeanContractorAdminServicePrx kmeansCAdminPrx=
		iBS::KMeanContractorAdminServicePrx::uncheckedCast(
			adapter->createProxy(id));

	pcPrx->RegisterByProxyStr(gVars.theContractorName, kmeansCAdminPrx->ice_toString());

	/*Ice::TCPEndpointInfoPtr tcpInfo = Ice::TCPEndpointInfoPtr::dynamicCast(
		adapter->getEndpoints()[0]->getInfo());
	if(tcpInfo)
	{
		ostringstream os;
		os<<id.name<<":default -p "<<tcpInfo->port;
		m_adminProxyStrNoHost=os.str();
		pcPrx->RegisterByCallerAdress(gVars.theContractorName,m_adminProxyStrNoHost);
	}*/

	adapter->activate();

	cout<<"Service Activated"<<endl;
	
	communicator()->waitForShutdown();

	//join worker threads
	kmeanWorkerMgr.UnInitilize();

	return EXIT_SUCCESS;
}



