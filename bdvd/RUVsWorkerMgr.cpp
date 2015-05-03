#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <RUVsWorkerMgr.h>
#include <GlobalVars.h>

CRUVsWorkerMgr::CRUVsWorkerMgr(int workerNum)
	:m_workerNum(workerNum)
{
}


CRUVsWorkerMgr::~CRUVsWorkerMgr()
{
}

bool CRUVsWorkerMgr::Initilize(Ice::Long batchValueCnt, int m, int n, int permutationCnt)
{
	cout<<"CRUVsWorkerMgr Initilize begin ..."<<endl; 
	bool rt=true;
	for(int i=0;i<m_workerNum;i++)
	{
		RUVsWorkerPtr worker = new CRUVsWorker(i, batchValueCnt, m, n, permutationCnt);
		if(worker->AllocateBatchY()==false)
		{
			rt=false;
		}
		worker->start();
		m_workers.push_back(worker);
	}
	cout<<"worker started, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVsWorkerMgr Initilize End"<<endl;
	return true;
}


void CRUVsWorkerMgr::RequestShutdownAllWorkers()
{
	cout<<"CRUVsWorkerMgr RequestShutdownAllWorkers begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		RUVsWorkerPtr worker= m_workers[i];
		if(worker)
			worker->RequestShutdown();
	}
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVsWorkerMgr RequestShutdownAllWorkers End"<<endl;
}

void CRUVsWorkerMgr::UnInitilize()
{
	cout<<"CRUVsWorkerMgr UnInitilize begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		RUVsWorkerPtr worker= m_workers[i];
		if(worker)
			worker->getThreadControl().join();
	}
	m_workers.clear();
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVsWorkerMgr UnInitilize End"<<endl;
}

RUVsWorkerPtr CRUVsWorkerMgr::GetRUVsWorker(int workerIdx)
{
	if(workerIdx<m_workerNum)
		return m_workers[workerIdx];
	else 
		return 0;
}

int CRUVsWorkerMgr::AssignItemToWorker(
	int workerIdx, const RUVsWorkItemPtr& wi)
{
	RUVsWorkerPtr worker = GetRUVsWorker(workerIdx);
	worker->AddWorkItem(wi);
	return 1;
}