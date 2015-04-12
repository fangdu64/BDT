#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <RUVVarDecmWorkerMgr.h>
#include <GlobalVars.h>

CRUVVarDecmWorkerMgr::CRUVVarDecmWorkerMgr(int workerNum)
	:m_workerNum(workerNum)
{
}


CRUVVarDecmWorkerMgr::~CRUVVarDecmWorkerMgr()
{
}

bool CRUVVarDecmWorkerMgr::Initilize(CRUVBuilder& ruvBuilder, Ice::Long batchRowCnt, Ice::Long batchValueCnt, size_t K)
{
	cout<<"CRUVVarDecmWorkerMgr Initilize begin ..."<<endl; 
	bool rt=true;
	for(int i=0;i<m_workerNum;i++)
	{
		RUVVarDecmWorkerPtr worker = new CRUVVarDecmWorker(ruvBuilder,i, batchRowCnt, batchValueCnt, K);
		if(worker->AllocateBatchYandRowMeans()==false)
		{
			rt=false;
		}
		worker->start();
		m_workers.push_back(worker);
	}
	cout<<"worker started, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVVarDecmWorkerMgr Initilize End"<<endl;
	return true;
}


void CRUVVarDecmWorkerMgr::RequestShutdownAllWorkers()
{
	cout<<"CRUVVarDecmWorkerMgr RequestShutdownAllWorkers begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		RUVVarDecmWorkerPtr worker = m_workers[i];
		if(worker)
			worker->RequestShutdown();
	}
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVVarDecmWorkerMgr RequestShutdownAllWorkers End"<<endl;
}

void CRUVVarDecmWorkerMgr::UnInitilize()
{
	cout<<"CRUVVarDecmWorkerMgr UnInitilize begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		RUVVarDecmWorkerPtr worker = m_workers[i];
		if(worker)
			worker->getThreadControl().join();
	}
	m_workers.clear();
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVVarDecmWorkerMgr UnInitilize End"<<endl;
}

RUVVarDecmWorkerPtr CRUVVarDecmWorkerMgr::GetWorker(int workerIdx)
{
	if(workerIdx<m_workerNum)
		return m_workers[workerIdx];
	else 
		return 0;
}

int CRUVVarDecmWorkerMgr::AssignItemToWorker(
	int workerIdx, const RUVsWorkItemPtr& wi)
{
	RUVVarDecmWorkerPtr worker = GetWorker(workerIdx);
	worker->AddWorkItem(wi);
	return 1;
}