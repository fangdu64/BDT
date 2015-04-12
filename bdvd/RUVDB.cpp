#include <bdt/bdtBase.h>
#include <IceUtil/Random.h>
#include <RUVDB.h>
#include <GlobalVars.h>
#include <algorithm>       // std::numeric_limits
#include <limits>       // std::numeric_limits

CRUVFacetDB::CRUVFacetDB(Ice::CommunicatorPtr communicator, 
							 const std::string& strEnvName,  const std::string& strDBName)
	:m_communicator(communicator),
	m_connection(Freeze::createConnection(communicator, strEnvName)),
	theMaxThreadRandomIdx(256),
	m_map(m_connection, strDBName)
{
	Initilize();
}

CRUVFacetDB::~CRUVFacetDB()
{
	
}

void CRUVFacetDB::Initilize()
{
	cout<<"CRUVFacetDB Initilize begin ..."<<endl; 
	//not in service yet, no concurrency issues

	RUVFreezeMap_T::const_iterator p;
	int cnt=0;
	m_facetIDMax=0;
	for(p=m_map.begin(); p!=m_map.end(); ++p)
	{
		cnt++;
		const iBS::RUVFacetInfo& di=p->second;
		if(di.FacetID>m_facetIDMax)
			m_facetIDMax=di.FacetID;
	}
	cout<<"TotalRUVsFacet="<<cnt<<endl;
	cout<<"CRUVFacetDB Initilize End"<<endl;
}

void CRUVFacetDB::SetRUVFacetInfoBlank(::iBS::RUVFacetInfo& rfi)
{
	rfi.FacetID=0;
	rfi.FacetReady=false;
	rfi.FacetStatus=iBS::RUVFacetStatusNone;
	rfi.RUVMode=iBS::RUVModeRUVs;
	rfi.CtrlSampleCnt=0;
    rfi.FacetName="untitled";
    rfi.FeatureFilterPolicy = iBS::RUVFeatureFilterPolicyMaxCntLowPysicalCopy;
	rfi.FeatureFilterMaxCntLowThreshold = 5;
	rfi.ControlFeaturePolicy = iBS::RUVControlFeaturePolicyMaxCntLow;
	rfi.ControlFeatureMaxCntUpBound = 30;
	rfi.ControlFeatureMaxCntLowBound = 4;
	rfi.CommonLibrarySize = 60000000;
	rfi.ObserverIDforControlFeatureIdxs = 0;
    rfi.Tol=0.0000001;
    rfi.MaxK=0;
    rfi.n=0;
    rfi.L=0;
    rfi.J=0;
    rfi.P=0;
    rfi.K=0;
    rfi.CtrlSampleCnt=0;
	rfi.ObserverIDforWts=0;
    rfi.ObserverIDforTs=0;
	rfi.ThreadRandomIdx=0;
	rfi.ObserverIDforZs=0;
	rfi.ObserverIDforGs=0;
	rfi.grandMeanY = 0;
	rfi.ControlFeatureCnt = 0;
	rfi.FeatureIdxFrom = 0;
	rfi.FeatureIdxTo = 0;
	rfi.SubRangeLibrarySizeAdjust = false;
	rfi.InputAdjust = iBS::RUVInputDoLogE;
	rfi.OIDforEigenValue = 0;
	rfi.OIDforEigenVectors = 0;
}

::Ice::Int
CRUVFacetDB::RqstNewRUVFacet(::iBS::RUVFacetInfo& rfi)
{
    //ensure thread safty
	IceUtil::Mutex::Lock lock(m_mutex);
	
	::Freeze::TransactionHolder tx(m_connection);
	SetRUVFacetInfoBlank(rfi);
	int facetID=++m_facetIDMax;
	rfi.FacetID=facetID;
	rfi.ThreadRandomIdx = IceUtilInternal::random(theMaxThreadRandomIdx);
	m_map.insert(std::make_pair(rfi.FacetID, rfi));
	tx.commit();
    return 1;
}

::Ice::Int CRUVFacetDB::RemoveRUVFacetInfo(Ice::Int facetID)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	RUVFreezeMap_T::iterator p
		= m_map.find(facetID);
	if (p != m_map.end())
	{
		m_map.erase(p);
		if (m_facetIDMax == facetID + 1)
		{
			m_facetIDMax = facetID;
		}
	}
	else
	{
		return 0;
	}

	return 1;
}

::Ice::Int CRUVFacetDB::GetRUVFacetInfo(
	Ice::Int facetID, ::iBS::RUVFacetInfo& rfi)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	RUVFreezeMap_T::const_iterator p
			= m_map.find(facetID);
	if(p !=m_map.end())
	{
		rfi=p->second;
	}
	else
	{
		SetRUVFacetInfoBlank(rfi);
		return 0;
	}

	return 1;
}

::Ice::Int CRUVFacetDB::SetRUVFacetInfo(
	Ice::Int facetID, const ::iBS::RUVFacetInfo& rfi)
{
	IceUtil::Mutex::Lock lock(m_mutex);

	::Freeze::TransactionHolder tx(m_connection); //exception safe
	RUVFreezeMap_T::iterator p
		= m_map.find(facetID);
	if(p ==m_map.end())
	{
		m_map.insert(std::make_pair(facetID, rfi));
	}
	else
	{
		p.set(rfi);
	}
	tx.commit();
	return 1;
}
