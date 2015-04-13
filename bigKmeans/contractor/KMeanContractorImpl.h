#ifndef __KMeanContractorImpl_h__
#define __KMeanContractorImpl_h__

#include <FCDCentralService.h>
#include <KMeanService.h>
#include <KMeanCommonDefine.h>

class CKMeanContractorAdminServiceImpl;
typedef IceUtil::Handle<CKMeanContractorAdminServiceImpl> CKMeanContractorAdminServiceImplPtr;

class CGlobalVars;

class CKMeanContractorAdminServiceImpl : virtual public iBS::KMeanContractorAdminService
{
public:
	CKMeanContractorAdminServiceImpl(CGlobalVars& globalVars)
		:m_gv(globalVars)
	{
	}

	virtual void StartNewContract_async(const ::iBS::AMD_KMeanContractorAdminService_StartNewContractPtr&,
		::Ice::Int,
		const ::iBS::KMeanServerServicePrx&,
		const Ice::Current&);

	virtual ::Ice::Int ResetRootProxy(const ::std::string&,
		const Ice::Current&);

private:
	CGlobalVars&	m_gv;
	
	IceUtil::Mutex	m_mutex;
};


#endif
