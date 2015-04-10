#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <bdtUtil/AMDTaskHelper.h>

CAMDTaskMgr::CAMDTaskMgr()
:m_taskIDMax(0)
{
	
}


CAMDTaskMgr::~CAMDTaskMgr()
{
}

void CAMDTaskMgr::Initilize()
{

}

void CAMDTaskMgr::UnInitilize()
{
}

Ice::Long CAMDTaskMgr::RegisterAMDTask(const ::std::string& taskName, Ice::Long progressTotalCnt)
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

int CAMDTaskMgr::GetAMDTask(Ice::Long taskID, ::iBS::AMDTaskInfo& task)
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

int CAMDTaskMgr::ChangeTaskName(Ice::Long taskID, const::std::string& taskName)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	Tasks_T::iterator it = m_tasks.find(taskID);
	if (it == m_tasks.end())
	{
		return 0;
	}
	else
	{
		it->second.TaskName = taskName;
		return 1;
	}
}

int CAMDTaskMgr::InitAMDSubTask(Ice::Long taskID, const::std::string& taskName, Ice::Long subTotalCnt)
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

int CAMDTaskMgr::UpdateAMDTaskProgress(Ice::Long taskID, Ice::Long changeCnt, Ice::Long totalCnt, iBS::AMDTaskStatusEnum status)
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
		if (totalCnt != -1)
		{
			it->second.TotalCnt = totalCnt;
		}
		it->second.Status = status;
		return 1;
	}
}

int CAMDTaskMgr::IncreaseAMDTaskTotalCnt(
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


int CAMDTaskMgr::SetAMDTaskDone(Ice::Long taskID)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	Tasks_T::iterator it = m_tasks.find(taskID);
	if (it == m_tasks.end())
	{
		return 0;
	}
	else
	{
		//it->second.FinishedCnt = it->second.TotalCnt;

		it->second.Status = iBS::AMDTaskStatusFinished;
		return 1;
	}
}