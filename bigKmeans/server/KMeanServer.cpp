#include <bdt/bdtBase.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <GlobalVars.h>
#include <KMeanServerImpl.h>
#include <KMeanProjectMgr.h>

using namespace std;
using namespace iBS;

class CKMeanServer : public Ice::Application
{
public:
	CKMeanServer()
	{
	}

	virtual int run(int argc, char* argv[]);
	virtual void interruptCallback(int);

private:
	std::string m_adminProxyStrNoHost;
	std::string m_serverProxyStrNoHost;
};

int main(int argc, char* argv[])
{
	CKMeanServer app;
	return app.main(argc, argv, "KMeansServer.config");
}

void
CKMeanServer::interruptCallback(int)
{
	CGlobalVars::get()->thePCPrx->UnRegister(CGlobalVars::get()->theServerName+"_Admin");
    communicator()->shutdown();
}

int CKMeanServer::run(int argc, char* argv[])
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
	gVars.theServerName=properties->getPropertyWithDefault("KMeansServer.Name","KMeanServer_untitled");

	CKMeanProjectMgr theKMeanMgr(gVars);
	gVars.theKMeanMgr=&theKMeanMgr;


	Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("AdptKMeanServer");

	Ice::Identity id;
	CKMeanServerAdminServiceImplPtr adminService = new CKMeanServerAdminServiceImpl(gVars);
	id.name="KMeanServerAdminService";
	adapter->add(adminService, id);

	iBS::KMeanServerAdminServicePrx kmeansSAdminPrx =
		iBS::KMeanServerAdminServicePrx::uncheckedCast(
		adapter->createProxy(id));
	pcPrx->RegisterByProxyStr(gVars.theServerName + "_Admin", kmeansSAdminPrx->ice_toString());

	CKMeanServerServiceImplPtr contractorService = new CKMeanServerServiceImpl(gVars);
	id.name="KMeanServerService";
	adapter->add(contractorService, id);
	
	adapter->activate();

	cout<<"Service Activated"<<endl;
	communicator()->waitForShutdown();

	return EXIT_SUCCESS;
}



