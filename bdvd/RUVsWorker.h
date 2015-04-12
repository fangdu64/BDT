#ifndef __RUVsWorker_h__
#define __RUVsWorker_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <RUVsWorkItem.h>
#include <armadillo>

class CRUVsWorker;
typedef IceUtil::Handle<CRUVsWorker> RUVsWorkerPtr;

class CRUVsWorker : public IceUtil::Thread
{
public:
	CRUVsWorker(int workerIdx, ::Ice::Long batchValueCnt,int m,int n);
	~CRUVsWorker();

public:
	 virtual void run();

	 void AddWorkItem(const RUVsWorkItemPtr& item);
	 void RequestShutdown();

	 bool AllocateBatchY();
	 Ice::Double* GetBatchY();

public:
	//YcsYcsT
	::arma::mat A;
	//YcscfYcscfT
	::arma::mat B;
	//YcfYcscfT
	::arma::mat C;

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
