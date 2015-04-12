#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/ScopedArray.h>
#include <GlobalVars.h>
#include <ProxyCentralServiceImpl.h>

::Ice::Int
CProxyCentralServiceImpl::RegisterByCallerAdress(const ::std::string& servantName,
                                                  const ::std::string& proxyStrNoHost,
                                                  const Ice::Current& current)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	if(current.con)
	{
		Ice::ConnectionInfoPtr info = current.con->getInfo();
		Ice::TCPConnectionInfoPtr tcpInfo = Ice::TCPConnectionInfoPtr::dynamicCast(info);
		if(tcpInfo)
		{
			ostringstream os;
			os<<proxyStrNoHost<<" -h "<<tcpInfo->remoteAddress;
			m_servantProxyStrs[servantName]=os.str();
			cout<<IceUtil::Time::now().toDateTime()<<" RegisterByCallerAdress, servant="<<servantName
				<<", Proxy="<<os.str()<<endl; 
			return 1;
		}

		
	}
	
    return 0;
}

::Ice::Int
CProxyCentralServiceImpl::RegisterByProxyStr(const ::std::string& servantName,
                                              const ::std::string& proxyStr,
                                              const Ice::Current& current)
{
	IceUtil::Mutex::Lock lock(m_mutex);
    m_servantProxyStrs[servantName]=proxyStr;
	cout<<IceUtil::Time::now().toDateTime()<<" RegisterByProxyStr, servant="<<servantName
				<<", Proxy="<<proxyStr<<endl; 
	return 1;
}

::Ice::Int
CProxyCentralServiceImpl::UnRegister(const ::std::string& servantName,
                                      const Ice::Current& current)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	Str2StrMap_T::iterator it=
		m_servantProxyStrs.find(servantName);
	if(it!=m_servantProxyStrs.end())
	{
		m_servantProxyStrs.erase(it);
		cout<<IceUtil::Time::now().toDateTime()<<" UnRegister, servant="<<servantName<<endl; 
	}
    return 1;
}

::Ice::Int
CProxyCentralServiceImpl::UnRegisterAll(const Ice::Current& current)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	m_servantProxyStrs.clear();
    return 1;
}

::Ice::Int
CProxyCentralServiceImpl::ListAll(::iBS::StringVec& proxyStrs,
                                   const Ice::Current& current)
{
	IceUtil::Mutex::Lock lock(m_mutex);
	for(Str2StrMap_T::iterator it=m_servantProxyStrs.begin();
		it!=m_servantProxyStrs.end();it++)
	{
		proxyStrs.push_back(it->first+"="+it->second);
	}
    return 1;
}

