#ifndef __RUVsWorker_h__
#define __RUVsWorker_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <RUVsWorkItem.h>
#include <armadillo>
#include <RandomHelper.h>

class CRUVsWorker;
typedef IceUtil::Handle<CRUVsWorker> RUVsWorkerPtr;

class CRUVsWorker : public IceUtil::Thread
{
public:
	CRUVsWorker(int workerIdx, ::Ice::Long batchValueCnt, int m, int n, int permutationCnt);
	~CRUVsWorker();

public:
	 virtual void run();

	 void AddWorkItem(const RUVsWorkItemPtr& item);
	 void RequestShutdown();

	 bool AllocateBatchY();
	 Ice::Double* GetBatchY();
	 CIndexPermutation& GetColIdxPermutation() { return m_colIdxPermutation; }
public:
	//YcsYcsT
	::arma::mat A;

	//permutated As
	std::vector< ::arma::mat> As;

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

	 boost::random::mt19937 m_mt19937;

	 CIndexPermutation m_colIdxPermutation;
};


#endif
