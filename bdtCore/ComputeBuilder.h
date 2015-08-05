#ifndef __ComputeBuider_h__
#define __ComputeBuider_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <FCDCentralService.h>
#include <ComputeService.h>


class CMultiThreadBuilder
{
	friend class ComputeWorkItemBase;
	friend class CComputeWorker;
public:
	CMultiThreadBuilder();
	virtual ~CMultiThreadBuilder();

protected:
	void NotifyWorkerBecomesFree(int workerIdx);
	
protected:
	bool m_needNotify;
	bool m_shutdownRequested;
	IceUtil::Monitor<IceUtil::Mutex>	m_monitor; //for worker thread to notify back

	typedef std::list<int> IntLsit_T;
	IntLsit_T m_freeWorkerIdxs;
	IntLsit_T m_processingWorkerIdxs;
};

class CPearsonCorrelationBuilder : public CMultiThreadBuilder
{
public:
	CPearsonCorrelationBuilder(const ::iBS::DistMatrixTask& task, const std::string& outFile );

	virtual ~CPearsonCorrelationBuilder();

public:
	void Calculate(::Ice::Int  threadCnt, ::Ice::Long ramMb);

private:
	bool CalcualteColumnMeans(::Ice::Long ramMb);
	bool MultithreadPC(::Ice::Int  threadCnt, ::Ice::Long ramMb);
private:
	iBS::DistMatrixTask m_task;
	iBS::DoubleVec m_colMeans;
	std::string m_outFile;
};

/////////////////////////////////////////////////////////////////////
class CEuclideanDistMatrixBuilder : public CMultiThreadBuilder
{
public:
	CEuclideanDistMatrixBuilder(const ::iBS::DistMatrixTask& task, const std::string& outFile);

	virtual ~CEuclideanDistMatrixBuilder();

public:
	void Calculate(::Ice::Int  threadCnt, ::Ice::Long ramMb);

private:
	iBS::DistMatrixTask m_task;
	std::string m_outFile;
};

////////////////////////////////////////////////////////////////
class CDegreeDistriBuilder
{
public:
	CDegreeDistriBuilder(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::DegreeDistriTask& task);

	virtual ~CDegreeDistriBuilder();

public:
	void Calculate(::Ice::Long ramMb = 250);

private:
	void CalculateByFeatureMajor(::Ice::Long ramMb);
	void CalculateBySampleMajor(::Ice::Long ramMb);
	void CalculateSingleSample(int sampleIdx, ::Ice::Long ramMb);

	void OutputMatrix();

private:
	::iBS::FeatureObserverSimpleInfoVec m_fois;
	::iBS::DegreeDistriTask m_task;
	::Ice::Long m_lowK;
	::iBS::LongVec m_lowKRowSum;
	::IceUtil::ScopedArray<Ice::Int> m_sampleLowPKs;
	typedef std::map<Ice::Int, Ice::Int> Int2Int_T;
	typedef std::vector<Int2Int_T>		 Int2IntList_T;
	Int2IntList_T m_sampleHighPKmaps;
};

////////////////////////////////////////////////////////////////
class CQuantileBuilder
{
public:
	CQuantileBuilder(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::QuantileTask& task,
		const ::iBS::AMD_ComputeService_QuantilePtr& cb);

	virtual ~CQuantileBuilder();

public:
	void Calculate(::Ice::Long ramMb = 250);

private:
	void CalculateByFeatureMajor(::Ice::Long ramMb, ::iBS::LongVecVec& qFeatureIdxs, ::iBS::DoubleVecVec& qValues);
	void CalculateBySampleMajor(::Ice::Long ramMb, ::iBS::LongVecVec& qFeatureIdxs, ::iBS::DoubleVecVec& qValues);
	void CalculateSingleSample(int sampleIdx, ::Ice::Long ramMb, ::iBS::LongVecVec& qFeatureIdxs, ::iBS::DoubleVecVec& qValues);

private:
	::iBS::FeatureObserverSimpleInfoVec m_fois;
	::iBS::QuantileTask m_task;
	::iBS::AMD_ComputeService_QuantilePtr m_cb;
};

////////////////////////////////////////////////////////////////
class CHighValueFeaturesBuilder
{
public:
	CHighValueFeaturesBuilder(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::HighValueFeaturesTask& task,
		const ::iBS::AMD_ComputeService_HighValueFeaturesPtr& cb);

	virtual ~CHighValueFeaturesBuilder();

public:
	void Calculate(::Ice::Long ramMb = 250);

private:
	void CalculateByFeatureMajor(::Ice::Long ramMb);
	void CalculateBySampleMajor(::Ice::Long ramMb);
	void CalculateSingleSample(int sampleIdx, ::Ice::Long ramMb);

private:
	::iBS::FeatureObserverSimpleInfoVec m_fois;
	::iBS::HighValueFeaturesTask m_task;
	::iBS::AMD_ComputeService_HighValueFeaturesPtr m_cb;

	typedef std::map<Ice::Long, iBS::IntVec> Long2IntVec_T;
	Long2IntVec_T m_hvFeatureIdx2SampleIDs;
};

////////////////////////////////////////////////////////////////
class CFeatureRowAdjustBuilder
{
public:
	CFeatureRowAdjustBuilder(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::FeatureRowAdjustTask& task,
		const ::iBS::AMD_ComputeService_FeatureRowAdjustPtr& cb);

	virtual ~CFeatureRowAdjustBuilder();

public:
	void Adjust(::Ice::Long ramMb = 250);

private:
	void AdjustByFeatureMajor(::Ice::Long ramMb);
	void AdjustBySampleMajor(::Ice::Long ramMb);
	void AdjustSingleSample(int sampleIdx, ::Ice::Long ramMb);

private:
	::iBS::FeatureObserverSimpleInfoVec m_fois;
	::iBS::FeatureRowAdjustTask m_task;
	::iBS::AMD_ComputeService_FeatureRowAdjustPtr m_cb;
	Ice::Long m_featureIdxFrom;
	Ice::Long m_featureIdxTo;
	iBS::ByteVec m_featureFlags;
};

////////////////////////////////////////////////////////////////
class CExportRowMatrixBuilder
{
public:
	CExportRowMatrixBuilder(
		const ::iBS::ExportRowMatrixTask& task, Ice::Long taskID);

	virtual ~CExportRowMatrixBuilder();

public:
	void Export(::Ice::Long ramMb = 250);
	void ReadRowMatrix(Ice::Long featureIdxFrom, Ice::Long featureIdxTo,
		std::pair<const Ice::Double*, const Ice::Double*>& ret,
		::IceUtil::ScopedArray<Ice::Double>&  retValues);
private:
	iBS::FeatureObserverSimpleInfoPtr GetOutputFOI();
private:
	::iBS::ExportRowMatrixTask m_task;
	Ice::Long m_taskID;
};


////////////////////////////////////////////////////////////////
class CVectors2MatrixBuilder
{
public:
	CVectors2MatrixBuilder(
		const ::iBS::Vectors2MatrixTask& task,
		const ::iBS::FeatureObserverSimpleInfoVec& inFOIs, 
		const ::iBS::FeatureObserverSimpleInfoVec& outFOIs);

	virtual ~CVectors2MatrixBuilder();

public:
	void DoWork(::Ice::Long ramMb = 250);

private:
	bool GetInRawY(
		::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Double*  RawY);
private:
	::iBS::Vectors2MatrixTask m_task;
	::iBS::FeatureObserverSimpleInfoVec m_inFOIs;
	::iBS::FeatureObserverSimpleInfoVec m_outFOIs;
};


////////////////////////////////////////////////////////////////
class CExportZeroOutBgRowMatrixBuilder
{
public:
	CExportZeroOutBgRowMatrixBuilder(
		const ::iBS::ExportZeroOutBgRowMatrixTask& task);

	virtual ~CExportZeroOutBgRowMatrixBuilder();

public:
	void Export(::Ice::Long ramMb = 250);

private:
	iBS::FeatureObserverSimpleInfoPtr GetOutputFOI();
	void GetFrontRowValue(Ice::Long featureIdx, iBS::DoubleVec& data);
	void GetBackRowValue(Ice::Long featureIdx, iBS::DoubleVec& data);
	void GetBgValue(Ice::Long featureIdx, Ice::Long windowHalfSize, iBS::DoubleVec& data);
private:
	::iBS::ExportZeroOutBgRowMatrixTask m_task;
	iBS::DoubleVec  m_backY;
	Ice::Long m_backYFromIdx;
	Ice::Long m_backYToIdx;

	iBS::DoubleVec  m_frontY;
	Ice::Long m_frontYFromIdx;
	Ice::Long m_frontYToIdx;

	iBS::DoubleVec m_bgWindowColSums;
	Ice::Long m_bgWinFromIdx;
	Ice::Long m_bgWinToIdx;

};

////////////////////////////////////////////////////////////////
class CExportByRowIdxsBuilder
{
public:
	CExportByRowIdxsBuilder(
		const ::iBS::ExportByRowIdxsTask& task, Ice::Long taskID);

	virtual ~CExportByRowIdxsBuilder();

public:
	void Export(::Ice::Long ramMb = 250);
	
private:
	void ExportBySampleRowMatrix(::Ice::Long ramMb = 250);
	void ExportByGetRowMatrix(::Ice::Long ramMb = 250);
	iBS::FeatureObserverSimpleInfoPtr GetOutputFOI();
private:
	::iBS::ExportByRowIdxsTask m_task;
	Ice::Long m_taskID;

};

////////////////////////////////////////////////////////////////
class CFeatureVariabilityBuilder
{
public:
	CFeatureVariabilityBuilder(
		const ::iBS::HighVariabilityFeaturesTask& task,
		const ::iBS::AMD_ComputeService_HighVariabilityFeaturesPtr& cb);

	virtual ~CFeatureVariabilityBuilder();

public:
	void Calculate(::Ice::Long ramMb = 250);

private:
	void TestByFeatureMajor(::Ice::Long ramMb);

private:
	::iBS::HighVariabilityFeaturesTask m_task;
	::iBS::AMD_ComputeService_HighVariabilityFeaturesPtr m_cb;
};

////////////////////////////////////////////////////////////////
class CRowANOVABuilder
{
public:
	CRowANOVABuilder(
		const ::iBS::RowANOVATask& task,
		const ::iBS::AMD_ComputeService_RowANOVAPtr& cb);

	virtual ~CRowANOVABuilder();

public:
	void Calculate(::Ice::Long ramMb = 250);

private:
	void ANOVAByFeatureMajor(::Ice::Long ramMb);

private:
	::iBS::RowANOVATask m_task;
	::iBS::AMD_ComputeService_RowANOVAPtr m_cb;

	typedef std::map<int, int> Int2Int_T;
	Int2Int_T m_sampleID2sampleIdx;
	iBS::IntVecVec  m_group2SampleIdxs;
};

////////////////////////////////////////////////////////////////
class CWithSignalFeaturesBuilder
{
public:
	CWithSignalFeaturesBuilder(
		const ::iBS::WithSignalFeaturesTask& task,
		const ::iBS::AMD_ComputeService_WithSignalFeaturesPtr& cb);

	virtual ~CWithSignalFeaturesBuilder();

public:
	void Calculate(::Ice::Long ramMb = 250);

private:
	void TestByFeatureMajor(::Ice::Long ramMb);

private:
	::iBS::WithSignalFeaturesTask m_task;
	::iBS::AMD_ComputeService_WithSignalFeaturesPtr m_cb;
};

////////////////////////////////////////////////////////////////
class CRuvVdAnovaBuilder
{
public:
	CRuvVdAnovaBuilder(
		const ::iBS::VdAnovaTask& task, Ice::Long amdTaskID);

	virtual ~CRuvVdAnovaBuilder();

public:
	void DoWork(iBS::VdAnovaResult& ret,::Ice::Long ramMb = 250);

private:
	Ice::Double GetGrandMean(::Ice::Long ramMb);
	void DoVD(::Ice::Long ramMb, Ice::Double grandMean, iBS::VdAnovaResult& ret);

private:
	::iBS::VdAnovaTask m_task;
	Ice::Long m_amdTaskID;

};

#endif
