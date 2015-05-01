#ifndef FCDCENTRAL_SERVICE_ICE
#define FCDCENTRAL_SERVICE_ICE
#include "BasicSliceDefine.ice"
 
module iBS
{
	enum GenomeEnum
	{
		GenomeOthers = 0,
		GenomeHG19 = 1,
		GenomeHG18 = 2,
		GenomeMM10 = 3,
		GenomeMM9 = 4
	};

	//generic node status enum (e.g., feature domain, feature observer...)
	enum NodeStatusEnum
	{
		NodeStatusUnknown  =0,
		NodeStatusIDOnly   =1,
		NodeStatusUploaded =2,
		NodeStatusUpdating =3,
	};

	//enum of genomic feature domains
	//internally, each feature is identified by int64 (long)
	enum FeatureDomainEnum
	{
		FeatureDomainUnknown,
		FeatureDomainIntRange,		//0 based, e.g., non-overlapping bins of genome
		FeatureDomainStringList,	//unique strings, e.g., genes
		FeatureDomainIntList		//unique integers, e.g., sampled features from another domain
	};

	//base pair level range
	struct BpRange
	{
		string	Ref;
		string	Name;
		long	BpIdxFrom;	//0-based
		long	BpIdxTo;	//0-based
	};
	sequence<BpRange> BpRangeVec;

	struct FeatureDomainInfo
	{
		int DomainID;
		string DomainName;
		long DomainSize;	//feature count
		FeatureDomainEnum DomainType;
		NodeStatusEnum Status;
		long CreateDT;		//datetime created
		long UpdateDT;		//datetime last updated
		string Description;
	};
	sequence<FeatureDomainInfo> FeatureDomainInfoVec;

	//for file store each sub file size S is determined by: S%rowBytes==0 and  S<256M*4=1G
	enum FeatureValueStorePolicyEnum
	{
		FeatureValueStorePolicyUnkown			  =0,
		FeatureValueStorePolicyInRAMNoSave		  =1,			//in RAM only, will not save
		FeatureValueStorePolicyBinaryFilesSingleObserver =2,    //binary feature value file (BFV specification)
		FeatureValueStorePolicyBinaryFilesObserverGroup  =3     //binary feature value file (BFV specification)
	};

	// values either all in RAM or all in store
	// when save values in RAM to store, all values in store will be overwritten
	enum FeatureValueSetPolicyEnum
	{
		FeatureValueSetPolicyDoNothing				=0,
		FeatureValueSetPolicyInRAMNoSave			=1,	//just in RAM, gone after shutdown

		FeatureValueSetPolicyInRAMAndSaveToStore	=2,	//set in RAM (will trigger loading necessary whole or involved batches from store to RAM), and immediately save update contents to store
		FeatureValueSetPolicyNoRAMImmediatelyToStore=3,	//immediately save to store (update required values), will not stay in RAM 
	};

	enum FeatureValueGetPolicyEnum
	{
		FeatureValueGetPolicyDoNothing				=0,
		FeatureValueGetPolicyAuto					=1, //let server decide
		FeatureValueGetPolicyGetFromRAM				=2,	//read from RAM (will trigger loading necessary whole or involved batches from store to RAM)
		FeatureValueGetPolicyGetForOneTimeRead	    =3,	//read from RAM first, if not exist, load required data from store, will not stay in RAM 
	};

	enum FeatureValueEnum
	{
		FeatureValueUnknown=0,
		FeatureValueDouble =1,
		FeatureValueFloat  =2,
		FeatureValueInt32  =3,
		FeatureValueInt64  =4,
		FeatureValueBit	   =6,
		FeatureValueByte   =7,
		FeatureValueInt16  =8
	};

	enum ByteArrayContentEnum
	{
		ByteArrayContentUnknown=0,
		ByteArrayContentByte =1,
		ByteArrayContentUINT16 =2,
		ByteArrayContentINT16  =3
	};

	enum ByteArrayEndianEnum
	{
		ByteArrayEndianUnknown=0,
		ByteArrayBigEndian =1,
		ByteArrayLittleEndian =2
	};

	enum FeatureValueStoreLocationEnum
	{
		FeatureValueStoreLocationDefault = 0,
		FeatureValueStoreLocationSpecified = 1
	};

	struct FeatureObserverInfo
	{
		int		ObserverID;		//globally unique
		string	ObserverName;	//e.g., sample name
		string  ContextName;	//e.g., cellline name
		int		DomainID;		//feature domain
		long	DomainSize;		//feature count
		FeatureValueEnum			ValueType;
		FeatureValueStorePolicyEnum StorePolicy; //default Store Policy
		FeatureValueGetPolicyEnum	GetPolicy;   //default Get Policy
		FeatureValueSetPolicyEnum	SetPolicy;   //default Set Policy
		NodeStatusEnum Status;
		int ThreadRandomIdx;	//to assign an observer to a thread
								//observer in a group will have same random idx
		int ObserverGroupID;	//ObserverGroupID = first ObservereID inGroup
		int ObserverGroupSize;  // number of observers in the group
		int IdxInObserverGroup;
		long CreateDT;			//datetime created
		long UpdateDT;			//datetime last updated
		int Version;			//value version
		int ParentObserverID;	//derived from which observer
		int MapbackObserverID;  //for mapping local back to parent feature index
		string Description;
		FeatureValueStoreLocationEnum StoreLocation;
		//e.g., /dcs01/featurevlauestore/gid_200005_19.bfv
		string SpecifiedPathPrefix; //if store location is specified , e.g., /dcs01/featurevlauestore/gid_200005

	};
	sequence<FeatureObserverInfo> FeatureObserverInfoVec;

	["cpp:class"] 
	struct FeatureObserverSimpleInfo
	{
		int ObserverID;
		int DomainID;
		long DomainSize;
		FeatureValueEnum ValueType;
		FeatureValueStorePolicyEnum StorePolicy;
		FeatureValueGetPolicyEnum	GetPolicy;
		FeatureValueSetPolicyEnum	SetPolicy;
		NodeStatusEnum Status;
		int ThreadRandomIdx;
		int ObserverGroupID;
		int ObserverGroupSize;
		int IdxInObserverGroup;
		FeatureValueStoreLocationEnum StoreLocation;
		string SpecifiedPathPrefix;
	};
	sequence<FeatureObserverSimpleInfo> FeatureObserverSimpleInfoVec;

	enum SpecialFeatureObserverEnum
	{
		SpecialFeatureObserverUnknown =0,
		SpecialFeatureObserverTestDoubles	=1,  // [0~3x10^9], value=index
		SpecialFeatureObserverTestFloats	=2,	 // [0~3x10^9], value=index
		SpecialFeatureObserverTestInt32		=3,	 // [0~3x10^9], value=index
		SpecialFeatureObserverTestInt64		=4,	 // [0~3x10^9], value=index
		SpecialFeatureObserverTestMaxID		=999,//
		SpecialFeatureObserverRAMOnlyMinID	=1000,	 //fore no save observers
		SpecialFeatureObserverRAMOnlyMaxID  =100000, //fore no save observers
		SpecialFeatureObserverMaxID			=200000,// Max ID for special observer
	};

	struct ObserverStatsInfo
	{
		int	ObserverID;
		int Version;
		long UpdateDT;	//datetime last updated

		//basic statistics
		double Cnt;
		double Max;
		double Min;
		double Sum;

		//other named statistics
		StringVec StatsNames;
		DoubleVec StatsValues;
		
	};
	sequence<ObserverStatsInfo> ObserverStatsInfoVec;

	enum InvertIndexEnum
	{
		InvertIndexUnknown =0,
		InvertIndexIntValueIntKey	=1,  // e.g., KMembers
		InvertIndexDoubleValueDoubleRangeKey=2,	 //e.g, histogram
	};

	struct ObserverIndexInfo
	{
		int IndexID;
		int	ObserverID;
		int Version;
		long UpdateDT;	//datetime last updated
		InvertIndexEnum IndexType;
		long	  KeyCnt;   //# unique keys
		StringVec KeyNames; // key names (if any)
		LongVec KeyIdx2RowIdxListStartIdx;
		LongVec KeyIdx2RowCnt;
		long    TotalRowCnt;

		IntVec IntKeys;
		DoubleVec DoubleRangeFromKeys;
		DoubleVec DoubleRangeToKeys;
		bool MakeIndexFile;
		int  IndexObserverID;
	};
	sequence<ObserverIndexInfo> ObserverIndexInfoVec;

	enum RowSelectorEnum
	{
		RowSelectorNone = 0,
		RowSelectorLowerThanThreshold  = 1,
		RowSelectorHigherThanThreshold = 2,
		RowSelectorWithinMinMax	 = 3,
	};

	enum RowStatisticEnum
	{
		RowStatisticNone = 0,
		RowStatisticMax = 1,
		RowStatisticMin = 2,
		RowStatisticSum = 3,
	};

	struct RowSelection
	{
		bool Enable;
		RowSelectorEnum  Selector;
		RowStatisticEnum Statistic;
		double Threshold;
		double Min; //not including
		double Max; //not including
		long SelectedCnt;
		long UnselectedCnt;
	};

	enum RowAdjustEnum
	{
		RowAdjustNone = 0,
		RowAdjustZeroMean = 1,					// zero mean 
		RowAdjustZeroMeanUnitSD = 2,			// zero mean and unit standard deviation (length or norm = sqrt(dimention))
												// constant vector, vi=0
		RowAdjustZeroMeanUnitLength = 3,		// zero mean and unit vector (length = 1), constant vector, vi=sqrt(1/D)
		RowAdjustZeroMeanUnitLengthConst0 = 4,	// zero mean and unit vector (length = 1), constant vector, vi=0
	};

	enum ValueAdjustEnum
	{
		ValueAdjustNone = 0,
		ValueAdjustLogToLinear		= 1,		// zero mean 
		ValueAdjustLogToLog2		= 2,
	};


	interface ProxyCentralService
	{
		//using remote address's ip as hostName, append it to  proxyStrNoHost
		int RegisterByCallerAdress(string servantName, string proxyStrNoHost);
		int RegisterByProxyStr(string servantName, string proxyStr);
		int UnRegister(string servantName);
		int UnRegisterAll();
		int ListAll(out StringVec proxyStrs);
	};

	interface FcdcReadService
	{
		int GetFeatureDomains(IntVec domainIDs, out FeatureDomainInfoVec domainInfos)
			throws ArgumentException;

		int GetFeatureObservers(IntVec observerIDs, out FeatureObserverInfoVec observerInfos)
			throws ArgumentException;

		//features are rows, observes are columns
		["amd"] 
		int GetDoublesColumnVector(int observerID, long featureIdxFrom, long featureIdxTo, 
			out ["cpp:array"] DoubleVec values)
			throws ArgumentException;

		["amd"] 
		int GetIntsColumnVector(int observerID, long featureIdxFrom, long featureIdxTo, 
			out ["cpp:array"] IntVec values)
			throws ArgumentException;

		//get all observers' value in a given group, array content:  rolled out by rows
		["amd"] 
		int GetDoublesRowMatrix(
			int observerGroupID, long featureIdxFrom, long featureIdxTo, out ["cpp:array"] DoubleVec values)
			throws ArgumentException;
		
		int GetObserverStats(int observerID, out ObserverStatsInfo stats)
			throws ArgumentException;

		int GetObserversStats(IntVec observerIDs, out ObserverStatsInfoVec observerStats)
			throws ArgumentException;

		["amd"]
		int GetRowMatrix(IntVec observerIDs, long featureIdxFrom, long featureIdxTo, optional(1) RowAdjustEnum rowAdjust, out["cpp:array"] DoubleVec values)
			throws ArgumentException;

		["amd"]
		int SampleRowMatrix(IntVec observerIDs, LongVec featureIdxs, optional(1) RowAdjustEnum rowAdjust, out["cpp:array"] DoubleVec values)
			throws ArgumentException;

		int GetObserverIndex(int observerID, out ObserverIndexInfo oii)
			throws ArgumentException;

		["amd", "nonmutating", "cpp:const"] 
		int GetFeatureIdxsByIntKeys(int observerID,
			IntVec keys, long maxFeatureCnt, out ["cpp:array"] LongVec featureIdxs)
			throws ArgumentException;

		//each k in Kidxs yields a cnt in featureCnts, len(kIdxs)==len(featureCnts)
		["amd", "nonmutating", "cpp:const"] 
		int GetFeatureCntsByIntKeys(int observerID,
			IntVec keys, out ["cpp:array"] LongVec featureCnts)
			throws ArgumentException;

		int GetAMDTaskInfo(long taskID, out AMDTaskInfo task)
			throws ArgumentException;

		int GetFeatureValueStoreDir(out string rootDir)
			throws ArgumentException;

		int GetFeatureValuePathPrefix(int observerID, out string pathPrefix)
			throws ArgumentException;
	};

	interface FcdcReadWriteService extends FcdcReadService
	{
		int SetFeatureDomains(FeatureDomainInfoVec domainInfos)
			throws ArgumentException;

		int SetFeatureObservers(FeatureObserverInfoVec observerInfos)
			throws ArgumentException;

		["amd"] 
		int SetDoublesColumnVector(int observerID,  long featureIdxFrom, long featureIdxTo, 
			["cpp:array"] DoubleVec values)
			throws ArgumentException;

		["amd"] 
		int SetBytesColumnVector(int observerID,  long featureIdxFrom, long featureIdxTo, 
			["cpp:array"] ByteVec bytes, ByteArrayContentEnum content, ByteArrayEndianEnum endian)
			throws ArgumentException;

		//column first
		["amd"] 
		int SetIntsColumnVector(int observerID,  long featureIdxFrom, long featureIdxTo, 
			["cpp:array"] IntVec values)
			throws ArgumentException;

		//array len= (featureIdxTo-featureIdxFrom)*observerGroupSize
		//array content: rolled out by rows (i.e., featureIdx), each row contains a number of observerGroupSize values
		["amd"] 
		int SetDoublesRowMatrix(
			int observerGroupID, long featureIdxFrom, long featureIdxTo, ["cpp:array"] DoubleVec values)
			throws ArgumentException;

		
	};

	interface FcdcAdminService extends FcdcReadWriteService 
	{
		void Shutdown();

		int RqstNewFeatureDomainID(out int domainID);
		int RqstNewFeatureObserverID(bool inRAMNoSave, out int observerID);
		int RqstNewFeatureObserversInGroup( int groupSize, bool inRAMNoSave, out IntVec observerIDs);

		int AttachBigMatrix(int colCnt, long rowCnt, StringVec colNames, string storePathPrefix, out IntVec OIDs)
			throws ArgumentException;

		int AttachBigVector(long rowCnt, string colName, string storePathPrefix, out int OID)
			throws ArgumentException;

	
		["amd", "nonmutating", "cpp:const"] 
		int ForceLoadInRAM(IntVec observerIDs)
			throws ArgumentException;

		["amd", "nonmutating", "cpp:const"] 
		int ForceLeaveRAM(IntVec observerIDs)
			throws ArgumentException;

		["amd"] 
		int RecalculateObserverStats(IntVec observerIDs)
			throws ArgumentException;

		["amd"] 
		int RecalculateObserverIndex(int observerID,bool saveIndexFile)
			throws ArgumentException;

		["amd"]
		int RemoveFeatureObservers(IntVec observerIDs, bool removeDataFile)
			throws ArgumentException;

		int SetObserverStats(ObserverStatsInfoVec osis)
			throws ArgumentException;
	};

	//facets define
	const string FcdcFacetNameDefault="";
	const string FcdcFacetNameDivideByColumnSum="FcdcFacetDivideByColumnSum";

	enum FcdcFacetEnum
	{
		FcdcFacetDefault =0,
		FcdcFacetDivideByColumnSum =1,
		FcdcFacetLogCount =2,	//log(rawCount+1), in natural logarithm
		FcdcFacetRUVs=3			//normalized counts = Y - Wa
	};

	enum RUVFeatureFilterPolicyEnum
	{
		RUVFeatureFilterPolicyNone =0, //no filtering
		RUVFeatureFilterPolicyMaxCntLowPysicalCopy =1 //filter rows with low counts, save as observer group
	};

	enum RUVControlFeaturePolicyEnum
	{
		RUVControlFeaturePolicyNone	=0, //all are control features
		RUVControlFeaturePolicyANOVA	=1,
		RUVControlFeaturePolicyMaxCntLow = 2,
		RUVControlFeaturePolicyFeatureIdxList =3,	//specify feature idxs
		RUVControlFeaturePolicyAllInUpperQuantile = 4,	
		RUVControlFeaturePolicyAllInLowerQuantile = 5	
	};

	enum RUVFacetStatusEnum
	{
		RUVFacetStatusNone =0,			//no filtering
		RUVFacetStatusFilteredOIDsReady =1, //no filtering
		RUVFacetStatusFeatureFiltered =2, //no filtering
		RUVFacetStatusReady =3 
	};

	enum RUVOutputScaleEnum
	{
		RUVOutputScaleLog = 0,	//
		RUVOutputScaleRaw = 1,	//
	};

	enum RUVInputAdjustEnum
	{
		RUVInputDoLogE = 0,	//
		RUVInputDoLog2 = 1,	//
		RUVInputDoNothing = 2,//
	};

	enum RUVModeEnum
	{
		RUVModeRUVs = 0,
		RUVModeRUVg = 1,
		RUVModeRUVsForVariation = 2,
		RUVModeRUVgForVariation = 3,
	};

	enum RUVOutputModeEnum
	{
		RUVOutputModeGroupMean = 0, // replicate group mean
		RUVOutputModeYminusWa = 1,	// a,b from Y OLS on W X
		RUVOutputModeXb = 2,		// a,b from Y OLS on W X
		RUVOutputModeWa = 3,		// a,b from Y OLS on W X
		RUVOutputModeYminusZY = 4,	// ZY=Wa, a from Y OLS on W
		RUVOutputModeZY = 5,		// ZY=Wa, a from Y OLS on W
		RUVOutputModeZYthenXb= 6,	// output Xb, where ZY=Wa, a from Y OLS on W, then b from (Y-Wa) OLS on X
		RUVOutputModeZYthenGroupMean =7,// (should be the same as 6) output group mean, where ZY=Wa, a from Y OLS on W, replicate group mean on (Y-Wa)
		RUVOutputModeYminusTYcs = 8,// RUVs only, TYcs=Wa, a from RUVs, W from Y(control features) OLS on a
		RUVOutputModeZYGetE = 9,// get e, (Xb+e)=Y-ZY
		RUVOutputModeYminusWaXb = 10// get e
	};

	struct RUVFacetInfo
	{
		int			FacetID;
		bool		FacetReady;
		RUVFacetStatusEnum FacetStatus;
		RUVModeEnum RUVMode;
		string		FacetName;
		string		Description;
		
		IntVec		SampleIDs;
		IntVecVec	ReplicateSampleIDs; //each condition contains several replicated observerIDs
		RUVFeatureFilterPolicyEnum  FeatureFilterPolicy;
		double			FeatureFilterMaxCntLowThreshold;
		RUVControlFeaturePolicyEnum ControlFeaturePolicy;
		double		ControlFeatureMaxCntLowBound;
		double		ControlFeatureMaxCntUpBound;
		double		CommonLibrarySize;
		DoubleVec	NormalizeFactors;
		int			ObserverIDforControlFeatureIdxs;
		DoubleVecVec KnownFactors;
		double		Tol; // tolerance to ensure >0
		long		MaxK; //max number of unwanted factors, up to MaxK
		int			ThreadRandomIdx;
		long	FeatureIdxFrom;
		long	FeatureIdxTo;
		bool	SubRangeLibrarySizeAdjust; //when dealing with sub ranges
		RUVInputAdjustEnum InputAdjust;
		int		PermutationCnt; //# of times to permute each row of data matrix independently to remove any structure in the matrix
		
		//no need to set parameters below
		IntVec		RawCountObserverIDs; //aftere filtering
		int			MapbackObserverID;  //for mapping local back to unfiltered feature index
		IntVecVec	ConditionObserverIDs;
		long	n; //number number of samples (oberverIDs)
		long	L; //number of control features
		long	J; //number of feature count
		long	P; //number of conditions (e.g., cell lines)
		int		K; //min(MaxK, #eigenvals>Tol)
		int		CtrlSampleCnt; //number of control samples
		int		ObserverIDforWts;//stored RUV obtained model data
		int		ObserverIDforTs; //stored RUV obtained model data
		int		ObserverIDforZs; //stored RUV obtained model data
		int		ObserverIDforGs; //stored RUV obtained model data
		double  grandMeanY; // before rowMean
		long	ControlFeatureCnt;
		int		OIDforEigenValue;
		int		OIDforEigenVectors;
		int		OIDforPermutatedEigenValues; // PermutationCnt by len(OIDforEigenValue)  matrix

	};

	struct ConditionInfo
	{
		string	Name;
		int		ConditionIdx;
		int		ObserverCnt;
	};
	sequence<ConditionInfo> ConditionInfoVec;

	struct RUVVarDecomposeInfo
	{
		int k;	  //number of unwanted factors used
		int extW; //number of known factors used
		double grandMean;
		double totalVar; //original total Var
		double Xb; //variance explained by Xb term
		double Wa; //variance explained by Wa term
		double XbWa; //variance explained by Xb+Wa term
		double e;  //variance accounted for noise
		long	featureIdxFrom;
		long	featureIdxTo;
		IntVec  wtVecIdxs;

		double xbGrandMean;
		double xbTotalVar;
		double xbLocusBgVar;
		double xbLocusWgVar;
	};
	sequence<RUVVarDecomposeInfo> RUVVarDecomposeInfoVec;

	interface FcdcRUVService extends FcdcReadService
	{
		//add additional known factors to W
		["amd"]
		int SetActiveK(int k, int extW)
			throws ArgumentException;

		int SetOutputMode(RUVOutputModeEnum mode)
			throws ArgumentException;

		int SetOutputSamples(IntVec sampleIDs)
			throws ArgumentException;

		int ExcludeSamplesForGroupMean(IntVec excludeSampleIDs)
			throws ArgumentException;

		int SetOutputScale(RUVOutputScaleEnum scale)
			throws ArgumentException;

		int SetOutputWorkerNum(int workerNum)
			throws ArgumentException;

		int SetCtrlQuantileValues(double quantile, DoubleVec qvalues, double fration);

		["amd"]
		int RebuildRUVModel(int threadCnt, long ramMb, out long taskID)
			throws ArgumentException;

		int GetConditionIdxs(IntVec observerIDs, out IntVec conditionIdxs)
			throws ArgumentException;

		int GetConditionInfos(out ConditionInfoVec conditions)
			throws ArgumentException;

		int GetSamplesInGroups(IntVec sampleIDs, out IntVecVec groupSampleIDs)
			throws ArgumentException;

		int GetG(out["cpp:array"] DoubleVec values)
			throws ArgumentException;

		int GetWt(out["cpp:array"] DoubleVec values)
			throws ArgumentException;

		int GetEigenVals(out DoubleVec values)
			throws ArgumentException;

		int SelectKByEigenVals(double minFraction, out int k, out DoubleVec fractions)
			throws ArgumentException;

		["amd"]
		int DecomposeVariance(IntVec ks, IntVec extWs, 
			IntVecVec wtVecIdxs,
			long featureIdxFrom, long featureIdxTo,
			int threadCnt, long ramMb, string outfile, out RUVVarDecomposeInfoVec vds, out long taskID)
			throws ArgumentException;

		int SetWtVectorIdxs(IntVec vecIdxs)
			throws ArgumentException;
	};

	interface BigMatrixService extends FcdcReadService
	{
		int SetOutputSamples(IntVec sampleIDs)
			throws ArgumentException;

		int SetRowAdjust(RowAdjustEnum rowAdjust)
			throws ArgumentException;

		["amd"]
		int RecalculateObserverStats(long ramMb, out long taskID)
			throws ArgumentException;
	};

	interface FcdcFacetAdminService
	{
		BigMatrixService* GetBigMatrixFacet(int gid)
			throws ArgumentException;
	};

	interface BdvdFacetAdminService
	{
		int RqstNewRUVFacet(out RUVFacetInfo rfi)
			throws ArgumentException;

		int RemoveRUVFacet(int facetID)
			throws ArgumentException;

		int SetRUVFacetInfo(RUVFacetInfo rfi)
			throws ArgumentException;

		int GetRUVFacetInfo(int facetID, out RUVFacetInfo rfi)
			throws ArgumentException;

		FcdcRUVService* GetRUVFacet(int facetID)
			throws ArgumentException;
	};


};

#endif