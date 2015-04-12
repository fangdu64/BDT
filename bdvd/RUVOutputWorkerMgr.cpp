#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <RUVOutputWorkerMgr.h>
#include <GlobalVars.h>

CRUVOutputWorkerMgr::CRUVOutputWorkerMgr()
{
	m_workerNum = 0;
}

CRUVOutputWorkerMgr::~CRUVOutputWorkerMgr()
{
}

bool CRUVOutputWorkerMgr::Initilize(int workerNum)
{
	if (!m_workers.empty())
	{
		return false;
	}
	m_workerNum = workerNum;
	
	cout<<"CRUVOutputWorkerMgr Initilize begin ..."<<endl; 
	bool rt=true;
	for(int i=0;i<m_workerNum;i++)
	{
		RUVOutputWorkerPtr worker = new CRUVOutputWorker(i);
		worker->start();
		m_workers.push_back(worker);
	}
	cout<<"worker started, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVOutputWorkerMgr Initilize End"<<endl;
	return true;
}


void CRUVOutputWorkerMgr::RequestShutdownAllWorkers()
{
	cout<<"CRUVOutputWorkerMgr RequestShutdownAllWorkers begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		RUVOutputWorkerPtr worker = m_workers[i];
		if(worker)
			worker->RequestShutdown();
	}
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVOutputWorkerMgr RequestShutdownAllWorkers End"<<endl;
}

void CRUVOutputWorkerMgr::UnInitilize()
{
	cout<<"CRUVVarDecmWorkerMgr UnInitilize begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		RUVOutputWorkerPtr worker = m_workers[i];
		if(worker)
			worker->getThreadControl().join();
	}
	m_workers.clear();
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVVarDecmWorkerMgr UnInitilize End"<<endl;
}

RUVOutputWorkerPtr CRUVOutputWorkerMgr::GetWorker(int workerIdx)
{
	if(workerIdx<m_workerNum)
		return m_workers[workerIdx];
	else 
		return 0;
}

int CRUVOutputWorkerMgr::AssignItemToWorker(
	int workerIdx, const RUVsWorkItemPtr& wi)
{
	RUVOutputWorkerPtr worker = GetWorker(workerIdx);
	worker->AddWorkItem(wi);
	return 1;
}