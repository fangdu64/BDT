#ifndef __RUVOutputWorker_h__
#define __RUVOutputWorker_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <RUVsWorkItem.h>


class CRUVOutputWorker;
typedef IceUtil::Handle<CRUVOutputWorker> RUVOutputWorkerPtr;

class CRUVOutputWorker : public IceUtil::Thread
{
public:
	CRUVOutputWorker(int workerIdx);
	~CRUVOutputWorker();

public:
	 virtual void run();
	 void AddWorkItem(const RUVsWorkItemPtr& item);
	 void RequestShutdown();

private:
	 int m_workerIdx;
     bool m_needNotify;
	 bool m_shutdownRequested;

	 IceUtil::Monitor<IceUtil::Mutex>	m_monitor;

	 typedef std::list<RUVsWorkItemPtr> RUVsWorkItemPtrLsit_T;
     RUVsWorkItemPtrLsit_T m_pendingItems;
	 RUVsWorkItemPtrLsit_T m_processingItems;
};

#endif
