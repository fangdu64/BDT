#ifndef __RUVOutputWorkerMgr_h__
#define __RUVOutputWorkerMgr_h__

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <RUVsWorkItem.h>
#include <RUVOutputWorker.h>

class CGlobalVars;
class CRUVOutputWorkerMgr
{
public:
	CRUVOutputWorkerMgr();
	~CRUVOutputWorkerMgr();

public:
	bool Initilize(int workerNum);
	void RequestShutdownAllWorkers();
	void UnInitilize();

	int AssignItemToWorker(int workerIdx, const RUVsWorkItemPtr& wi);
	//get a determined worker by observer info
	RUVOutputWorkerPtr GetWorker(int workerIdx);

private:
	int m_workerNum;
	typedef std::vector<RUVOutputWorkerPtr> RUVOutputWorkers_T;
	RUVOutputWorkers_T m_workers;
};
#endif

