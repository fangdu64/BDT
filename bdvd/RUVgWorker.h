#ifndef __RUVgWorker_h__
#define __RUVgWorker_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <RUVsWorkItem.h>
#include <armadillo>

class CRUVgWorker;
typedef IceUtil::Handle<CRUVgWorker> RUVgWorkerPtr;

class CRUVgWorker : public IceUtil::Thread
{
public:
	CRUVgWorker(int workerIdx, ::Ice::Long batchValueCnt,int n);
	~CRUVgWorker();

public:
	 virtual void run();

	 void AddWorkItem(const RUVsWorkItemPtr& item);
	 void RequestShutdown();

	 bool AllocateBatchY();
	 Ice::Double* GetBatchY();

public:
	//YcfYcfT
	::arma::mat A;
private:
	 int m_workerIdx;
     bool m_needNotify;
	 bool m_shutdownRequested;

	 IceUtil::Monitor<IceUtil::Mutex>	m_monitor;

	 typedef std::list<RUVsWorkItemPtr> RUVsWorkItemPtrLsit_T;
     RUVsWorkItemPtrLsit_T m_pendingItems;
	 RUVsWorkItemPtrLsit_T m_processingItems;

	 //working RAM
	 ::IceUtil::ScopedArray<Ice::Double>  m_Y;
	 const Ice::Long m_batchValueCnt;

	 
};


#endif
