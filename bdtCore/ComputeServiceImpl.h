#ifndef __ComputeServiceImpl_h__
#define __ComputeServiceImpl_h__

#include <FCDCentralService.h>
#include <ComputeService.h>

class CComputeServiceImpl;
typedef IceUtil::Handle<CComputeServiceImpl> CComputeServiceImplPtr;

class CComputeServiceImpl : virtual public iBS::ComputeService
{
public:
	CComputeServiceImpl()
	{
	}

	virtual ~CComputeServiceImpl(){}

public:
	virtual ::iBS::DistMatrixTask GetBlankDistMatrixTask(const Ice::Current&);

	virtual void DistMatrix_async(const ::iBS::AMD_ComputeService_DistMatrixPtr&,
		const ::iBS::DistMatrixTask&,
		const Ice::Current&);

	virtual ::iBS::RUVDistMatrixBatchTask GetBlankRUVDistMatrixBatchTask(const Ice::Current&);

	virtual void RUVDistMatrixBatch_async(const ::iBS::AMD_ComputeService_RUVDistMatrixBatchPtr&,
		const ::iBS::RUVDistMatrixBatchTask&,
		const Ice::Current&);

	virtual ::iBS::DegreeDistriTask GetBlankDegreeDistriTask(const Ice::Current&);

	virtual void DegreeDistribution_async(const ::iBS::AMD_ComputeService_DegreeDistributionPtr&,
		const ::iBS::DegreeDistriTask&,
		const Ice::Current&);

	virtual ::iBS::QuantileTask GetBlankQuantileTask(const Ice::Current&);

	virtual void Quantile_async(const ::iBS::AMD_ComputeService_QuantilePtr&,
		const ::iBS::QuantileTask&,
		const Ice::Current&);

	virtual ::Ice::Int GetSampleValByRefVal(::Ice::Double,
		::Ice::Long,
		const ::iBS::IntVec&,
		::iBS::DoubleVec&,
		const Ice::Current&);

	virtual ::iBS::HighValueFeaturesTask GetBlankHighValueFeaturesTask(const Ice::Current&);

	virtual void HighValueFeatures_async(const ::iBS::AMD_ComputeService_HighValueFeaturesPtr&,
		const ::iBS::HighValueFeaturesTask&,
		const Ice::Current&);

	virtual ::iBS::FeatureRowAdjustTask GetBlankFeatureRowAdjustTask(const Ice::Current&);

	virtual void FeatureRowAdjust_async(const ::iBS::AMD_ComputeService_FeatureRowAdjustPtr&,
		const ::iBS::FeatureRowAdjustTask&,
		const Ice::Current&);

	virtual ::iBS::ExportRowMatrixTask GetBlankExportRowMatrixTask(const Ice::Current&);

	virtual void ExportRowMatrix_async(const ::iBS::AMD_ComputeService_ExportRowMatrixPtr&,
		const ::iBS::ExportRowMatrixTask&,
		const Ice::Current&);

	virtual ::iBS::Vectors2MatrixTask GetBlankVectors2MatrixTask(const Ice::Current&);

	virtual void Vectors2Matrix_async(const ::iBS::AMD_ComputeService_Vectors2MatrixPtr&,
		const ::iBS::Vectors2MatrixTask&,
		const Ice::Current&);

	virtual ::iBS::ExportZeroOutBgRowMatrixTask GetBlankExportZeroOutBgRowMatrixTask(const Ice::Current&);

	virtual void ExportZeroOutBgRowMatrix_async(const ::iBS::AMD_ComputeService_ExportZeroOutBgRowMatrixPtr&,
		const ::iBS::ExportZeroOutBgRowMatrixTask&,
		const Ice::Current&);

	virtual void GetRowMatrix_async(const ::iBS::AMD_ComputeService_GetRowMatrixPtr&,
		const ::iBS::ExportRowMatrixTask&,
		::Ice::Long,
		::Ice::Long,
		const Ice::Current&);

	virtual ::iBS::ExportByRowIdxsTask GetBlankExportByRowIdxsTask(const Ice::Current&);
	virtual ::iBS::RUVExportByRowIdxsBatchTask GetBlankRUVExportByRowIdxsBatchTask(const Ice::Current&);
	virtual void RUVExportByRowIdxsBatch_async(const ::iBS::AMD_ComputeService_RUVExportByRowIdxsBatchPtr&,
		const ::iBS::RUVExportByRowIdxsBatchTask&,
		const Ice::Current&);

	virtual void ExportByRowIdxs_async(const ::iBS::AMD_ComputeService_ExportByRowIdxsPtr&,
		const ::iBS::ExportByRowIdxsTask&,
		const Ice::Current&);

	virtual ::iBS::RUVExportRowMatrixBatchTask GetBlankRUVExportRowMatrixBatchTask(const Ice::Current&);

	virtual void RUVExportRowMatrixBatch_async(const ::iBS::AMD_ComputeService_RUVExportRowMatrixBatchPtr&,
		const ::iBS::RUVExportRowMatrixBatchTask&,
		const Ice::Current&);

	virtual ::iBS::HighVariabilityFeaturesTask GetBlankHighVariabilityFeaturesTask(const Ice::Current&);

	virtual void HighVariabilityFeatures_async(const ::iBS::AMD_ComputeService_HighVariabilityFeaturesPtr&,
		const ::iBS::HighVariabilityFeaturesTask&,
		const Ice::Current&);

	virtual ::iBS::RowANOVATask GetBlankRowANOVATask(const Ice::Current&);

	virtual void RowANOVA_async(const ::iBS::AMD_ComputeService_RowANOVAPtr&,
		const ::iBS::RowANOVATask&,
		const Ice::Current&);

	virtual ::iBS::BetweenGroupTestTask GetBlankBetweenGroupTestTask(const Ice::Current&);

	virtual void BetweenGroupTest_async(const ::iBS::AMD_ComputeService_BetweenGroupTestPtr&,
		const ::iBS::BetweenGroupTestTask&,
		const Ice::Current&);

	virtual ::iBS::WithSignalFeaturesTask GetBlankWithSignalFeaturesTask(const Ice::Current&);

	virtual void WithSignalFeatures_async(const ::iBS::AMD_ComputeService_WithSignalFeaturesPtr&,
		const ::iBS::WithSignalFeaturesTask&,
		const Ice::Current&);

	virtual ::iBS::VdAnovaTask GetBlankVdAnovaTask(const Ice::Current&);

	virtual ::iBS::RuvVdAnovaBatchTask GetBlankRuvVdAnovaBatchTask(const Ice::Current&);

	virtual void RuvVdAnovaBatch_async(const ::iBS::AMD_ComputeService_RuvVdAnovaBatchPtr&,
		const ::iBS::RuvVdAnovaBatchTask&,
		const Ice::Current&);

};
#endif
