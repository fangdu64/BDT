#ifndef __KMeanWorkerMgr_h__
#define __KMeanWorkerMgr_h__

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <KMeanWorkItem.h>
#include <KMeanWorker.h>
class CGlobalVars;
class CKMeanWorkerMgr
{
public:
	CKMeanWorkerMgr(CGlobalVars& gv, int workerNum);
	~CKMeanWorkerMgr();

public:

	void Initilize();
	void RequestShutdownAllWorkers();
	void UnInitilize();

	int AssignItemToWorker(int workerIdx, const KMeansWorkItemPtr& wi);
	//get a determined worker by observer info
	KMeanWorkerPtr GetKMeanWorker(int workerIdx);
private:

	
private:
	CGlobalVars&	m_gv;
	const int m_workerNum;
	typedef std::vector<KMeanWorkerPtr> KMeanWorkers_T;
	KMeanWorkers_T m_workers;
};
#endif

