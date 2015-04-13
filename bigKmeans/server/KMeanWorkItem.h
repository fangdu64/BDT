#ifndef __KMeanWorkItem_h__
#define __KMeanWorkItem_h__

#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <KMeanService.h>

using namespace iBS;

class CGlobalVars;

class CRequestToBeContractor;
typedef IceUtil::Handle<CRequestToBeContractor> RequestToBeContractorPtr;

class CRequestToBeContractor : public IceUtil::Shared
{

public:
		CRequestToBeContractor(
			const ::iBS::AMD_KMeanServerService_RequestToBeContractorPtr& cb,
			::Ice::Int projectID,
			const ::std::string& contractorName,
			::Ice::Int workerCnt,
			::Ice::Int ramSize)
			:m_cb(cb),
			m_projectID(projectID),m_contractorName(contractorName),
			m_workerCnt(workerCnt), m_ramSize(ramSize)
		{
		}

		virtual ~CRequestToBeContractor();

public:
	::iBS::AMD_KMeanServerService_RequestToBeContractorPtr m_cb;
	::Ice::Int		m_projectID;
	::std::string   m_contractorName;
	::Ice::Int		m_workerCnt;
	::Ice::Int		m_ramSize;
};

class CReportKCntsAndSums;
typedef IceUtil::Handle<CReportKCntsAndSums> ReportKCntsAndSumsPtr;
class CReportKCntsAndSums : public IceUtil::Shared
{

public:
		CReportKCntsAndSums(
			const ::iBS::AMD_KMeanServerService_ReportKCntsAndSumsPtr& cb,
			::Ice::Int projectID,
			::Ice::Int contractorIdx,
			const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& KCnts,
			const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& KSums,
			::Ice::Long KChangeCnt,
			::Ice::Double distortion);

		virtual ~CReportKCntsAndSums();

public:
	::iBS::AMD_KMeanServerService_ReportKCntsAndSumsPtr m_cb;
    ::Ice::Int  m_projectID;
    ::Ice::Int  m_contractorIdx;
    ::std::pair<const ::Ice::Double*, const ::Ice::Double*>  m_KCnts;
	::std::pair<const ::Ice::Double*, const ::Ice::Double*>  m_KSums;
	::Ice::Long m_KChangeCnt;
	::Ice::Double m_distortion;
};


class CReportKMembers;
typedef IceUtil::Handle<CReportKMembers> ReportKMembersPtr;
class CReportKMembers : public IceUtil::Shared
{

public:
		CReportKMembers(
			const ::iBS::AMD_KMeanServerService_ReportKMembersPtr& cb,
			::Ice::Int projectID,
            ::Ice::Int contractorIdx,
            ::Ice::Long featureIdxFrom,
            ::Ice::Long featureIdxTo,
            const ::std::pair<const ::Ice::Long*, const ::Ice::Long*>& featureIdx2ClusterIdx);

		virtual ~CReportKMembers();

public:
	::iBS::AMD_KMeanServerService_ReportKMembersPtr m_cb;
    ::Ice::Int  m_projectID;
    ::Ice::Int  m_contractorIdx;
	::Ice::Long m_featureIdxFrom;
	::Ice::Long m_featureIdxTo;
	::std::pair<const ::Ice::Long*, const ::Ice::Long*> m_featureIdx2ClusterIdx;
};

//////////////////////////////////////////////////////////////////////////////
class CReportPPDistSum;
typedef IceUtil::Handle<CReportPPDistSum> ReportPPDistSumPtr;
class CReportPPDistSum : public IceUtil::Shared
{

public:
	CReportPPDistSum(
		const ::iBS::AMD_KMeanServerService_ReportPPDistSumPtr& cb,
		::Ice::Int projectID,
		::Ice::Int contractorIdx,
		::Ice::Double distSum);

	virtual ~CReportPPDistSum();

public:
	::iBS::AMD_KMeanServerService_ReportPPDistSumPtr m_cb;
	::Ice::Int  m_projectID;
	::Ice::Int  m_contractorIdx;
	::Ice::Double m_distSum;
};


class CReportNewSeed;
typedef IceUtil::Handle<CReportNewSeed> ReportNewSeedPtr;
class CReportNewSeed : public IceUtil::Shared
{

public:
	CReportNewSeed(
		const ::iBS::AMD_KMeanServerService_ReportNewSeedPtr& cb,
		::Ice::Int projectID,
		::Ice::Int contractorIdx,
		::Ice::Long seedFeatureIdx,
		const ::iBS::DoubleVec& center);

	virtual ~CReportNewSeed();

public:
	::iBS::AMD_KMeanServerService_ReportNewSeedPtr m_cb;
	::Ice::Int  m_projectID;
	::Ice::Int  m_contractorIdx;
	::Ice::Long m_seedFeatureIdx;
	::iBS::DoubleVec m_center;
};

class CGetNextTask;
typedef IceUtil::Handle<CGetNextTask> GetNextTaskPtr;
class CGetNextTask : public IceUtil::Shared
{

public:
	CGetNextTask(
		const ::iBS::AMD_KMeanServerService_GetNextTaskPtr& cb,
		::Ice::Int projectID,
		::Ice::Int contractorIdx);

	virtual ~CGetNextTask();

public:
	::iBS::AMD_KMeanServerService_GetNextTaskPtr m_cb;
	::Ice::Int  m_projectID;
	::Ice::Int  m_contractorIdx;
};


#endif

