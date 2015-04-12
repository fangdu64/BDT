#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <FeatureValueWorker.h>
#include <RUVFacetWorkItem.h>
#include <algorithm>    // std::copy
#include <GlobalVars.h>
#include <FeatureValueRAM.h>
#include <FeatureValueStoreMgr.h>
#include <FeatureObserverDB.h>
#include <ObserverStatsDB.h>
#include <CommonHelper.h>
#include <RUVBuilder.h>
#include <bdtUtil/RowAdjustHelper.h>
#include <FeatureValueWorkerMgr.h>
//////////////////////////////////////////////////////////////////////
RUVs::CRebuildRUVsModel::~CRebuildRUVsModel()
{

}

void
RUVs::CRebuildRUVsModel::DoWork()
{
	m_ruvBuilder.RebuildRUVModel(m_threadCnt,m_ramMb,m_taskID);
}

void
RUVs::CRebuildRUVsModel::CancelWork()
{

}

//////////////////////////////////////////////////////////////////////
RUVs::CSetActiveK::~CSetActiveK()
{

}

void
RUVs::CSetActiveK::DoWork()
{
	m_ruvBuilder.SetActiveK(m_k, m_extW);
	m_cb->ice_response(1);
}

void
RUVs::CSetActiveK::CancelWork()
{
}

//////////////////////////////////////////////////////////////////////
RUVs::CDecomposeVariance::~CDecomposeVariance()
{

}

void
RUVs::CDecomposeVariance::DoWork()
{
	iBS::RUVVarDecomposeInfoVec vds;
	vds.reserve(m_ks.size());
	for (int i = 0; i < m_ks.size(); i++)
	{
		iBS::RUVVarDecomposeInfo vd;
		vd.k = m_ks[i];
		vd.extW = m_extWs[i];
		vd.grandMean = 0;
		vd.Wa = 0;
		vd.Xb = 0;
		vd.XbWa = 0;
		vd.e = 0;
		vd.totalVar = 0;
		vd.xbGrandMean = 0;
		vd.xbTotalVar = 0;
		vd.xbLocusWgVar = 0;
		vd.xbLocusBgVar = 0;
		vd.wtVecIdxs = m_wtVecIdxs[i];
		vd.featureIdxFrom = m_featureIdxFrom;
		vd.featureIdxTo = m_featureIdxTo;
		vds.push_back(vd);
	}

	Ice::Long taskID = CGlobalVars::get()->theFeatureValueWorkerMgr->RegisterAMDTask(
		"DecomposeVariance", 0);
	m_cb->ice_response(1, vds, taskID);
	m_ruvBuilder.DecomposeVariance(vds, m_threadCnt, m_ramMb, m_outfile, taskID);
}

void
RUVs::CDecomposeVariance::CancelWork()
{
}

//////////////////////////////////////////////////////////////////////
RUVs::CGetDoublesColumnVector::~CGetDoublesColumnVector()
{

}

void
RUVs::CGetDoublesColumnVector::DoWork()
{
	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);
	m_cb->ice_response(1,ret); 
}


///////////////////////////////////////////////////////////////////////


RUVs::CGetDoublesRowMatrix::~CGetDoublesRowMatrix()
{

}

void
RUVs::CGetDoublesRowMatrix::DoWork()
{
	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);
	m_cb->ice_response(1,ret); 
}


///////////////////////////////////////////////////////////////////////////////////////

RUVs::CGetRowMatrix::~CGetRowMatrix()
{

}

void
RUVs::CGetRowMatrix::DoWork()
{
	Ice::Long colCnt = 0;
	Ice::Long rowCnt=m_featureIdxTo-m_featureIdxFrom;

	::IceUtil::ScopedArray<Ice::Double>  retValues(
		m_ruvBuilder.GetNormalizedCnts(
			m_featureIdxFrom, m_featureIdxTo, 
			m_observerIDs,m_rowAdjust, colCnt));
	Ice::Long totalValueCnt = rowCnt*colCnt;

	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);
	ret.first = retValues.get();
	ret.second = retValues.get() + totalValueCnt;
	m_cb->ice_response(1,ret);
}

///////////////////////////////////////////////////////////////////////////////////////

RUVs::CSampleRowMatrix::~CSampleRowMatrix()
{

}

void
RUVs::CSampleRowMatrix::DoWork()
{
	Ice::Long colCnt = 0;
	Ice::Long rowCnt = m_featureIdxs.size();

	::IceUtil::ScopedArray<Ice::Double>  retValues(
		m_ruvBuilder.SampleNormalizedCnts(m_featureIdxs, m_observerIDs, m_rowAdjust, colCnt));
	Ice::Long totalValueCnt = rowCnt*colCnt;

	CRowAdjustHelper::Adjust(retValues.get(), rowCnt, colCnt, m_rowAdjust);
	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);
	ret.first = retValues.get();
	ret.second = retValues.get() + totalValueCnt;
	m_cb->ice_response(1, ret);
}
