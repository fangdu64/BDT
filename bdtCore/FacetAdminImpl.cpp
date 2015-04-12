#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/ScopedArray.h>
#include <GlobalVars.h>
#include <FacetAdminImpl.h>
#include <FeatureDomainDB.h>
#include <FeatureObserverDB.h>
#include <FeatureValueRAM.h>
#include <FeatureValueWorker.h>
#include <FeatureValueWorkItem.h>
#include <FeatureValueWorkerMgr.h>
#include <ObserverStatsDB.h>
#include <ObserverIndexDB.h>
#include <BigMatrixFacetImpl.h>
//=================================================================
// FcdcFacetAdminService
//=================================================================

::iBS::BigMatrixServicePrx
CFcdcFacetAdminServiceImpl::GetBigMatrixFacet(::Ice::Int gid,
const Ice::Current& current)
{
	Ice::Identity id;
	id.name = CBigMatrixServiceImpl::GetServantName(gid);

	if (current.adapter->find(id).get())
	{
		return iBS::BigMatrixServicePrx::uncheckedCast(
			current.adapter->createProxy(id));
	}

	iBS::FeatureObserverSimpleInfoPtr g_foi
		= CGlobalVars::get()->theObserversDB.GetFeatureObserver(gid);

	if (!g_foi || g_foi->ObserverGroupSize<1)
	{
		::iBS::ArgumentException ex;
		ex.reason = "illegal observer group id";
		throw ex;
	}

	int observerCnt = g_foi->ObserverGroupSize;
	iBS::IntVec observerIDs(observerCnt, 0);
	for (int i = 0; i < observerCnt; i++)
	{
		observerIDs[i] = gid + i;
	}

	iBS::FeatureObserverSimpleInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs, fois);
	

	CBigMatrixServiceImplPtr bigMatrixService
		= new CBigMatrixServiceImpl((*CGlobalVars::get()), fois);

	return iBS::BigMatrixServicePrx::uncheckedCast(
		current.adapter->add(bigMatrixService, id));
}