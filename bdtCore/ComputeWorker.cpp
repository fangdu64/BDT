#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <ComputeWorker.h>
#include <algorithm>    // std::copy
#include <GlobalVars.h>
#include <ComputeBuilder.h>


CComputeWorker::CComputeWorker(int workerIdx)
	: m_workerIdx(workerIdx), m_needNotify(false),
	m_shutdownRequested(false)
{
}
CComputeWorker::~CComputeWorker()
{
}

void
CComputeWorker::run()
{

	bool bNeedExit = false;

	while (!bNeedExit)
	{

		{
			//entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while (!m_shutdownRequested)
			{
				if (m_pendingItems.size() == 0)
				{
					m_needNotify = true;
					m_monitor.wait();
				}
				if (!m_pendingItems.empty())
				{
					std::copy(m_pendingItems.begin(), m_pendingItems.end(),
						std::back_inserter(m_processingItems));
					m_pendingItems.clear();
					break;
				}
			}
			//if control request shutdown
			if (m_shutdownRequested)
			{

				for (ComputeWorkItemPtrLsit_T::const_iterator it = m_pendingItems.begin();
					it != m_pendingItems.end(); ++it)
				{
					//cancle any outstanding requests.
					(*it)->CancelWork();
				}
				bNeedExit = true;

				//
				cout << "CPearsonCorrelationWorker m_shutdownRequested" << endl;
			}

			//leaving critical region
			m_needNotify = false;
		}

		//these items are inserted before shutdown request, need to finish these items anyway
		while (!m_processingItems.empty())
		{
			ComputeWorkItemPtr wi = m_processingItems.front();
			wi->DoWork();

			//notify back to RUVBuilder
			wi->m_builder.NotifyWorkerBecomesFree(m_workerIdx);

			m_processingItems.pop_front();
		}
	}


}

void CComputeWorker::AddWorkItem(const ComputeWorkItemPtr& item)
{
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);

	if (!m_shutdownRequested)
	{
		m_pendingItems.push_back(item);
		if (m_needNotify)
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

void CComputeWorker::RequestShutdown()
{
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);

	m_shutdownRequested = true;
	if (m_needNotify)
	{
		m_monitor.notify();
	}
}

///////////////////////////////////////////////////////////////////////
CPearsonCorrelationWorker::CPearsonCorrelationWorker(
	int workerIdx, ::Ice::Long batchValueCnt, ::Ice::Long colCnt)
	: CComputeWorker(workerIdx),
	A((int)colCnt, (int)colCnt, arma::fill::zeros),
	ColSumSquares((int)colCnt, 1, arma::fill::zeros),
	m_batchValueCnt(batchValueCnt)
{
	m_Y.resize(m_batchValueCnt);
}

CPearsonCorrelationWorker::~CPearsonCorrelationWorker()
{
}


iBS::DoubleVec& CPearsonCorrelationWorker::GetBatchY()
{
	return m_Y;
}

///////////////////////////////////////////////////////////////////////
CEuclideanDistMatrixWorker::CEuclideanDistMatrixWorker(
	int workerIdx, ::Ice::Long batchValueCnt, ::Ice::Long colCnt)
	: CComputeWorker(workerIdx),
	A((int)colCnt, (int)colCnt, arma::fill::zeros),
	m_batchValueCnt(batchValueCnt)
{
	m_Y.resize(m_batchValueCnt);
}

CEuclideanDistMatrixWorker::~CEuclideanDistMatrixWorker()
{
}


iBS::DoubleVec& CEuclideanDistMatrixWorker::GetBatchY()
{
	return m_Y;
}


