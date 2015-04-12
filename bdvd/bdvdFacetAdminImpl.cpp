#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/ScopedArray.h>
#include <GlobalVars.h>
#include <bdvdGlobalVars.h>
#include <bdvdFacetAdminImpl.h>
#include <FeatureDomainDB.h>
#include <FeatureObserverDB.h>
#include <FeatureValueRAM.h>
#include <FeatureValueWorker.h>
#include <FeatureValueWorkItem.h>
#include <FeatureValueWorkerMgr.h>
#include <ObserverStatsDB.h>
#include <ObserverIndexDB.h>
#include <RUVDB.h>
#include <RUVFacetImpl.h>
#include <BigMatrixFacetImpl.h>
//=================================================================
// FcdcFacetAdminService
//=================================================================
::Ice::Int
CBdvdFacetAdminServiceImpl::RqstNewRUVFacet(::iBS::RUVFacetInfo& rfi,
                                              const Ice::Current& current)
{
	CRUVFacetDB *pRUVsDB = CBdvdGlobalVars::get()->theRUVFacetDB;

	return pRUVsDB->RqstNewRUVFacet(rfi);
}

::Ice::Int
CBdvdFacetAdminServiceImpl::GetRUVFacetInfo(::Ice::Int facetID,
                                              ::iBS::RUVFacetInfo& rfi,
                                              const Ice::Current& current)
{
	CRUVFacetDB *pRUVsDB = CBdvdGlobalVars::get()->theRUVFacetDB;
	return pRUVsDB->GetRUVFacetInfo(facetID,rfi);
}


::Ice::Int
CBdvdFacetAdminServiceImpl::RemoveRUVFacet(::Ice::Int facetID,
const Ice::Current& current)
{
	::iBS::RUVFacetInfo rfi;
	CRUVFacetDB *pRUVsDB = CBdvdGlobalVars::get()->theRUVFacetDB;
	::Ice::Int rt= pRUVsDB->GetRUVFacetInfo(facetID, rfi);
	if (rt==0)
	{
		return 0;
	}

	pRUVsDB->RemoveRUVFacetInfo(facetID);
	return 1;
}

::Ice::Int
CBdvdFacetAdminServiceImpl::SetRUVFacetInfo(const ::iBS::RUVFacetInfo& rfi,
                                              const Ice::Current& current)
{
	CRUVFacetDB *pRUVsDB = CBdvdGlobalVars::get()->theRUVFacetDB;
	return pRUVsDB->SetRUVFacetInfo(rfi.FacetID,rfi);
}

::iBS::FcdcRUVServicePrx
CBdvdFacetAdminServiceImpl::GetRUVFacet(::Ice::Int facetID,
                                               const Ice::Current& current)
{
	
	Ice::Identity id;
	id.name = CFcdcRUVServiceImpl::GetServantName(facetID);

	if (current.adapter->find(id).get())
	{
		return iBS::FcdcRUVServicePrx::uncheckedCast(
			current.adapter->createProxy(id));
	}


	CRUVFacetDB *pRUVsDB = CBdvdGlobalVars::get()->theRUVFacetDB;
	::iBS::RUVFacetInfo rfi;
	int rt = pRUVsDB->GetRUVFacetInfo(facetID, rfi);
	if (rt == 0)
	{
		::iBS::ArgumentException ex;
		ex.reason = "invalid facetID";
		throw ex;
	}

	iBS::IntVec& observerIDs = rfi.SampleIDs; //pre-filtered observers

	size_t observerCnt = observerIDs.size();
	if (observerCnt == 0)
	{
		::iBS::ArgumentException ex;
		ex.reason = "empty observer";
		throw ex;
	}

	iBS::FeatureObserverSimpleInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs, fois);
	Ice::Long domainSize = 0;
	for (size_t i = 0; i<observerCnt; i++)
	{
		iBS::FeatureObserverSimpleInfoPtr foi = fois[i];
		if (!foi){
			::iBS::ArgumentException ex;
			ex.reason = "illegal observer ID";
			throw ex;
		}
		if (domainSize == 0)
		{
			domainSize = foi->DomainSize;
		}
		if (domainSize != foi->DomainSize)
		{
			::iBS::ArgumentException ex;
			ex.reason = "observers not with same domain size";
			throw ex;
		}
	}

	//make sure observer stats is ready
	::iBS::ObserverStatsInfoVec observerStats;
	if (rfi.CommonLibrarySize > 0)
	{
		rt = CGlobalVars::get()->theObserverStatsDB->GetObserversStats(observerIDs, observerStats);
		if (rt != 1)
		{
			ostringstream os;
			os << " stats not ready, observer ID=" << observerIDs[observerStats.size()];
			::iBS::ArgumentException ex;
			ex.reason = os.str();
			throw ex;
		}
	}


	CFcdcRUVServiceImplPtr RUVsService
		= new CFcdcRUVServiceImpl((*CGlobalVars::get()), rfi, fois, observerStats);

	return iBS::FcdcRUVServicePrx::uncheckedCast(
		current.adapter->add(RUVsService, id));
}
