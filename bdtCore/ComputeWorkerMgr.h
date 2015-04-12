#ifndef __ComputeWorkerMgr_h__
#define __ComputeWorkerMgr_h__

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <ComputeWorkItem.h>
#include <ComputeWorker.h>

class CGlobalVars;

class CComputeWorkerMgr
{
public:
	CComputeWorkerMgr(int workerNum);
	virtual ~CComputeWorkerMgr();
public:
	void RequestShutdownAllWorkers();
	void UnInitilize();

	int AssignItemToWorker(int workerIdx, const ComputeWorkItemPtr& wi);
	//get a determined worker by observer info
	ComputeWorkerPtr GetComputeWorker(int workerIdx);

protected:
	const int m_workerNum;
	typedef std::vector<ComputeWorkerPtr> ComputeWorkers_T;
	ComputeWorkers_T m_workers;
};

class CPearsonCorrelationWorkerMgr : public CComputeWorkerMgr
{
public:
	CPearsonCorrelationWorkerMgr(int workerNum)
		:CComputeWorkerMgr(workerNum){}
	virtual ~CPearsonCorrelationWorkerMgr();
public:
	bool Initilize(::Ice::Long batchValueCnt, ::Ice::Long colCnt);
};

class CEuclideanDistMatrixWorkerMgr : public CComputeWorkerMgr
{
public:
	CEuclideanDistMatrixWorkerMgr(int workerNum)
		:CComputeWorkerMgr(workerNum){}
	virtual ~CEuclideanDistMatrixWorkerMgr();
public:
	bool Initilize(::Ice::Long batchValueCnt, ::Ice::Long colCnt);
};

//////////////////////////////////////////////////////////////////

#endif

