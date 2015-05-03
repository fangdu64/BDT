#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <FeatureValueWorker.h>
#include <RUVBuilder.h>
#include <algorithm>    // std::copy
#include <limits>       // std::numeric_limits
#include <GlobalVars.h>
#include <bdvdGlobalVars.h>
#include <FeatureValueRAM.h>
#include <FeatureValueStoreMgr.h>
#include <FeatureObserverDB.h>
#include <CommonHelper.h>
#include <math.h>
#include <armadillo>
#include <RUVDB.h>
#include <RUVsWorkerMgr.h>
#include <RUVsWorker.h>
#include <RUVRowANOVAWorkerMgr.h>
#include <RUVRowANOVAWorker.h>
#include <RUVVarDecmWorkerMgr.h>
#include <RUVVarDecmWorker.h>
#include <RUVgWorkerMgr.h>
#include <RUVgWorker.h>
#include <RUVsWorkItem.h>
#include <StatisticsHelper.h>
#include <bdtUtil/RowAdjustHelper.h>
#include <FeatureValueWorkerMgr.h>

CRUVBuilder::CRUVBuilder(const ::iBS::RUVFacetInfo& RUVInfo, 
const ::iBS::FeatureObserverSimpleInfoVec& fois,
const ::iBS::ObserverStatsInfoVec& osis)
:m_RUVInfo(RUVInfo), m_preFilterFOIs(fois),m_osis(osis)
{
	m_shutdownRequested = false;
	m_needNotify=false;
	m_controlFeatureTotalCnt=0;
	m_activeK=0;
	m_extentW = 0;
	m_grandMeanY = std::numeric_limits< ::Ice::Double >::quiet_NaN();
	m_outputScale = iBS::RUVOutputScaleLog;
	m_outputMode = iBS::RUVOutputModeYminusZY;
	m_outputWorkerNum = 1;
	m_VDfeatureIdxFrom = 0;
	m_VDfeatureIdxTo = 0;
	m_rebuidRUVTaskID = 0;
	m_toplevelDVTaskID = 0;
	m_ctrlQuantile = 0;
	m_ctrlQuantileAllInFraction = 1.0;
	Initialize();
}
CRUVBuilder::~CRUVBuilder()
{
	
}

bool CRUVBuilder::Initialize()
{
	if(m_RUVInfo.FacetStatus==iBS::RUVFacetStatusNone)
	{
		return Initialize_PreFilter();
	}
	else
	{
		return Initialize_AfterFilter();
	}

}
bool CRUVBuilder::Initialize_PreFilter()
{
	//sample count
	m_RUVInfo.n=m_RUVInfo.SampleIDs.size();
	Ice::Long sampleCnt=m_RUVInfo.n;

	//condition(e.g.,cell line) count
	m_RUVInfo.P=m_RUVInfo.ReplicateSampleIDs.size();
	Ice::Long conditionCnt=m_RUVInfo.P;

	SetupLibraryFactors();
	//feature count (all samples should have a common feature count)
	m_RUVInfo.J=0;
	m_preFilterSampleID2sampleIdx.clear();
	for(int i=0;i<sampleCnt;i++)
	{
		int observerID=m_RUVInfo.SampleIDs[i];
		::iBS::FeatureObserverSimpleInfoPtr foi=m_preFilterFOIs[i];
		if(m_RUVInfo.J==0)
		{
			m_RUVInfo.J=foi->DomainSize;
		}
		else if(m_RUVInfo.J!=foi->DomainSize)
		{
			//should have the same size
			return false;
		}

		int sampleID = m_RUVInfo.SampleIDs[i];
		m_preFilterSampleID2sampleIdx.insert(std::pair<int, int>(sampleID, i));
	}

	createObserverGroupForFilteredY();
	return true;
}

void CRUVBuilder::SetupLibraryFactors()
{
	m_libraryFactors.clear();

	if (m_RUVInfo.NormalizeFactors.size() == m_RUVInfo.n)
	{
		//if set, highest priority
		m_libraryFactors = m_RUVInfo.NormalizeFactors;
	}
	else
	{
		m_libraryFactors.resize(m_RUVInfo.n, m_RUVInfo.CommonLibrarySize);

		for (int i = 0; i<m_RUVInfo.n; i++)
		{
			if (m_RUVInfo.CommonLibrarySize>0)
			{
				m_libraryFactors[i] /= m_osis[i].Sum;
			}
			else
			{
				m_libraryFactors[i] = 1.0;
			}
		}
	}

}

bool CRUVBuilder::Initialize_AfterFilter()
{
	m_fois.clear();
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(m_RUVInfo.RawCountObserverIDs,m_fois);

	//sample count
	m_RUVInfo.n=m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long sampleCnt=m_RUVInfo.n;

	//condition(e.g.,cell line) count
	m_RUVInfo.P=m_RUVInfo.ConditionObserverIDs.size();
	Ice::Long conditionCnt=m_RUVInfo.P;

	SetupLibraryFactors();

	//feature count (all samples should have a common feature count)
	m_RUVInfo.J=0;
	for(int i=0;i<sampleCnt;i++)
	{
		int observerID=m_RUVInfo.RawCountObserverIDs[i];
		::iBS::FeatureObserverSimpleInfoPtr foi=m_fois[i];
		if(m_RUVInfo.J==0)
		{
			m_RUVInfo.J=foi->DomainSize;
		}
		else if(m_RUVInfo.J!=foi->DomainSize)
		{
			//should have the same size
			return false;
		}
	}

	//build a map from observerID (global) to sample idx (0-based, local)
	m_preFilterSampleID2sampleIdx.clear();
	for(int i=0;i<sampleCnt;i++)
	{
		int obseverID=m_RUVInfo.RawCountObserverIDs[i];
		m_observerID2sampleIdx.insert(std::pair<int,int>(obseverID,i));
		int sampleID = m_RUVInfo.SampleIDs[i];
		m_preFilterSampleID2sampleIdx.insert(std::pair<int, int>(sampleID, i));
	}

	//derive number of control samples, build maps from sample idx to its condition idx, ctrl sample idx [0-CtrlSampleCnt)
	m_RUVInfo.CtrlSampleCnt=0;
	m_sampleIdx2ConditionIdx.resize(sampleCnt);
	m_sampleIdx2CtrlSampleIdx.resize(sampleCnt);
	m_conditionSampleIdxs.resize(conditionCnt);
	for(int i=0;i<conditionCnt;i++)
	{
		int conditionIdx=i;
		const iBS::IntVec& observerIDs=m_RUVInfo.ConditionObserverIDs[i];
		m_conditionSampleIdxs[conditionIdx].resize(observerIDs.size());
		for(int j=0;j<observerIDs.size();j++)
		{
			
			int sampleIdx=m_observerID2sampleIdx[observerIDs[j]];
			m_conditionSampleIdxs[conditionIdx][j]=sampleIdx;
			m_sampleIdx2ConditionIdx[sampleIdx]=conditionIdx;
			m_sampleIdx2CtrlSampleIdx[sampleIdx]=-1; //is not a control sample
			if(observerIDs.size()>1)
			{
				m_sampleIdx2CtrlSampleIdx[sampleIdx]=m_RUVInfo.CtrlSampleCnt;
				m_RUVInfo.CtrlSampleCnt++;
				m_ctrlSampleIdx2SampleIdx.push_back(sampleIdx);
			}
		}
	}

	if(m_RUVInfo.ObserverIDforTs>0)
	{
		iBS::FeatureObserverSimpleInfoPtr ts_foi
			=CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.ObserverIDforTs);

		if(ts_foi){
			int maxK=(int)(ts_foi->DomainSize/m_RUVInfo.n);
			m_RUVInfo.K=maxK;
		}
	}

	return true;
}

::Ice::Int
CRUVBuilder::GetSampleIdxsBySampleIDs(const ::iBS::IntVec& sampleIDs, ::iBS::IntVec& sampleIdxs) const
{
	int rt = 1;
	sampleIdxs.resize(sampleIDs.size(), -1);
	for (int i = 0; i<sampleIDs.size(); i++)
	{
		
		Int2Int_T::const_iterator p = m_preFilterSampleID2sampleIdx.find(sampleIDs[i]);
		if (p == m_preFilterSampleID2sampleIdx.end())
		{
			rt = 0;
		}
		else
		{
			sampleIdxs[i] = p->second;
		}
	}
	return rt;
}

::Ice::Int
CRUVBuilder::GetConditionIdxs(const ::iBS::IntVec& observerIDs,
                                        ::iBS::IntVec& conditionIdxs)
{
	for(int i=0;i<observerIDs.size();i++)
	{
		Int2Int_T::const_iterator p = m_observerID2sampleIdx.find(observerIDs[i]);
		if(p==m_observerID2sampleIdx.end())
		{
			return 0;
		}
		else
		{
			int sampleIdx=p->second;
			conditionIdxs.push_back(m_sampleIdx2ConditionIdx[sampleIdx]);
		}
	}
	return 1;
}

bool CRUVBuilder::GetPreFilterRawY(
	::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Double*  RawY) const
{
	if(!RawY)
	{
		//should already allocated 
		return false;
	}
	
	Ice::Long colCnt= m_RUVInfo.SampleIDs.size();
	Ice::Long rowCnt=featureIdxTo-featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	Ice::Double* rawCnts=RawY;

	::iBS::AMD_FcdcReadService_GetRowMatrixPtr nullcb;
	::Original::CGetRowMatrix wi(
		m_preFilterFOIs,nullcb,m_RUVInfo.SampleIDs,featureIdxFrom,featureIdxTo);
	wi.getRetValues(rawCnts);

	return true;
}

bool CRUVBuilder::GetRawY(
	::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Double*  RawY) const
{
	if (!RawY)
	{
		//should already allocated 
		return false;
	}

	Ice::Long colCnt = m_RUVInfo.SampleIDs.size();
	Ice::Long rowCnt = featureIdxTo - featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	Ice::Double* rawCnts = RawY;

	::iBS::AMD_FcdcReadService_GetRowMatrixPtr nullcb;
	::Original::CGetRowMatrix wi(
		m_fois, nullcb, m_RUVInfo.RawCountObserverIDs, featureIdxFrom, featureIdxTo);
	wi.getRetValues(rawCnts);

	return true;
}

::Ice::Double* 
CRUVBuilder::GetY(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo) const
{
	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt=featureIdxTo-featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	::IceUtil::ScopedArray<Ice::Double>  logCnts(new ::Ice::Double[totalValueCnt]);
	if(!logCnts.get())
	{
		return 0;
	}
	
	bool rt=GetY(featureIdxFrom,featureIdxTo,logCnts.get());
	if(!rt)
		return 0;
	//caller needs to delete RAM
	return logCnts.release();
}

bool CRUVBuilder::GetYVariation(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Double*  pY) const
{
	GetY(featureIdxFrom, featureIdxTo, pY);
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt = featureIdxTo - featureIdxFrom;
	
	CRowAdjustHelper::Adjust(pY, rowCnt, colCnt, iBS::RowAdjustZeroMeanUnitLengthConst0);
	return true;
}

bool CRUVBuilder::GetY(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Double*  pY) const
{
	if(!pY)
	{
		//should already allocated 
		return false;
	}
	
	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt=featureIdxTo-featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	Ice::Double* rawCnts=pY;

	::iBS::AMD_FcdcReadService_GetRowMatrixPtr nullcb;
	::Original::CGetRowMatrix wi(
		m_fois, nullcb, m_RUVInfo.RawCountObserverIDs, featureIdxFrom, featureIdxTo);
	wi.getRetValues(rawCnts);
	
	Ice::Double rowMean=0;
	Ice::Long cidx=0;
	for(Ice::Long i=0;i<rowCnt;i++)
	{
		rowMean=0;
		for(Ice::Long j=0;j<colCnt;j++)
		{
			//not check negative
			cidx=i*colCnt+j;
			pY[cidx]=log(rawCnts[cidx]*m_libraryFactors[j]+1);
			rowMean+=pY[cidx];
		}

		rowMean/=(Ice::Double)colCnt;
		for(Ice::Long j=0;j<colCnt;j++)
		{
			cidx=i*colCnt+j;
			pY[cidx]-=rowMean;
		}
	}

	return true;
}

bool CRUVBuilder::GetYandRowMeans(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, 
											 Ice::Double *pRowMeans, ::Ice::Double* pY) const
{
	if(!pY || !pRowMeans)
	{
		//should already allocated 
		return false;
	}
	
	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt=featureIdxTo-featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	Ice::Double* rawCnts=pY;

	::iBS::AMD_FcdcReadService_GetRowMatrixPtr nullcb;
	::Original::CGetRowMatrix wi(
		m_fois, nullcb,m_RUVInfo.RawCountObserverIDs, featureIdxFrom, featureIdxTo);
	wi.getRetValues(rawCnts);
	
	Ice::Double rowMean=0;
	Ice::Long cidx=0;
	for(Ice::Long i=0;i<rowCnt;i++)
	{
		rowMean=0;
		for(Ice::Long j=0;j<colCnt;j++)
		{
			//not check negative
			cidx=i*colCnt+j;
			pY[cidx]=log(rawCnts[cidx]*m_libraryFactors[j]+1);
			rowMean+=pY[cidx];
		}

		rowMean/=(Ice::Double)colCnt;
		pRowMeans[i]=rowMean;
		for(Ice::Long j=0;j<colCnt;j++)
		{
			cidx=i*colCnt+j;
			pY[cidx]-=rowMean;
		}
	}

	return true;
}

bool CRUVBuilder::SampleYandRowMeans(iBS::LongVec featureIdxs,
	Ice::Double *pRowMeans, ::Ice::Double* pY) const
{
	if (!pY || !pRowMeans)
	{
		//should already allocated 
		return false;
	}

	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt = featureIdxs.size();

	Ice::Double* rawCnts = pY;

	::iBS::AMD_FcdcReadService_SampleRowMatrixPtr nullcb;
	::Original::CSampleRowMatrix wi(
		m_fois, nullcb, m_RUVInfo.RawCountObserverIDs, featureIdxs);
	wi.getRetValues(rawCnts);

	Ice::Double rowMean = 0;
	Ice::Long cidx = 0;
	for (Ice::Long i = 0; i<rowCnt; i++)
	{
		rowMean = 0;
		for (Ice::Long j = 0; j<colCnt; j++)
		{
			//not check negative
			cidx = i*colCnt + j;
			pY[cidx] = log(rawCnts[cidx] * m_libraryFactors[j] + 1);
			rowMean += pY[cidx];
		}

		rowMean /= (Ice::Double)colCnt;
		pRowMeans[i] = rowMean;
		for (Ice::Long j = 0; j<colCnt; j++)
		{
			cidx = i*colCnt + j;
			pY[cidx] -= rowMean;
		}
	}

	return true;
}


//thread safe
bool CRUVBuilder::UpdateYcsYcsT(::Ice::Double* Y, Ice::Long featureIdxFrom, Ice::Long featureIdxTo, arma::mat& A, 
	CIndexPermutation& colIdxPermuttion, std::vector<::arma::mat>& As)
{
	iBS::ByteVec controlFeatureFlags;
	bool computePermutation = !As.empty();
	bool rt = UpdateYcscfYcscfT(Y, featureIdxFrom, featureIdxTo, controlFeatureFlags, A, computePermutation, colIdxPermuttion, As);
	return rt;
}

//thread safe
bool CRUVBuilder::UpdateYcscfYcscfT(::Ice::Double* Y, Ice::Long featureIdxFrom, Ice::Long featureIdxTo, 
	const iBS::ByteVec& controlFeatureFlags, arma::mat& B, bool computePermutation,
		CIndexPermutation& colIdxPermuttion, std::vector<::arma::mat>& As)
{
	int n=(int)m_RUVInfo.n;
	int m=m_RUVInfo.CtrlSampleCnt;

	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt=featureIdxTo-featureIdxFrom;
	Ice::Long F=rowCnt;
	int P = (int)As.size();
	Ice::Long conditionCnt=m_RUVInfo.P;
	iBS::DoubleVec conditionSums(conditionCnt,0);
	iBS::DoubleVec centeredY(n,0);
	//Ice::Double centeredY_sum = 0;
	//Ice::Double centeredY_cnt = m;
	bool needCheckControlFeatures=!controlFeatureFlags.empty();
	for(Ice::Long f=0;f<F;f++)
	{
		bool noneCtrlFeature= needCheckControlFeatures && (controlFeatureFlags[f+featureIdxFrom]==0);
		if(noneCtrlFeature)
			continue;

		for(int i=0;i<conditionCnt;i++)
		{
			conditionSums[i]=0.0;
		}

		for(int i=0;i<n;i++)
		{
			int sampleIdx=i;
			int conditionIdx=m_sampleIdx2ConditionIdx[sampleIdx];
			Ice::Long colIdx=i;
			Ice::Long rowIdx=f;
			Ice::Double y=Y[rowIdx*colCnt+colIdx];
			conditionSums[conditionIdx]+=y;
			centeredY[sampleIdx]=y;
		}

		//get centered Y
		//centeredY_sum = 0;
		for(int i=0;i<n;i++)
		{
			int sampleIdx=i;
			int conditionIdx=m_sampleIdx2ConditionIdx[sampleIdx];
			Ice::Double rcnt = (Ice::Double)m_RUVInfo.ConditionObserverIDs[conditionIdx].size();
			if(rcnt>1)
			{
				// y(i) in centeredY[sampleIdx]
				centeredY[sampleIdx]=(rcnt*centeredY[sampleIdx]-conditionSums[conditionIdx])/(rcnt-1);
				//centeredY_sum += centeredY[sampleIdx];
			}
			else
			{
				centeredY[sampleIdx]=0;
			}
		}

		//row centering (no need, centeredY already with zero mean)
		/*centeredY_sum /= centeredY_cnt;
		for (int i = 0; i<n; i++)
		{
			int sampleIdx = i;
			int conditionIdx = m_sampleIdx2ConditionIdx[sampleIdx];
			Ice::Double rcnt = (Ice::Double)m_RUVInfo.ConditionObserverIDs[conditionIdx].size();
			if (rcnt>1)
			{
			// y(i) in centeredY[sampleIdx]
			centeredY[sampleIdx] -= centeredY_sum;
			}
		}*/


		for(int i=0;i<m;i++)
		{
			int sampleIdx_i=m_ctrlSampleIdx2SampleIdx[i];
			for(int j=i;j<m;j++)
			{
				int sampleIdx_j=m_ctrlSampleIdx2SampleIdx[j];
				B(i,j)+=centeredY[sampleIdx_i]*centeredY[sampleIdx_j];
			}
		}

		if (computePermutation)
		{
			for (int p = 0; p < P; p++)
			{
				::arma::mat& A = As[p];
				const iBS::IntVec& colIdxs = colIdxPermuttion.Permutate();
				for (int i = 0; i<m; i++)
				{
					int sampleIdx_i = m_ctrlSampleIdx2SampleIdx[colIdxs[i]];
					for (int j = i; j<m; j++)
					{
						int sampleIdx_j = m_ctrlSampleIdx2SampleIdx[colIdxs[j]];
						A(i, j) += centeredY[sampleIdx_i] * centeredY[sampleIdx_j];
					}
				}
			}

		}
	}
	return true;
}

//thread safe
bool CRUVBuilder::UpdateYcfYcscfT(::Ice::Double* Y, Ice::Long featureIdxFrom, Ice::Long featureIdxTo, 
		const iBS::ByteVec& controlFeatureFlags, arma::mat& C)
{
	int n=(int)m_RUVInfo.n;
	int m=m_RUVInfo.CtrlSampleCnt;

	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt=featureIdxTo-featureIdxFrom;
	Ice::Long F=rowCnt;
	
	Ice::Long conditionCnt=m_RUVInfo.P;
	iBS::DoubleVec conditionSums(conditionCnt,0);
	iBS::DoubleVec centeredY(n,0);
	//Ice::Double centeredY_sum = 0;
	//Ice::Double centeredY_cnt = m;

	bool needCheckControlFeatures=!controlFeatureFlags.empty();
	for(Ice::Long f=0;f<F;f++)
	{
		bool noneCtrlFeature= needCheckControlFeatures && (controlFeatureFlags[f+featureIdxFrom]==0);
		if(noneCtrlFeature)
			continue;

		for(int i=0;i<conditionCnt;i++)
		{
			conditionSums[i]=0.0;
		}

		for(int i=0;i<n;i++)
		{
			int sampleIdx=i;
			int conditionIdx=m_sampleIdx2ConditionIdx[sampleIdx];
			Ice::Long colIdx=i;
			Ice::Long rowIdx=f;
			Ice::Double y=Y[rowIdx*colCnt+colIdx];
			conditionSums[conditionIdx]+=y;
			centeredY[sampleIdx]=y;
		}

		//get centered Y
		//centeredY_sum = 0;
		for(int i=0;i<n;i++)
		{
			int sampleIdx=i;
			int conditionIdx=m_sampleIdx2ConditionIdx[sampleIdx];
			double rcnt = (double) m_RUVInfo.ConditionObserverIDs[conditionIdx].size();
			if(rcnt>1)
			{
				// y(i) in centeredY[sampleIdx]
				centeredY[sampleIdx]=(rcnt*centeredY[sampleIdx]-conditionSums[conditionIdx])/(rcnt-1);
				//centeredY_sum += centeredY[sampleIdx];
			}
			else
			{
				centeredY[sampleIdx]=0;
			}
		}

		//row centering (no need, centeredY already with zero mean)
		/*centeredY_sum /= centeredY_cnt;
		for (int i = 0; i<n; i++)
		{
			int sampleIdx = i;
			int conditionIdx = m_sampleIdx2ConditionIdx[sampleIdx];
			Ice::Double rcnt = (Ice::Double)m_RUVInfo.ConditionObserverIDs[conditionIdx].size();
			if (rcnt>1)
			{
				// y(i) in centeredY[sampleIdx]
				centeredY[sampleIdx] -= centeredY_sum;
			}
		}*/

		Ice::Double* originalY=Y+(f*colCnt);

		for(int i=0;i<n;i++)
		{
			int sampleIdx_i=i;
			for(int j=0;j<m;j++)
			{
				int sampleIdx_j=m_ctrlSampleIdx2SampleIdx[j];
				C(i,j)+=originalY[sampleIdx_i]*centeredY[sampleIdx_j];
			}
		}
	}
	return true;
}

bool CRUVBuilder::UpdateYcfYcfT(::Ice::Double* Y, Ice::Long featureIdxFrom, Ice::Long featureIdxTo, 
		const iBS::ByteVec& controlFeatureFlags, arma::mat& A)
{
	int n=(int)m_RUVInfo.n;

	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt=featureIdxTo-featureIdxFrom;
	Ice::Long F=rowCnt;
	
	bool needCheckControlFeatures=!controlFeatureFlags.empty();
	for(Ice::Long f=0;f<F;f++)
	{
		bool noneCtrlFeature= needCheckControlFeatures && (controlFeatureFlags[f+featureIdxFrom]==0);
		if(noneCtrlFeature)
			continue;

		Ice::Double* originalY=Y+(f*colCnt);

		for(int i=0;i<n;i++)
		{
			int sampleIdx_i=i;
			for(int j=i;j<n;j++)
			{
				int sampleIdx_j=j;
				A(i,j)+=originalY[sampleIdx_i]*originalY[sampleIdx_j];
			}
		}

	}
	return true;
}

bool CRUVBuilder::UpdateYcfYcfT_Permuation(::Ice::Double* Y, Ice::Long featureIdxFrom, Ice::Long featureIdxTo,
	const iBS::ByteVec& controlFeatureFlags, CIndexPermutation& colIdxPermuttion, std::vector<::arma::mat>& As)
{
	int n = (int)m_RUVInfo.n;

	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt = featureIdxTo - featureIdxFrom;
	Ice::Long F = rowCnt;
	int P = (int)As.size();

	bool needCheckControlFeatures = !controlFeatureFlags.empty();
	for (Ice::Long f = 0; f<F; f++)
	{
		bool noneCtrlFeature = needCheckControlFeatures && (controlFeatureFlags[f + featureIdxFrom] == 0);
		if (noneCtrlFeature)
			continue;

		Ice::Double* originalY = Y + (f*colCnt);

		for (int p = 0; p < P; p++)
		{
			::arma::mat& A = As[p];
			const iBS::IntVec& colIdxs = colIdxPermuttion.Permutate();
			for (int i = 0; i<n; i++)
			{
				int colIdx_i = colIdxs[i];
				for (int j = i; j<n; j++)
				{
					int colIdx_j = colIdxs[j];
					A(i, j) += originalY[colIdx_i] * originalY[colIdx_j];
				}
			}
		}
	}
	return true;
}

bool CRUVBuilder::createOIDForEigenValues()
{
	Ice::Long domainSize = 0;
	if (m_RUVInfo.RUVMode == iBS::RUVModeRUVs
		|| m_RUVInfo.RUVMode == iBS::RUVModeRUVsForVariation)
	{
		domainSize = m_RUVInfo.CtrlSampleCnt;
	}
	else
	{
		domainSize = m_RUVInfo.n;
	}

	Ice::Int observerID;
	int m = m_RUVInfo.CtrlSampleCnt;
	if (m_RUVInfo.OIDforEigenValue == 0)
	{
		CGlobalVars::get()->theObserversDB.RqstNewFeatureObserverID(observerID, false);
		m_RUVInfo.OIDforEigenValue = observerID;
	}
	else
	{
		observerID = m_RUVInfo.OIDforEigenValue;
	}

	iBS::FeatureObserverInfoVec fois;
	iBS::IntVec observerIDs;
	observerIDs.push_back(observerID);
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs, fois);
	
	{
		iBS::FeatureObserverInfo& foi = fois[0];
		foi.ObserverName = "Eigen Values";
		foi.DomainSize = domainSize;
		foi.SetPolicy = iBS::FeatureValueSetPolicyDoNothing; //readonly outside, interannly writalbe
		foi.GetPolicy = iBS::FeatureValueGetPolicyGetForOneTimeRead;
	}

	CGlobalVars::get()->theObserversDB.SetFeatureObservers(fois);

	return true;
}

bool CRUVBuilder::createOIDForEigenVectors()
{
	Ice::Long n = 0;
	if (m_RUVInfo.RUVMode == iBS::RUVModeRUVs
		|| m_RUVInfo.RUVMode == iBS::RUVModeRUVsForVariation)
	{
		n = m_RUVInfo.CtrlSampleCnt;
	}
	else
	{
		n = m_RUVInfo.n;
	}

	iBS::IntVec observerIDs;
	if (m_RUVInfo.OIDforEigenVectors == 0)
	{
		CGlobalVars::get()->theObserversDB.RqstNewFeatureObserversInGroup((Ice::Int)n, observerIDs, false);
		m_RUVInfo.OIDforEigenVectors = observerIDs[0];
	}
	else
	{
		observerIDs.resize(n);
		for (int i = 0; i<n; i++)
		{
			observerIDs[i] = m_RUVInfo.OIDforEigenVectors + i;
		}
	}

	iBS::FeatureObserverInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs, fois);
	Ice::Long domainSize = n;
	for (int i = 0; i<n; i++)
	{
		iBS::FeatureObserverInfo& foi = fois[i];
		foi.DomainSize = domainSize;
		foi.SetPolicy = iBS::FeatureValueSetPolicyDoNothing; //readonly outside, interannly writalbe
		foi.GetPolicy = iBS::FeatureValueGetPolicyGetForOneTimeRead;
	}

	CGlobalVars::get()->theObserversDB.SetFeatureObservers(fois);

	return true;
}

bool CRUVBuilder::createOIDForPermutatedEigenValues()
{
	if (m_RUVInfo.PermutationCnt < 1)
	{
		return false;
	}

	Ice::Long domainSize = 0;
	if (m_RUVInfo.RUVMode == iBS::RUVModeRUVs
		|| m_RUVInfo.RUVMode == iBS::RUVModeRUVsForVariation)
	{
		domainSize = m_RUVInfo.CtrlSampleCnt;
	}
	else
	{
		domainSize = m_RUVInfo.n;
	}

	Ice::Long n = m_RUVInfo.PermutationCnt;

	iBS::IntVec observerIDs;
	if (m_RUVInfo.OIDforPermutatedEigenValues == 0)
	{
		CGlobalVars::get()->theObserversDB.RqstNewFeatureObserversInGroup((Ice::Int)n, observerIDs, false);
		m_RUVInfo.OIDforPermutatedEigenValues = observerIDs[0];
	}
	else
	{
		observerIDs.resize(n);
		for (int i = 0; i<n; i++)
		{
			observerIDs[i] = m_RUVInfo.OIDforPermutatedEigenValues + i;
		}
	}

	iBS::FeatureObserverInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs, fois);
	for (int i = 0; i<n; i++)
	{
		iBS::FeatureObserverInfo& foi = fois[i];
		foi.DomainSize = domainSize;
		foi.SetPolicy = iBS::FeatureValueSetPolicyDoNothing; //readonly outside, interannly writalbe
		foi.GetPolicy = iBS::FeatureValueGetPolicyGetForOneTimeRead;
	}

	CGlobalVars::get()->theObserversDB.SetFeatureObservers(fois);

	return true;
}

bool CRUVBuilder::createObserverGroupForTs(int maxK)
{
	iBS::IntVec observerIDs;
	int m=m_RUVInfo.CtrlSampleCnt;
	if(m_RUVInfo.ObserverIDforTs==0)
	{
		CGlobalVars::get()->theObserversDB.RqstNewFeatureObserversInGroup(m,observerIDs,false);
		m_RUVInfo.ObserverIDforTs=observerIDs[0];
	}
	else
	{
		observerIDs.resize(m);
		for(int i=0;i<m;i++)
		{
			observerIDs[i]=m_RUVInfo.ObserverIDforTs+i;
		}
	}

	iBS::FeatureObserverInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs,fois);
	Ice::Long domainSize=m_RUVInfo.n*maxK; //save T with different ks
	for(int i=0;i<m;i++)
	{
		iBS::FeatureObserverInfo& foi=fois[i];
		ostringstream os;
		os<<"RUVs_m("<<i+1<<"/"<<m<<")";
		foi.ObserverName=os.str();
		foi.DomainSize=domainSize;
		foi.SetPolicy=iBS::FeatureValueSetPolicyDoNothing; //readonly outside, interannly writalbe
		foi.GetPolicy=iBS::FeatureValueGetPolicyGetForOneTimeRead;
	}

	CGlobalVars::get()->theObserversDB.SetFeatureObservers(fois);
	

	return true;
}

bool CRUVBuilder::createObserverGroupForZs(int maxK)
{
	iBS::IntVec observerIDs;
	if(m_RUVInfo.ObserverIDforZs==0)
	{
		CGlobalVars::get()->theObserversDB.RqstNewFeatureObserversInGroup((int)m_RUVInfo.n,observerIDs,false);
		m_RUVInfo.ObserverIDforZs=observerIDs[0];
	}
	else
	{
		observerIDs.resize(m_RUVInfo.n);
		for(int i=0;i<m_RUVInfo.n;i++)
		{
			observerIDs[i]=m_RUVInfo.ObserverIDforZs+i;
		}
	}

	iBS::FeatureObserverInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs,fois);
	Ice::Long domainSize=m_RUVInfo.n*maxK; //save Z with different ks
	for(int i=0;i<m_RUVInfo.n;i++)
	{
		iBS::FeatureObserverInfo& foi=fois[i];
		ostringstream os;
		os<<"RUVs_Z("<<i+1<<"/"<<m_RUVInfo.n<<")";
		foi.ObserverName=os.str();
		foi.DomainSize=domainSize;
		foi.SetPolicy=iBS::FeatureValueSetPolicyDoNothing; //readonly outside, interannly writalbe
		foi.GetPolicy=iBS::FeatureValueGetPolicyGetForOneTimeRead;
	}

	CGlobalVars::get()->theObserversDB.SetFeatureObservers(fois);
	

	return true;
}

bool CRUVBuilder::createObserverGroupForGs(int maxK)
{
	int n=(int)m_RUVInfo.n;
	int p=(int)m_RUVInfo.P;

	iBS::IntVec observerIDs;
	if(m_RUVInfo.ObserverIDforGs==0)
	{
		CGlobalVars::get()->theObserversDB.RqstNewFeatureObserversInGroup(n,observerIDs,false);
		m_RUVInfo.ObserverIDforGs=observerIDs[0];
	}
	else
	{
		observerIDs.resize(n);
		for(int i=0;i<n;i++)
		{
			observerIDs[i]=m_RUVInfo.ObserverIDforGs+i;
		}
	}

	iBS::FeatureObserverInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs,fois);
	Ice::Long domainSize=p*maxK+(maxK*(maxK+1))/2; //save G with different ks
	for(int i=0;i<n;i++)
	{
		iBS::FeatureObserverInfo& foi=fois[i];
		ostringstream os;
		os<<"RUVs_G("<<i+1<<"/"<<m_RUVInfo.n<<")";
		foi.ObserverName=os.str();
		foi.DomainSize=domainSize;
		foi.SetPolicy=iBS::FeatureValueSetPolicyDoNothing; //readonly outside, interannly writalbe
		foi.GetPolicy=iBS::FeatureValueGetPolicyGetForOneTimeRead;
	}

	CGlobalVars::get()->theObserversDB.SetFeatureObservers(fois);
	

	return true;
}

bool CRUVBuilder::createObserverGroupForWts(int maxK)
{
	int n=(int)m_RUVInfo.n;

	iBS::IntVec observerIDs;
	if(m_RUVInfo.ObserverIDforWts==0)
	{
		CGlobalVars::get()->theObserversDB.RqstNewFeatureObserversInGroup(n,observerIDs,false);
		m_RUVInfo.ObserverIDforWts=observerIDs[0];
	}
	else
	{
		observerIDs.resize(n);
		for(int i=0;i<n;i++)
		{
			observerIDs[i]=m_RUVInfo.ObserverIDforWts+i;
		}
	}

	iBS::FeatureObserverInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs,fois);
	Ice::Long domainSize=(maxK*(maxK+1))/2; //save Wt with different ks
	for(int i=0;i<n;i++)
	{
		iBS::FeatureObserverInfo& foi=fois[i];
		ostringstream os;
		os<<"RUVs_Wt("<<i+1<<"/"<<m_RUVInfo.n<<")";
		foi.ObserverName=os.str();
		foi.DomainSize=domainSize;
		foi.SetPolicy=iBS::FeatureValueSetPolicyDoNothing; //readonly outside, interannly writalbe
		foi.GetPolicy=iBS::FeatureValueGetPolicyGetForOneTimeRead;
	}

	CGlobalVars::get()->theObserversDB.SetFeatureObservers(fois);
	

	return true;
}

bool CRUVBuilder::createObserverGroupForFilteredY()
{
	iBS::IntVec observerIDs;
	Ice::Long colCnt= m_RUVInfo.SampleIDs.size();
	CGlobalVars::get()->theObserversDB.RqstNewFeatureObserversInGroup(
		(int)colCnt,observerIDs,false);
	m_RUVInfo.RawCountObserverIDs=observerIDs;
	iBS::FeatureObserverInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs,fois);
	Ice::Long domainSize=m_RUVInfo.J; // max size, not really size
	for(int i=0;i<colCnt;i++)
	{
		iBS::FeatureObserverInfo& foi=fois[i];
		ostringstream os;
		os<<"Filtered_"<<m_preFilterFOIs[i]->ObserverID;
		foi.ObserverName=os.str();
		foi.DomainSize=domainSize;
		foi.SetPolicy=iBS::FeatureValueSetPolicyDoNothing;	//readonly outside, interannly writalbe
		foi.GetPolicy=iBS::FeatureValueGetPolicyGetForOneTimeRead;
	}
	CGlobalVars::get()->theObserversDB.SetFeatureObservers(fois);

	{
		CGlobalVars::get()->theObserversDB.RqstNewFeatureObserverID(m_RUVInfo.MapbackObserverID,false);
		iBS::FeatureObserverInfoVec mapbackfois;
		CGlobalVars::get()->theObserversDB.GetFeatureObservers(
			iBS::IntVec(1,m_RUVInfo.MapbackObserverID),mapbackfois);
		iBS::FeatureObserverInfo& foi=mapbackfois[0];
		foi.ObserverName="mapback";
		foi.DomainSize=domainSize;
		foi.SetPolicy=iBS::FeatureValueSetPolicyDoNothing;	//readonly outside, interannly writalbe
		foi.GetPolicy=iBS::FeatureValueGetPolicyGetForOneTimeRead;
		CGlobalVars::get()->theObserversDB.SetFeatureObservers(mapbackfois);
	}

	m_fois.clear();
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(m_RUVInfo.RawCountObserverIDs,m_fois);


	Int2Int_T sampleID2FilteredObserverID;
	for(int i=0;i<colCnt;i++)
	{
		int sampleID=m_RUVInfo.SampleIDs[i];
		sampleID2FilteredObserverID.insert(std::pair<int,int>(sampleID,observerIDs[i]));
	}

	int conditionCnt=(int)m_RUVInfo.ReplicateSampleIDs.size();
	m_RUVInfo.ConditionObserverIDs=m_RUVInfo.ReplicateSampleIDs;

	for(int i=0;i<conditionCnt;i++)
	{
		const iBS::IntVec& replicateSampleIDs=m_RUVInfo.ReplicateSampleIDs[i];
		for(size_t j=0;j<replicateSampleIDs.size();j++)
		{
			int filteredOID=sampleID2FilteredObserverID[replicateSampleIDs[j]];
			m_RUVInfo.ConditionObserverIDs[i][j]=filteredOID;
		}
	}

	m_RUVInfo.FacetStatus=iBS::RUVFacetStatusFilteredOIDsReady;
	//update 
	CBdvdGlobalVars::get()->theRUVFacetDB->SetRUVFacetInfo(m_RUVInfo.FacetID,m_RUVInfo);

	return true;
}


void CRUVBuilder::NotifyWorkerBecomesFree(int workerIdx)
{
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);

	m_freeWorkerIdxs.push_back(workerIdx);

	if(m_needNotify)
	{
		m_monitor.notify();
	}
}

bool CRUVBuilder::FilterOriginalFeatures(::Ice::Long ramMb )
{
	Ice::Long colCnt= m_RUVInfo.SampleIDs.size();

	ramMb/=sizeof(Ice::Double);

	::Ice::Long batchValueCnt=1024*1024*ramMb;
	if(batchValueCnt%colCnt!=0){
		batchValueCnt-=(batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::IceUtil::ScopedArray<Ice::Double>  preFilterRawY(new ::Ice::Double[batchValueCnt]);
	if(!preFilterRawY.get()){
		return false;
	}

	::IceUtil::ScopedArray<Ice::Double>  filteredRawY(new ::Ice::Double[batchValueCnt]);
	if(!filteredRawY.get()){
		return false;
	}

	

	::Ice::Long batchRowCnt=batchValueCnt/colCnt;
	::Ice::Long batchCnt= m_RUVInfo.J/batchRowCnt+1;

	::IceUtil::ScopedArray<Ice::Double>  keptFeatureIdxs(new ::Ice::Double[batchRowCnt]);
	if(!keptFeatureIdxs.get()){
		return false;
	}
	
	int batchIdx=0;
	::Ice::Long remainCnt=m_RUVInfo.J;
	::Ice::Long thisBatchRowCnt=0;
	::Ice::Long featureIdxFrom=0;
	::Ice::Long featureIdxTo=0;

	::Ice::Long keptFeatureTotalCnt=0;
	::Ice::Long keptFeatureIdxFrom=0;
	::Ice::Long keptFeatureIdxTo=0;
	::Ice::Long thisBatchKeptRowCnt=0;
	::Ice::Double lowCntThreashold=m_RUVInfo.FeatureFilterMaxCntLowThreshold;

	iBS::FeatureObserverSimpleInfoPtr mapback_foi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.MapbackObserverID);
	iBS::FeatureObserverSimpleInfoPtr filtered_foi=m_fois[0];
	while(remainCnt>0)
	{	
		if(remainCnt>batchRowCnt){
			thisBatchRowCnt=batchRowCnt;
		}
		else{
			thisBatchRowCnt=remainCnt;
		}
		batchIdx++;
		thisBatchKeptRowCnt=0;

		std::cout<<IceUtil::Time::now().toDateTime()<<" FilterOriginalFeatures batch "<<batchIdx<<"/"<<batchCnt<<" begin"<<endl; 

		featureIdxTo=featureIdxFrom+thisBatchRowCnt;

		GetPreFilterRawY(featureIdxFrom,featureIdxTo,preFilterRawY.get());

		::Ice::Long cidx=0;
		for(Ice::Long i=0;i<thisBatchRowCnt;i++)
		{
			for(Ice::Long j=0;j<colCnt;j++)
			{
				cidx=i*colCnt+j;
				
				Ice::Double adjustedCnt = preFilterRawY[cidx]*m_libraryFactors[j];
				if(adjustedCnt>lowCntThreashold)
				{
					Ice::Double *pSource=preFilterRawY.get()+i*colCnt;
					Ice::Double *pDest=filteredRawY.get()+thisBatchKeptRowCnt*colCnt;

					std::copy(pSource,pSource+colCnt,pDest);
					keptFeatureIdxs.get()[thisBatchKeptRowCnt]=(Ice::Double)(featureIdxFrom+i);
					keptFeatureTotalCnt++;
					thisBatchKeptRowCnt++;
					break;
				}
			}
		}

		keptFeatureIdxTo=keptFeatureIdxFrom+thisBatchKeptRowCnt;

		//save filtered to store
		if(thisBatchKeptRowCnt>0)
		{
			std::pair<const Ice::Double*, const Ice::Double*> values(
				filteredRawY.get(), filteredRawY.get()+thisBatchKeptRowCnt*colCnt);

			Ice::Int foiObserverID=filtered_foi->ObserverID;
			Ice::Int foiStoreObserverID=filtered_foi->ObserverID;

			Ice::Long foiStoreDomainSize = filtered_foi->ObserverGroupSize*filtered_foi->DomainSize;
			Ice::Long foiDomainSize = filtered_foi->DomainSize;
			Ice::Long s_featureIdxFrom = keptFeatureIdxFrom * filtered_foi->ObserverGroupSize; //index in store
			Ice::Long s_featureIdxTo = keptFeatureIdxTo * filtered_foi->ObserverGroupSize;  //index in store

			filtered_foi->DomainSize=foiStoreDomainSize; //convert to store size
			filtered_foi->ObserverID=foiStoreObserverID;
			CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
					filtered_foi,s_featureIdxFrom, s_featureIdxTo, values);
			filtered_foi->ObserverID=foiObserverID; 
			filtered_foi->DomainSize=foiDomainSize; //convert back


			values.first=keptFeatureIdxs.get();
			values.second=keptFeatureIdxs.get()+thisBatchKeptRowCnt;
			CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
					mapback_foi,keptFeatureIdxFrom, keptFeatureIdxTo, values);

		}

		keptFeatureIdxFrom+=thisBatchKeptRowCnt;

		featureIdxFrom+=thisBatchRowCnt;
		remainCnt-=thisBatchRowCnt;

		std::cout<<IceUtil::Time::now().toDateTime()<<" FilterOriginalFeatures batch "<<batchIdx<<"/"<<batchCnt<<" end"<<endl;

		
	}

	std::cout<<IceUtil::Time::now().toDateTime()<<" FilterOriginalFeatures totoal kept rows "<<keptFeatureTotalCnt<<endl;

	//update new domainsize
	iBS::FeatureObserverInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(m_RUVInfo.RawCountObserverIDs,fois);
	for(int i=0;i<colCnt;i++)
	{
		iBS::FeatureObserverInfo& foi=fois[i];
		foi.DomainSize=keptFeatureTotalCnt;
		m_fois[i]->DomainSize=keptFeatureTotalCnt;
	}

	CGlobalVars::get()->theObserversDB.SetFeatureObservers(fois);

	{
		iBS::FeatureObserverInfoVec mapbackfois;
		CGlobalVars::get()->theObserversDB.GetFeatureObservers(
			iBS::IntVec(1,m_RUVInfo.MapbackObserverID),mapbackfois);
		iBS::FeatureObserverInfo& foi=mapbackfois[0];
		foi.DomainSize=keptFeatureTotalCnt;
		CGlobalVars::get()->theObserversDB.SetFeatureObservers(mapbackfois);
	}

	m_RUVInfo.FacetStatus=iBS::RUVFacetStatusFeatureFiltered;
	Initialize_AfterFilter();
	//update 
	CBdvdGlobalVars::get()->theRUVFacetDB->SetRUVFacetInfo(m_RUVInfo.FacetID,m_RUVInfo);


	return true;
}

bool CRUVBuilder::ControlFeatureByMaxCntLow()
{
	::Ice::Long ramMb = 512;
	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();

	m_controlFeatureFlags.resize(m_RUVInfo.J, 0);
	m_controlFeatureTotalCnt = 0;

	Ice::Long J = m_RUVInfo.J;
	if (m_RUVInfo.FeatureIdxFrom > 0 || m_RUVInfo.FeatureIdxTo > 0)
	{
		J = m_RUVInfo.FeatureIdxTo - m_RUVInfo.FeatureIdxFrom;
	}

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;
	

	::IceUtil::ScopedArray<Ice::Double>  rawY(new ::Ice::Double[batchValueCnt]);
	if (!rawY.get()){
		return false;
	}

	
	::Ice::Long batchCnt = J / batchRowCnt + 1;

	
	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(m_rebuidRUVTaskID, "prepare control features", batchCnt);

	int batchIdx = 0;
	::Ice::Long remainCnt = J;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_RUVInfo.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	::Ice::Double maxCnt_UpBound = m_RUVInfo.ControlFeatureMaxCntUpBound;
	::Ice::Double maxCnt_LowBound = m_RUVInfo.ControlFeatureMaxCntLowBound;
	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		
		cout << IceUtil::Time::now().toDateTime() << " ControlFeatureByMaxCntLow batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		GetRawY(featureIdxFrom, featureIdxTo, rawY.get());

		bool noSignal = true;
		::Ice::Long cidx = 0;
		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			noSignal = true;
			for (Ice::Long j = 0; j<colCnt; j++)
			{
				cidx = i*colCnt + j;

				Ice::Double adjustedCnt = rawY[cidx] * m_libraryFactors[j];

				if (noSignal && adjustedCnt>maxCnt_LowBound)
				{
					noSignal = false;
					m_controlFeatureFlags[i + featureIdxFrom] = 1;
					m_controlFeatureTotalCnt++;
				}

				if (adjustedCnt>maxCnt_UpBound)
				{
					m_controlFeatureFlags[i + featureIdxFrom] = 0;
					m_controlFeatureTotalCnt--;
					break;
				}
			}
		}

		
		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;

		Ice::Double cratio = (Ice::Double)m_controlFeatureTotalCnt;
		cratio /= J;
		std::cout << IceUtil::Time::now().toDateTime() << " ControlFeatureByMaxCntLow Control Feature " << m_controlFeatureTotalCnt << "/" << J << "=" << cratio << endl;
		m_RUVInfo.ControlFeatureCnt = m_controlFeatureTotalCnt;
		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_rebuidRUVTaskID, 1);
	}
	
	return true;
}

bool CRUVBuilder::ControlFeatureByRowIdxs()
{
	iBS::FeatureObserverSimpleInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(
		iBS::IntVec(1, m_RUVInfo.ObserverIDforControlFeatureIdxs), fois);
	if (fois.empty())
	{
		std::cout << IceUtil::Time::now().toDateTime() << " ObserverIDforControlFeatureIdxs not exist" << endl;
		return false;
	}

	iBS::FeatureObserverSimpleInfoPtr& foi = fois[0];

	Ice::Long rowCnt = foi->DomainSize;

	::IceUtil::ScopedArray<Ice::Double>  rowIdxs(new ::Ice::Double[rowCnt]);
	if (!rowIdxs.get()){
		return false;
	}

	CFeatureValueHelper::GetDoubleVecInRAM(foi, rowIdxs.get());

	Ice::Long J = m_RUVInfo.J;
	m_controlFeatureFlags.resize(J, 0);
	m_controlFeatureTotalCnt = 0;

	for (Ice::Long i = 0; i < rowCnt; i++)
	{
		Ice::Long rowIdx = (Ice::Long)rowIdxs[i];
		m_controlFeatureFlags[rowIdx] = 1;
		m_controlFeatureTotalCnt++;
	}
	Ice::Double cratio = (Ice::Double)m_controlFeatureTotalCnt;
	cratio /= J;
	std::cout << IceUtil::Time::now().toDateTime() << " ControlFeatureByRowIdxs Control Feature " << m_controlFeatureTotalCnt << "/" << J << "=" << cratio << endl;
	return true;
}

bool CRUVBuilder::ControlFeatureByAllInQuantile(int mode)
{
	::Ice::Long ramMb = 512;
	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();

	m_controlFeatureFlags.resize(m_RUVInfo.J, 0);
	m_controlFeatureTotalCnt = 0;

	Ice::Long J = m_RUVInfo.J;
	if (m_RUVInfo.FeatureIdxFrom > 0 || m_RUVInfo.FeatureIdxTo > 0)
	{
		J = m_RUVInfo.FeatureIdxTo - m_RUVInfo.FeatureIdxFrom;
	}

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;


	::IceUtil::ScopedArray<Ice::Double>  rawY(new ::Ice::Double[batchValueCnt]);
	if (!rawY.get()){
		return false;
	}

	::Ice::Long batchCnt = J / batchRowCnt + 1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(m_rebuidRUVTaskID, "prepare control features", batchCnt);

	int batchIdx = 0;
	::Ice::Long remainCnt = J;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_RUVInfo.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	int qSampleCntThrehold = (int)(colCnt*m_ctrlQuantileAllInFraction);

	std::cout << "qSampleCntThrehold = " << qSampleCntThrehold << endl;
	for (Ice::Long i = 0; i<colCnt; i++)
	{
		std::cout << "m_ctrlQvalues[" << i << "] = "<<m_ctrlQvalues[i] << endl;
	}

	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;


		std::cout << IceUtil::Time::now().toDateTime() << " ControlFeatureByAllInQuantile batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		GetRawY(featureIdxFrom, featureIdxTo, rawY.get());

		int qSampleCnt = 0;
		::Ice::Long cidx = 0;
		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			qSampleCnt = 0;
			for (Ice::Long j = 0; j<colCnt; j++)
			{
				cidx = i*colCnt + j;

				Ice::Double adjustedCnt = rawY[cidx] * m_libraryFactors[j];

				if (mode==1&&(adjustedCnt>m_ctrlQvalues[j]))
				{
					qSampleCnt++;
				}
				else if (mode == 0 && (adjustedCnt <= m_ctrlQvalues[j]))
				{
					qSampleCnt++;
				}

				if (qSampleCnt >= qSampleCntThrehold)
				{
					m_controlFeatureFlags[i + featureIdxFrom] = 1;
					m_controlFeatureTotalCnt++;
					break;
				}
			}
		}


		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;

		Ice::Double cratio = (Ice::Double)m_controlFeatureTotalCnt;
		cratio /= J;
		std::cout << IceUtil::Time::now().toDateTime() << " ControlFeatureByAllInQuantile Control Feature " << m_controlFeatureTotalCnt << "/" << J << "=" << cratio << endl;
		m_RUVInfo.ControlFeatureCnt = m_controlFeatureTotalCnt;
		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_rebuidRUVTaskID, 1);
	}

	return true;
}


bool CRUVBuilder::MultithreadRowANOVA(::Ice::Int  threadCnt, ::Ice::Long ramMb )
{
	int n=(int)m_RUVInfo.n;
	m_controlFeatureFlags.resize(m_RUVInfo.J,0);

	//"between-group degree of freedom"
	Ice::Double bgDF=(Ice::Double)(m_RUVInfo.P-1);

	//"within-group variability"
	Ice::Double wgDF=(Ice::Double)(n-m_RUVInfo.P);
	Ice::Double PValueControlFeatureANOVA = 0.5;
	m_controlFeatureANOVAFStatistics=CStatisticsHelper::GetCriticalFStatistics(
		bgDF, wgDF, PValueControlFeatureANOVA);
	


	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();

	ramMb/=sizeof(Ice::Double);
	ramMb/=threadCnt;

	::Ice::Long batchValueCnt=1024*1024*ramMb;
	if(batchValueCnt%colCnt!=0){
		batchValueCnt-=(batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt=batchValueCnt/colCnt;

	CRUVRowANOVAWorkerMgr workerMgr(threadCnt);
	//each worker will allocate Y with size of batchValueCnt 
	if(workerMgr.Initilize(batchRowCnt,batchValueCnt)==false)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
		return false;
	}

	Ice::Long J = m_RUVInfo.J;
	if (m_RUVInfo.FeatureIdxFrom > 0 || m_RUVInfo.FeatureIdxTo > 0)
	{
		J = m_RUVInfo.FeatureIdxTo - m_RUVInfo.FeatureIdxFrom;
	}

	::Ice::Long remainCnt=J;
	::Ice::Long thisBatchRowCnt=0;
	::Ice::Long featureIdxFrom = m_RUVInfo.FeatureIdxFrom;
	::Ice::Long featureIdxTo=0;
	::Ice::Long batchCnt= remainCnt/batchRowCnt+1;

	bool bContinue=true;
	bool bNeedExit=false;
	m_needNotify = false;

	int batchIdx=0;
	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();

	for(int i=0;i<threadCnt;i++)
	{
		m_freeWorkerIdxs.push_back(i);
	}

	while(!bNeedExit)
	{
		
		{
			//entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while(!m_shutdownRequested)
			{
				//merge freeworker that has not been lauched due to remainCnt==0
				if(!m_processingWorkerIdxs.empty())
				{
					std::copy(m_processingWorkerIdxs.begin(),m_processingWorkerIdxs.end(),
						std::back_inserter(m_freeWorkerIdxs));
					m_processingWorkerIdxs.clear();
				}

				//have to wait if 1: remain count>0, but no free worker; 
				//or 2. remain count==0, worker not finished yet
				if((remainCnt>0 && m_freeWorkerIdxs.empty())
					|| (remainCnt == 0 && (int)m_freeWorkerIdxs.size()<threadCnt))
				{
					m_needNotify = true;
					m_monitor.wait();
				}

				//free worker need to finish left items
				if(remainCnt>0 && !m_freeWorkerIdxs.empty())
				{
					std::copy(m_freeWorkerIdxs.begin(),m_freeWorkerIdxs.end(),
						std::back_inserter(m_processingWorkerIdxs));
					m_freeWorkerIdxs.clear();
					break;
				}
				else if (remainCnt == 0 && (int)m_freeWorkerIdxs.size() == threadCnt)
				{
					bNeedExit=true;
					break;
					
				}
			}

			//if control request shutdown
			if(m_shutdownRequested)
			{
				bContinue=false;
				bNeedExit=true;
				cout<<"CRUVBuilder m_shutdownRequested==true ..."<<endl; 
			}

			//leaving critical region
			m_needNotify=false;
		}

		//these workers are free, need to use them to process
		while(!m_processingWorkerIdxs.empty() && remainCnt>0)
		{
			int workerIdx = m_processingWorkerIdxs.front();
			
			if(remainCnt>batchRowCnt){
				thisBatchRowCnt=batchRowCnt;
			}
			else{
				thisBatchRowCnt=remainCnt;
			}
			batchIdx++;

			cout<<IceUtil::Time::now().toDateTime()<<" MultithreadRowANOVA batch "<<batchIdx<<"/"<<batchCnt<<" begin"<<endl; 

			featureIdxTo=featureIdxFrom+thisBatchRowCnt;

			RUVRowANOVAWorkerPtr worker=workerMgr.GetWorker(workerIdx);
			GetY(featureIdxFrom,featureIdxTo,worker->GetBatchY());

			RUVsWorkItemPtr wi=new CRUVComputeRowANOVA(
				*this, workerIdx,featureIdxFrom,featureIdxTo, 
				worker->GetBatchY(), worker->GetBatchFStatistics());

			worker->AddWorkItem(wi);

			m_processingWorkerIdxs.pop_front();
			featureIdxFrom+=thisBatchRowCnt;
			remainCnt-=thisBatchRowCnt;
		}

	}

	if(!bContinue)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
		return false;
	}

	cout<<IceUtil::Time::now().toDateTime()<<" MultithreadRowANOVA batch "<<batchIdx<<"/"<<batchCnt<<" end"<<endl; 
	
	workerMgr.RequestShutdownAllWorkers();
	workerMgr.UnInitilize();

	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();

	m_controlFeatureTotalCnt=0;
	for(Ice::Long i=0;i<m_RUVInfo.J;i++)
	{
		if(m_controlFeatureFlags[i]>0)
		{
			m_controlFeatureTotalCnt++;
		}
	}
	Ice::Double cratio=(Ice::Double)m_controlFeatureTotalCnt;
	cratio/=J;
	cout<<IceUtil::Time::now().toDateTime()<<" MultithreadRowANOVA Control Feature "<<m_controlFeatureTotalCnt<<"/"<<J<<"="<<cratio<<endl; 
	

	return true;
}

bool CRUVBuilder::MultithreadGetABC(
	::arma::mat& A, ::arma::mat& B, ::arma::mat& C, ::Ice::Int  threadCnt, ::Ice::Long ramMb )
{
	int n=(int)m_RUVInfo.n;
	int m=m_RUVInfo.CtrlSampleCnt;

	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();
	const iBS::ByteVec& controlFeatureFlags = m_controlFeatureFlags;

	Ice::Long J = m_RUVInfo.J;
	if (m_RUVInfo.FeatureIdxFrom > 0 || m_RUVInfo.FeatureIdxTo > 0)
	{
		J = m_RUVInfo.FeatureIdxTo - m_RUVInfo.FeatureIdxFrom;
	}

	Ice::Long totalRam = J*colCnt*sizeof(Ice::Double);
	totalRam /= (1024 * 1024);
	if (totalRam < ramMb)
	{
		ramMb = totalRam;
	}

	ramMb/=sizeof(Ice::Double);
	ramMb/=threadCnt;

	if (ramMb == 0)
	{
		ramMb = 1;
	}

	::Ice::Long batchValueCnt=1024*1024*ramMb;
	if(batchValueCnt%colCnt!=0){
		batchValueCnt-=(batchValueCnt%colCnt); //data row not spread in two batch files
	}

	CRUVsWorkerMgr workerMgr(threadCnt);
	//each worker will allocate Y with size of batchValueCnt 
	if(workerMgr.Initilize(batchValueCnt,m,n, m_RUVInfo.PermutationCnt)==false)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
		return false;
	}

	::Ice::Long batchRowCnt=batchValueCnt/colCnt;
	::Ice::Long remainCnt=J;

	::Ice::Long thisBatchRowCnt=0;
	::Ice::Long featureIdxFrom = m_RUVInfo.FeatureIdxFrom;
	::Ice::Long featureIdxTo=0;
	::Ice::Long batchCnt= remainCnt/batchRowCnt+1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(
		m_rebuidRUVTaskID, "MultithreadGetABC", batchCnt);


	bool AequalB=controlFeatureFlags.empty();
	bool bContinue=true;
	bool bNeedExit=false;

	int batchIdx=0;

	for(int i=0;i<threadCnt;i++)
	{
		m_freeWorkerIdxs.push_back(i);
	}

	while(!bNeedExit)
	{
		
		{
			//entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while(!m_shutdownRequested)
			{
				//merge freeworker that has not been lauched due to remainCnt==0
				if(!m_processingWorkerIdxs.empty())
				{
					std::copy(m_processingWorkerIdxs.begin(),m_processingWorkerIdxs.end(),
						std::back_inserter(m_freeWorkerIdxs));
					m_processingWorkerIdxs.clear();
				}

				//have to wait if 1: remain count>0, but no free worker; 
				//or 2. remain count==0, worker not finished yet
				if((remainCnt>0 && m_freeWorkerIdxs.empty())
					|| (remainCnt == 0 && (int)m_freeWorkerIdxs.size()<threadCnt))
				{
					m_needNotify = true;
					m_monitor.wait();
				}

				//free worker need to finish left items
				if(remainCnt>0 && !m_freeWorkerIdxs.empty())
				{
					std::copy(m_freeWorkerIdxs.begin(),m_freeWorkerIdxs.end(),
						std::back_inserter(m_processingWorkerIdxs));
					m_freeWorkerIdxs.clear();
					break;
				}
				else if (remainCnt == 0 && (int)m_freeWorkerIdxs.size() == threadCnt)
				{
					bNeedExit=true;
					break;
					
				}
			}

			//if control request shutdown
			if(m_shutdownRequested)
			{
				bContinue=false;
				bNeedExit=true;
				cout<<"CRUVBuilder m_shutdownRequested==true ..."<<endl; 
			}

			//leaving critical region
			m_needNotify=false;
		}

		//these workers are free, need to use them to process
		while(!m_processingWorkerIdxs.empty() && remainCnt>0)
		{
			int workerIdx = m_processingWorkerIdxs.front();
			
			if(remainCnt>batchRowCnt){
				thisBatchRowCnt=batchRowCnt;
			}
			else{
				thisBatchRowCnt=remainCnt;
			}
			batchIdx++;

			cout<<IceUtil::Time::now().toDateTime()<<" RebuildRUVsModel batch "<<batchIdx<<"/"<<batchCnt<<" begin"<<endl; 

			featureIdxTo=featureIdxFrom+thisBatchRowCnt;

			RUVsWorkerPtr worker=workerMgr.GetRUVsWorker(workerIdx);
			

			if (m_RUVInfo.RUVMode == iBS::RUVModeRUVsForVariation)
			{
				GetYVariation(featureIdxFrom, featureIdxTo, worker->GetBatchY());
			}
			else
			{
				GetY(featureIdxFrom, featureIdxTo, worker->GetBatchY());
			}

			/*if(workerIdx==0)
			{
				worker->A.save("A.mat.csv",arma::csv_ascii);
				worker->B.save("B.mat.csv",arma::csv_ascii);
				worker->C.save("C.mat.csv",arma::csv_ascii);
			}*/
			RUVsWorkItemPtr wi=new CRUVsComputeABC(
				*this, workerIdx,featureIdxFrom,featureIdxTo, worker->GetBatchY(),
				controlFeatureFlags,worker->A,worker->B,worker->C,AequalB,
				worker->As,
				worker->GetColIdxPermutation());

			worker->AddWorkItem(wi);

			m_processingWorkerIdxs.pop_front();
			featureIdxFrom+=thisBatchRowCnt;
			remainCnt-=thisBatchRowCnt;
			CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_rebuidRUVTaskID, 1);
		}

	}

	if(!bContinue)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
		return false;
	}

	cout<<IceUtil::Time::now().toDateTime()<<" RebuildRUVsModel batch "<<batchIdx<<"/"<<batchCnt<<" end"<<endl; 
	
	for(int i=0;i<threadCnt;i++)
	{
		RUVsWorkerPtr worker=workerMgr.GetRUVsWorker(i);
		A+=worker->A;
		B+=worker->B;
		C+=worker->C;
	}

	// creating permutated metrics
	if (m_RUVInfo.PermutationCnt > 0)
	{
		// permutated eigen values matrix
		Ice::Long pevColCnt = m_RUVInfo.PermutationCnt;
		Ice::Long pevRowCnt = m;

		::IceUtil::ScopedArray<Ice::Double>  permutatedEigVals(new ::Ice::Double[pevRowCnt*pevColCnt]);
		if (!permutatedEigVals.get()){
			return false;
		}

		for (int p = 0; p < pevColCnt; p++)
		{
			::arma::mat A_null(m, m, arma::fill::zeros);
			for (int i = 0; i<threadCnt; i++)
			{
				RUVsWorkerPtr worker = workerMgr.GetRUVsWorker(i);
				// merge A_null from workers
				A_null += worker->As[p];
			}

			// update the lower left triagle of A_null
			for (int i = 0; i<m; i++)
			{
				for (int j = i + 1; j<m; j++)
				{
					A_null(j, i) = A_null(i, j);
				}
			}

			// eigen decomposition
			arma::vec eigval;
			arma::mat U;

			//output is the same as in Matlab's eig, The eigenvalues are in ascending order
			//right most column of U corresponds largest eigenvalue
			if (!arma::eig_sym(eigval, U, A_null, "std"))
			{
				// add some error log
				continue;
			}

			for (Ice::Long i = pevRowCnt - 1; i >= 0; i--)
			{
				Ice::Long idx = ((pevRowCnt - 1) - i)*pevColCnt + p; //in desc order
				permutatedEigVals.get()[idx] = eigval(i);
			}
		}

		createOIDForPermutatedEigenValues();
		savePermutatedEigenValues(permutatedEigVals.get());
	}

	workerMgr.RequestShutdownAllWorkers();
	workerMgr.UnInitilize();

	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();

	if(AequalB)
	{
		B=A;
	}
	//update the lower left triagle of A and B
	for(int i=0;i<m;i++)
	{
		for(int j=i+1;j<m;j++)
		{
			A(j,i)=A(i,j);
			B(j,i)=B(i,j);
		}
	}

	return true;
}

bool CRUVBuilder::MultithreadGetA(
	::arma::mat& A, ::Ice::Int  threadCnt, ::Ice::Long ramMb )
{
	int n=(int)m_RUVInfo.n;

	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();

	const iBS::ByteVec& controlFeatureFlags = m_controlFeatureFlags;

	Ice::Long J = m_RUVInfo.J;
	if (m_RUVInfo.FeatureIdxFrom > 0 || m_RUVInfo.FeatureIdxTo > 0)
	{
		J = m_RUVInfo.FeatureIdxTo - m_RUVInfo.FeatureIdxFrom;
	}

	Ice::Long totalRam = J*colCnt*sizeof(Ice::Double);
	totalRam /= (1024 * 1024);
	if (totalRam < ramMb)
	{
		ramMb = totalRam;
	}

	ramMb/=sizeof(Ice::Double);
	ramMb/=threadCnt;

	if (ramMb == 0)
	{
		ramMb = 1;
	}

	::Ice::Long batchValueCnt=1024*1024*ramMb;
	if(batchValueCnt%colCnt!=0){
		batchValueCnt-=(batchValueCnt%colCnt); //data row not spread in two batch files
	}

	CRUVgWorkerMgr workerMgr(threadCnt);
	//each worker will allocate Y with size of batchValueCnt 
	if (workerMgr.Initilize(batchValueCnt, n, m_RUVInfo.PermutationCnt) == false)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
		return false;
	}

	::Ice::Long batchRowCnt=batchValueCnt/colCnt;
	::Ice::Long remainCnt=J;

	::Ice::Long thisBatchRowCnt=0;
	::Ice::Long featureIdxFrom = m_RUVInfo.FeatureIdxFrom;
	::Ice::Long featureIdxTo=0;
	::Ice::Long batchCnt= remainCnt/batchRowCnt+1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(
		m_rebuidRUVTaskID, "MultithreadGetA", batchCnt);

	bool bContinue=true;
	bool bNeedExit=false;

	int batchIdx=0;

	for(int i=0;i<threadCnt;i++)
	{
		m_freeWorkerIdxs.push_back(i);
	}

	while(!bNeedExit)
	{
		
		{
			//entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while(!m_shutdownRequested)
			{
				//merge freeworker that has not been lauched due to remainCnt==0
				if(!m_processingWorkerIdxs.empty())
				{
					std::copy(m_processingWorkerIdxs.begin(),m_processingWorkerIdxs.end(),
						std::back_inserter(m_freeWorkerIdxs));
					m_processingWorkerIdxs.clear();
				}

				//have to wait if 1: remain count>0, but no free worker; 
				//or 2. remain count==0, worker not finished yet
				if((remainCnt>0 && m_freeWorkerIdxs.empty())
					|| (remainCnt == 0 && (int)m_freeWorkerIdxs.size()<threadCnt))
				{
					m_needNotify = true;
					m_monitor.wait();
				}

				//free worker need to finish left items
				if(remainCnt>0 && !m_freeWorkerIdxs.empty())
				{
					std::copy(m_freeWorkerIdxs.begin(),m_freeWorkerIdxs.end(),
						std::back_inserter(m_processingWorkerIdxs));
					m_freeWorkerIdxs.clear();
					break;
				}
				else if(remainCnt==0 && (int)m_freeWorkerIdxs.size()==threadCnt)
				{
					bNeedExit=true;
					break;
					
				}
			}

			//if control request shutdown
			if(m_shutdownRequested)
			{
				bContinue=false;
				bNeedExit=true;
				cout<<"CRUVBuilder m_shutdownRequested==true ..."<<endl; 
			}

			//leaving critical region
			m_needNotify=false;
		}

		//these workers are free, need to use them to process
		while(!m_processingWorkerIdxs.empty() && remainCnt>0)
		{
			int workerIdx = m_processingWorkerIdxs.front();
			
			if(remainCnt>batchRowCnt){
				thisBatchRowCnt=batchRowCnt;
			}
			else{
				thisBatchRowCnt=remainCnt;
			}
			batchIdx++;

			cout<<IceUtil::Time::now().toDateTime()<<" RebuildRUVgModel batch "<<batchIdx<<"/"<<batchCnt<<" begin"<<endl; 

			featureIdxTo=featureIdxFrom+thisBatchRowCnt;

			RUVgWorkerPtr worker=workerMgr.GetRUVgWorker(workerIdx);
			

			if (m_RUVInfo.RUVMode == iBS::RUVModeRUVgForVariation)
			{
				GetYVariation(featureIdxFrom, featureIdxTo, worker->GetBatchY());
			}
			else
			{
				GetY(featureIdxFrom, featureIdxTo, worker->GetBatchY());
			}

			RUVsWorkItemPtr wi=new CRUVgComputeA(
				*this, workerIdx,featureIdxFrom,featureIdxTo, worker->GetBatchY(),
				controlFeatureFlags,worker->A,
				worker->As,
				worker->GetColIdxPermutation());

			worker->AddWorkItem(wi);

			m_processingWorkerIdxs.pop_front();
			featureIdxFrom+=thisBatchRowCnt;
			remainCnt-=thisBatchRowCnt;

			CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_rebuidRUVTaskID, 1);
		}

	}

	if(!bContinue)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
		return false;
	}

	cout<<IceUtil::Time::now().toDateTime()<<" RebuildRUVgModel batch "<<batchIdx<<"/"<<batchCnt<<" end"<<endl; 
	
	for(int i=0;i<threadCnt;i++)
	{
		RUVgWorkerPtr worker=workerMgr.GetRUVgWorker(i);
		A+=worker->A;
	}

	// creating permutated metrics
	if (m_RUVInfo.PermutationCnt > 0)
	{
		// permutated eigen values matrix
		Ice::Long pevColCnt = m_RUVInfo.PermutationCnt;
		Ice::Long pevRowCnt = n;

		::IceUtil::ScopedArray<Ice::Double>  permutatedEigVals(new ::Ice::Double[pevRowCnt*pevColCnt]);
		if (!permutatedEigVals.get()){
			return false;
		}

		for (int p = 0; p < pevColCnt; p++)
		{
			::arma::mat A_null(n, n, arma::fill::zeros);
			for (int i = 0; i<threadCnt; i++)
			{
				RUVgWorkerPtr worker = workerMgr.GetRUVgWorker(i);
				// merge A_null from workers
				A_null += worker->As[p];
			}

			// update the lower left triagle of A_null
			for (int i = 0; i<n; i++)
			{
				for (int j = i + 1; j<n; j++)
				{
					A_null(j, i) = A_null(i, j);
				}
			}

			// eigen decomposition
			arma::vec eigval;
			arma::mat U;

			//output is the same as in Matlab's eig, The eigenvalues are in ascending order
			//right most column of U corresponds largest eigenvalue
			if (!arma::eig_sym(eigval, U, A_null, "std"))
			{
				// add some error log
				continue;
			}

			for (Ice::Long i = pevRowCnt - 1; i >= 0; i--)
			{
				Ice::Long idx = ((pevRowCnt - 1) - i)*pevColCnt + p; //in desc order
				permutatedEigVals.get()[idx] = eigval(i);
			}
		}

		createOIDForPermutatedEigenValues();
		savePermutatedEigenValues(permutatedEigVals.get());
	}
	
	workerMgr.RequestShutdownAllWorkers();
	workerMgr.UnInitilize();

	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();

	//update the lower left triagle of A
	for(int i=0;i<n;i++)
	{
		for(int j=i+1;j<n;j++)
		{
			A(j,i)=A(i,j);
		}
	}

	return true;
}


bool CRUVBuilder::RebuildRUVModel(::Ice::Int  threadCnt, ::Ice::Long ramMb, ::Ice::Long taskID)
{
	m_rebuidRUVTaskID = taskID;
	if(m_RUVInfo.FacetStatus==iBS::RUVFacetStatusFilteredOIDsReady)
	{
		FilterOriginalFeatures(1024);
	}

	if (m_RUVInfo.ControlFeaturePolicy == iBS::RUVControlFeaturePolicyANOVA)
	{
		MultithreadRowANOVA(2, 256);
	}
	else if (m_RUVInfo.ControlFeaturePolicy == iBS::RUVControlFeaturePolicyMaxCntLow)
	{
		ControlFeatureByMaxCntLow();
	}
	else if (m_RUVInfo.ControlFeaturePolicy == iBS::RUVControlFeaturePolicyFeatureIdxList)
	{
		ControlFeatureByRowIdxs();
	}
	else if (m_RUVInfo.ControlFeaturePolicy == iBS::RUVControlFeaturePolicyAllInUpperQuantile)
	{
		ControlFeatureByAllInQuantile(1);
	}
	else if (m_RUVInfo.ControlFeaturePolicy == iBS::RUVControlFeaturePolicyAllInLowerQuantile)
	{
		ControlFeatureByAllInQuantile(0);
	}
	
	bool ret=false;
	if(m_RUVInfo.RUVMode==iBS::RUVModeRUVs
		|| m_RUVInfo.RUVMode == iBS::RUVModeRUVsForVariation)
	{
		ret = RebuildRUVModel_RUVs(threadCnt,ramMb);
	}
	else 
	{
		ret = RebuildRUVModel_RUVg(threadCnt,ramMb);
	}

	CGlobalVars::get()->theFeatureValueWorkerMgr->SetAMDTaskDone(m_rebuidRUVTaskID);
	m_rebuidRUVTaskID = 0;
	return ret;
}

bool CRUVBuilder::RebuildRUVModel_RUVs(::Ice::Int  threadCnt, ::Ice::Long ramMb)
{
	int n=(int)m_RUVInfo.n;
	int m=m_RUVInfo.CtrlSampleCnt;

	//YcsYcsT
	::arma::mat A(m,m,arma::fill::zeros);
	//YcscfYcscfT
	::arma::mat B(m,m,arma::fill::zeros);
	//YcfYcscfT
	::arma::mat C(n,m,arma::fill::zeros);

	MultithreadGetABC(A, B, C, threadCnt, ramMb);

	// eigen decomposition
	arma::vec eigval;
	arma::mat U;

	//output same as in Matlab's eig, The eigenvalues are in ascending order
	//right most column of U corresponds largest eigenvalue
	//if(!arma::eig_sym(eigval, U, A))
		//return false;
	A.save("A.mat.csv",arma::csv_ascii);
	B.save("B.mat.csv",arma::csv_ascii);
	C.save("C.mat.csv",arma::csv_ascii);

	if(!arma::eig_sym(eigval, U, A, "std"))
		return false;

	createOIDForEigenValues();
	saveEigenValues(eigval);

	createOIDForEigenVectors();
	saveEigenVectors(U);

	eigval.save("eigval.mat.csv",arma::csv_ascii);
	U.save("U.mat.csv",arma::csv_ascii);

	int maxK=0;
	for(int i=m-1;i>=0;i--)
	{
		if(eigval(i)>m_RUVInfo.Tol){
			maxK++;
		}

		if(maxK>=m_RUVInfo.MaxK)
			break;
	}
	if(maxK>m_RUVInfo.P)
	{
		maxK=(int)m_RUVInfo.P;
	}

	m_RUVInfo.K=maxK;
	if(maxK==0)
		return false;

	//createObserverGroupForTs(maxK);
	//createObserverGroupForZs(maxK);
	//createObserverGroupForGs(maxK);
	createObserverGroupForWts(maxK);
	
	for(int k=0;k<maxK;k++)
	{
		::arma::mat Uk(m,k+1);
		for(int i=0;i<k+1;i++)
		{
			Uk(arma::span::all,i)=U(arma::span::all,m-1-i);
		}
		::arma::mat D=Uk.t()*B*Uk;
		::arma::mat D_inv=::arma::inv_sympd(D);
		::arma::mat W=C*Uk*D_inv;
		//::arma::mat T=W*Uk.t();

		//saveT(k,T);
		//saveZ(k,W);
		//saveG(k+1,W);
		saveWt(k+1,W);
		
	}

	CBdvdGlobalVars::get()->theRUVFacetDB->SetRUVFacetInfo(m_RUVInfo.FacetID,m_RUVInfo);
	return true;
}

bool CRUVBuilder::RebuildRUVModel_RUVg(::Ice::Int  threadCnt, ::Ice::Long ramMb)
{
	int n=(int)m_RUVInfo.n;

	//YcfYcfT
	::arma::mat A(n,n,arma::fill::zeros);
	
	MultithreadGetA(A,threadCnt,ramMb);

	// eigen decomposition
	arma::vec eigval;
	arma::mat U;

	//output same as in Matlab's eig, The eigenvalues are in ascending order
	//right most column of U corresponds largest eigenvalue
	//if(!arma::eig_sym(eigval, U, A))
		//return false;
	A.save("A.mat.RUVg.csv",arma::csv_ascii);

	if(!arma::eig_sym(eigval, U, A, "std"))
		return false;

	createOIDForEigenValues();
	saveEigenValues(eigval);

	createOIDForEigenVectors();
	saveEigenVectors(U);

	eigval.save("eigval.mat.RUVg.csv",arma::csv_ascii);
	U.save("U.mat.RUVg.csv",arma::csv_ascii);

	int maxK=0;
	for(int i=n-1;i>=0;i--)
	{
		if(eigval(i)>m_RUVInfo.Tol){
			maxK++;
		}

		if(maxK>=m_RUVInfo.MaxK)
			break;
	}
	if(maxK>m_RUVInfo.P)
	{
		maxK=(int)m_RUVInfo.P;
	}

	m_RUVInfo.K=maxK;
	if(maxK==0)
		return false;

	//createObserverGroupForZs(maxK);
	//createObserverGroupForGs(maxK);
	createObserverGroupForWts(maxK);
	
	for(int k=0;k<maxK;k++)
	{
		::arma::mat W(n,k+1); // multiply with sqrt(eigvalue)
		for(int i=0;i<k+1;i++)
		{
			W(arma::span::all,i)=U(arma::span::all,n-1-i)*sqrt(eigval(n-1-i));
		}

		//saveZ(k,W);
		//saveG(k+1,W);
		saveWt(k+1,W);
		
	}

	CBdvdGlobalVars::get()->theRUVFacetDB->SetRUVFacetInfo(m_RUVInfo.FacetID,m_RUVInfo);
	return true;
}

Ice::Int CRUVBuilder::SetWtVectorIdxs(const ::iBS::IntVec& vecIdxs)
{
	m_WtVectorIdxs = vecIdxs;
	return 1;
}

bool CRUVBuilder::saveEigenVectors(const ::arma::mat& U)
{
	iBS::FeatureObserverSimpleInfoPtr foi
		= CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.OIDforEigenVectors);

	Ice::Long n = 0;
	if (m_RUVInfo.RUVMode == iBS::RUVModeRUVs
		|| m_RUVInfo.RUVMode == iBS::RUVModeRUVsForVariation)
	{
		n = m_RUVInfo.CtrlSampleCnt;
	}
	else
	{
		n = m_RUVInfo.n;
	}

	Ice::Long totalValueCnt = n*n;
	::IceUtil::ScopedArray<Ice::Double>  tvals(new ::Ice::Double[totalValueCnt]);
	if (!tvals.get()){
		return false;
	}

	for (int i = 0; i<n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			Ice::Long idx = i*n + j;
			tvals.get()[idx] = U(j, n - 1 - i);
		}
	}

	//save G to store
	std::pair<const Ice::Double*, const Ice::Double*> values(
		tvals.get(), tvals.get() + totalValueCnt);

	Ice::Int foiObserverID = foi->ObserverID;
	Ice::Int foiStoreObserverID = foi->ObserverID;

	Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
	Ice::Long foiDomainSize = foi->DomainSize;

	Ice::Long rowIdxFrom = 0;
	Ice::Long rowIdxTo = n;

	Ice::Long s_featureIdxFrom = rowIdxFrom * foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = rowIdxTo*foi->ObserverGroupSize;  //index in store


	foi->DomainSize = foiStoreDomainSize; //convert to store size
	foi->ObserverID = foiStoreObserverID;
	CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
		foi, s_featureIdxFrom, s_featureIdxTo, values);
	foi->ObserverID = foiObserverID;
	foi->DomainSize = foiDomainSize; //convert back

	return true;
}

bool CRUVBuilder::getEigenVectors(::arma::mat& U)
{
	iBS::FeatureObserverSimpleInfoPtr foi
		= CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.OIDforEigenVectors);

	Ice::Long n = 0;
	if (m_RUVInfo.RUVMode == iBS::RUVModeRUVs
		|| m_RUVInfo.RUVMode == iBS::RUVModeRUVsForVariation)
	{
		n = m_RUVInfo.CtrlSampleCnt;
	}
	else
	{
		n = m_RUVInfo.n;
	}

	Ice::Int foiObserverID = foi->ObserverID;
	Ice::Int foiStoreObserverID = foi->ObserverID;

	Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
	Ice::Long foiDomainSize = foi->DomainSize;

	Ice::Long rowIdxFrom = 0;
	Ice::Long rowIdxTo = n;

	Ice::Long s_featureIdxFrom = rowIdxFrom * foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = rowIdxTo*foi->ObserverGroupSize;  //index in store


	foi->DomainSize = foiStoreDomainSize; //convert to store size
	foi->ObserverID = foiStoreObserverID;
	::IceUtil::ScopedArray<Ice::Double> tvals(
		CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
		foi, s_featureIdxFrom, s_featureIdxTo));
	foi->ObserverID = foiObserverID;
	foi->DomainSize = foiDomainSize; //convert back

	U = arma::mat(n, n, arma::fill::zeros);
	for (int i = 0; i<n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			Ice::Long idx = i*n + j;
			U(j, n - 1 - i) = tvals.get()[idx];
		}
	}

	return true;
}

bool CRUVBuilder::saveEigenValues(const ::arma::vec& eigenVals)
{
	iBS::FeatureObserverSimpleInfoPtr ts_foi
		= CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.OIDforEigenValue);
	int m = (int)ts_foi->DomainSize;
	Ice::Long totalValueCnt = m;
	::IceUtil::ScopedArray<Ice::Double>  tvals(new ::Ice::Double[totalValueCnt]);
	if (!tvals.get()){
		return false;
	}

	for (int i = m - 1; i >= 0; i--)
	{
		tvals.get()[m - 1 - i] = eigenVals(i); //in desc order
	}

	
	//save T to store
	std::pair<const Ice::Double*, const Ice::Double*> values(
		tvals.get(), tvals.get() + totalValueCnt);

	Ice::Int foiObserverID = ts_foi->ObserverID;
	Ice::Int foiStoreObserverID = ts_foi->ObserverID;

	Ice::Long foiStoreDomainSize = ts_foi->ObserverGroupSize*ts_foi->DomainSize;
	Ice::Long foiDomainSize = ts_foi->DomainSize;
	Ice::Long s_featureIdxFrom = 0 * ts_foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = m* ts_foi->ObserverGroupSize;  //index in store

	ts_foi->DomainSize = foiStoreDomainSize; //convert to store size
	ts_foi->ObserverID = foiStoreObserverID;
	CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
		ts_foi, s_featureIdxFrom, s_featureIdxTo, values);
	ts_foi->ObserverID = foiObserverID;
	ts_foi->DomainSize = foiDomainSize; //convert back

	return true;
}

bool CRUVBuilder::getEigenValues(arma::vec& eigenVals)
{
	iBS::FeatureObserverSimpleInfoPtr ts_foi
		= CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.OIDforEigenValue);
	int m = (int)ts_foi->DomainSize;

	Ice::Int foiObserverID = ts_foi->ObserverID;
	Ice::Int foiStoreObserverID = ts_foi->ObserverID;

	Ice::Long foiStoreDomainSize = ts_foi->ObserverGroupSize*ts_foi->DomainSize;
	Ice::Long foiDomainSize = ts_foi->DomainSize;
	Ice::Long s_featureIdxFrom = 0 * ts_foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = m* ts_foi->ObserverGroupSize;  //index in store

	ts_foi->DomainSize = foiStoreDomainSize; //convert to store size
	ts_foi->ObserverID = foiStoreObserverID;
	::IceUtil::ScopedArray<Ice::Double> tvals(
		CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
		ts_foi, s_featureIdxFrom, s_featureIdxTo));
	ts_foi->ObserverID = foiObserverID;
	ts_foi->DomainSize = foiDomainSize; //convert back
	
	eigenVals = arma::vec(m, 1, arma::fill::zeros);
	for (int i = 0; i <m; i++)
	{
		eigenVals(i) = tvals.get()[i];
	}

	return true;
}

::Ice::Int CRUVBuilder::GetEigenVals(::iBS::DoubleVec& values)
{
	arma::vec evs;
	getEigenValues(evs);
	int nrow = (int)evs.n_rows;
	values.resize(nrow, 0);
	for (int i = 0; i < nrow; i++)
	{
		values[i] = evs(i);
	}
	return 1;
}

bool CRUVBuilder::savePermutatedEigenValues(Ice::Double *pValues)
{
	iBS::FeatureObserverSimpleInfoPtr foi
		= CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.OIDforPermutatedEigenValues);

	Ice::Long domainSize = 0;
	if (m_RUVInfo.RUVMode == iBS::RUVModeRUVs
		|| m_RUVInfo.RUVMode == iBS::RUVModeRUVsForVariation)
	{
		domainSize = m_RUVInfo.CtrlSampleCnt;
	}
	else
	{
		domainSize = m_RUVInfo.n;
	}
	Ice::Long n = m_RUVInfo.PermutationCnt;
	Ice::Long totalValueCnt = domainSize * n;

	//save G to store
	std::pair<const Ice::Double*, const Ice::Double*> values(
		pValues, pValues + totalValueCnt);

	Ice::Int foiObserverID = foi->ObserverID;
	Ice::Int foiStoreObserverID = foi->ObserverID;

	Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
	Ice::Long foiDomainSize = foi->DomainSize;

	Ice::Long rowIdxFrom = 0;
	Ice::Long rowIdxTo = domainSize;

	Ice::Long s_featureIdxFrom = rowIdxFrom * foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = rowIdxTo*foi->ObserverGroupSize;  //index in store

	foi->DomainSize = foiStoreDomainSize; //convert to store size
	foi->ObserverID = foiStoreObserverID;
	CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
		foi, s_featureIdxFrom, s_featureIdxTo, values);
	foi->ObserverID = foiObserverID;
	foi->DomainSize = foiDomainSize; //convert back

	return true;
}


::Ice::Int CRUVBuilder::SelectKByEigenVals(::Ice::Double minFraction, 
	::Ice::Int& k, ::iBS::DoubleVec& fractions)
{
	arma::vec evs;
	getEigenValues(evs);
	int nrow = (int)evs.n_rows;
	fractions.resize(nrow, 0);
	Ice::Double sum = 0;
	for (int i = 0; i<nrow; i++)
	{
		if (evs(i)<m_RUVInfo.Tol)
		{
			break;
		}
		sum += evs(i);
		fractions[i] = evs(i) / sum;
		if (fractions[i]>minFraction)
		{
			k = i + 1; // 1-based
		}
	}

	return 1;
}

bool CRUVBuilder::saveT(int k, const ::arma::mat& T)
{
	int n=(int)m_RUVInfo.n;
	int m=m_RUVInfo.CtrlSampleCnt;

	iBS::FeatureObserverSimpleInfoPtr ts_foi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.ObserverIDforTs);

	Ice::Long totalValueCnt=n*ts_foi->ObserverGroupSize;
	::IceUtil::ScopedArray<Ice::Double>  tvals(new ::Ice::Double[totalValueCnt]);
	if(!tvals.get()){
		return false;
	}

	//copy T matrix to memory tvals
	for(int i=0;i<n;i++)
	{
		for(int j=0;j<m;j++)
		{
			tvals.get()[i*m+j]=T(i,j);
		}
	}

	//save T to store
	std::pair<const Ice::Double*, const Ice::Double*> values(
		tvals.get(), tvals.get()+totalValueCnt);

	Ice::Int foiObserverID=ts_foi->ObserverID;
	Ice::Int foiStoreObserverID=ts_foi->ObserverID;

	Ice::Long foiStoreDomainSize = ts_foi->ObserverGroupSize*ts_foi->DomainSize;
	Ice::Long foiDomainSize = ts_foi->DomainSize;
	Ice::Long s_featureIdxFrom = (k*n) * ts_foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = (k+1)*n* ts_foi->ObserverGroupSize;  //index in store

	ts_foi->DomainSize=foiStoreDomainSize; //convert to store size
	ts_foi->ObserverID=foiStoreObserverID;
	CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
			ts_foi,s_featureIdxFrom, s_featureIdxTo, values);
	ts_foi->ObserverID=foiObserverID; 
	ts_foi->DomainSize=foiDomainSize; //convert back

	return true;
}

bool CRUVBuilder::saveZ(int k, const ::arma::mat& W)
{
	int n=(int)m_RUVInfo.n;
	::arma::mat WtW_inv=::arma::inv_sympd(W.t()*W);
	::arma::mat Z=W*WtW_inv*W.t();

	iBS::FeatureObserverSimpleInfoPtr foi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.ObserverIDforZs);

	Ice::Long totalValueCnt=n*n;
	::IceUtil::ScopedArray<Ice::Double>  vals(new ::Ice::Double[totalValueCnt]);
	if(!vals.get()){
		return false;
	}

	//copy Z matrix to memory vals
	for(int i=0;i<n;i++)
	{
		for(int j=0;j<n;j++)
		{
			vals.get()[i*n+j]=Z(i,j);
		}
	}

	//save G to store
	std::pair<const Ice::Double*, const Ice::Double*> values(
		vals.get(), vals.get()+totalValueCnt);

	Ice::Int foiObserverID=foi->ObserverID;
	Ice::Int foiStoreObserverID=foi->ObserverID;

	Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
	Ice::Long foiDomainSize = foi->DomainSize;

	Ice::Long rowIdxFrom=k*n;
	Ice::Long rowIdxTo=(k+1)*n;
	Ice::Long s_featureIdxFrom = rowIdxFrom * foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = rowIdxTo*foi->ObserverGroupSize;  //index in store
	foi->DomainSize=foiStoreDomainSize; //convert to store size
	foi->ObserverID=foiStoreObserverID;
	CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
			foi,s_featureIdxFrom, s_featureIdxTo, values);
	foi->ObserverID=foiObserverID; 
	foi->DomainSize=foiDomainSize; //convert back

	return true;
}

//k should be 1 based
bool CRUVBuilder::saveG(int k, const ::arma::mat& W)
{
	int n=(int)m_RUVInfo.n;
	int p=(int)m_RUVInfo.P;
	::arma::mat A(n,p+k,arma::fill::zeros);

	for(int i=0;i<n;i++)
	{
		int conditionIdx=m_sampleIdx2ConditionIdx[i];
		A(i,conditionIdx)=1;
		for(int j=0;j<k;j++)
		{
			A(i,j+p)=W(i,j);
		}
	}
	
	::arma::mat AtA_inv=::arma::inv_sympd(A.t()*A);
	::arma::mat G=AtA_inv*A.t();

	iBS::FeatureObserverSimpleInfoPtr gs_foi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.ObserverIDforGs);

	Ice::Long totalValueCnt=(p+k)*n;
	::IceUtil::ScopedArray<Ice::Double>  vals(new ::Ice::Double[totalValueCnt]);
	if(!vals.get()){
		return false;
	}

	//copy G matrix to memory vals
	for(int i=0;i<p+k;i++)
	{
		for(int j=0;j<n;j++)
		{
			vals.get()[i*n+j]=G(i,j);
		}
	}

	//save G to store
	std::pair<const Ice::Double*, const Ice::Double*> values(
		vals.get(), vals.get()+totalValueCnt);

	Ice::Int foiObserverID=gs_foi->ObserverID;
	Ice::Int foiStoreObserverID=gs_foi->ObserverID;

	Ice::Long foiStoreDomainSize = gs_foi->ObserverGroupSize*gs_foi->DomainSize;
	Ice::Long foiDomainSize = gs_foi->DomainSize;

	Ice::Long rowIdxFrom=p*(k-1)+((k-1)*k)/2;
	Ice::Long rowIdxTo=rowIdxFrom+(p+k);

	Ice::Long s_featureIdxFrom = rowIdxFrom * gs_foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = rowIdxTo*gs_foi->ObserverGroupSize;  //index in store


	gs_foi->DomainSize=foiStoreDomainSize; //convert to store size
	gs_foi->ObserverID=foiStoreObserverID;
	CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
			gs_foi,s_featureIdxFrom, s_featureIdxTo, values);
	gs_foi->ObserverID=foiObserverID; 
	gs_foi->DomainSize=foiDomainSize; //convert back

	return true;
}

//k should be 1 based
bool CRUVBuilder::saveWt(int k, const ::arma::mat& W)
{
	int n=(int)m_RUVInfo.n;
	::arma::mat Wt=W.t();

	iBS::FeatureObserverSimpleInfoPtr foi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.ObserverIDforWts);

	Ice::Long totalValueCnt=k*n;
	::IceUtil::ScopedArray<Ice::Double>  vals(new ::Ice::Double[totalValueCnt]);
	if(!vals.get()){
		return false;
	}

	//copy Wt matrix to memory vals
	for(int i=0;i<k;i++)
	{
		for(int j=0;j<n;j++)
		{
			vals.get()[i*n+j]=Wt(i,j);
		}
	}

	//save G to store
	std::pair<const Ice::Double*, const Ice::Double*> values(
		vals.get(), vals.get()+totalValueCnt);

	Ice::Int foiObserverID=foi->ObserverID;
	Ice::Int foiStoreObserverID=foi->ObserverID;

	Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
	Ice::Long foiDomainSize = foi->DomainSize;

	Ice::Long rowIdxFrom=((k-1)*k)/2;
	Ice::Long rowIdxTo=rowIdxFrom+ k;

	Ice::Long s_featureIdxFrom = rowIdxFrom * foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = rowIdxTo*foi->ObserverGroupSize;  //index in store


	foi->DomainSize=foiStoreDomainSize; //convert to store size
	foi->ObserverID=foiStoreObserverID;
	CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
			foi,s_featureIdxFrom, s_featureIdxTo, values);
	foi->ObserverID=foiObserverID; 
	foi->DomainSize=foiDomainSize; //convert back

	return true;
}

::Ice::Int
CRUVBuilder::SetNormalizeMode(::iBS::RUVModeEnum nmode)
{
	m_RUVInfo.RUVMode=nmode;
    return 1;
}

::Ice::Int
CRUVBuilder::SetOutputScale(::iBS::RUVOutputScaleEnum scale)
{
	m_outputScale = scale;
	return 1;
}

::Ice::Int
CRUVBuilder::SetOutputMode(::iBS::RUVOutputModeEnum mode)
{
	m_outputMode = mode;
	return 1;
}

::Ice::Int
CRUVBuilder::SetOutputWorkerNum(::Ice::Int workerNum)
{
	m_outputWorkerNum = workerNum;
	m_outputWorkerMgr.Initilize(m_outputWorkerNum);
	return 1;
}

::Ice::Int
CRUVBuilder::SetCtrlQuantileValues(::Ice::Double quantile, const ::iBS::DoubleVec& qvalues, ::Ice::Double fraction)
{
	m_ctrlQuantile = quantile;
	m_ctrlQvalues = qvalues;
	m_ctrlQuantileAllInFraction = fraction;
	return 1;
}

::Ice::Int
CRUVBuilder::SetOutputSamples(const ::iBS::IntVec& sampleIDs)
{
	m_outputSampleIDs = sampleIDs;
	return 1;
}

::Ice::Int
CRUVBuilder::SetExcludedSamplesForGroupMean(const ::iBS::IntVec& excludeSampleIDs)
{
	m_excludeSampleIDsForGroupMean = excludeSampleIDs;
	return 1;
}

bool CRUVBuilder::SetActiveK(int k, int extW)
{
	if (m_RUVInfo.KnownFactors.empty())
	{
		m_extentW = 0;
	}
	else if (extW>0 && (int)m_RUVInfo.KnownFactors.size() >= extW)
	{
		m_extentW = extW;
	}
	else
	{
		m_extentW = 0;
	}

	if (m_extentW == 0)
	{
		m_activeK=k;
		//SetActiveK_T(k);
		SetActiveK_Wt(k);
		//SetActiveK_Z(k);
		//SetActiveK_G(k);
		//SetActiveK_AG(k);

		CreateZbyWt(k + m_extentW);
		CreateGbyWt(k + m_extentW);
		SetActiveK_AG(k + m_extentW);
	}
	else
	{
		m_activeK = k;
		SetActiveK_Wt(k);
		ExtentWt(k, m_extentW);
		CreateZbyWt(k + m_extentW);
		CreateGbyWt(k + m_extentW);
		SetActiveK_AG(k + m_extentW);
	}
	return true;
}

bool CRUVBuilder::SetActiveK_T(int k)
{
	k=k-1;
	if(k<0)
	{
		m_activeT.reset();
		return true;
	}
	
	iBS::FeatureObserverSimpleInfoPtr ts_foi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.ObserverIDforTs);
	if(!ts_foi)
		return false;

	int n=(int)m_RUVInfo.n;
	int maxK=(int)(ts_foi->DomainSize/n);
	if(k>=maxK)
		return false;

	Ice::Int foiObserverID=ts_foi->ObserverID;
	Ice::Int foiStoreObserverID=ts_foi->ObserverID;

	Ice::Long foiStoreDomainSize = ts_foi->ObserverGroupSize*ts_foi->DomainSize;
	Ice::Long foiDomainSize = ts_foi->DomainSize;
	Ice::Long s_featureIdxFrom = (k*n) * ts_foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = (k+1)*n* ts_foi->ObserverGroupSize;     //index in store

	ts_foi->DomainSize=foiStoreDomainSize; //convert to store size
	ts_foi->ObserverID=foiStoreObserverID;
	m_activeT.reset(CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
			ts_foi,s_featureIdxFrom, s_featureIdxTo));
	ts_foi->ObserverID=foiObserverID; 
	ts_foi->DomainSize=foiDomainSize; //convert back

	return true;
}

bool CRUVBuilder::SetActiveK_Z(int k)
{
	k=k-1;
	if(k<0)
	{
		m_activeZ.reset();
		return true;
	}
	
	iBS::FeatureObserverSimpleInfoPtr zs_foi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.ObserverIDforZs);
	if(!zs_foi)
		return false;

	int n=(int)m_RUVInfo.n;
	int maxK=(int)(zs_foi->DomainSize/n);
	if(k>=maxK)
		return false;

	Ice::Int foiObserverID=zs_foi->ObserverID;
	Ice::Int foiStoreObserverID=zs_foi->ObserverID;

	Ice::Long foiStoreDomainSize = zs_foi->ObserverGroupSize*zs_foi->DomainSize;
	Ice::Long foiDomainSize = zs_foi->DomainSize;
	Ice::Long rowIdxFrom=k*n;
	Ice::Long rowIdxTo=(k+1)*n;
	Ice::Long s_featureIdxFrom = rowIdxFrom * zs_foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = rowIdxTo*zs_foi->ObserverGroupSize;  //index in store

	zs_foi->DomainSize=foiStoreDomainSize; //convert to store size
	zs_foi->ObserverID=foiStoreObserverID;
	m_activeZ.reset(CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
			zs_foi,s_featureIdxFrom, s_featureIdxTo));
	zs_foi->ObserverID=foiObserverID; 
	zs_foi->DomainSize=foiDomainSize; //convert back

	return true;
}

bool CRUVBuilder::SetActiveK_G(int k)
{
	k=k-1;
	if(k<0)
	{
		m_activeG.reset();
		return true;
	}

	k=k+1; //convert to 1 based
	int p=(int)m_RUVInfo.P;
	
	iBS::FeatureObserverSimpleInfoPtr foi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.ObserverIDforGs);
	if(!foi)
		return false;
	
	Ice::Int foiObserverID=foi->ObserverID;
	Ice::Int foiStoreObserverID=foi->ObserverID;

	Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
	Ice::Long foiDomainSize = foi->DomainSize;
	Ice::Long rowIdxFrom=p*(k-1)+((k-1)*k)/2;
	Ice::Long rowIdxTo=rowIdxFrom+(p+k);
	Ice::Long s_featureIdxFrom = rowIdxFrom * foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = rowIdxTo*foi->ObserverGroupSize;  //index in store

	foi->DomainSize=foiStoreDomainSize; //convert to store size
	foi->ObserverID=foiStoreObserverID;
	m_activeG.reset(CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
			foi,s_featureIdxFrom, s_featureIdxTo));
	foi->ObserverID=foiObserverID; 
	foi->DomainSize=foiDomainSize; //convert back

	return true;
}


bool CRUVBuilder::ExtentWt(int k, int extW)
{
	//k is one based
	//Wt already fetched
	Ice::Double *Wt = m_activeWt.get();
	int n = (int)m_RUVInfo.n;
	int p = (int)m_RUVInfo.P;

	::arma::mat A(n, p + k + extW, arma::fill::zeros);

	for (int i = 0; i<n; i++)
	{
		int conditionIdx = m_sampleIdx2ConditionIdx[i];
		A(i, conditionIdx) = 1;
		for (int j = 0; j<k; j++)
		{
			Ice::Long idx = j*n + i;
			A(i, j + p) = Wt[idx];
		}
		for (int j = 0; j<extW; j++)
		{
			const iBS::DoubleVec& extColumnVec = m_RUVInfo.KnownFactors[j];
			A(i, p + k + j) = extColumnVec[i];
		}
	}

	//update Wt
	int rowCnt = k + extW;;
	int colCnt = n;
	Ice::Long totalValueCnt = rowCnt*colCnt;
	m_activeWt.reset(new ::Ice::Double[totalValueCnt]);
	if (!m_activeWt.get()){
		return false;
	}

	for (int i = 0; i<rowCnt; i++)
	{
		for (int j = 0; j<colCnt; j++)
		{
			Ice::Long idx = i*colCnt + j;
			//we need Wt(i,j) =W(j,i)= A(j,p+i)
			m_activeWt.get()[idx] = A(j, p + i);
		}
	}

	return true;
}

bool CRUVBuilder::CreateZbyWt(int k)
{
	if (k == 0)
	{
		m_activeZ.reset();
		return true;
	}

	//k is one based, already included extW
	//Wt already fetched, combined with extW
	Ice::Double *Wt = m_activeWt.get();
	int n = (int)m_RUVInfo.n;

	::arma::mat W(n, k, arma::fill::zeros);

	for (int i = 0; i<n; i++)
	{
		for (int j = 0; j<k; j++)
		{
			Ice::Long idx = j*n + i;
			W(i, j) = Wt[idx];
		}
	}

	::arma::mat WtW_inv = ::arma::inv_sympd(W.t()*W);
	::arma::mat Z = W*WtW_inv*W.t();

	int rowCnt = n;
	int colCnt = n;
	Ice::Long totalValueCnt = rowCnt*colCnt;
	m_activeZ.reset(new ::Ice::Double[totalValueCnt]);
	if (!m_activeZ.get()){
		return false;
	}

	//copy G matrix to memory vals

	for (int i = 0; i<rowCnt; i++)
	{
		for (int j = 0; j<colCnt; j++)
		{
			m_activeZ.get()[i*colCnt + j] = Z(i, j);
		}
	}

	return true;
}

bool CRUVBuilder::CreateGbyWt(int k)
{
	if (k==0)
	{
		m_activeG.reset();
		return true;
	}

	//k is one based, already included extW
	//Wt already fetched, combined with extW
	Ice::Double *Wt = m_activeWt.get();
	int n = (int)m_RUVInfo.n;
	int p = (int)m_RUVInfo.P;
	
	::arma::mat A(n, p + k, arma::fill::zeros);

	for (int i = 0; i<n; i++)
	{
		int conditionIdx = m_sampleIdx2ConditionIdx[i];
		A(i, conditionIdx) = 1;
		for (int j = 0; j<k; j++)
		{
			Ice::Long idx = j*n + i;
			A(i, j + p) = Wt[idx];
		}
	}

	::arma::mat G;
	try{
		::arma::mat AtA_inv = ::arma::inv_sympd(A.t()*A);
		G = AtA_inv*A.t();
	}
	catch (std::runtime_error e)
	{
		cout << IceUtil::Time::now().toDateTime() << " CreateGbyWt A.t()*A is sigular" << endl;
		return false;
	}
	

	int rowCnt = p + k;
	int colCnt = n;
	Ice::Long totalValueCnt = rowCnt*colCnt;
	m_activeG.reset(new ::Ice::Double[totalValueCnt]);
	if (!m_activeG.get()){
		return false;
	}

	//copy G matrix to memory vals
	
	for (int i = 0; i<rowCnt; i++)
	{
		for (int j = 0; j<colCnt; j++)
		{
			m_activeG.get()[i*colCnt + j] = G(i, j);
		}
	}

	return true;
}

bool CRUVBuilder::SetActiveK_Wt(int k)
{
	//1 based
	if (k<1)
	{
		m_activeWt.reset();
		return true;
	}

	if (m_WtVectorIdxs.empty())
	{
		return SetActiveK_Wt_noReorder(k);
	}
	else
	{
		int maxK = m_RUVInfo.K;
		SetActiveK_Wt_noReorder(maxK);
		::IceUtil::ScopedArray<Ice::Double> Wt(m_activeWt.release());
		int n = (int)m_RUVInfo.n;
		Ice::Long totalValueCnt = k*n;
		m_activeWt.reset(new ::Ice::Double[totalValueCnt]);
		if (!m_activeWt.get()){
			return false;
		}

		//copy Wt matrix to memory vals
		for (int i = 0; i<k; i++)
		{
			int vecIdx = m_WtVectorIdxs[i];
			for (int j = 0; j<n; j++)
			{
				m_activeWt.get()[i*n + j] = Wt.get()[vecIdx*n + j];
			}
		}
		return true;
	}
}


bool CRUVBuilder::SetActiveK_Wt_noReorder(int k)
{
	k=k-1;
	if(k<0)
	{
		m_activeWt.reset();
		return true;
	}

	k=k+1; //convert to 1 based

	iBS::FeatureObserverSimpleInfoPtr foi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_RUVInfo.ObserverIDforWts);
	if(!foi)
		return false;
	
	Ice::Int foiObserverID=foi->ObserverID;
	Ice::Int foiStoreObserverID=foi->ObserverID;

	Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
	Ice::Long foiDomainSize = foi->DomainSize;
	Ice::Long rowIdxFrom=((k-1)*k)/2;
	Ice::Long rowIdxTo=rowIdxFrom+ k;
	Ice::Long s_featureIdxFrom = rowIdxFrom * foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = rowIdxTo*foi->ObserverGroupSize;  //index in store

	foi->DomainSize=foiStoreDomainSize; //convert to store size
	foi->ObserverID=foiStoreObserverID;
	m_activeWt.reset(CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
			foi,s_featureIdxFrom, s_featureIdxTo));
	foi->ObserverID=foiObserverID; 
	foi->DomainSize=foiDomainSize; //convert back

	return true;
}

//k 1 based
bool CRUVBuilder::SetActiveK_AG(int k)
{
	if (k == 0 || m_activeG.get()==NULL)
	{
		m_activeAG_0W.reset();
		m_activeAG_X0.reset();
		return true;
	}

	Ice::Double *Wt = m_activeWt.get();
	int n=(int)m_RUVInfo.n;
	int p=(int)m_RUVInfo.P;
	::arma::mat A_0W(n, p+k,arma::fill::zeros);
	::arma::mat A_X0(n, p+k, arma::fill::zeros);
	::arma::mat G(p+k,n,arma::fill::zeros);

	//A=[0 W]
	for(int i=0;i<n;i++)
	{
		int conditionIdx=m_sampleIdx2ConditionIdx[i];
		A_X0(i, conditionIdx) = 1;
		for(int j=0;j<k;j++)
		{
			//A(i,j+p)=W(i,j);
			//we need A(i,j+p)=Wt(j,i);
			Ice::Long idx=j*n+i;
			A_0W(i, j + p) = Wt[idx];
		}
	}

	//A.save("AG_A.mat.csv",arma::csv_ascii);
	Ice::Double *pG = m_activeG.get();
	for(int i=0;i<p+k;i++)
	{
		for(int j=0;j<n;j++)
		{
			Ice::Long idx=i*n+j;
			G(i,j)=pG[idx];
		}
	}

	::arma::mat AG_0W=A_0W*G;
	::arma::mat AG_X0 = A_X0*G;

	Ice::Long totalValueCnt=n*n;
	m_activeAG_0W.reset(new ::Ice::Double[totalValueCnt]);
	if(!m_activeAG_0W.get()){
		return false;
	}

	m_activeAG_X0.reset(new ::Ice::Double[totalValueCnt]);
	if (!m_activeAG_X0.get()){
		return false;
	}

	//copy AG matrix to memory vals
	for(int i=0;i<n;i++)
	{
		for(int j=0;j<n;j++)
		{
			m_activeAG_0W.get()[i*n + j] = AG_0W(i, j);
			m_activeAG_X0.get()[i*n + j] = AG_X0(i, j);
		}
	}
	return true;
}


bool CRUVBuilder::SetTemp_AG_byZ()
{
	int n = (int)m_RUVInfo.n;
	int p = (int)m_RUVInfo.P;
	::arma::mat Z(n,n, arma::fill::zeros);
	::arma::mat I(n, n, arma::fill::zeros);
	for (int i = 0; i < n; i++)
	{
		I(i, i) = 1;
		for (int j = 0; j < n; j++)
		{
			Z(i, j) = m_activeZ.get()[i*n + j];
		}
	}

	::arma::mat X(n, p, arma::fill::zeros);
	for (int i = 0; i<n; i++)
	{
		int conditionIdx = m_sampleIdx2ConditionIdx[i];
		X(i, conditionIdx) = 1;
	}

	::arma::mat AG_0W = Z;
	::arma::mat XtX_inv = ::arma::inv_sympd(X.t()*X);
	::arma::mat AG_X0 = X*XtX_inv*X.t()*(I-Z);

	Ice::Long totalValueCnt = n*n;
	m_activeAG_0W.reset(new ::Ice::Double[totalValueCnt]);
	if (!m_activeAG_0W.get()){
		return false;
	}

	m_activeAG_X0.reset(new ::Ice::Double[totalValueCnt]);
	if (!m_activeAG_X0.get()){
		return false;
	}

	//copy AG matrix to memory vals
	for (int i = 0; i<n; i++)
	{
		for (int j = 0; j<n; j++)
		{
			m_activeAG_0W.get()[i*n + j] = AG_0W(i, j);
			m_activeAG_X0.get()[i*n + j] = AG_X0(i, j);
		}
	}
	return true;
}

bool CRUVBuilder::SetTemp_AG_byK0extW0()
{
	int n = (int)m_RUVInfo.n;
	int p = (int)m_RUVInfo.P;

	::arma::mat A(n, p, arma::fill::zeros);

	for (int i = 0; i<n; i++)
	{
		int conditionIdx = m_sampleIdx2ConditionIdx[i];
		A(i, conditionIdx) = 1;
	}

	::arma::mat G;
	try{
		::arma::mat AtA_inv = ::arma::inv_sympd(A.t()*A);
		G = AtA_inv*A.t(); //G is p by n
	}
	catch (std::runtime_error e)
	{
		cout << IceUtil::Time::now().toDateTime() << " SetTemp_AG_byK0extW0 A.t()*A is sigular" << endl;
		return false;
	}

	::arma::mat A_0W(n, p, arma::fill::zeros);
	::arma::mat A_X0(n, p, arma::fill::zeros);


	//A=[0 W]
	for (int i = 0; i<n; i++)
	{
		int conditionIdx = m_sampleIdx2ConditionIdx[i];
		A_X0(i, conditionIdx) = 1;
	}

	::arma::mat AG_0W(n, n, arma::fill::zeros);
	::arma::mat AG_X0 = A_X0*G;

	Ice::Long totalValueCnt = n*n;
	m_activeAG_0W.reset(new ::Ice::Double[totalValueCnt]);
	if (!m_activeAG_0W.get()){
		return false;
	}

	m_activeAG_X0.reset(new ::Ice::Double[totalValueCnt]);
	if (!m_activeAG_X0.get()){
		return false;
	}

	//copy AG matrix to memory vals
	for (int i = 0; i<n; i++)
	{
		for (int j = 0; j<n; j++)
		{
			m_activeAG_0W.get()[i*n + j] = AG_0W(i, j);
			m_activeAG_X0.get()[i*n + j] = AG_X0(i, j);
		}
	}
	return true;
}

::Ice::Double*
CRUVBuilder::GetNormalizedCnts(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, 
const iBS::IntVec& sampleIDs, iBS::RowAdjustEnum rowAdjust,::Ice::Long& resultColCnt)
{
	Ice::Long fullColCnt = 0;
	Ice::Long rowCnt = featureIdxTo - featureIdxFrom;

	::IceUtil::ScopedArray<Ice::Double>  retValues(
		GetNormalizedCnts(featureIdxFrom, featureIdxTo, fullColCnt));

	bool no_re_arrange = (sampleIDs == m_RUVInfo.RawCountObserverIDs)
		|| (sampleIDs == m_RUVInfo.SampleIDs);

	bool need_re_arrange = (!no_re_arrange) || (!m_outputSampleIDs.empty());

	Ice::Long colCnt = fullColCnt;
	Ice::Long totalValueCnt = rowCnt*colCnt;
	::IceUtil::ScopedArray<Ice::Double>  rgValues;
	if (need_re_arrange)
	{
		iBS::IntVec colIdxs;
		if (!m_outputSampleIDs.empty())
		{
			GetSampleIdxsBySampleIDs(m_outputSampleIDs, colIdxs);
		}
		else
		{
			GetSampleIdxsBySampleIDs(sampleIDs, colIdxs);
		}

		colCnt = colIdxs.size();
		totalValueCnt = rowCnt*colCnt;
		rgValues.reset(new Ice::Double[totalValueCnt]);
		if (!rgValues.get())
		{
			resultColCnt = 0;
		}

		CRowAdjustHelper::ReArrangeByColIdxs(
			retValues.get(), rowCnt, fullColCnt, colIdxs, rgValues.get());

		CRowAdjustHelper::Adjust(rgValues.get(), rowCnt, colCnt, rowAdjust);
		resultColCnt = colCnt;
		return rgValues.release();
	}
	else
	{
		colCnt = fullColCnt;
		CRowAdjustHelper::Adjust(retValues.get(), rowCnt, colCnt, rowAdjust);
		resultColCnt = fullColCnt;
		return retValues.release();
	}
	
}
::Ice::Double*
CRUVBuilder::GetNormalizedCnts(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Long& resultColCnt)
{
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt = featureIdxTo - featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	::IceUtil::ScopedArray<Ice::Double>  saY(new ::Ice::Double[totalValueCnt]);
	::IceUtil::ScopedArray<Ice::Double>  saRowMeans(new ::Ice::Double[rowCnt]);
	if (!saY.get() || !saRowMeans.get())
	{
		return 0;
	}

	Ice::Double *Y = saY.get();
	Ice::Double *rowMeans = saRowMeans.get();

	if (GetYandRowMeans(featureIdxFrom, featureIdxTo, rowMeans, Y) == false)
	{
		return 0;
	}

	/*if (m_RUVInfo.RUVMode == iBS::RUVModeRUVsForVariation
		|| m_RUVInfo.RUVMode == iBS::RUVModeRUVgForVariation)
	{
		CRowAdjustHelper::AdjustByZeroMeanUnitSD(Y, rowCnt, colCnt);
		for (Ice::Long i = 0; i < rowCnt; i++)
		{
			rowMeans[i] = 0;
		}
	}*/

	resultColCnt = (::Ice::Long)m_RUVInfo.RawCountObserverIDs.size();

	Ice::Long ramMb = 16;
	ramMb /= sizeof(Ice::Double);
	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;

	if (totalValueCnt > batchValueCnt && m_outputWorkerNum>1 && (m_activeK + m_extentW)>0)
	{
		Multithread_GetNormalizedCnts(rowCnt, Y, rowMeans);
	}
	else
	{
		GetNormalizedCnts(rowCnt, Y, rowMeans);
	}
	

	if (m_outputScale == iBS::RUVOutputScaleRaw)
	{
		CRowAdjustHelper::ConvertFromLogToRawCnt(Y, rowCnt, resultColCnt);
	}

	return saY.release();

}


void CRUVBuilder::Multithread_GetNormalizedCnts(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans)
{
	
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long ramMb = 16;
	ramMb /= sizeof(Ice::Double);
	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt_min = batchValueCnt / colCnt;

	::Ice::Long batchRowCnt = rowCnt / m_outputWorkerNum;
	if (batchRowCnt < batchRowCnt_min)
	{
		batchRowCnt = batchRowCnt_min;
	}
	
	::Ice::Long remainCnt = rowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = 0;
	::Ice::Long featureIdxTo = 0;
	//::Ice::Long batchCnt = remainCnt / batchRowCnt + 1;

	bool bContinue = true;
	bool bNeedExit = false;
	m_needNotify = false;

	int batchIdx = 0;
	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();
	int threadCnt = m_outputWorkerNum;
	for (int i = 0; i<threadCnt; i++)
	{
		m_freeWorkerIdxs.push_back(i);
	}

	while (!bNeedExit)
	{

		{
			//entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while (!m_shutdownRequested)
			{
				//merge freeworker that has not been lauched due to remainCnt==0
				if (!m_processingWorkerIdxs.empty())
				{
					std::copy(m_processingWorkerIdxs.begin(), m_processingWorkerIdxs.end(),
						std::back_inserter(m_freeWorkerIdxs));
					m_processingWorkerIdxs.clear();
				}

				//have to wait if 1: remain count>0, but no free worker; 
				//or 2. remain count==0, worker not finished yet
				if ((remainCnt>0 && m_freeWorkerIdxs.empty())
					|| (remainCnt == 0 && (int)m_freeWorkerIdxs.size()<threadCnt))
				{
					m_needNotify = true;
					m_monitor.wait();
				}

				//free worker need to finish left items
				if (remainCnt>0 && !m_freeWorkerIdxs.empty())
				{
					std::copy(m_freeWorkerIdxs.begin(), m_freeWorkerIdxs.end(),
						std::back_inserter(m_processingWorkerIdxs));
					m_freeWorkerIdxs.clear();
					break;
				}
				else if (remainCnt == 0 && (int)m_freeWorkerIdxs.size() == threadCnt)
				{
					bNeedExit = true;
					break;

				}
			}

			//if control request shutdown
			if (m_shutdownRequested)
			{
				bContinue = false;
				bNeedExit = true;
				cout << "CRUVBuilder m_shutdownRequested==true ..." << endl;
			}

			//leaving critical region
			m_needNotify = false;
		}

		//these workers are free, need to use them to process
		while (!m_processingWorkerIdxs.empty() && remainCnt>0)
		{
			int workerIdx = m_processingWorkerIdxs.front();

			if (remainCnt>batchRowCnt){
				thisBatchRowCnt = batchRowCnt;
			}
			else{
				thisBatchRowCnt = remainCnt;
			}
			batchIdx++;

			//cout << IceUtil::Time::now().toDateTime() << " Multithread_GetNormalizedCnts batch " << batchIdx << "/" << batchCnt << " begin" << endl;

			featureIdxTo = featureIdxFrom + thisBatchRowCnt;

			Ice::Double *batchY = Y + featureIdxFrom*colCnt;
			Ice::Double *batchRowMeans = rowMeans + featureIdxFrom;

			RUVOutputWorkerPtr worker = m_outputWorkerMgr.GetWorker(workerIdx);
			RUVsWorkItemPtr wi = new CRUVGetOutput(
				*this, workerIdx, thisBatchRowCnt, batchY, batchRowMeans);

			worker->AddWorkItem(wi);

			m_processingWorkerIdxs.pop_front();
			featureIdxFrom += thisBatchRowCnt;
			remainCnt -= thisBatchRowCnt;
		}

	}


	//cout << IceUtil::Time::now().toDateTime() << " Multithread_GetNormalizedCnts batch " << batchIdx << "/" << batchCnt << " end" << endl;

	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();
}

void CRUVBuilder::GetNormalizedCnts(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans)
{
	if(m_outputMode==iBS::RUVOutputModeYminusTYcs)
	{
		GetNormalizedCnts_ModeT(rowCnt, Y, rowMeans);
	}
	else if (m_outputMode == iBS::RUVOutputModeYminusZY)
	{
		GetNormalizedCnts_ModeYminusZY(rowCnt, Y, rowMeans);
	}
	else if (m_outputMode == iBS::RUVOutputModeZY)
	{
		GetNormalizedCnts_ModeZY(rowCnt, Y, rowMeans);
	}
	else if (m_outputMode == iBS::RUVOutputModeZYthenGroupMean)
	{
		GetNormalizedCnts_ModeZYthenGroupMean(rowCnt, Y, rowMeans);
	}
	else if (m_outputMode == iBS::RUVOutputModeYminusWa)
	{
		GetNormalizedCnts_ModeG(rowCnt, Y, rowMeans);
	}
	else if (m_outputMode == iBS::RUVOutputModeXb)
	{
		GetNormalizedCnts_ModeBeta(rowCnt, Y, rowMeans);
	}
	else if (m_outputMode == iBS::RUVOutputModeGroupMean)
	{
		GetNormalizedCnts_GroupMean(rowCnt, Y, rowMeans);
	}
	else if (m_outputMode == RUVOutputModeWa)
	{
		GetNormalizedCnts_ModeWa(rowCnt, Y, rowMeans);
	}
	else if (m_outputMode == RUVOutputModeZYGetE)
	{
		GetNormalizedCnts_ModeZYGetE(rowCnt, Y, rowMeans);
	}
	else if (m_outputMode == RUVOutputModeYminusWaXb)
	{
		GetNormalizedCnts_YminusWaXb(rowCnt, Y, rowMeans);
	}
	else
	{
		GetNormalizedCnts_ModeG(rowCnt, Y, rowMeans);
	}

}

::Ice::Double* 
CRUVBuilder::SampleNormalizedCnts(const iBS::LongVec& featureIdxs,
const iBS::IntVec& sampleIDs, iBS::RowAdjustEnum rowAdjust, ::Ice::Long& resultColCnt)
{
	Ice::Long fullColCnt = 0;
	Ice::Long rowCnt = (Ice::Long)featureIdxs.size();

	::IceUtil::ScopedArray<Ice::Double>  retValues(
		SampleNormalizedCnts(featureIdxs, fullColCnt));

	bool no_re_arrange = (sampleIDs == m_RUVInfo.RawCountObserverIDs)
		|| (sampleIDs == m_RUVInfo.SampleIDs);

	bool need_re_arrange = (!no_re_arrange) || (!m_outputSampleIDs.empty());

	Ice::Long colCnt = fullColCnt;
	Ice::Long totalValueCnt = rowCnt*colCnt;
	::IceUtil::ScopedArray<Ice::Double>  rgValues;
	if (need_re_arrange)
	{
		iBS::IntVec colIdxs;
		if (!m_outputSampleIDs.empty())
		{
			GetSampleIdxsBySampleIDs(m_outputSampleIDs, colIdxs);
		}
		else
		{
			GetSampleIdxsBySampleIDs(sampleIDs, colIdxs);
		}

		colCnt = colIdxs.size();
		totalValueCnt = rowCnt*colCnt;
		rgValues.reset(new Ice::Double[totalValueCnt]);
		if (!rgValues.get())
		{
			resultColCnt = 0;
		}

		CRowAdjustHelper::ReArrangeByColIdxs(
			retValues.get(), rowCnt, fullColCnt, colIdxs, rgValues.get());

		CRowAdjustHelper::Adjust(rgValues.get(), rowCnt, colCnt, rowAdjust);
		resultColCnt = colCnt;
		return rgValues.release();
	}
	else
	{
		colCnt = fullColCnt;
		CRowAdjustHelper::Adjust(retValues.get(), rowCnt, colCnt, rowAdjust);
		resultColCnt = fullColCnt;
		return retValues.release();
	}
}

::Ice::Double*
CRUVBuilder::SampleNormalizedCnts(const iBS::LongVec& featureIdxs, ::Ice::Long& resultColCnt)
{
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt = featureIdxs.size();
	Ice::Long totalValueCnt = rowCnt*colCnt;

	::IceUtil::ScopedArray<Ice::Double>  saY(new ::Ice::Double[totalValueCnt]);
	::IceUtil::ScopedArray<Ice::Double>  saRowMeans(new ::Ice::Double[rowCnt]);
	if (!saY.get() || !saRowMeans.get())
	{
		return 0;
	}

	if (SampleYandRowMeans(featureIdxs, saRowMeans.get(), saY.get()) == false)
	{
		return 0;
	}

	Ice::Double *Y = saY.get();
	Ice::Double *rowMeans = saRowMeans.get();
	resultColCnt = (::Ice::Long)m_RUVInfo.RawCountObserverIDs.size();

	Ice::Long ramMb = 16;
	ramMb /= sizeof(Ice::Double);
	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;

	if (totalValueCnt > batchValueCnt && m_outputWorkerNum>1 && (m_activeK + m_extentW)>0)
	{
		Multithread_GetNormalizedCnts(rowCnt, Y, rowMeans);
	}
	else
	{
		GetNormalizedCnts(rowCnt, Y, rowMeans);
	}


	if (m_outputScale == iBS::RUVOutputScaleRaw)
	{
		CRowAdjustHelper::ConvertFromLogToRawCnt(Y, rowCnt, resultColCnt);
	}

	return saY.release();

}

void CRUVBuilder::GetNormalizedCnts_ModeT(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const
{
	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();
	if(!m_activeT.get())
	{
		CRowAdjustHelper::AddRowMeansBackToY(rowCnt, colCnt, Y, rowMeans);
		return;
	}

	int n=(int)m_RUVInfo.n;
	int m=m_RUVInfo.CtrlSampleCnt;
	Ice::Long F=rowCnt;
	
	Ice::Long conditionCnt=m_RUVInfo.P;
	iBS::DoubleVec conditionSums(conditionCnt,0);
	iBS::DoubleVec centeredY(n,0);
	iBS::DoubleVec unwantedY(n,0);

	for(Ice::Long f=0;f<F;f++)
	{
		for(int i=0;i<conditionCnt;i++)
		{
			conditionSums[i]=0.0;
		}

		for(int i=0;i<n;i++)
		{
			int sampleIdx=i;
			int conditionIdx=m_sampleIdx2ConditionIdx[sampleIdx];
			Ice::Long colIdx=i;
			Ice::Long rowIdx=f;
			Ice::Double y=Y[rowIdx*colCnt+colIdx];
			conditionSums[conditionIdx]+=y;
			centeredY[sampleIdx]=y;
		}

		//get centered Y
		for(int i=0;i<n;i++)
		{
			int sampleIdx=i;
			int conditionIdx=m_sampleIdx2ConditionIdx[sampleIdx];
			double rcnt = (double) m_RUVInfo.ConditionObserverIDs[conditionIdx].size();
			if(rcnt>1)
			{
				// y(i) in centeredY[sampleIdx]
				centeredY[sampleIdx]=(rcnt*centeredY[sampleIdx]-conditionSums[conditionIdx])/(rcnt-1);
			}
			else
			{
				centeredY[sampleIdx]=0;
			}
		}

		Ice::Double* originalY=Y+(f*colCnt);

		Ice::Double* T = m_activeT.get();

		for(int i=0;i<n;i++)
		{
			unwantedY[i]=0;
			for(int j=0;j<m;j++)
			{
				unwantedY[i]+=T[i*m+j]*centeredY[j];
			}

			originalY[i]-=unwantedY[i];
			originalY[i]+=rowMeans[f];
			
		}
		
	}
}


void CRUVBuilder::GetNormalizedCnts_ModeYminusZY(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const
{
	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();
	if (!m_activeZ.get())
	{
		CRowAdjustHelper::AddRowMeansBackToY(rowCnt, colCnt, Y, rowMeans);
		return;
	}

	int n=(int)m_RUVInfo.n;
	Ice::Long F=rowCnt;

	iBS::DoubleVec unwantedY(n,0);
	iBS::DoubleVec resultedY(n,0);

	for(Ice::Long f=0;f<F;f++)
	{
		Ice::Double *originalY=Y+(f*colCnt);

		Ice::Double *Z = m_activeZ.get();

		for(int i=0;i<n;i++)
		{
			unwantedY[i]=0;
			for(int j=0;j<n;j++)
			{
				unwantedY[i]+=Z[i*n+j]*originalY[j];
			}

			resultedY[i]=originalY[i]-unwantedY[i]+rowMeans[f];
		}

		for(int i=0;i<n;i++)
		{
			originalY[i]=resultedY[i];
		}
		
	}

}

void CRUVBuilder::GetNormalizedCnts_ModeZY(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const
{
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();
	if (!m_activeZ.get())
	{
		CRowAdjustHelper::AddRowMeansBackToY(rowCnt, colCnt, Y, rowMeans);
		return;
	}

	int n = (int)m_RUVInfo.n;
	Ice::Long F = rowCnt;
	iBS::DoubleVec unwantedY(n, 0);
	for (Ice::Long f = 0; f<F; f++)
	{
		Ice::Double *originalY = Y + (f*colCnt);

		Ice::Double *Z = m_activeZ.get();

		for (int i = 0; i<n; i++)
		{
			unwantedY[i] = 0;
			for (int j = 0; j<n; j++)
			{
				unwantedY[i] += Z[i*n + j] * originalY[j];
			}
		}

		for (int i = 0; i<n; i++)
		{
			originalY[i] = unwantedY[i];
		}
	}
}


void
CRUVBuilder::GetNormalizedCnts_ModeG(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const
{
	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();

	if(!m_activeAG_0W.get())
	{
		CRowAdjustHelper::AddRowMeansBackToY(rowCnt, colCnt, Y, rowMeans);
		return;
	}

	
	int n=(int)m_RUVInfo.n;
	Ice::Long F=rowCnt;
	iBS::DoubleVec unwantedY(n,0);
	iBS::DoubleVec resultedY(n,0);
	
	Ice::Double *AG = m_activeAG_0W.get(); //n by n

	for(Ice::Long f=0;f<F;f++)
	{
		Ice::Double *originalY=Y+(f*colCnt);

		for(int i=0;i<n;i++)
		{
			unwantedY[i]=0;
			for(int j=0;j<n;j++)
			{
				unwantedY[i]+=AG[i*n+j]*originalY[j];
			}

			resultedY[i]=originalY[i]-unwantedY[i]+rowMeans[f];
		}

		for(int i=0;i<n;i++)
		{
			originalY[i]=resultedY[i];
		}
		
	}
}

void
CRUVBuilder::GetNormalizedCnts_ModeWa(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const
{
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();

	if (!m_activeAG_0W.get())
	{
		CRowAdjustHelper::AddRowMeansBackToY(rowCnt, colCnt, Y, rowMeans);
		return;
	}

	int n = (int)m_RUVInfo.n;
	Ice::Long F = rowCnt;
	iBS::DoubleVec unwantedY(n, 0);

	Ice::Double *AG = m_activeAG_0W.get(); //n by n

	for (Ice::Long f = 0; f<F; f++)
	{
		Ice::Double *originalY = Y + (f*colCnt);

		for (int i = 0; i<n; i++)
		{
			unwantedY[i] = 0;
			for (int j = 0; j<n; j++)
			{
				unwantedY[i] += AG[i*n + j] * originalY[j];
			}
		}

		for (int i = 0; i<n; i++)
		{
			originalY[i] = unwantedY[i];
		}

	}
}

::Ice::Int CRUVBuilder::GetG(::iBS::DoubleVec& values)
{
	Ice::Long K = m_activeK + m_extentW;
	Ice::Long rowCnt = m_RUVInfo.P + K;
	Ice::Long colCnt = m_RUVInfo.n;

	if (!m_activeG.get())
	{
		return 0;
	}

	values.resize(rowCnt*colCnt);

	Ice::Double *G = m_activeG.get(); //n by n

	for (int i = 0; i < rowCnt; i++)
	{
		for (int j = 0; j < colCnt; j++)
		{
			values[i*colCnt + j] = G[i*colCnt + j];
		}
	}
	return 1;
}

::Ice::Int CRUVBuilder::GetWt(::iBS::DoubleVec& values)
{
	Ice::Long K = m_activeK + m_extentW;
	Ice::Long rowCnt =K;
	Ice::Long colCnt = m_RUVInfo.n;

	if (!m_activeWt.get())
	{
		return 0;
	}

	values.resize(rowCnt*colCnt);

	Ice::Double *Wt = m_activeWt.get(); //K by n

	for (int i = 0; i < rowCnt; i++)
	{
		for (int j = 0; j < colCnt; j++)
		{
			values[i*colCnt + j] = Wt[i*colCnt + j];
		}
	}
	return 1;
}

void CRUVBuilder::GetNormalizedCnts_ModeBeta(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const
{
	Ice::Long colCnt= m_RUVInfo.RawCountObserverIDs.size();

	if (!m_activeG.get())
	{
		GetNormalizedCnts_GroupMean(rowCnt, Y, rowMeans);
		return;
	}
	
	int n=(int)m_RUVInfo.n;
	int p=(int)m_RUVInfo.P;

	Ice::Long F=rowCnt;
	iBS::DoubleVec beta(p,0);
	
	Ice::Double *G = m_activeG.get(); //n by n

	for(Ice::Long f=0;f<F;f++)
	{
		Ice::Double *originalY=Y+(f*colCnt);

		for(int i=0;i<p;i++)
		{
			beta[i]=0;
			for(int j=0;j<n;j++)
			{
				beta[i]+=G[i*n+j]*originalY[j];
			}
		}

		for(int i=0;i<n;i++)
		{
			int cidx=m_sampleIdx2ConditionIdx[i];
			originalY[i]=beta[cidx]+rowMeans[f];
		}
		
	}
}



void CRUVBuilder::GetNormalizedCnts_ModeZYthenGroupMean(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const
{
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();
	if (!m_activeZ.get())
	{
		GetNormalizedCnts_GroupMean(rowCnt, Y, rowMeans);
		return;
	}

	int n = (int)m_RUVInfo.n;
	Ice::Long F = rowCnt;
	Ice::Long conditionCnt = m_RUVInfo.P;
	iBS::DoubleVec conditionSums(conditionCnt, 0);
	iBS::DoubleVec conditionSampleCnts(conditionCnt, 0);

	iBS::DoubleVec unwantedY(n, 0);
	for (Ice::Long f = 0; f<F; f++)
	{
		Ice::Double *originalY = Y + (f*colCnt);

		Ice::Double *Z = m_activeZ.get();

		for (int i = 0; i < conditionCnt; i++)
		{
			conditionSums[i] = 0.0;
			if (f == 0)
			{
				conditionSampleCnts[i] = (Ice::Double)m_RUVInfo.ConditionObserverIDs[i].size();
			}
		}

		for (int i = 0; i<n; i++)
		{
			unwantedY[i] = 0;
			for (int j = 0; j<n; j++)
			{
				unwantedY[i] += Z[i*n + j] * originalY[j];
			}
			int conditionIdx = m_sampleIdx2ConditionIdx[i];
			conditionSums[conditionIdx] += (originalY[i] - unwantedY[i]);
		}

		for (int i = 0; i < n; i++)
		{
			int conditionIdx = m_sampleIdx2ConditionIdx[i];
			originalY[i] = conditionSums[conditionIdx] / conditionSampleCnts[conditionIdx] + rowMeans[f];
		}

	}

}

void CRUVBuilder::GetNormalizedCnts_ModeZYGetE(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const
{
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();
	if (!m_activeZ.get())
	{
		GetNormalizedCnts_GroupMean(rowCnt, Y, rowMeans);
		return;
	}

	int n = (int)m_RUVInfo.n;
	Ice::Long F = rowCnt;
	Ice::Long conditionCnt = m_RUVInfo.P;
	iBS::DoubleVec conditionSums(conditionCnt, 0);
	iBS::DoubleVec conditionSampleCnts(conditionCnt, 0);

	iBS::DoubleVec unwantedY(n, 0);
	for (Ice::Long f = 0; f<F; f++)
	{
		Ice::Double *originalY = Y + (f*colCnt);

		Ice::Double *Z = m_activeZ.get();

		for (int i = 0; i < conditionCnt; i++)
		{
			conditionSums[i] = 0.0;
			if (f == 0)
			{
				conditionSampleCnts[i] = (Ice::Double)m_RUVInfo.ConditionObserverIDs[i].size();
			}
		}

		for (int i = 0; i<n; i++)
		{
			unwantedY[i] = 0;
			for (int j = 0; j<n; j++)
			{
				unwantedY[i] += Z[i*n + j] * originalY[j];
			}
			int conditionIdx = m_sampleIdx2ConditionIdx[i];
			conditionSums[conditionIdx] += (originalY[i] - unwantedY[i]);
		}

		for (int i = 0; i < n; i++)
		{
			int conditionIdx = m_sampleIdx2ConditionIdx[i];
			originalY[i] = (originalY[i] - unwantedY[i]) - conditionSums[conditionIdx] / conditionSampleCnts[conditionIdx];
		}

	}
}

void CRUVBuilder::GetNormalizedCnts_YminusWaXb(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const
{
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();

	if (!m_activeAG_0W.get())
	{
		CRowAdjustHelper::AddRowMeansBackToY(rowCnt, colCnt, Y, rowMeans);
		return;
	}


	int n = (int)m_RUVInfo.n;
	Ice::Long F = rowCnt;
	iBS::DoubleVec unwantedY(n, 0);
	iBS::DoubleVec resultedY(n, 0);

	Ice::Double *AG_0W = m_activeAG_0W.get(); //n by n
	Ice::Double *AG_X0 = m_activeAG_X0.get(); //n by n

	for (Ice::Long f = 0; f<F; f++)
	{
		Ice::Double *originalY = Y + (f*colCnt);

		for (int i = 0; i<n; i++)
		{
			unwantedY[i] = 0;
			for (int j = 0; j<n; j++)
			{
				unwantedY[i] += AG_0W[i*n + j] * originalY[j];
				unwantedY[i] += AG_X0[i*n + j] * originalY[j];
			}

			resultedY[i] = originalY[i] - unwantedY[i];
		}

		for (int i = 0; i<n; i++)
		{
			originalY[i] = resultedY[i];
		}

	}
}

void
CRUVBuilder::GetNormalizedCnts_GroupMean(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const
{
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long conditionCnt = m_RUVInfo.P;
	iBS::DoubleVec conditionSums(conditionCnt, 0);
	iBS::DoubleVec conditionSampleCnts(conditionCnt, 0);
	iBS::IntVec colFlags(colCnt, 1);
	bool needHandleColFlags = !m_excludeSampleIDsForGroupMean.empty();

	for (int i = 0; i < conditionCnt; i++)
	{
		conditionSampleCnts[i] = (Ice::Double)m_conditionSampleIdxs[i].size();
	}

	if (needHandleColFlags)
	{
		iBS::IntVec excludedColIdxs;
		GetSampleIdxsBySampleIDs(m_excludeSampleIDsForGroupMean, excludedColIdxs);
		for (int i = 0; i < excludedColIdxs.size(); i++)
		{
			int sampleIdx = excludedColIdxs[i];
			colFlags[sampleIdx] = 0;
			int conditionIdx = m_sampleIdx2ConditionIdx[sampleIdx];
			conditionSampleCnts[conditionIdx]--;
		}
	}

	for (Ice::Long f = 0; f < rowCnt; f++)
	{
		for (int i = 0; i < conditionCnt; i++)
		{
			conditionSums[i] = 0.0;
		}

		for (int i = 0; i < colCnt; i++)
		{
			int sampleIdx = i;
			if (colFlags[i]==0)
			{
				continue;
			}
			int conditionIdx = m_sampleIdx2ConditionIdx[sampleIdx];
			Ice::Long colIdx = i;
			Ice::Long rowIdx = f;
			Ice::Double y = Y[rowIdx*colCnt + colIdx];
			conditionSums[conditionIdx] += y;
		}

		for (int i = 0; i < colCnt; i++)
		{
			int sampleIdx = i;
			int conditionIdx = m_sampleIdx2ConditionIdx[sampleIdx];
			Ice::Long colIdx = i;
			Ice::Long rowIdx = f;
			if (colFlags[i] == 0)
			{
				Y[rowIdx*colCnt + colIdx] = 0;
				continue;
			}
			Y[rowIdx*colCnt + colIdx] = conditionSums[conditionIdx] / conditionSampleCnts[conditionIdx] + rowMeans[f];
		}
	}
}

bool CRUVBuilder::calculateGrandMeanY()
{
	::Ice::Long ramMb = 512;
	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	::IceUtil::ScopedArray<Ice::Double>  saY(new ::Ice::Double[batchValueCnt]);
	if (!saY.get()){
		return false;
	}

	::IceUtil::ScopedArray<Ice::Double>  saRowMeans(new ::Ice::Double[batchRowCnt]);
	if (!saRowMeans.get()){
		return false;
	}

	Ice::Long J = m_RUVInfo.J;
	if (m_VDfeatureIdxFrom > 0 || m_VDfeatureIdxTo > 0)
	{
		J = m_VDfeatureIdxTo - m_VDfeatureIdxFrom;
	}

	::Ice::Long batchCnt = J / batchRowCnt + 1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(
		m_toplevelDVTaskID, "calculateGrandMean", batchCnt);

	int batchIdx = 0;
	::Ice::Long remainCnt = J;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_VDfeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	Ice::Double totalSum = 0.0;
	Ice::Double totalCnt = 0;

	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		std::cout << IceUtil::Time::now().toDateTime() << " CalcualteGrandMeanY batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		GetYandRowMeans(featureIdxFrom, featureIdxTo, saRowMeans.get(), saY.get());

		::Ice::Long cidx = 0;
		for (Ice::Long i = 0; i < thisBatchRowCnt; i++)
		{
			for (Ice::Long j = 0; j < colCnt; j++)
			{
				cidx = i*colCnt + j;

				totalSum += (saY[cidx] + saRowMeans[i]);
				totalCnt++;
			}
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;

		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_toplevelDVTaskID, 1);
	}
	m_grandMeanY = totalSum / totalCnt;
	std::cout << IceUtil::Time::now().toDateTime() << " CalcualteGrandMeanY m_grandMeanY= " << m_grandMeanY << endl;

	return true;
}

bool CRUVBuilder::DecomposeVariance(iBS::RUVVarDecomposeInfoVec& vds, 
	::Ice::Int  threadCnt, ::Ice::Long ramMb, const ::std::string& outfile, ::Ice::Long taskID)
{
	m_toplevelDVTaskID = taskID;
	m_VDfeatureIdxFrom = vds[0].featureIdxFrom;
	m_VDfeatureIdxTo = vds[0].featureIdxTo;

	if (true)
	{
		// mean for rows in [m_VDfeatureIdxFrom,m_VDfeatureIdxTo]
		calculateGrandMeanY();
	}
	int K = (int) vds.size();
	Ice::Long n = (Ice::Long)m_RUVInfo.RawCountObserverIDs.size();

	Ice::Long ksAGTotalValCnt = K*n*n;
	m_kIdx_AG_0W.reset(new ::Ice::Double[ksAGTotalValCnt]);
	m_kIdx_AG_X0.reset(new ::Ice::Double[ksAGTotalValCnt]);
	if (!m_kIdx_AG_0W.get() || !m_kIdx_AG_X0.get())
	{
		return false;
	}
	m_kIdx_xbGrandMean.clear();
	m_kIdx_xbGrandMean.resize(K, 0);
	m_kIdx_extW.clear();
	m_kIdx_extW.resize(K, 0);
	for (int i = 0; i < K; i++)
	{
		m_kIdx_extW[i] = vds[i].extW;
		SetWtVectorIdxs(vds[i].wtVecIdxs);

		if (vds[i].k == 0 && vds[i].extW == 0)
		{
			SetTemp_AG_byK0extW0();
		}
		else
		{

			SetActiveK(vds[i].k, vds[i].extW);

			if (m_outputMode == iBS::RUVOutputModeYminusZY)
			{
				SetTemp_AG_byZ();
			}
		}

		Ice::Long copyCnt = n*n;
		Ice::Long koffSet = i*copyCnt;

		Ice::Double *AG_0W = m_kIdx_AG_0W.get() + koffSet;
		Ice::Double *AG_X0 = m_kIdx_AG_X0.get() + koffSet;

		std::copy(m_activeAG_0W.get(), m_activeAG_0W.get() + copyCnt, AG_0W);
		std::copy(m_activeAG_X0.get(), m_activeAG_X0.get() + copyCnt, AG_X0);
	}

	DecomposeVariance_ModeG(RUVVarDecomposeStageAfterRUVGrandMean,vds, threadCnt, ramMb);
	DecomposeVariance_ModeG(RUVVarDecomposeStageToplevelAndLocus, vds, threadCnt, ramMb);
	m_kIdx_AG_0W.reset();
	m_kIdx_AG_X0.reset();

	std::ofstream ofs(outfile.c_str(), std::ofstream::out);
	if (true)
	{
		//output header
		ofs << "k" << '\t'
			<< "extW" << '\t'
			<< "GrandMean" << '\t'
			<< "SS_t" << '\t'
			<< "SS_b" << '\t'
			<< "SS_u" << '\t'
			<< "SS_bu" << '\t'
			<< "SS_r" << '\t'
			<< "b_GrandMean" << '\t'
			<< "b_SS_t" << '\t'
			<< "b_SS_bl" << '\t'
			<< "b_SS_wl" << endl;
	}
	for (int i = 0; i < K; i++)
	{
		ofs << vds[i].k << '\t'
			<< vds[i].extW << '\t'
			<< vds[i].grandMean << '\t'
			<< vds[i].totalVar << '\t'
			<< vds[i].Xb << '\t'
			<< vds[i].Wa << '\t'
			<< vds[i].XbWa << '\t'
			<< vds[i].e << '\t'
			<< vds[i].xbGrandMean << '\t'
			<< vds[i].xbTotalVar << '\t'
			<< vds[i].xbLocusBgVar << '\t'
			<< vds[i].xbLocusWgVar << endl;
	}
	ofs.close();

	CGlobalVars::get()->theFeatureValueWorkerMgr->SetAMDTaskDone(m_toplevelDVTaskID);
	m_toplevelDVTaskID = 0;
	return true;
}

bool CRUVBuilder::DecomposeVariance_ModeG(RUVVarDecomposeStageEnum stage, iBS::RUVVarDecomposeInfoVec& vds, ::Ice::Int  threadCnt, ::Ice::Long ramMb)
{
	int K = (int)vds.size();
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();

	Ice::Long J = m_RUVInfo.J;
	if (m_VDfeatureIdxFrom > 0 || m_VDfeatureIdxTo > 0)
	{
		J = m_VDfeatureIdxTo - m_VDfeatureIdxFrom;
	}

	Ice::Long totalRam = J*colCnt*sizeof(Ice::Double);
	totalRam /= (1024 * 1024);
	if (totalRam < ramMb)
	{
		ramMb = totalRam + 1;
	}
	cout << IceUtil::Time::now().toDateTime() << " ramMB " << ramMb << " thread " << threadCnt << endl;

	::Ice::Long batchValueCnt = (1024 * 1024 * ramMb) / (sizeof(Ice::Double)*threadCnt);
	if (batchValueCnt < colCnt)
	{
		batchValueCnt = colCnt;
	}

	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	CRUVVarDecmWorkerMgr workerMgr(threadCnt);
	//each worker will allocate Y with size of batchValueCnt 
	if (workerMgr.Initilize(*this, batchRowCnt, batchValueCnt, K) == false)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
		return false;
	}

	::Ice::Long remainCnt = J;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_VDfeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;
	::Ice::Long batchCnt = remainCnt / batchRowCnt + 1;

	std::string subTaskName = "DecomposeVariance";
	if (stage == RUVVarDecomposeStageAfterRUVGrandMean)
	{
		subTaskName = "DecomposeVariance_XbGrandMean";
	}
	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(
		m_toplevelDVTaskID, subTaskName, batchCnt);
	
	bool bContinue = true;
	bool bNeedExit = false;
	m_needNotify = false;

	int batchIdx = 0;
	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();

	for (int i = 0; i<threadCnt; i++)
	{
		m_freeWorkerIdxs.push_back(i);
	}

	while (!bNeedExit)
	{

		{
			//entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while (!m_shutdownRequested)
			{
				//merge freeworker that has not been lauched due to remainCnt==0
				if (!m_processingWorkerIdxs.empty())
				{
					std::copy(m_processingWorkerIdxs.begin(), m_processingWorkerIdxs.end(),
						std::back_inserter(m_freeWorkerIdxs));
					m_processingWorkerIdxs.clear();
				}

				//have to wait if 1: remain count>0, but no free worker; 
				//or 2. remain count==0, worker not finished yet
				if ((remainCnt>0 && m_freeWorkerIdxs.empty())
					|| (remainCnt == 0 && (int)m_freeWorkerIdxs.size()<threadCnt))
				{
					m_needNotify = true;
					m_monitor.wait();
				}

				//free worker need to finish left items
				if (remainCnt>0 && !m_freeWorkerIdxs.empty())
				{
					std::copy(m_freeWorkerIdxs.begin(), m_freeWorkerIdxs.end(),
						std::back_inserter(m_processingWorkerIdxs));
					m_freeWorkerIdxs.clear();
					break;
				}
				else if (remainCnt == 0 && (int)m_freeWorkerIdxs.size() == threadCnt)
				{
					bNeedExit = true;
					break;

				}
			}

			//if control request shutdown
			if (m_shutdownRequested)
			{
				bContinue = false;
				bNeedExit = true;
				cout << "CRUVBuilder m_shutdownRequested==true ..." << endl;
			}

			//leaving critical region
			m_needNotify = false;
		}

		//these workers are free, need to use them to process
		while (!m_processingWorkerIdxs.empty() && remainCnt>0)
		{
			int workerIdx = m_processingWorkerIdxs.front();

			if (remainCnt>batchRowCnt){
				thisBatchRowCnt = batchRowCnt;
			}
			else{
				thisBatchRowCnt = remainCnt;
			}
			batchIdx++;

			cout << IceUtil::Time::now().toDateTime() << " "<< subTaskName<<" batch " << batchIdx << "/" << batchCnt << " begin" << endl;

			featureIdxTo = featureIdxFrom + thisBatchRowCnt;

			RUVVarDecmWorkerPtr worker = workerMgr.GetWorker(workerIdx);
			if (GetYandRowMeans(featureIdxFrom, featureIdxTo, worker->GetBatchRowMeans(), worker->GetBatchY()) == false)
			{
				return false;
			}

			RUVsWorkItemPtr wi = new CRUVVarDecompose(
				*this, worker.get(), workerIdx, stage, featureIdxFrom, featureIdxTo,
				worker->GetBatchY(), worker->GetBatchRowMeans());

			worker->AddWorkItem(wi);

			m_processingWorkerIdxs.pop_front();
			featureIdxFrom += thisBatchRowCnt;
			remainCnt -= thisBatchRowCnt;

			CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_toplevelDVTaskID, 1);
		}

	}

	if (!bContinue)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
		return false;
	}

	cout << IceUtil::Time::now().toDateTime() << " DecomposeVariance_ModeG batch " << batchIdx << "/" << batchCnt << " end" << endl;

	for (int k = 0; k < K; k++)
	{
		Ice::Double ssGrandMean = 0;
		Ice::Double ssXb = 0;
		Ice::Double ssWa = 0;
		Ice::Double ssXbWa = 0;

		Ice::Double xbGrandMean=0;
		Ice::Double xbTotalVar=0;
		Ice::Double xbLocusBgVar=0;
		Ice::Double xbLocusWgVar=0;

		for (int i = 0; i<threadCnt; i++)
		{
			RUVVarDecmWorkerPtr worker = workerMgr.GetWorker(i);
			ssGrandMean += worker->m_ssGrandMean[k];
			ssXb += worker->m_ssXb[k];
			ssWa += worker->m_ssWa[k];
			ssXbWa += worker->m_ssXbWa[k];

			xbGrandMean += worker->m_xbGrandMean[k];
			cout << " k=" << k << " worker " << i << " xbGrandMean " << xbGrandMean << endl;

			xbTotalVar += worker->m_ssXbTotalVar[k];
			xbLocusBgVar += worker->m_ssXbBgVar[k];
			xbLocusWgVar += worker->m_ssXbWgVar[k];
		}
		xbGrandMean /= (J*colCnt);
		iBS::RUVVarDecomposeInfo& vd = vds[k];
		//vd.grandMean = m_grandMeanY;
		//vd.Xb = 1.0 - ssXb / ssGrandMean;
		//vd.Wa = 1.0 - ssWa / ssGrandMean;
		//vd.XbWa = 1.0 - ssXbWa / ssGrandMean;
		//vd.e = 1.0 - vd.XbWa;
		vd.totalVar = ssGrandMean;
		vd.grandMean = m_grandMeanY;
		vd.Xb = ssGrandMean-ssXb;
		vd.Wa = ssGrandMean - ssWa;
		vd.XbWa = ssGrandMean - ssXbWa;
		vd.e = ssXbWa;
		vd.xbGrandMean = xbGrandMean;
		vd.xbTotalVar = xbTotalVar;
		vd.xbLocusBgVar = xbLocusBgVar;
		vd.xbLocusWgVar = xbLocusWgVar;

		if (stage == RUVVarDecomposeStageAfterRUVGrandMean
			&& vd.extW == 0)
		{
			//xbGrandMean is equal to m_grandMeanY
			m_kIdx_xbGrandMean[k] = m_grandMeanY;
		}
		else
		{
			m_kIdx_xbGrandMean[k] = xbGrandMean;
		}

		cout << " k=" << k << "m_kIdx_xbGrandMean[k] " << m_kIdx_xbGrandMean[k]<< endl;
	}

	workerMgr.RequestShutdownAllWorkers();
	workerMgr.UnInitilize();

	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();


	return true;
}

bool CRUVBuilder::DecomposeVariance_ModeG(RUVVarDecomposeBatchParams& params) const
{
	Ice::Long colCnt = m_RUVInfo.RawCountObserverIDs.size();
	Ice::Long rowCnt = params.featureIdxTo - params.featureIdxFrom;
	int K = (int)params.ssGrandMean.size();
	int n = (int)m_RUVInfo.n;
	Ice::Long F = rowCnt;
	iBS::DoubleVec Wa(n, 0);
	iBS::DoubleVec Xb(n, 0);

	Ice::Double *AG_0W = 0; //n by n
	Ice::Double *AG_X0 = 0; //n by n
	Ice::Long AGValCnt = n*n;
	Ice::Long koffSet = 0;
	Ice::Double y = 0;
	Ice::Double xbRowMean = 0;
	Ice::Double dColCnt = (Ice::Double)n;
	for (Ice::Long f = 0; f<F; f++)
	{
		Ice::Double *originalY = params.Y + (f*colCnt);

		for (int k = 0; k < K; k++)
		{
			if (params.stage == RUVVarDecomposeStageAfterRUVGrandMean
				&& m_kIdx_extW[k] == 0)
			{
				//xbGrandMean is equal to m_grandMeanY, no need to compute this k
				continue;
			}
				
			
			koffSet = k*AGValCnt;
			AG_0W = m_kIdx_AG_0W.get() + koffSet; //n by n
			AG_X0 = m_kIdx_AG_X0.get() + koffSet; //n by n

			xbRowMean = 0;
			for (int i = 0; i<n; i++)
			{
				Wa[i] = 0;
				Xb[i] = 0;
				for (int j = 0; j<n; j++)
				{
					Wa[i] += AG_0W[i*n + j] * originalY[j];
					Xb[i] += AG_X0[i*n + j] * originalY[j];
				}

				Xb[i] += params.rowMeans[f]; //add locus mean to biological component

				xbRowMean += Xb[i];	//sum of biological component, to get grand mean
			}
			params.xbGrandMean[k] += xbRowMean;
			xbRowMean /= dColCnt;
			if (params.stage == RUVVarDecomposeStageAfterRUVGrandMean)
				continue;

			params.ssXbBgVar[k] += (dColCnt*(xbRowMean - m_kIdx_xbGrandMean[k])*(xbRowMean - m_kIdx_xbGrandMean[k]));
			for (int i = 0; i<n; i++)
			{
				y = originalY[i] + params.rowMeans[f];
				params.ssGrandMean[k] += (y - m_grandMeanY)*(y - m_grandMeanY);
				params.ssXb[k] += (y - Xb[i])*(y - Xb[i]);
				params.ssWa[k] += (y - Wa[i] - params.rowMeans[f])*(y - Wa[i] - params.rowMeans[f]);
				params.ssXbWa[k] += (y - Xb[i] - Wa[i])*(y - Xb[i] - Wa[i]);

				params.ssXbTotalVar[k] += (m_kIdx_xbGrandMean[k] - Xb[i])*(m_kIdx_xbGrandMean[k] - Xb[i]);
				params.ssXbWgVar[k] += (xbRowMean - Xb[i])*(xbRowMean - Xb[i]);
			}

		}

	}

	return true;
}