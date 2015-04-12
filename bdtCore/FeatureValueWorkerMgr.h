#ifndef __FeatureValueWorkerMgr_h__
#define __FeatureValueWorkerMgr_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/Mutex.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <FeatureValueWorker.h>
class CGlobalVars;
class CFeatureValueWorkerMgr
{
public:
	CFeatureValueWorkerMgr(CGlobalVars& gv, int workerNum);
	~CFeatureValueWorkerMgr();

public:

	void Initilize();
	void RequestShutdownAllWorkers();
	void UnInitilize();

	int AssignItemToWorker(const iBS::FeatureObserverSimpleInfoPtr& foi, 
		const FeatureValueWorkItemPtr& wi);
	//get a determined worker by observer info
	FeatureValueWorkerPtr GetFeatureValueWorker(
			const iBS::FeatureObserverSimpleInfoPtr& foi);

	int AssignItemToWorkerByTime(const FeatureValueWorkItemPtr& wi);

	Ice::Long RegisterAMDTask(const ::std::string& taskName, Ice::Long progressTotalCnt);
	int GetAMDTask(Ice::Long taskID, ::iBS::AMDTaskInfo& task);
	int InitAMDSubTask(Ice::Long taskID, const::std::string& taskName, Ice::Long subTotalCnt);
	int UpdateAMDTaskProgress(Ice::Long taskID, Ice::Long changeCnt, Ice::Long totalCnt = -1, iBS::AMDTaskStatusEnum status = iBS::AMDTaskStatusNormal);
	int IncreaseAMDTaskTotalCnt(Ice::Long taskID, Ice::Long increment);
	int SetAMDTaskDone(Ice::Long taskID);
	

private:

	
private:
	CGlobalVars&	m_gv;
	const int m_workerNum;
	typedef std::vector<FeatureValueWorkerPtr> FeatureValueWorkers_T;
	FeatureValueWorkers_T m_workers;
	IceUtil::Mutex		m_mutex;
	typedef std::map<Ice::Long, iBS::AMDTaskInfo> Tasks_T;
	Ice::Long m_taskIDMax;
	Tasks_T m_tasks;
};
#endif

