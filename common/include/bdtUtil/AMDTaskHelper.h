#ifndef __AMDTaskHelper_h__
#define __AMDTaskHelper_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/Mutex.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>

class CAMDTaskMgr
{
public:
	CAMDTaskMgr();
	~CAMDTaskMgr();

public:

	void Initilize();
	void UnInitilize();

	Ice::Long RegisterAMDTask(const ::std::string& taskName, Ice::Long progressTotalCnt);
	int GetAMDTask(Ice::Long taskID, ::iBS::AMDTaskInfo& task);
	int InitAMDSubTask(Ice::Long taskID, const::std::string& taskName, Ice::Long subTotalCnt);
	int UpdateAMDTaskProgress(Ice::Long taskID, Ice::Long changeCnt, Ice::Long totalCnt = -1, iBS::AMDTaskStatusEnum status = iBS::AMDTaskStatusNormal);
	int IncreaseAMDTaskTotalCnt(Ice::Long taskID, Ice::Long increment);
	int ChangeTaskName(Ice::Long taskID, const::std::string& taskName);
	int SetAMDTaskDone(Ice::Long taskID);

private:


private:
	IceUtil::Mutex		m_mutex;
	typedef std::map<Ice::Long, iBS::AMDTaskInfo> Tasks_T;
	Ice::Long m_taskIDMax;
	Tasks_T m_tasks;
};
#endif