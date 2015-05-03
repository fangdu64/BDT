#ifndef __RUVsBuilder_h__
#define __RUVsBuilder_h__

#include <RUVsCommonDefine.h>
#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <FCDCentralService.h>
#include <FeatureValueWorker.h>
#include <FeatureValueWorkItem.h>
#include <armadillo>
#include <RUVOutputWorkerMgr.h>

class CFcdcRUVServiceImpl;
class CIndexPermutation;

class CRUVBuilder
{
	friend class  CFcdcRUVServiceImpl;
	friend class  CRUVsWorker;
	
	friend class  CRUVsComputeABC;
	friend class  CRUVRowANOVAWorker;
	friend class  CRUVComputeRowANOVA;
	friend class  CRUVgComputeA;
	friend class  CRUVgWorker;
	friend class  CRUVVarDecompose;
	friend class  CRUVVarDecmWorker;

	friend class  CRUVGetOutput;
	friend class  CRUVOutputWorker;
	
public:
	CRUVBuilder(const ::iBS::RUVFacetInfo& RUVInfo, 
		const ::iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::ObserverStatsInfoVec& osis);

	virtual ~CRUVBuilder();

public:
	
	const ::iBS::RUVFacetInfo& GetRUVInfo() const { return m_RUVInfo; }
	bool RebuildRUVModel(::Ice::Int  threadCnt, ::Ice::Long ramMb, ::Ice::Long taskID);

	//in read mode, set k as the # of top eigen vectors used in the RUVs model
	bool SetActiveK(int k, int extW);
	::Ice::Int SetNormalizeMode(::iBS::RUVModeEnum nmode);

	::Ice::Int SetOutputScale(::iBS::RUVOutputScaleEnum scale);
	::Ice::Int SetOutputMode(::iBS::RUVOutputModeEnum mode);
	::Ice::Int SetOutputSamples(const ::iBS::IntVec& sampleIDs);
	::Ice::Int SetExcludedSamplesForGroupMean(const ::iBS::IntVec& excludeSampleIDs);
	::Ice::Int SetOutputWorkerNum(::Ice::Int workerNum);
	::Ice::Int SetCtrlQuantileValues(::Ice::Double quantile, const ::iBS::DoubleVec& qvalues, ::Ice::Double fraction);
	Ice::Int GetConditionIdxs(const ::iBS::IntVec& observerIDs,
                                        ::iBS::IntVec& conditionIdxs);

	//SampleIDs (public) RawCountObserverIDs (internal)
	Ice::Int GetSampleIdxsBySampleIDs(const ::iBS::IntVec& sampleIDs,
		::iBS::IntVec& sampleIdxs) const;

	//read all samples' normalized counts in feature range, (featureIdxTo -featureIdxFrom) by sampleCnt matrix
	::Ice::Double* GetNormalizedCnts(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Long& resultColCnt);
	::Ice::Double* GetNormalizedCnts(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo,
		const iBS::IntVec& sampleIDs, iBS::RowAdjustEnum rowAdjust, ::Ice::Long& resultColCnt);

	::Ice::Double* SampleNormalizedCnts(const iBS::LongVec& featureIdxs, ::Ice::Long& resultColCnt);
	::Ice::Double* SampleNormalizedCnts(const iBS::LongVec& featureIdxs,
		const iBS::IntVec& sampleIDs, iBS::RowAdjustEnum rowAdjust, ::Ice::Long& resultColCnt);

	bool DecomposeVariance(iBS::RUVVarDecomposeInfoVec& vds, 
		::Ice::Int  threadCnt, ::Ice::Long ramMb, const ::std::string& outfile, ::Ice::Long taskID);

	const iBS::IntVec& GetOutputSamples() const { return m_outputSampleIDs; }

	::Ice::Int GetG(::iBS::DoubleVec& values);
	::Ice::Int GetWt(::iBS::DoubleVec& values);
	::Ice::Int GetEigenVals(::iBS::DoubleVec& values);

	::Ice::Int SetWtVectorIdxs(const ::iBS::IntVec& vecIdxs);
	::Ice::Int SelectKByEigenVals(::Ice::Double minFraction, ::Ice::Int& k, ::iBS::DoubleVec& fractions);
private:
	//derive constants from fois 
	bool Initialize();
	bool Initialize_PreFilter();
	bool Initialize_AfterFilter();
	bool GetPreFilterRawY(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Double*  RawY) const;
	bool GetRawY(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Double*  RawY) const;

	//get all samples' Y=log(rawcount+1), rowcentered, pY already allocated
	bool GetY(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo,::Ice::Double*  pY) const;

	bool GetYVariation(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Double*  pY) const;

	//get all samples' Y=log(rawcount+1), rowcentered, caller need to delete mem
	::Ice::Double* GetY(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo) const;

	bool GetYandRowMeans(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, Ice::Double *pRowMeans, ::Ice::Double* pY) const;
	bool SampleYandRowMeans(iBS::LongVec featureIdxs, Ice::Double *pRowMeans, ::Ice::Double* pY) const;

	//update YcsYcs' for Y in a given region
	bool UpdateYcsYcsT(::Ice::Double* Y, Ice::Long featureIdxFrom, Ice::Long featureIdxTo, arma::mat& A,
		CIndexPermutation& colIdxPermuttion, std::vector<::arma::mat>& As);

	bool UpdateYcscfYcscfT(::Ice::Double* Y, Ice::Long featureIdxFrom, Ice::Long featureIdxTo, 
		const iBS::ByteVec& controlFeatureFlags, arma::mat& B, bool computePermutation,
		CIndexPermutation& colIdxPermuttion, std::vector<::arma::mat>& As);

	bool UpdateYcfYcscfT(::Ice::Double* Y, Ice::Long featureIdxFrom, Ice::Long featureIdxTo, 
		const iBS::ByteVec& controlFeatureFlags, arma::mat& C);

	bool UpdateYcfYcfT(::Ice::Double* Y, Ice::Long featureIdxFrom, Ice::Long featureIdxTo, 
		const iBS::ByteVec& controlFeatureFlags, arma::mat& A);

	bool UpdateYcfYcfT_Permuation(::Ice::Double* Y, Ice::Long featureIdxFrom, Ice::Long featureIdxTo,
		const iBS::ByteVec& controlFeatureFlags, CIndexPermutation& colIdxPermuttion, std::vector<::arma::mat>& As);

	void NotifyWorkerBecomesFree(int workerIdx);
	bool createObserverGroupForFilteredY();
	bool FilterOriginalFeatures(::Ice::Long ramMb );
	bool MultithreadGetABC(::arma::mat& A, ::arma::mat& B, ::arma::mat& C, ::Ice::Int  threadCnt, ::Ice::Long ramMb );
	
	bool ControlFeatureByMaxCntLow();
	bool ControlFeatureByRowIdxs();
	bool ControlFeatureByAllInQuantile(int mode);

	bool MultithreadRowANOVA(::Ice::Int  threadCnt, ::Ice::Long ramMb );
	bool createOIDForEigenValues();
	bool saveEigenValues(const arma::vec& eigenVals);
	bool getEigenValues(arma::vec& eigenVals);
	bool createOIDForEigenVectors();
	bool createOIDForPermutatedEigenValues();
	bool savePermutatedEigenValues(Ice::Double *pValues);
	bool saveEigenVectors(const ::arma::mat& U);
	bool getEigenVectors(::arma::mat& U);
	bool createObserverGroupForTs(int maxK);
	bool saveT(int k, const ::arma::mat& T);
	bool createObserverGroupForZs(int maxK);
	bool saveZ(int k, const ::arma::mat& W);
	bool createObserverGroupForGs(int maxK);
	bool saveG(int k, const ::arma::mat& W);
	bool createObserverGroupForWts(int maxK);
	bool saveWt(int k, const ::arma::mat& W);

	bool SetActiveK_T(int k);
	bool SetActiveK_Z(int k);
	bool SetActiveK_G(int k);
	bool SetActiveK_Wt_noReorder(int k);
	bool SetActiveK_Wt(int k);
	bool SetActiveK_AG(int k);

	bool ExtentWt(int k, int extW);
	bool CreateZbyWt(int k);
	bool CreateGbyWt(int k);

	//to decompose variance
	bool SetTemp_AG_byZ();
	bool SetTemp_AG_byK0extW0();

	void GetNormalizedCnts(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans);
	void Multithread_GetNormalizedCnts(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans);

	void GetNormalizedCnts_ModeT(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const;
	void GetNormalizedCnts_ModeYminusZY(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const;
	void GetNormalizedCnts_ModeZY(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const;
	void GetNormalizedCnts_ModeBeta(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const;
	void GetNormalizedCnts_ModeG(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const;
	void GetNormalizedCnts_ModeWa(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const;
	void GetNormalizedCnts_GroupMean(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const;
	void GetNormalizedCnts_ModeZYthenGroupMean(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const;
	
	void GetNormalizedCnts_ModeZYGetE(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const;
	void GetNormalizedCnts_YminusWaXb(Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans) const;

	bool MultithreadGetA(::arma::mat& A, ::Ice::Int  threadCnt, ::Ice::Long ramMb );
	bool RebuildRUVModel_RUVs(::Ice::Int  threadCnt, ::Ice::Long ramMb);
	bool RebuildRUVModel_RUVg(::Ice::Int  threadCnt, ::Ice::Long ramMb);

	bool calculateGrandMeanY();
	bool DecomposeVariance_ModeG(RUVVarDecomposeStageEnum stage, iBS::RUVVarDecomposeInfoVec& vds, ::Ice::Int  threadCnt, ::Ice::Long ramMb);
	bool DecomposeVariance_ModeG(
		RUVVarDecomposeBatchParams& params) const;

	void SetupLibraryFactors();

private:
	::iBS::RUVFacetInfo m_RUVInfo;

	::iBS::FeatureObserverSimpleInfoVec m_preFilterFOIs; //index by sampleIdx
	::iBS::FeatureObserverSimpleInfoVec m_fois; //index by sampleIdx
	::iBS::ObserverStatsInfoVec m_osis;			//index by sampleIdx

	typedef std::map<int, int> Int2Int_T;
	Int2Int_T m_preFilterSampleID2sampleIdx;
	Int2Int_T m_observerID2sampleIdx;
	iBS::IntVec		m_sampleIdx2ConditionIdx;  //index by sampleIdx

	iBS::IntVec		m_ctrlSampleIdx2SampleIdx;
	iBS::IntVec		m_sampleIdx2CtrlSampleIdx; //-1 if a sample is not control sample
	iBS::IntVecVec  m_conditionSampleIdxs;

	int m_activeK;
	int m_extentW;
	::IceUtil::ScopedArray<Ice::Double> m_activeT;
	::IceUtil::ScopedArray<Ice::Double> m_activeZ;
	::IceUtil::ScopedArray<Ice::Double> m_activeG;
	::IceUtil::ScopedArray<Ice::Double> m_activeWt;
	::IceUtil::ScopedArray<Ice::Double> m_activeAG_0W;
	::IceUtil::ScopedArray<Ice::Double> m_activeAG_X0;
	::IceUtil::ScopedArray<Ice::Double> m_kIdx_AG_0W;
	::IceUtil::ScopedArray<Ice::Double> m_kIdx_AG_X0;
	::iBS::DoubleVec m_kIdx_xbGrandMean;
	::iBS::IntVec m_kIdx_extW;
	bool m_needNotify;
	bool m_shutdownRequested;
	IceUtil::Monitor<IceUtil::Mutex>	m_monitor; //for worker thread to notify back

	typedef std::list<int> IntLsit_T;
    IntLsit_T m_freeWorkerIdxs;
	IntLsit_T m_processingWorkerIdxs;

	iBS::DoubleVec m_libraryFactors;

	Ice::Double m_controlFeatureANOVAFStatistics;
	iBS::ByteVec m_controlFeatureFlags;
	Ice::Long	m_controlFeatureTotalCnt;

	Ice::Double m_grandMeanY;
	::Ice::Long m_VDfeatureIdxFrom;
	::Ice::Long m_VDfeatureIdxTo;

	::iBS::RUVOutputScaleEnum m_outputScale;
	::iBS::RUVOutputModeEnum m_outputMode;
	::iBS::IntVec m_outputSampleIDs;
	::iBS::IntVec m_excludeSampleIDsForGroupMean;
	int m_outputWorkerNum;
	CRUVOutputWorkerMgr m_outputWorkerMgr;
	::iBS::IntVec m_WtVectorIdxs;

	Ice::Long m_rebuidRUVTaskID;
	Ice::Long m_toplevelDVTaskID;

	::Ice::Double m_ctrlQuantile; 
	::iBS::DoubleVec m_ctrlQvalues;
	::Ice::Double m_ctrlQuantileAllInFraction;
};

#endif
