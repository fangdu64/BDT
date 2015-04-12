#ifndef __ProxyCentralServiceImpl_h__
#define __ProxyCentralServiceImpl_h__

#include <IceUtil/Mutex.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>

class CProxyCentralServiceImpl;
typedef IceUtil::Handle<CProxyCentralServiceImpl> CProxyCentralServiceImplPtr;
class CGlobalVars;

class CProxyCentralServiceImpl : virtual public iBS::ProxyCentralService
{
public:
	CProxyCentralServiceImpl(CGlobalVars& globalVars)
		:m_gv(globalVars)
	{
	}

	virtual ::Ice::Int RegisterByCallerAdress(const ::std::string&,
                                              const ::std::string&,
                                              const Ice::Current&);

    virtual ::Ice::Int RegisterByProxyStr(const ::std::string&,
                                          const ::std::string&,
                                          const Ice::Current&);

    virtual ::Ice::Int UnRegister(const ::std::string&,
                                  const Ice::Current&);

    virtual ::Ice::Int UnRegisterAll(const Ice::Current&);

	virtual ::Ice::Int ListAll(::iBS::StringVec&,
                               const Ice::Current&);

private:
	CGlobalVars&	m_gv;
	typedef std::map<std::string, std::string> Str2StrMap_T;
	Str2StrMap_T m_servantProxyStrs;
	IceUtil::Mutex				m_mutex;
};


#endif
