#ifndef COMPUTE_SERVICE_ICE
#define COMPUTE_SERVICE_ICE

#include "BasicSliceDefine.ice"
#include "FCDCentralService.ice"

module iBS
{
	enum DistEnum
	{
		DistUnknown = 0,
		DistEuclidean = 1,
		DistPCC = 2
	};

	struct DistMatrixTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		DistEnum Distance;
		IntVec	SampleIDs;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		int		ThreadCnt;
		long	RamMb;
		RowSelection Subset;
		string  OutPath;
	};
	sequence<DistMatrixTask> DistMatrixTaskVec;

	struct RUVDistMatrixBatchTask
	{
		long	TaskID;
		string	TaskName;
		FcdcRUVService* ruv;
		IntVec	ks;
		IntVec  extWs;
		DistMatrixTaskVec Tasks;
	};

	//calculate distance between ref and rows specified by FeatureGroupIDs
	struct RefRowDistTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		DistEnum Distance;
		IntVec	SampleIDs;
		LongVec	FeatureIdxs;
		IntVec	FeatureGroupIDs;
		DoubleVec RefVector;
	};
	sequence<RefRowDistTask> RefRowDistTaskVec;

	struct DegreeDistriTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec	SampleIDs;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
	};

	struct QuantileTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec	SampleIDs;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		DoubleVec Quantiles;
		string	OutFile;
	};

	struct HighValueFeaturesTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec	SampleIDs;
		DoubleVec  UpperLimits;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
	};

	struct FeatureRowAdjustTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadWriteService* writer;
		IntVec	SampleIDs;
		DoubleVec  AdjustedValues;
		LongVec FeatureIdxs;
		bool RecalculateStats;
		FcdcAdminService* admin;
	};

	struct ExportRowMatrixTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec	SampleIDs;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		long	FileSizeLimitInMBytes;
		int     OutID;
		string	OutFile;
		string  OutPath;
		FeatureValueEnum ConvertToType;
		RowAdjustEnum RowAdjust;
		ValueAdjustEnum ValueAdjust;
	};

	struct ExportZeroOutBgRowMatrixTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec	SampleIDs;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		long	FileSizeLimitInMBytes;
		bool	NeedLogFirst;
		double  BgSignalDifference;
		DoubleVec LibrarySizeFactors;
		long	BgWindowRadius;
		int     OutID;
		string	OutFile;
		string  OutPath;
		FeatureValueEnum ConvertToType;
	};

	struct ExportByRowIdxsTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec	SampleIDs;
		LongVec FeatureIdxs;
		long	FileSizeLimitInMBytes;
		int     OutID;
		string	OutFile;
		string  OutPath;
		long	PerRqstLimitInMBytes;
		RowAdjustEnum RowAdjust;
		ValueAdjustEnum ValueAdjust;
		int		FeatureIdxsOid;
	};
	sequence<ExportByRowIdxsTask> ExportByRowIdxsTaskVec;

	struct RUVExportByRowIdxsBatchTask
	{
		long	TaskID;
		string	TaskName;
		FcdcRUVService* ruv;
		IntVec	ks;
		IntVec  extWs;
		ExportByRowIdxsTaskVec Tasks;
	};

	struct VdAnovaResult
	{
		double totalVar = 0;

		//"explained variance" by locus
		double bgVar = 0;
		//"unexplained variance", or "within-group variability"
		double wgVar = 0;
	};

	struct VdAnovaTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec	SampleIDs;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		long	PerRqstLimitInMBytes;
	};
	sequence<VdAnovaTask> VdAnovaTaskVec;

	struct RuvVdAnovaBatchTask
	{
		long	TaskID;
		string	TaskName;
		FcdcRUVService* ruv;
		IntVec	ks;
		IntVec  extWs;
		VdAnovaTaskVec Tasks;
		string  OutPath;
	};

	enum VariabilityTestEnum
	{
		VariabilityTestNone = 0,
		VariabilityTestCV	= 1,	//coefficient of variation
		VariabilityTestGCV	= 2		//geometric CV
	};

	struct HighVariabilityFeaturesTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec	SampleIDs;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		long	SamplingFeatureCnt; //0 without sampling
		RowAdjustEnum RowAdjust;
		ValueAdjustEnum ValueAdjust;
		double	FeatureFilterMaxCntLowThreshold;
		VariabilityTestEnum VariabilityTest;
		double	VariabilityCutoff;
	};

	struct RowANOVATask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec		SampleIDs;
		IntVecVec	GroupSampleIDs;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		long	SamplingFeatureCnt;
		double	FeatureFilterMaxCntLowThreshold;
		RowAdjustEnum RowAdjust;
		ValueAdjustEnum ValueAdjust;
	};
	sequence<RowANOVATask> RowANOVATaskVec;

	struct RUVRowANOVABatchTask
	{
		long	TaskID;
		string	TaskName;
		FcdcRUVService* ruv;
		IntVec	ks;
		IntVec  extWs;
		RowANOVATaskVec Tasks;
	};

	struct WithSignalFeaturesTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec	SampleIDs;
		double	SignalThreshold;
		int		SampleCntAboveThreshold; //at least as many samples whose reads should above the threshold
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		long	SamplingFeatureCnt; //0 without sampling
		RowAdjustEnum RowAdjust;
		long	PerRqstLimitInMBytes;
	};

	struct NoneBgSignalFeaturesTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec	SampleIDs;
		double	DifferenceThreshold;
		long	BackgroundRadius;
		int		SampleCntAboveThreshold; //at least as many samples whose reads should above the threshold
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		long	SamplingFeatureCnt; //0 without sampling
		RowAdjustEnum RowAdjust;
		long	PerRqstLimitInMBytes;
	};

	//BetweenGroupTest
	enum BGTestEnum
	{
		BGTestNone=0,
		TTestEqualVar	= 1,
		TTestUnequalVar = 2
	};

	struct BetweenGroupTestTask
	{
		long	TaskID;
		string	TaskName;
		FcdcReadService* reader;
		IntVec		SampleIDs;
		IntVecVec	GroupSampleIDs;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		long	SamplingFeatureCnt;
		double	FeatureFilterMaxCntLowThreshold;
		RowAdjustEnum RowAdjust;
		ValueAdjustEnum ValueAdjust;
		BGTestEnum	Test;
	};
	sequence<BetweenGroupTestTask> BetweenGroupTestTaskVec;


	struct Vectors2MatrixTask
	{
		long	TaskID;
		string	TaskName;
		IntVec	InOIDs;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		IntVec  OutOIDs;
		long	PerBatchInMBytes;
	};

	interface ComputeService
	{
		DistMatrixTask GetBlankDistMatrixTask();
		
		["amd"]
		int DistMatrix(DistMatrixTask task, out long taskID)
			throws ArgumentException;

		RUVDistMatrixBatchTask GetBlankRUVDistMatrixBatchTask();

		["amd"]
		int RUVDistMatrixBatch(RUVDistMatrixBatchTask task)
			throws ArgumentException;

		DegreeDistriTask GetBlankDegreeDistriTask();
		["amd"]
		int DegreeDistribution(DegreeDistriTask task)
			throws ArgumentException;

		QuantileTask GetBlankQuantileTask();
		["amd"]
		int Quantile(QuantileTask task, out LongVecVec featureIdxs, out DoubleVecVec values)
			throws ArgumentException;

		int GetSampleValByRefVal(double refVal, long refLibSize, IntVec SampleIDs, out DoubleVec sampleVal)
			throws ArgumentException;

		HighValueFeaturesTask GetBlankHighValueFeaturesTask();
		["amd"]
		int HighValueFeatures(HighValueFeaturesTask task,
			out LongVec featureIdxs, out IntVecVec sampleIDs)
			throws ArgumentException;

		FeatureRowAdjustTask GetBlankFeatureRowAdjustTask();
		["amd"]
		int FeatureRowAdjust(FeatureRowAdjustTask task)
			throws ArgumentException;

		ExportRowMatrixTask GetBlankExportRowMatrixTask();

		["amd"]
		int ExportRowMatrix(ExportRowMatrixTask task, out long taskID)
			throws ArgumentException;

		Vectors2MatrixTask GetBlankVectors2MatrixTask();
		["amd"]
		int Vectors2Matrix(Vectors2MatrixTask task, out IntVec OIDs, out long taskID)
			throws ArgumentException;


		ExportZeroOutBgRowMatrixTask GetBlankExportZeroOutBgRowMatrixTask();

		["amd"]
		int ExportZeroOutBgRowMatrix(ExportZeroOutBgRowMatrixTask task)
			throws ArgumentException;

		["amd"]
		int GetRowMatrix(ExportRowMatrixTask task, long featureIdxFrom, long featureIdxTo, out["cpp:array"] DoubleVec values)
			throws ArgumentException;
		
		ExportByRowIdxsTask GetBlankExportByRowIdxsTask();

		RUVExportByRowIdxsBatchTask GetBlankRUVExportByRowIdxsBatchTask();

		["amd"]
		int ExportByRowIdxs(ExportByRowIdxsTask task, out long taskID)
			throws ArgumentException;

		["amd"]
		int RUVExportByRowIdxsBatch(RUVExportByRowIdxsBatchTask task,out long taskID)
			throws ArgumentException;

		HighVariabilityFeaturesTask GetBlankHighVariabilityFeaturesTask();
		["amd"]
		int HighVariabilityFeatures(HighVariabilityFeaturesTask task,
			out LongVec featureIdxs, out DoubleVec variabilities)
			throws ArgumentException;

		RowANOVATask GetBlankRowANOVATask();
		["amd"]
		int RowANOVA(RowANOVATask task,
			out LongVec featureIdxs, out DoubleVec Fs)
			throws ArgumentException;

		BetweenGroupTestTask GetBlankBetweenGroupTestTask();
		["amd"]
		int BetweenGroupTest(BetweenGroupTestTask task,
			out LongVec featureIdxs, out DoubleVec values)
			throws ArgumentException;

		WithSignalFeaturesTask GetBlankWithSignalFeaturesTask();
		["amd"]
		int WithSignalFeatures(WithSignalFeaturesTask task, out LongVec featureIdxs)
			throws ArgumentException;

		VdAnovaTask GetBlankVdAnovaTask();

		RuvVdAnovaBatchTask GetBlankRuvVdAnovaBatchTask();

		["amd"]
		int RuvVdAnovaBatch(RuvVdAnovaBatchTask task, out long taskID)
			throws ArgumentException;
	};
};

#endif