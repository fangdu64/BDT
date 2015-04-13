#ifndef __KMeanServiceImpl_h__
#define __KMeanServiceImpl_h__

#include <FCDCentralService.h>
#include <KMeanService.h>

class CKMeanServerAdminServiceImpl;
typedef IceUtil::Handle<CKMeanServerAdminServiceImpl> CKMeanServerAdminServiceImplPtr;

class CKMeanServerServiceImpl;
typedef IceUtil::Handle<CKMeanServerServiceImpl> CKMeanServerServiceImplPtr;


class CGlobalVars;

class CKMeanServerAdminServiceImpl : virtual public iBS::KMeanServerAdminService
{
public:
	CKMeanServerAdminServiceImpl(CGlobalVars& globalVars)
		:m_gv(globalVars)
	{
	}
	virtual ::iBS::KMeanServerServicePrx GetKMeansSeverProxy(const Ice::Current&);

	virtual ::Ice::Int GetBlankProject(::iBS::KMeanProjectInfoPtr&,
                                       const Ice::Current&);

    virtual ::Ice::Int CreateProjectAndWaitForContractors(const ::iBS::KMeanProjectInfoPtr&,
                                                          ::iBS::KMeanProjectInfoPtr&,
                                                          const Ice::Current&);

	virtual ::Ice::Int LaunchProjectWithCurrentContractors(::Ice::Int,
		::Ice::Long&,
		const Ice::Current&);

	virtual ::Ice::Int DestroyProject(::Ice::Int,
                                      const Ice::Current&);
	virtual ::Ice::Int GetAMDTaskInfo(::Ice::Long,
		::iBS::AMDTaskInfo&,
		const Ice::Current&);
private:
	CGlobalVars&	m_gv;
};

class CKMeanServerServiceImpl : virtual public iBS::KMeanServerService
{
public:
	CKMeanServerServiceImpl(CGlobalVars& globalVars)
		:m_gv(globalVars)
	{
	}

    virtual ::Ice::Int ReportStatus(::Ice::Int,
                                    ::Ice::Int,
                                    const ::std::string&,
                                    const ::std::string&,
                                    ::iBS::KMeanSvrMsgEnum&,
                                    const Ice::Current&);

    virtual void RequestToBeContractor_async(const ::iBS::AMD_KMeanServerService_RequestToBeContractorPtr&,
                                             ::Ice::Int,
                                             const ::std::string&,
                                             ::Ice::Int,
                                             ::Ice::Int,
                                             const Ice::Current&);

    virtual void ReportKCntsAndSums_async(const ::iBS::AMD_KMeanServerService_ReportKCntsAndSumsPtr&,
                                          ::Ice::Int,
                                          ::Ice::Int,
                                          const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>&,
                                          const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>&,
										  ::Ice::Long,
										  ::Ice::Double,
                                          const Ice::Current&);

	 virtual void ReportKMembers_async(const ::iBS::AMD_KMeanServerService_ReportKMembersPtr&,
                                      ::Ice::Int,
                                      ::Ice::Int,
                                      ::Ice::Long,
                                      ::Ice::Long,
                                      const ::std::pair<const ::Ice::Long*, const ::Ice::Long*>&,
                                      const Ice::Current&);

	 virtual void GetKClusters_async(const ::iBS::AMD_KMeanServerService_GetKClustersPtr&,
		 ::Ice::Int,
		 const Ice::Current&);

	 virtual void GetKMembers_async(const ::iBS::AMD_KMeanServerService_GetKMembersPtr&,
                                   ::Ice::Int,
                                   ::Ice::Long,
                                   ::Ice::Long,
                                   const Ice::Current&);

    virtual void GetKCnts_async(const ::iBS::AMD_KMeanServerService_GetKCntsPtr&,
                                ::Ice::Int,
                                const Ice::Current&);

	virtual void ReportPPDistSum_async(const ::iBS::AMD_KMeanServerService_ReportPPDistSumPtr&,
		::Ice::Int,
		::Ice::Int,
		::Ice::Double,
		const Ice::Current&);

	virtual void ReportNewSeed_async(const ::iBS::AMD_KMeanServerService_ReportNewSeedPtr&,
		::Ice::Int,
		::Ice::Int,
		::Ice::Long,
		const ::iBS::DoubleVec&,
		const Ice::Current&);

	virtual void GetPPSeedFeatureIdxs_async(const ::iBS::AMD_KMeanServerService_GetPPSeedFeatureIdxsPtr&,
		::Ice::Int,
		const Ice::Current&);

	virtual void GetNextTask_async(const ::iBS::AMD_KMeanServerService_GetNextTaskPtr&,
		::Ice::Int,
		::Ice::Int,
		const Ice::Current&);

	virtual void GetKMeansResults_async(const ::iBS::AMD_KMeanServerService_GetKMeansResultsPtr&,
		::Ice::Int,
		const Ice::Current&);

	virtual void GetKSeeds_async(const ::iBS::AMD_KMeanServerService_GetKSeedsPtr&,
		::Ice::Int,
		const Ice::Current&);

private:
	CGlobalVars&	m_gv;
};


#endif
