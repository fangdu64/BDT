#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/ScopedArray.h>
#include <GlobalVars.h>
#include <KMeanContractorImpl.h>
#include <KMeanContract.h>

void
CKMeanContractorAdminServiceImpl::StartNewContract_async(
const ::iBS::AMD_KMeanContractorAdminService_StartNewContractPtr& cb,
::Ice::Int projectID,
const ::iBS::KMeanServerServicePrx& kmeansServer,
const Ice::Current& current)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	if(m_gv.theOngoingContractCnt>0)
	{
		::iBS::ArgumentException ex;
		ex.reason = " contractor is already working";
		cb->ice_exception(ex);
		return;
	}
	m_gv.theKMeanPrx = kmeansServer;

	cb->ice_response(1);
	CKMeanContractL2Ptr kmeanL2Ptr = new CKMeanContractL2(m_gv, projectID);
	kmeanL2Ptr->Start();

}

::Ice::Int
CKMeanContractorAdminServiceImpl::ResetRootProxy( const ::std::string& pcProxyStr,
                                                  const Ice::Current& current)
{
	return 1;
}
