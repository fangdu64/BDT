#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <KMeanWorkerMgr.h>
#include <GlobalVars.h>

CKMeanWorkerMgr::CKMeanWorkerMgr(CGlobalVars& gv, int workerNum)
	:m_gv(gv), m_workerNum(workerNum)
{
}


CKMeanWorkerMgr::~CKMeanWorkerMgr()
{
}

void CKMeanWorkerMgr::Initilize()
{
	cout<<"CKMeanWorkerMgr Initilize begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		KMeanWorkerPtr worker= new CKMeanWorker();
		worker->start();
		m_workers.push_back(worker);
	}
	cout<<"worker started, workerNum="<<m_workerNum<<endl;
	cout<<"CFeatureValueWorkerMgr Initilize End"<<endl;
}


void CKMeanWorkerMgr::RequestShutdownAllWorkers()
{
	cout<<"CKMeanWorkerMgr RequestShutdownAllWorkers begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		KMeanWorkerPtr worker= m_workers[i];
		if(worker)
			worker->RequestShutdown();
	}
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CKMeanWorkerMgr RequestShutdownAllWorkers End"<<endl;
}

void CKMeanWorkerMgr::UnInitilize()
{
	cout<<"CKMeanWorkerMgr UnInitilize begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		KMeanWorkerPtr worker= m_workers[i];
		if(worker)
			worker->getThreadControl().join();
	}
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CKMeanWorkerMgr UnInitilize End"<<endl;
}

KMeanWorkerPtr CKMeanWorkerMgr::GetKMeanWorker(int workerIdx)
{
	if(workerIdx<m_workerNum)
		return m_workers[workerIdx];
	else 
		return 0;
}

int CKMeanWorkerMgr::AssignItemToWorker(
	int workerIdx, const KMeansWorkItemPtr& wi)
{
	KMeanWorkerPtr worker = GetKMeanWorker(workerIdx);
	worker->AddWorkItem(wi);
	return 1;
}