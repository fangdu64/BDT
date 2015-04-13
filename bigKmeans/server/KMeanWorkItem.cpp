#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/Time.h>
#include <GlobalVars.h>
#include <algorithm>    // std::copy
#include <KMeanProject.h>
#include <KMeanProjectMgr.h>
#include <math.h>

CRequestToBeContractor::~CRequestToBeContractor()
{
}

CReportKCntsAndSums::CReportKCntsAndSums(
	const ::iBS::AMD_KMeanServerService_ReportKCntsAndSumsPtr& cb,
	::Ice::Int projectID,
	::Ice::Int contractorIdx,
	const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& KCnts,
	const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& KSums,
	::Ice::Long KChangeCnt,
	::Ice::Double distortion)
	:m_cb(cb),m_projectID(projectID),m_contractorIdx(contractorIdx),
	m_KCnts(KCnts), m_KSums(KSums), m_KChangeCnt(KChangeCnt), m_distortion(distortion)
{
	//values are owned by caller, should not save for other thread to use

}


CReportKCntsAndSums::~CReportKCntsAndSums()
{
}

//================================================
CReportKMembers::~CReportKMembers()
{
}

CReportKMembers::CReportKMembers(
	const ::iBS::AMD_KMeanServerService_ReportKMembersPtr& cb,
	::Ice::Int projectID,
    ::Ice::Int contractorIdx,
    ::Ice::Long featureIdxFrom,
    ::Ice::Long featureIdxTo,
	const ::std::pair<const ::Ice::Long*, const ::Ice::Long*>& featureIdx2ClusterIdx)
	:m_cb(cb),m_projectID(projectID),m_contractorIdx(contractorIdx),
	m_featureIdxFrom(featureIdxFrom),m_featureIdxTo(featureIdxTo),
	m_featureIdx2ClusterIdx(featureIdx2ClusterIdx)
{
	//values are owned by caller, should not save for other thread to use

}

////////////////////////////////////////////////////////////////////////////
CReportPPDistSum::CReportPPDistSum(
	const ::iBS::AMD_KMeanServerService_ReportPPDistSumPtr& cb,
	::Ice::Int projectID,
	::Ice::Int contractorIdx,
	::Ice::Double distSum)
	:m_cb(cb), m_projectID(projectID), m_contractorIdx(contractorIdx), m_distSum(distSum)
{

}


CReportPPDistSum::~CReportPPDistSum()
{
}

////////////////////////////////////////////////////////////////////////////
CReportNewSeed::CReportNewSeed(
	const ::iBS::AMD_KMeanServerService_ReportNewSeedPtr& cb,
	::Ice::Int projectID,
	::Ice::Int contractorIdx,
	::Ice::Long seedFeatureIdx,
	const ::iBS::DoubleVec& center)
	:m_cb(cb), m_projectID(projectID), m_contractorIdx(contractorIdx),
	m_seedFeatureIdx(seedFeatureIdx), m_center(center)
{

}


CReportNewSeed::~CReportNewSeed()
{
}

////////////////////////////////////////////////////////////////////////////
CGetNextTask::CGetNextTask(
	const ::iBS::AMD_KMeanServerService_GetNextTaskPtr& cb,
	::Ice::Int projectID,
	::Ice::Int contractorIdx)
	:m_cb(cb), m_projectID(projectID), m_contractorIdx(contractorIdx)
{

}

CGetNextTask::~CGetNextTask()
{
}