#ifndef __ComputeWorker_h__
#define __ComputeWorker_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <ComputeWorkItem.h>
#include <armadillo>

class CComputeWorker;
typedef IceUtil::Handle<CComputeWorker> ComputeWorkerPtr;

class CComputeWorker : public IceUtil::Thread
{
public:
	CComputeWorker(int workerIdx);
	virtual ~CComputeWorker();

public:
	 virtual void run();

	 void AddWorkItem(const ComputeWorkItemPtr& item);
	 void RequestShutdown();

protected:
	 int m_workerIdx;
     bool m_needNotify;
	 bool m_shutdownRequested;

	 IceUtil::Monitor<IceUtil::Mutex>	m_monitor;

	 typedef std::list<ComputeWorkItemPtr> ComputeWorkItemPtrLsit_T;
	 ComputeWorkItemPtrLsit_T m_pendingItems;
	 ComputeWorkItemPtrLsit_T m_processingItems;
};

////////////////////////////////////////////////////////////////
class CPearsonCorrelationWorker : public CComputeWorker
{
public:
	CPearsonCorrelationWorker(int workerIdx, ::Ice::Long batchValueCnt, ::Ice::Long colCnt);
	virtual ~CPearsonCorrelationWorker();

	iBS::DoubleVec& GetBatchY();

public:
	//YcfYcfT
	::arma::mat A;
	::arma::mat ColSumSquares;

private:
	//working RAM
	iBS::DoubleVec  m_Y;
	const Ice::Long m_batchValueCnt;
};

/////////////////////////////////////////////////////////////////
class CEuclideanDistMatrixWorker : public CComputeWorker
{
public:
	CEuclideanDistMatrixWorker(int workerIdx, ::Ice::Long batchValueCnt, ::Ice::Long colCnt);
	virtual ~CEuclideanDistMatrixWorker();

	iBS::DoubleVec& GetBatchY();

public:
	::arma::mat A;
private:
	//working RAM
	iBS::DoubleVec  m_Y;
	const Ice::Long m_batchValueCnt;
};
//////////////////////////////////////////////////////////////////

#endif
