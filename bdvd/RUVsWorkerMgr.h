#ifndef __RUVsWorkerMgr_h__
#define __RUVsWorkerMgr_h__

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <RUVsWorkItem.h>
#include <RUVsWorker.h>
class CGlobalVars;
class CRUVsWorkerMgr
{
public:
	CRUVsWorkerMgr(int workerNum);
	~CRUVsWorkerMgr();

public:

	bool Initilize(Ice::Long batchValueCnt,int m, int n);
	void RequestShutdownAllWorkers();
	void UnInitilize();

	int AssignItemToWorker(int workerIdx, const RUVsWorkItemPtr& wi);
	//get a determined worker by observer info
	RUVsWorkerPtr GetRUVsWorker(int workerIdx);
private:

	
private:
	const int m_workerNum;
	typedef std::vector<RUVsWorkerPtr> RUVsWorkers_T;
	RUVsWorkers_T m_workers;
};
#endif

