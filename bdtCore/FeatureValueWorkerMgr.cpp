#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <FeatureValueWorkerMgr.h>
#include <GlobalVars.h>

CFeatureValueWorkerMgr::CFeatureValueWorkerMgr(CGlobalVars& gv, int workerNum)
:m_gv(gv), m_workerNum(workerNum)
{
	m_taskIDMax = 0;
}


CFeatureValueWorkerMgr::~CFeatureValueWorkerMgr()
{
}

void CFeatureValueWorkerMgr::Initilize()
{
	cout<<"CFeatureValueWorkerMgr Initilize begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		FeatureValueWorkerPtr worker= new CFeatureValueWorker();
		worker->start();
		m_workers.push_back(worker);
	}
	cout<<"worker started, workerNum="<<m_workerNum<<endl;
	cout<<"CFeatureValueWorkerMgr Initilize End"<<endl;
}


void CFeatureValueWorkerMgr::RequestShutdownAllWorkers()
{
	cout<<"CFeatureValueWorkerMgr RequestShutdownAllWorkers begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		FeatureValueWorkerPtr worker= m_workers[i];
		if(worker)
			worker->RequestShutdown();
	}
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CFeatureValueWorkerMgr RequestShutdownAllWorkers End"<<endl;
}

void CFeatureValueWorkerMgr::UnInitilize()
{
	cout<<"CFeatureValueWorkerMgr UnInitilize begin ..."<<endl; 
	
	for(int i=0;i<m_workerNum;i++)
	{
		FeatureValueWorkerPtr worker= m_workers[i];
		if(worker)
			worker->getThreadControl().join();
	}
	cout<<"worker stopped, workerNum="<<m_workerNum<<endl;
	cout<<"CFeatureValueWorkerMgr UnInitilize End"<<endl;
}

FeatureValueWorkerPtr CFeatureValueWorkerMgr::GetFeatureValueWorker(
			const iBS::FeatureObserverSimpleInfoPtr& foi)
{
	int workerIdx=foi->ThreadRandomIdx%m_workerNum;
	return m_workers[workerIdx];
}

int CFeatureValueWorkerMgr::AssignItemToWorker(
	const iBS::FeatureObserverSimpleInfoPtr& foi, const FeatureValueWorkItemPtr& wi)
{
	FeatureValueWorkerPtr worker = GetFeatureValueWorker(foi);
	worker->AddWorkItem(wi);
	return 1;
}

int CFeatureValueWorkerMgr::AssignItemToWorkerByTime(const FeatureValueWorkItemPtr& wi)
{
	Ice::Long ms=IceUtil::Time::now().toMicroSeconds();
	int workerIdx=ms%m_workerNum;
	if(workerIdx<0)
	{
		return 0;
	}
	m_workers[workerIdx]->AddWorkItem(wi);
	return 1;
}


Ice::Long CFeatureValueWorkerMgr::RegisterAMDTask(const ::std::string& taskName, Ice::Long progressTotalCnt)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	Ice::Long taskID = ++m_taskIDMax;
	iBS::AMDTaskInfo task;
	task.TaskID = taskID;
	task.TaskName = taskName;
	task.TotalCnt = progressTotalCnt;
	task.FinishedCnt = 0;
	task.Status = iBS::AMDTaskStatusNormal;
	m_tasks.insert(std::pair < Ice::Long, iBS::AMDTaskInfo >(taskID, task));
	return taskID;
}

int CFeatureValueWorkerMgr::GetAMDTask(Ice::Long taskID, ::iBS::AMDTaskInfo& task)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	Tasks_T::const_iterator it = m_tasks.find(taskID);
	if (it == m_tasks.end())
	{
		task.TaskID = taskID;
		task.TotalCnt = 0;
		task.FinishedCnt = 0;
		task.Status = iBS::AMDTaskStatusNotExist;
		return 0;
	}
	else
	{
		task = it->second;
		return 1;
	}
}

int CFeatureValueWorkerMgr::InitAMDSubTask(Ice::Long taskID, const::std::string& taskName, Ice::Long subTotalCnt)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	Tasks_T::iterator it = m_tasks.find(taskID);
	if (it == m_tasks.end())
	{
		return 0;
	}
	else
	{
		it->second.FinishedCnt = 0;
		it->second.TaskName = taskName;
		it->second.TotalCnt = subTotalCnt;
		return 1;
	}
}

int CFeatureValueWorkerMgr::UpdateAMDTaskProgress(Ice::Long taskID, Ice::Long changeCnt, Ice::Long totalCnt, iBS::AMDTaskStatusEnum status)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	Tasks_T::iterator it = m_tasks.find(taskID);
	if (it == m_tasks.end())
	{
		return 0;
	}
	else
	{
		it->second.FinishedCnt += changeCnt;
		if (totalCnt!=-1)
		{
			it->second.TotalCnt = totalCnt;
		}
		it->second.Status = status;
		return 1;
	}
}

int CFeatureValueWorkerMgr::IncreaseAMDTaskTotalCnt(
	Ice::Long taskID, Ice::Long increment)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	Tasks_T::iterator it = m_tasks.find(taskID);
	if (it == m_tasks.end())
	{
		return 0;
	}
	else
	{
		it->second.TotalCnt += increment;
		return 1;
	}
}


int CFeatureValueWorkerMgr::SetAMDTaskDone(Ice::Long taskID)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	Tasks_T::iterator it = m_tasks.find(taskID);
	if (it == m_tasks.end())
	{
		return 0;
	}
	else
	{
		it->second.FinishedCnt = it->second.TotalCnt;
		
		it->second.Status = iBS::AMDTaskStatusFinished;
		return 1;
	}
}