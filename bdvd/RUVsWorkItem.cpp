#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/Time.h>
#include <GlobalVars.h>
#include <algorithm>    // std::copy
#include <RUVsWorkItem.h>
#include <math.h>
#include <RUVBuilder.h>
#include <StatisticsHelper.h>
#include <RUVVarDecmWorker.h>

void
CRUVsComputeABC::DoWork()
{
	m_ruvBuilder.UpdateYcsYcsT(m_Y, m_featureIdxFrom,m_featureIdxTo,m_A);
	if(!m_AequalB)
	{
		m_ruvBuilder.UpdateYcscfYcscfT(m_Y, m_featureIdxFrom,m_featureIdxTo,m_controlFeatureFlags, m_B);
	}
	m_ruvBuilder.UpdateYcfYcscfT(m_Y,  m_featureIdxFrom,m_featureIdxTo,m_controlFeatureFlags, m_C);
	
}

void
CRUVsComputeABC::CancelWork()
{

}


//////////////////////////////////////////////////////////////////////////////////////////////

void
CRUVComputeRowANOVA::DoWork()
{
	Ice::Long colCnt= m_ruvBuilder.m_RUVInfo.RawCountObserverIDs.size();
	CStatisticsHelper::GetOneWayANOVA(m_Y,colCnt,
		m_featureIdxFrom,m_featureIdxTo,m_ruvBuilder.m_conditionSampleIdxs, m_FStatistics);

	Ice::Long rowCnt=m_featureIdxTo-m_featureIdxFrom;
	for(int i=0;i<rowCnt;i++)
	{
		if(m_FStatistics[i]<m_ruvBuilder.m_controlFeatureANOVAFStatistics)
		{
			m_ruvBuilder.m_controlFeatureFlags[i+m_featureIdxFrom]=1;
		}
	}

}

void
CRUVComputeRowANOVA::CancelWork()
{

}

//////////////////////////////////////////////////////////////////////////////////////////////

void
CRUVVarDecompose::DoWork()
{
	RUVVarDecomposeBatchParams params = {
		m_stage,
		m_featureIdxFrom,
		m_featureIdxTo,
		m_rowMeans,
		m_Y,
		m_pVarDecmWorker->m_ssGrandMean,
		m_pVarDecmWorker->m_ssXb,
		m_pVarDecmWorker->m_ssWa,
		m_pVarDecmWorker->m_ssXbWa,
		m_pVarDecmWorker->m_xbGrandMean,
		m_pVarDecmWorker->m_ssXbTotalVar,
		m_pVarDecmWorker->m_ssXbBgVar,
		m_pVarDecmWorker->m_ssXbWgVar
	};
	m_ruvBuilder.DecomposeVariance_ModeG(params);
}

void
CRUVVarDecompose::CancelWork()
{

}

//////////////////////////////////////////////////////////////////
void
CRUVgComputeA::DoWork()
{
	m_ruvBuilder.UpdateYcfYcfT(m_Y,  m_featureIdxFrom,m_featureIdxTo,m_controlFeatureFlags, m_A);

	if (!m_As.empty())
	{
		m_ruvBuilder.UpdateYcfYcfT_Permuation(
			m_Y, m_featureIdxFrom, m_featureIdxTo,
			m_controlFeatureFlags,
			m_colIdxPermutation, m_As);
	}
}

void
CRUVgComputeA::CancelWork()
{

}

/////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
void
CRUVGetOutput::DoWork()
{
	m_ruvBuilder.GetNormalizedCnts(m_rowCnt, m_Y, m_rowMeans);
}

void
CRUVGetOutput::CancelWork()
{
}