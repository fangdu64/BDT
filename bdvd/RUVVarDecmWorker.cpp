#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <RUVVarDecmWorker.h>
#include <algorithm>    // std::copy
#include <GlobalVars.h>
#include <RUVBuilder.h>

CRUVVarDecmWorker::CRUVVarDecmWorker(CRUVBuilder& ruvBuilder, int workerIdx, ::Ice::Long batchRowCnt, ::Ice::Long batchValueCnt, size_t K)
:m_ruvBuilder(ruvBuilder), m_workerIdx(workerIdx), m_needNotify(false),
	m_shutdownRequested(false),m_batchRowCnt(batchRowCnt), m_batchValueCnt(batchValueCnt)
{
	m_ssGrandMean.resize(K,0);
	m_ssXb.resize(K, 0);
	m_ssWa.resize(K, 0);
	m_ssXbWa.resize(K, 0);

	m_xbGrandMean.resize(K, 0);
	m_ssXbTotalVar.resize(K, 0);
	m_ssXbBgVar.resize(K, 0);
	m_ssXbWgVar.resize(K, 0);
}


CRUVVarDecmWorker::~CRUVVarDecmWorker()
{
}

bool CRUVVarDecmWorker::AllocateBatchYandRowMeans()
{
	m_Y.reset(new ::Ice::Double[m_batchValueCnt]);
	if(!m_Y.get()){
		return false;
	}

	m_rowMeans.reset(new ::Ice::Double[m_batchRowCnt]);
	if (!m_rowMeans.get()){
		return false;
	}

	return true;
}

Ice::Double* CRUVVarDecmWorker::GetBatchY()
{
	return m_Y.get();
}

Ice::Double* CRUVVarDecmWorker::GetBatchRowMeans()
{
	return m_rowMeans.get();
}

void
CRUVVarDecmWorker::run()
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
				cout<<"CRUVVarDecmWorker m_shutdownRequested==true ..."<<endl; 
			}

			//leaving critical region
			m_needNotify=false;
		}

		//these items are inserted before shutdown request, need to finish these items anyway
		bool needNotifyWorkerBecomesFree = false;
		while(!m_processingItems.empty())
		{
			RUVsWorkItemPtr wi = m_processingItems.front();
			wi->DoWork();

			m_processingItems.pop_front();
			needNotifyWorkerBecomesFree = true;
		}
		
		if (needNotifyWorkerBecomesFree)
		{
			m_ruvBuilder.NotifyWorkerBecomesFree(m_workerIdx);
		}
	}


}

void CRUVVarDecmWorker::AddWorkItem(const RUVsWorkItemPtr& item)
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

void CRUVVarDecmWorker::RequestShutdown()
{
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);

	m_shutdownRequested = true;
	if(m_needNotify)
	{
		m_monitor.notify();
	}
}

