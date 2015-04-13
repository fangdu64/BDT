#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <KMeanWorker.h>
#include <algorithm>    // std::copy
#include <GlobalVars.h>

CKMeanWorker::CKMeanWorker()
	:m_needNotify(false), m_shutdownRequested(false)
{
}


CKMeanWorker::~CKMeanWorker()
{
}

void
CKMeanWorker::run()
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

				for(KMeansWorkItemPtrLsit_T::const_iterator it= m_pendingItems.begin(); 
					it!= m_pendingItems.end(); ++it)
				{
					//cancle any outstanding requests.
					(*it)->CancelWork();
				}
				bNeedExit=true;

				//
				cout<<"CKMeanWorker m_shutdownRequested==true ..."<<endl; 
			}

			//leaving critical region
			m_needNotify=false;
		}

		//these items are inserted before shutdown request, need to finish these items anyway
		while(!m_processingItems.empty())
		{
			KMeansWorkItemPtr wi = m_processingItems.front();
			wi->DoWork();
			
			//notify back to CKMeanContratL2
			wi->m_kmeanL2Ptr->NotifyWorkerItemDone(wi);

			m_processingItems.pop_front();
		}
	}

}

void CKMeanWorker::AddWorkItem(const KMeansWorkItemPtr& item)
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

void CKMeanWorker::RequestShutdown()
{
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
	m_shutdownRequested = true;
	if(m_needNotify)
	{
		m_monitor.notify();
	}
}

