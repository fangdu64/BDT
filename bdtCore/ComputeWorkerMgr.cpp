#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <ComputeWorkerMgr.h>
#include <GlobalVars.h>
#include <ComputeWorker.h>

CComputeWorkerMgr::CComputeWorkerMgr(int workerNum)
	:m_workerNum(workerNum)
{
}


CComputeWorkerMgr::~CComputeWorkerMgr()
{
}

void CComputeWorkerMgr::RequestShutdownAllWorkers()
{
	cout<<"CComputeWorkerMgr RequestShutdownAllWorkers begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		ComputeWorkerPtr worker= m_workers[i];
		if(worker)
			worker->RequestShutdown();
	}
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CComputeWorkerMgr RequestShutdownAllWorkers End"<<endl;
}

void CComputeWorkerMgr::UnInitilize()
{
	cout<<"CComputeWorkerMgr UnInitilize begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		ComputeWorkerPtr worker= m_workers[i];
		if(worker)
			worker->getThreadControl().join();
	}
	m_workers.clear();
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CComputeWorkerMgr UnInitilize End"<<endl;
}

ComputeWorkerPtr CComputeWorkerMgr::GetComputeWorker(int workerIdx)
{
	if(workerIdx<m_workerNum)
		return m_workers[workerIdx];
	else 
		return 0;
}

int CComputeWorkerMgr::AssignItemToWorker(
	int workerIdx, const ComputeWorkItemPtr& wi)
{
	ComputeWorkerPtr worker = GetComputeWorker(workerIdx);
	worker->AddWorkItem(wi);
	return 1;
}

////////////////////////////////////////////////////////////
CPearsonCorrelationWorkerMgr::~CPearsonCorrelationWorkerMgr()
{

}
bool CPearsonCorrelationWorkerMgr::Initilize(::Ice::Long batchValueCnt, ::Ice::Long colCnt)
{
	cout << "CPearsonCorrelationWorkerMgr Initilize begin ..." << endl;
	bool rt = true;
	for (int i = 0; i<m_workerNum; i++)
	{
		ComputeWorkerPtr worker = new CPearsonCorrelationWorker(i, batchValueCnt, colCnt);
		worker->start();
		m_workers.push_back(worker);
	}
	cout << "worker started, workerNum=" << m_workerNum << endl;
	cout << "CPearsonCorrelationWorkerMgr Initilize End" << endl;
	return true;
}

/////////////////////////////////////////////////////////////
CEuclideanDistMatrixWorkerMgr::~CEuclideanDistMatrixWorkerMgr()
{

}
bool CEuclideanDistMatrixWorkerMgr::Initilize(::Ice::Long batchValueCnt, ::Ice::Long colCnt)
{
	cout << "CEuclideanDistMatrixWorkerMgr Initilize begin ..." << endl;
	bool rt = true;
	for (int i = 0; i<m_workerNum; i++)
	{
		ComputeWorkerPtr worker = new CEuclideanDistMatrixWorker(i, batchValueCnt, colCnt);
		worker->start();
		m_workers.push_back(worker);
	}
	cout << "worker started, workerNum=" << m_workerNum << endl;
	cout << "CEuclideanDistMatrixWorkerMgr Initilize End" << endl;
	return true;
}