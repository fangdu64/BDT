#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/Time.h>
#include <GlobalVars.h>
#include <algorithm>    // std::copy
#include <ComputeWorkItem.h>
#include <math.h>
#include <ComputeBuilder.h>
#include <bdtUtil/RowAdjustHelper.h>

void
CPearsonCorrelation::DoWork()
{
	bool needRowFlag = m_rowSelection.Enable;
	iBS::ByteVec rowFlags;
	if (needRowFlag)
	{
		rowFlags.resize(m_rowCnt, 0);
		Ice::Double *pY = (Ice::Double*)(&m_Y[0]);
		Ice::Byte *pRowFlags = reinterpret_cast<Ice::Byte*>(&rowFlags[0]);
		Ice::Long selectedCnt =
			CRowAdjustHelper::RowSelectedFlags(pY, m_rowCnt, m_colCnt, m_rowSelection, pRowFlags);
	}

	Ice::Long idx_j = 0;
	Ice::Long idx_k = 0;
	for (Ice::Long i = 0; i < m_rowCnt; i++)
	{
		if (needRowFlag && rowFlags[i]==0)
		{
			continue;
		}

		for (int j = 0; j < m_colCnt; j++)
		{
			idx_j = i*m_colCnt + j;
			m_colSumSquares[j] += (m_Y[idx_j] - m_colMeans[j])*(m_Y[idx_j] - m_colMeans[j]);
			for (int k = j + 1; k < m_colCnt; k++)
			{
				idx_k = i*m_colCnt + k;
				m_A(j, k) += (m_Y[idx_j] - m_colMeans[j])*(m_Y[idx_k] - m_colMeans[k]);
			}
		}
	}
}

void
CPearsonCorrelation::CancelWork()
{

}

///////////////////////////////////////////////////////////////
void
CEuclideanDist::DoWork()
{
	bool needRowFlag = m_rowSelection.Enable;
	iBS::ByteVec rowFlags;
	if (needRowFlag)
	{
		rowFlags.resize(m_rowCnt, 0);
		Ice::Double *pY = (Ice::Double*)(&m_Y[0]);
		Ice::Byte *pRowFlags = reinterpret_cast<Ice::Byte*>(&rowFlags[0]);
		Ice::Long selectedCnt =
			CRowAdjustHelper::RowSelectedFlags(pY, m_rowCnt, m_colCnt, m_rowSelection, pRowFlags);
	}

	Ice::Long idx_j = 0;
	Ice::Long idx_k = 0;
	for (Ice::Long i = 0; i < m_rowCnt; i++)
	{
		if (needRowFlag && rowFlags[i] == 0)
		{
			continue;
		}

		for (int j = 0; j < m_colCnt; j++)
		{
			idx_j = i*m_colCnt + j;
			for (int k = j + 1; k < m_colCnt; k++)
			{
				idx_k = i*m_colCnt + k;
				m_A(j, k) += (m_Y[idx_j] - m_Y[idx_k])*(m_Y[idx_j] - m_Y[idx_k]);
			}
		}
	}
}

void
CEuclideanDist::CancelWork()
{

}