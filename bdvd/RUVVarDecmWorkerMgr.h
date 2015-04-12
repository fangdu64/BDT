#ifndef __RUVVarDecmWorkerMgr_h__
#define __RUVVarDecmWorkerMgr_h__

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <RUVsWorkItem.h>
#include <RUVVarDecmWorker.h>

class CGlobalVars;
class CRUVBuilder;
class CRUVVarDecmWorkerMgr
{
public:
	CRUVVarDecmWorkerMgr(int workerNum);
	~CRUVVarDecmWorkerMgr();

public:

	bool Initilize(CRUVBuilder& ruvBuilder, Ice::Long batchRowCnt, Ice::Long batchValueCnt, size_t K);
	void RequestShutdownAllWorkers();
	void UnInitilize();

	int AssignItemToWorker(int workerIdx, const RUVsWorkItemPtr& wi);
	//get a determined worker by observer info
	RUVVarDecmWorkerPtr GetWorker(int workerIdx);
private:

	
private:
	const int m_workerNum;
	typedef std::vector<RUVVarDecmWorkerPtr> RUVVarDecmWorkers_T;
	RUVVarDecmWorkers_T m_workers;
};
#endif

