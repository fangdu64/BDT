#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <RUVRowANOVAWorkerMgr.h>
#include <GlobalVars.h>

CRUVRowANOVAWorkerMgr::CRUVRowANOVAWorkerMgr(int workerNum)
	:m_workerNum(workerNum)
{
}


CRUVRowANOVAWorkerMgr::~CRUVRowANOVAWorkerMgr()
{
}

bool CRUVRowANOVAWorkerMgr::Initilize(Ice::Long batchRowCnt, Ice::Long batchValueCnt)
{
	cout<<"CRUVRowANOVAWorkerMgr Initilize begin ..."<<endl; 
	bool rt=true;
	for(int i=0;i<m_workerNum;i++)
	{
		RUVRowANOVAWorkerPtr worker= new CRUVRowANOVAWorker(i,batchRowCnt, batchValueCnt);
		if(worker->AllocateBatchYandFStatistics()==false)
		{
			rt=false;
		}
		worker->start();
		m_workers.push_back(worker);
	}
	cout<<"worker started, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVRowANOVAWorkerMgr Initilize End"<<endl;
	return true;
}


void CRUVRowANOVAWorkerMgr::RequestShutdownAllWorkers()
{
	cout<<"CRUVRowANOVAWorkerMgr RequestShutdownAllWorkers begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		RUVRowANOVAWorkerPtr worker= m_workers[i];
		if(worker)
			worker->RequestShutdown();
	}
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVRowANOVAWorkerMgr RequestShutdownAllWorkers End"<<endl;
}

void CRUVRowANOVAWorkerMgr::UnInitilize()
{
	cout<<"CRUVRowANOVAWorkerMgr UnInitilize begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		RUVRowANOVAWorkerPtr worker= m_workers[i];
		if(worker)
			worker->getThreadControl().join();
	}
	m_workers.clear();
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CRUVRowANOVAWorkerMgr UnInitilize End"<<endl;
}

RUVRowANOVAWorkerPtr CRUVRowANOVAWorkerMgr::GetWorker(int workerIdx)
{
	if(workerIdx<m_workerNum)
		return m_workers[workerIdx];
	else 
		return 0;
}

int CRUVRowANOVAWorkerMgr::AssignItemToWorker(
	int workerIdx, const RUVsWorkItemPtr& wi)
{
	RUVRowANOVAWorkerPtr worker = GetWorker(workerIdx);
	worker->AddWorkItem(wi);
	return 1;
}