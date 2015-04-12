#ifndef __RUVRowANOVAWorker_h__
#define __RUVRowANOVAWorker_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <RUVsWorkItem.h>


class CRUVRowANOVAWorker;
typedef IceUtil::Handle<CRUVRowANOVAWorker> RUVRowANOVAWorkerPtr;

class CRUVRowANOVAWorker : public IceUtil::Thread
{
public:
	CRUVRowANOVAWorker(int workerIdx, ::Ice::Long batchRowCnt, ::Ice::Long batchValueCnt);
	~CRUVRowANOVAWorker();

public:
	 virtual void run();

	 void AddWorkItem(const RUVsWorkItemPtr& item);
	 void RequestShutdown();

	 bool AllocateBatchYandFStatistics();
	 Ice::Double* GetBatchY();
	 Ice::Double* GetBatchFStatistics();

public:
	::Ice::Long m_featureIdxFrom;
	::Ice::Long m_featureIdxTo;

private:
	 int m_workerIdx;
     bool m_needNotify;
	 bool m_shutdownRequested;

	 IceUtil::Monitor<IceUtil::Mutex>	m_monitor;

	 typedef std::list<RUVsWorkItemPtr> RUVsWorkItemPtrLsit_T;
     RUVsWorkItemPtrLsit_T m_pendingItems;
	 RUVsWorkItemPtrLsit_T m_processingItems;

	 const Ice::Long m_batchRowCnt;
	 //working RAM
	 ::IceUtil::ScopedArray<Ice::Double>  m_Y;
	 const Ice::Long m_batchValueCnt;

	 //F-statistics for [m_featureIdxFrom,m_featureIdxTo)
	::IceUtil::ScopedArray<Ice::Double>  m_FStatistics;
	
	 
};


#endif
