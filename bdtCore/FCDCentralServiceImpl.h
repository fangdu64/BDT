#ifndef __FCDCentralServiceImpl_h__
#define __FCDCentralServiceImpl_h__

#include <FCDCentralService.h>

class CFcdcReadServiceImpl;
typedef IceUtil::Handle<CFcdcReadServiceImpl> CFcdcReadServiceImplPtr;

class CFcdcReadWriteServiceImpl;
typedef IceUtil::Handle<CFcdcReadWriteServiceImpl> CFcdcReadWriteServiceImplPtr;

class CFcdcAdminServiceImpl;
typedef IceUtil::Handle<CFcdcAdminServiceImpl> CFcdcAdminServiceImplPtr;

class CGlobalVars;

class CFcdcReadServiceImpl : virtual public iBS::FcdcReadService
{
public:
	CFcdcReadServiceImpl(CGlobalVars& globalVars)
		:m_gv(globalVars)
	{
	}
	virtual ~CFcdcReadServiceImpl(){}

public:
    virtual ::Ice::Int GetFeatureDomains(const ::iBS::IntVec&,
                                         ::iBS::FeatureDomainInfoVec&,
                                         const Ice::Current&);

    virtual ::Ice::Int GetFeatureObservers(const ::iBS::IntVec&,
                                           ::iBS::FeatureObserverInfoVec&,
                                           const Ice::Current&);

    virtual void GetDoublesColumnVector_async(const ::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr&,
                                              ::Ice::Int,
                                              ::Ice::Long,
                                              ::Ice::Long,
                                              const Ice::Current&);

    virtual void GetIntsColumnVector_async(const ::iBS::AMD_FcdcReadService_GetIntsColumnVectorPtr&,
                                           ::Ice::Int,
                                           ::Ice::Long,
                                           ::Ice::Long,
                                           const Ice::Current&);

    virtual void GetDoublesRowMatrix_async(const ::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr&,
                                           ::Ice::Int,
                                           ::Ice::Long,
                                           ::Ice::Long,
                                           const Ice::Current&);

    virtual ::Ice::Int GetObserverStats(::Ice::Int,
                                        ::iBS::ObserverStatsInfo&,
                                        const Ice::Current&);

    virtual ::Ice::Int GetObserversStats(const ::iBS::IntVec&,
                                         ::iBS::ObserverStatsInfoVec&,
                                         const Ice::Current&);

	virtual void GetRowMatrix_async(const ::iBS::AMD_FcdcReadService_GetRowMatrixPtr&,
		const ::iBS::IntVec&,
		::Ice::Long,
		::Ice::Long,
		const IceUtil::Optional< ::iBS::RowAdjustEnum>&,
		const Ice::Current&);

	virtual void SampleRowMatrix_async(const ::iBS::AMD_FcdcReadService_SampleRowMatrixPtr&,
		const ::iBS::IntVec&,
		const ::iBS::LongVec&,
		const IceUtil::Optional< ::iBS::RowAdjustEnum>&,
		const Ice::Current&);

    virtual ::Ice::Int GetObserverIndex(::Ice::Int,
                                        ::iBS::ObserverIndexInfo&,
                                        const Ice::Current&);

    virtual void GetFeatureIdxsByIntKeys_async(const ::iBS::AMD_FcdcReadService_GetFeatureIdxsByIntKeysPtr&,
                                               ::Ice::Int,
                                               const ::iBS::IntVec&,
                                               ::Ice::Long,
                                               const Ice::Current&) const;

    virtual void GetFeatureCntsByIntKeys_async(const ::iBS::AMD_FcdcReadService_GetFeatureCntsByIntKeysPtr&,
                                               ::Ice::Int,
                                               const ::iBS::IntVec&,
                                               const Ice::Current&) const;

	virtual ::Ice::Int GetAMDTaskInfo(::Ice::Long,
		::iBS::AMDTaskInfo&,
		const Ice::Current&);

	virtual ::Ice::Int GetFeatureValueStoreDir(::std::string&,
		const Ice::Current&);

	virtual ::Ice::Int GetFeatureValuePathPrefix(::Ice::Int,
		::std::string&,
		const Ice::Current&);

protected:
	bool rvGetDoublesColumnVector(const ::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr& cb,
									::Ice::Int observerID,
									::Ice::Long featureIdxFrom,
									::Ice::Long featureIdxTo,
									iBS::FeatureObserverSimpleInfoPtr& foi ) const;

	bool rvGetDoublesRowMatrix(const ::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr& cb,
                                ::Ice::Int observerGroupID,
                                ::Ice::Long featureIdxFrom,
                                ::Ice::Long featureIdxTo,
								iBS::FeatureObserverSimpleInfoPtr& foi) const;

	bool rvGetJoinedRowMatrix(	const ::iBS::IntVec& observerIDs,
								::Ice::Long featureIdxFrom,
								::Ice::Long featureIdxTo, iBS::FeatureObserverSimpleInfoVec& fois,
								std::string& reason) const;

	bool rvSampleJoinedRowMatrix(const ::iBS::IntVec& observerIDs,
								Ice::Long rowCnt, 
								iBS::FeatureObserverSimpleInfoVec& fois,
								std::string& reason) const;

protected:
	CGlobalVars&	m_gv;
};


class CFcdcReadWriteServiceImpl : virtual public iBS::FcdcReadWriteService,
                                  virtual public CFcdcReadServiceImpl
{
public:
	CFcdcReadWriteServiceImpl(CGlobalVars& globalVars)
		:CFcdcReadServiceImpl(globalVars)
	{
	}

	virtual ~CFcdcReadWriteServiceImpl(){}

public:

    virtual ::Ice::Int SetFeatureDomains(const ::iBS::FeatureDomainInfoVec&,
                                         const Ice::Current&);

    virtual ::Ice::Int SetFeatureObservers(const ::iBS::FeatureObserverInfoVec&,
                                           const Ice::Current&);

    virtual void SetDoublesColumnVector_async(const ::iBS::AMD_FcdcReadWriteService_SetDoublesColumnVectorPtr&,
                                              ::Ice::Int,
                                              ::Ice::Long,
                                              ::Ice::Long,
                                              const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>&,
                                              const Ice::Current&);

    virtual void SetBytesColumnVector_async(const ::iBS::AMD_FcdcReadWriteService_SetBytesColumnVectorPtr&,
                                            ::Ice::Int,
                                            ::Ice::Long,
                                            ::Ice::Long,
                                            const ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*>&,
                                            ::iBS::ByteArrayContentEnum,
                                            ::iBS::ByteArrayEndianEnum,
                                            const Ice::Current&);

    virtual void SetIntsColumnVector_async(const ::iBS::AMD_FcdcReadWriteService_SetIntsColumnVectorPtr&,
                                           ::Ice::Int,
                                           ::Ice::Long,
                                           ::Ice::Long,
                                           const ::std::pair<const ::Ice::Int*, const ::Ice::Int*>&,
                                           const Ice::Current&);

    virtual void SetDoublesRowMatrix_async(const ::iBS::AMD_FcdcReadWriteService_SetDoublesRowMatrixPtr&,
                                           ::Ice::Int,
                                           ::Ice::Long,
                                           ::Ice::Long,
                                           const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>&,
                                           const Ice::Current&);
};


class CFcdcAdminServiceImpl : virtual public iBS::FcdcAdminService,
							  virtual public CFcdcReadWriteServiceImpl
{
public:
	CFcdcAdminServiceImpl(CGlobalVars& globalVars)
		:CFcdcReadServiceImpl(globalVars),CFcdcReadWriteServiceImpl(globalVars)
	{
	}

	virtual ~CFcdcAdminServiceImpl(){}

public:

    virtual void Shutdown(const Ice::Current&);

    virtual ::Ice::Int RqstNewFeatureDomainID(::Ice::Int&,
                                              const Ice::Current&);

    virtual ::Ice::Int RqstNewFeatureObserverID(bool,
                                                ::Ice::Int&,
                                                const Ice::Current&);

    virtual ::Ice::Int RqstNewFeatureObserversInGroup(::Ice::Int,
                                                      bool,
                                                      ::iBS::IntVec&,
                                                      const Ice::Current&);

	virtual ::Ice::Int AttachBigMatrix(::Ice::Int,
		::Ice::Long,
		const ::iBS::StringVec&,
		const ::std::string&,
		::iBS::IntVec&,
		const Ice::Current&);

	virtual ::Ice::Int AttachBigVector(::Ice::Long,
		const ::std::string&,
		const ::std::string&,
		::Ice::Int&,
		const Ice::Current&);

    virtual void ForceLoadInRAM_async(const ::iBS::AMD_FcdcAdminService_ForceLoadInRAMPtr&,
                                      const ::iBS::IntVec&,
                                      const Ice::Current&) const;

    virtual void ForceLeaveRAM_async(const ::iBS::AMD_FcdcAdminService_ForceLeaveRAMPtr&,
                                     const ::iBS::IntVec&,
                                     const Ice::Current&) const;

    virtual void RecalculateObserverStats_async(const ::iBS::AMD_FcdcAdminService_RecalculateObserverStatsPtr&,
												const ::iBS::IntVec&,
                                                const Ice::Current&);

    virtual void RecalculateObserverIndex_async(const ::iBS::AMD_FcdcAdminService_RecalculateObserverIndexPtr&,
                                                ::Ice::Int,
                                                bool,
                                                const Ice::Current&);
	virtual void RemoveFeatureObservers_async(const ::iBS::AMD_FcdcAdminService_RemoveFeatureObserversPtr&,
		const ::iBS::IntVec&,
		bool,
		const Ice::Current&);

	virtual ::Ice::Int SetObserverStats(const ::iBS::ObserverStatsInfoVec&,
		const Ice::Current&);
};


#endif
