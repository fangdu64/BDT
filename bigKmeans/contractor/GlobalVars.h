#ifndef __GlobalVars_h__
#define __GlobalVars_h__

#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <KMeanService.h>

class CGlobalVars;
class CKMeanWorkerMgr;

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
		iBS::ProxyCentralServicePrx& pcPrx)
		:theCommunicator(communicator),
		thePCPrx(pcPrx)
	{
		iBSInternal::GlobalVars::_ptrGV = this;
		theWorkerCnt=4;
		theMemSize=20000;
		theKMeanWorkerMgr=0;
		theOngoingContractCnt =0;
	}

	static CGlobalVars* get();
public:
	Ice::CommunicatorPtr theCommunicator;
	iBS::ProxyCentralServicePrx& thePCPrx;
	iBS::KMeanServerServicePrx theKMeanPrx;
	CKMeanWorkerMgr* theKMeanWorkerMgr;
	int theWorkerCnt;
	int theMemSize;//in megabytes
	std::string theContractorName;
	int theOngoingContractCnt;
	
};

#endif
