#ifndef __GlobalVars_h__
#define __GlobalVars_h__

#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <KMeanService.h>
#include <bdtUtil/AMDTaskHelper.h>

class CKMeanProjectMgr;
class CGlobalVars;

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
		Ice::CommunicatorPtr communicator)
		:theCommunicator(communicator)
	{
		iBSInternal::GlobalVars::_ptrGV = this;
		theKMeanMgr=0;
	}

	static CGlobalVars* get();

public:
	Ice::CommunicatorPtr theCommunicator;
	CKMeanProjectMgr* theKMeanMgr;
	std::string theServerName;
	CAMDTaskMgr theAMDTaskMgr;
};

#endif
