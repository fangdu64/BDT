#ifndef __RUVRowANOVAWorkerMgr_h__
#define __RUVRowANOVAWorkerMgr_h__

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <RUVsWorkItem.h>
#include <RUVRowANOVAWorker.h>

class CGlobalVars;
class CRUVRowANOVAWorkerMgr
{
public:
	CRUVRowANOVAWorkerMgr(int workerNum);
	~CRUVRowANOVAWorkerMgr();

public:

	bool Initilize(Ice::Long batchRowCnt, Ice::Long batchValueCnt);
	void RequestShutdownAllWorkers();
	void UnInitilize();

	int AssignItemToWorker(int workerIdx, const RUVsWorkItemPtr& wi);
	//get a determined worker by observer info
	RUVRowANOVAWorkerPtr GetWorker(int workerIdx);
private:

	
private:
	const int m_workerNum;
	typedef std::vector<RUVRowANOVAWorkerPtr> RUVRowANOVAWorkers_T;
	RUVRowANOVAWorkers_T m_workers;
};
#endif

