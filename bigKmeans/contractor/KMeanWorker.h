#ifndef __KMeanWorker_h__
#define __KMeanWorker_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <FCDCentralService.h>
#include <KMeanService.h>
#include <KMeanWorkItem.h>

class CKMeanWorker;
typedef IceUtil::Handle<CKMeanWorker> KMeanWorkerPtr;

class CKMeanWorker : public IceUtil::Thread
{
public:
	CKMeanWorker();
	~CKMeanWorker();

public:
	 virtual void run();

	 void AddWorkItem(const KMeansWorkItemPtr& item);
	 void RequestShutdown();

private:
     bool m_needNotify;
	 bool m_shutdownRequested;

	 IceUtil::Monitor<IceUtil::Mutex>	m_monitor;

	 typedef std::list<KMeansWorkItemPtr> KMeansWorkItemPtrLsit_T;
     KMeansWorkItemPtrLsit_T m_pendingItems;
	 KMeansWorkItemPtrLsit_T m_processingItems;

};


#endif
