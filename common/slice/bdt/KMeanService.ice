#ifndef KMEAN_SERVICE_ICE
#define KMEAN_SERVICE_ICE

#include "BasicSliceDefine.ice"
#include "FCDCentralService.ice"

module iBS
{
	enum KMeanProjectStatusEnum
	{
		KMeanProjectStatusUnknown	=0,
		KMeanProjectStatusAborted	=1,
		KMeanProjectStatusCreated	=2,
		KMeanProjectStatusWaitingForInitialContractors =3,
		KMeanProjectStatusRunning =4,
		KMeanProjectStatusPausedWaitingForRelayContractors =5
	};

	enum KMeansDistEnum
	{
		KMeansDistUnknown =0,
		KMeansDistEuclidean		=1,
		KMeansDistCorrelation	=2
	};

	enum KMeansSeedingEnum
	{
		KMeansSeedingKMeansRandom = 0,
		KMeansSeedingKMeansPlusPlus = 1
	};

	enum KMeansTaskEnum
	{
		KMeansTaskNone	= 0,
		KMeansTaskRunKMeans = 1,
		KMeansTaskPPSeeds	= 2		//k-means center initialization using k-means++
	};
	sequence<KMeansTaskEnum> KMeansTaskEnumVec;

	//K-Mean Clustering Services
	["cpp:class"] 
	struct KMeanProjectInfo
	{
		int		ProjectID;
		string	ProjectName;
		KMeansTaskEnum Task;
		long	K;					// active K
		KMeansSeedingEnum Seeding;
		LongVec BatchKs;
		KMeansTaskEnumVec BatchTasks;

		KMeansDistEnum Distance;
		long    MaxIteration;
		long	MinChangeCnt;
		double	MinExplainedChanged;

		// Observer info for clustering data
		FcdcReadService* FcdcReader;
		IntVec	ObserverIDs;		//Observers as columns (clustering by rows)
		long    FeatureIdxFrom;		//Global featureIdx range for clustering obersers
		long    FeatureIdxTo;

		FcdcReadWriteService* FcdcForKMeans;
		int		GIDForClusterSeeds;			// data for cluster centers
		long    SeedsFeatureIdxFrom;		// top [SeedsFeatureIdxFrom,K+SeedsFeatureIdxFrom) from ObserverGroupIDForSeeds

		int     GIDForKClusters;			// observer group ID for K clusters results, to write
		int		OIDForFeature2ClusterIdx;	// observer ID for feature's clusterIdx
		
		//not for setting
		long	ObserverCnt;
		long    TotalRowCnt;    //FeatureIdxTo-FeatureIdxFrom
		KMeanProjectStatusEnum ProjectStatus;
		int		ContractorCnt;
	};

	enum KMeanSvrMsgEnum
	{
		KMeanSvrMsgNone		=0,
		KMeanSvrMsgAllDone	=1,
		KMeanSvrMsgAbort	=3
	};


	["cpp:class"] 
	struct KMeanContractInfo
	{
		KMeansTaskEnum Task; //changeable during the course of pipeline
		KMeansSeedingEnum Seeding;
		bool	AcceptedAsContractor;
		int		ProjectID;
		string	ProjectName;
		string	ContractorName;
		int		ContractorCnt;	//how many contractors in total
		int		ContractorIdx;  //which index I am
		int		ContractorLastFinishedStage;
	
		FcdcReadService* FcdcReader;
		IntVec	ObserverIDs;
		long    FeatureIdxFrom; //Global feature idx
		long    FeatureIdxTo;   //Global feature idx
		long	K;				//changeable during the course of pipeline
		KMeansDistEnum Distance;
	};

	struct KMeansResult
	{
		KMeansTaskEnum Task;
		KMeansSeedingEnum Seeding;
		long	ColCnt;
		long	RowCnt;
		long	K;
		KMeansDistEnum Distance;
		double	MinExplainedChanged;
		DoubleVec	Distorsions;
		DoubleVec	Explaineds;
		LongVec		ChangedCnts;
		double	WallTimeSeconds;
	};
	sequence<KMeansResult> KMeansResultVec;

	interface KMeanServerService
	{
		int ReportStatus(int projectID, int contractorIdx, 
				string contractorName, string strStatus, 
				out KMeanSvrMsgEnum svrMsg);

		//if project==0, willing to accept any new project
		//ramSize in Mega bytes
		["amd"] 
		int RequestToBeContractor(int projectID, string contractorName, 
			int workerCnt, int ramSize, out KMeanContractInfo contract);

		//if len(KCnts)=len(KSums)=0, meaning no updates, just rqst KClusters from KMeanServer
		//get back updated KClusters in KMeanServer
		["amd"] 
		int ReportKCntsAndSums(int projectID, int contractorIdx, 
			["cpp:array"] DoubleVec KCnts, ["cpp:array"] DoubleVec KSums, 
			long KChangeCnt, double distortion,
			out ["cpp:array"] DoubleVec KClusters);
		
		["amd"] 
		int ReportKMembers(int projectID, int contractorIdx, long featureIdxFrom, long featureIdxTo,
			["cpp:array"] LongVec featureIdx2ClusterIdx);

		["amd"] 
		int GetKMembers(int projectID,long featureIdxFrom, long featureIdxTo,
			out ["cpp:array"] LongVec clusterIdx);

		["amd"]
		int GetKClusters(int projectID, out["cpp:array"] DoubleVec KClusters);

		["amd"] 
		int GetKCnts(int projectID, out ["cpp:array"] DoubleVec KCnts);

		// KMeans++ seeds
		["amd"]
		int ReportPPDistSum(int projectID, int contractorIdx,
			double distSum, out double selectSum);
		["amd"]
		int ReportNewSeed(int projectID, int contractorIdx,
			long seedFeatureIdx, DoubleVec center, out["cpp:array"] DoubleVec KClusters);

		["amd"]
		int GetPPSeedFeatureIdxs(int projectID, out LongVec featureIdxs);

		// Pipeline control
		["amd"]
		int GetNextTask(int projectID, int contractorIdx, out KMeansTaskEnum task, out long K);

		["amd"]
		int GetKMeansResults(int projectID, out KMeansResultVec results);

		["amd"]
		int GetKSeeds(int projectID, out["cpp:array"] DoubleVec KSeeds);
		
	};

	interface KMeanServerAdminService
	{
		KMeanServerService* GetKMeansSeverProxy();

		int GetBlankProject(out KMeanProjectInfo retProjectInfo)
			throws ArgumentException;

		int CreateProjectAndWaitForContractors(
			KMeanProjectInfo rqstProjectInfo,
			out KMeanProjectInfo retProjectInfo)
			throws ArgumentException;

		int LaunchProjectWithCurrentContractors(int projectID, out long taskID)
			throws ArgumentException;

		int DestroyProject(int projectID)
			throws ArgumentException;

		int GetAMDTaskInfo(long taskID, out AMDTaskInfo task)
			throws ArgumentException;
	};

	interface KMeanContractorAdminService
	{
		["amd"]
		int StartNewContract(int projectID, KMeanServerService* kmeansServer)
			throws ArgumentException;

		int ResetRootProxy(string pcProxyStr)
			throws ArgumentException;
	};

};

#endif