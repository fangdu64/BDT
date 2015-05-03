#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <RUVsWorker.h>
#include <algorithm>    // std::copy
#include <GlobalVars.h>
#include <RUVBuilder.h>
#include <ctime>

CRUVsWorker::CRUVsWorker(int workerIdx, ::Ice::Long batchValueCnt, int m, int n, int permutationCnt)
	:A(m,m,arma::fill::zeros),
	B(m,m,arma::fill::zeros),
	C(n,m,arma::fill::zeros),
	m_workerIdx(workerIdx), m_needNotify(false), 
	m_shutdownRequested(false),m_batchValueCnt(batchValueCnt),
	m_mt19937(static_cast<unsigned int>(std::time(0))),
	m_colIdxPermutation(m_mt19937, m)
{
	if (permutationCnt > 0)
	{
		As.reserve(permutationCnt);
		for (int i = 0; i < permutationCnt; ++i)
		{
			As.push_back(::arma::mat(m, m, arma::fill::zeros));
		}
	}
}


CRUVsWorker::~CRUVsWorker()
{
}

bool CRUVsWorker::AllocateBatchY()
{
	m_Y.reset(new ::Ice::Double[m_batchValueCnt]);
	if(!m_Y.get()){
		return false;
	}
	return true;
}

Ice::Double* CRUVsWorker::GetBatchY()
{
	return m_Y.get();
}

void
CRUVsWorker::run()
{
	
	bool bNeedExit=false;

	while(!bNeedExit)
	{
		
		{
			//entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while(!m_shutdownRequested)
			{
				if(m_pendingItems.size() == 0)
				{
					m_needNotify = true;
					m_monitor.wait();
				}
				if(!m_pendingItems.empty())
				{
					std::copy(m_pendingItems.begin(),m_pendingItems.end(),
						std::back_inserter(m_processingItems));
					m_pendingItems.clear();
					break;
				}
			}
			//if control request shutdown
			if(m_shutdownRequested)
			{

				for(RUVsWorkItemPtrLsit_T::const_iterator it= m_pendingItems.begin(); 
					it!= m_pendingItems.end(); ++it)
				{
					//cancle any outstanding requests.
					(*it)->CancelWork();
				}
				bNeedExit=true;

				//
				cout<<"CRUVsWorker m_shutdownRequested==true ..."<<endl; 
			}

			//leaving critical region
			m_needNotify=false;
		}

		//these items are inserted before shutdown request, need to finish these items anyway
		while(!m_processingItems.empty())
		{
			RUVsWorkItemPtr wi = m_processingItems.front();
			wi->DoWork();
			
			//notify back to RUVBuilder
			CRUVsComputeABC *pComputeABC
				=dynamic_cast<CRUVsComputeABC*>(wi.get());
			if(pComputeABC)
			{
				pComputeABC->m_ruvBuilder.NotifyWorkerBecomesFree(m_workerIdx);
			}

			m_processingItems.pop_front();
		}
	}


}

void CRUVsWorker::AddWorkItem(const RUVsWorkItemPtr& item)
{
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);

	if(!m_shutdownRequested)
    {
		m_pendingItems.push_back(item);
		if(m_needNotify)
		{
			m_monitor.notify();
		}
	}
	else
	{
		//control already issued shutdown request, cancel it
		item->CancelWork();
	}
}

void CRUVsWorker::RequestShutdown()
{
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);

	m_shutdownRequested = true;
	if(m_needNotify)
	{
		m_monitor.notify();
	}
}

