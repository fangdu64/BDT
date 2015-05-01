#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <RUVgWorkerMgr.h>
#include <GlobalVars.h>

CRUVgWorkerMgr::CRUVgWorkerMgr(int workerNum)
	:m_workerNum(workerNum)
{
}


CRUVgWorkerMgr::~CRUVgWorkerMgr()
{
}

bool CRUVgWorkerMgr::Initilize(Ice::Long batchValueCnt, int n, int permutationCnt)
{
	cout<<"CRUVgWorkerMgr Initilize begin ..."<<endl; 
	bool rt=true;
	for(int i=0;i<m_workerNum;i++)
	{
		RUVgWorkerPtr worker = new CRUVgWorker(i, batchValueCnt, n, permutationCnt);
		if(worker->AllocateBatchY()==false)
		{
			rt=false;
		}
		worker->start();
		m_workers.push_back(worker);
	}
	cout<<"worker started, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVgWorkerMgr Initilize End"<<endl;
	return true;
}


void CRUVgWorkerMgr::RequestShutdownAllWorkers()
{
	cout<<"CRUVgWorkerMgr RequestShutdownAllWorkers begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		RUVgWorkerPtr worker= m_workers[i];
		if(worker)
			worker->RequestShutdown();
	}
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVgWorkerMgr RequestShutdownAllWorkers End"<<endl;
}

void CRUVgWorkerMgr::UnInitilize()
{
	cout<<"CRUVgWorkerMgr UnInitilize begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		RUVgWorkerPtr worker= m_workers[i];
		if(worker)
			worker->getThreadControl().join();
	}
	m_workers.clear();
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVgWorkerMgr UnInitilize End"<<endl;
}

RUVgWorkerPtr CRUVgWorkerMgr::GetRUVgWorker(int workerIdx)
{
	if(workerIdx<m_workerNum)
		return m_workers[workerIdx];
	else 
		return 0;
}

int CRUVgWorkerMgr::AssignItemToWorker(
	int workerIdx, const RUVsWorkItemPtr& wi)
{
	RUVgWorkerPtr worker = GetRUVgWorker(workerIdx);
	worker->AddWorkItem(wi);
	return 1;
}