#ifndef __RUVVarDecmAWorker_h__
#define __RUVVarDecmAWorker_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <RUVsWorkItem.h>


class CRUVVarDecmWorker;
typedef IceUtil::Handle<CRUVVarDecmWorker> RUVVarDecmWorkerPtr;

class CRUVBuilder;
class CRUVVarDecmWorker : public IceUtil::Thread
{
public:
	CRUVVarDecmWorker(CRUVBuilder& ruvBuilder, int workerIdx, ::Ice::Long batchRowCnt, ::Ice::Long batchValueCnt, size_t K);
	~CRUVVarDecmWorker();

public:
	 virtual void run();

	 void AddWorkItem(const RUVsWorkItemPtr& item);
	 void RequestShutdown();

	 bool AllocateBatchYandRowMeans();
	 Ice::Double* GetBatchY();
	 Ice::Double* GetBatchRowMeans();

public:
	iBS::DoubleVec m_ssGrandMean;
	iBS::DoubleVec m_ssXb;
	iBS::DoubleVec m_ssWa;
	iBS::DoubleVec m_ssXbWa;

	::iBS::DoubleVec m_xbGrandMean;
	::iBS::DoubleVec m_ssXbTotalVar;
	::iBS::DoubleVec m_ssXbBgVar;
	::iBS::DoubleVec m_ssXbWgVar;

private:
	CRUVBuilder& m_ruvBuilder;
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
	 ::IceUtil::ScopedArray<Ice::Double>  m_rowMeans;
	 const Ice::Long m_batchValueCnt;

	 
};


#endif
