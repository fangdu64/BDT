#ifndef __RUVgWorkerMgr_h__
#define __RUVgWorkerMgr_h__

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <RUVsWorkItem.h>
#include <RUVgWorker.h>
class CGlobalVars;
class CRUVgWorkerMgr
{
public:
	CRUVgWorkerMgr(int workerNum);
	~CRUVgWorkerMgr();

public:

	bool Initilize(Ice::Long batchValueCnt,int n);
	void RequestShutdownAllWorkers();
	void UnInitilize();

	int AssignItemToWorker(int workerIdx, const RUVsWorkItemPtr& wi);
	//get a determined worker by observer info
	RUVgWorkerPtr GetRUVgWorker(int workerIdx);
private:

	
private:
	const int m_workerNum;
	typedef std::vector<RUVgWorkerPtr> RUVgWorkers_T;
	RUVgWorkers_T m_workers;
};
#endif

