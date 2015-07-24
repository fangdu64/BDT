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
    communicator()->shutdown();
}

int CKMeanServer::run(int argc, char* argv[])
{

	 callbackOnInterrupt();

	Ice::PropertiesPtr properties = communicator()->getProperties();

	CGlobalVars gVars(communicator());
	gVars.theServerName=properties->getPropertyWithDefault("KMeansServer.Name","KMeanServer_untitled");

	CKMeanProjectMgr theKMeanMgr(gVars);
	gVars.theKMeanMgr=&theKMeanMgr;


	Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("AdptKMeanServer");

	Ice::Identity id;
	CKMeanServerAdminServiceImplPtr adminService = new CKMeanServerAdminServiceImpl(gVars);
	id.name="KMeanServerAdminService";
	adapter->add(adminService, id);

	CKMeanServerServiceImplPtr contractorService = new CKMeanServerServiceImpl(gVars);
	id.name="KMeanServerService";
	adapter->add(contractorService, id);
	
	adapter->activate();

	cout<<"Service Activated"<<endl;
	communicator()->waitForShutdown();

	return EXIT_SUCCESS;
}



