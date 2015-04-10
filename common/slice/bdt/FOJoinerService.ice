#ifndef FeatureObserverJoiner_SERVICE_ICE
#define FeatureObserverJoiner_SERVICE_ICE

#include "BasicSliceDefine.ice"
#include "FCDCentralService.ice"

module iBS
{
	enum FOJoinerProjectStatusEnum
	{
		FOJoinerProjectStatusUnknown	=0,
		FOJoinerProjectStatusAborted	=1,
		FOJoinerProjectStatusCreated	=2,
		FOJoinerProjectStatusWaitingForInitialContractors =3,
		FOJoinerProjectStatusRunning =4,
		FOJoinerProjectStatusPausedWaitingForRelayContractors =5
	};

	//quantile normalization joiner services
	["cpp:class"] 
	struct QNJoinerProjectInfo
	{
		int ProjectID;
		string ProjectName;
		IntVec InputObserverIDs;			//Observers to be quantile normalized
		int ObserverCnt;
		long DomainSize;
		int    OutputObserverGroupID;		//
		FOJoinerProjectStatusEnum ProjectStatus; //
		int ContractorCnt;
		long RowCntPerBatch;
		long CreateDT;					//datetime created
		long EndDT;

	};

	enum QNJoinerSvrMsgEnum
	{
		QNJoinerSvrMsgNone	  =0,
		QNJoinerSvrMsgAllDone =1,
		QNJoinerSvrMsgAbort   =3
	};

	["cpp:class"] 
	struct QNJoinerContractInfo
	{
		bool AcceptedAsContractor;
		int ProjectID;
		string ProjectName;
		//how many contractors and in which index am I
		string ContractorName;
		int ContractorIdx;
		int ContractorCnt;
		int ContractorLastFinishedtStage;

		//step 1
		//get all feature values for these observers
		//sort these vectors (ascending order)
		IntVec ObserverIDs;

		//step 2 
		//report sorted rowSums to server
		//each batch report a number of RowCntPerBatch rows (same for all contractors)
		long RowCntPerBatch;

		//server will return mean vector, contractor store them in local for later use

		//step 3
		//contractor update feature values according sort index and mean vector
		//report rowMatrix (order by feature idx) to server
		//each batch report a number of RowCntPerBatch rows (same for all contractors)
		//server will combine rowMatrix from all contractors to combined rowMatrix and save them to FCDCentral

	};


	interface QNJoinerAdminService
	{
		int CreateProjectAndWaitForContractors( string projectName, 
			IntVec inObserverIDs, 
			int outObserverGroupID, 
			out QNJoinerProjectInfo projectInfo);

		int LaunchProjectWithCurrentContractors(int projectID);
	};

	interface QNJoinerContractorService
	{

		int ReportStatus(int projectID, int contractorIdx, 
				string contractorName, string strStatus, 
				out QNJoinerSvrMsgEnum svrMsg);

		//if project==0, willing to accept any new project
		//ramSize in Mega bytes
		["amd"] 
		int RequestToBeContractor(int projectID, string contractorName, 
			int workerCnt, int ramSize, out QNJoinerContractInfo contract);

		["amd"] 
		int ReportSortedRowSum(int projectID, int contractorIdx, 
			long rowIdxFrom, long rowIdxTo, ["cpp:array"] DoubleVec values, 
			out ["cpp:array"] DoubleVec sortedMeans);

		["amd"] 
		int ReportUpdatedRowMatrix(int projectID, int contractorIdx, 
			long featureIdxFrom, long featureIdxTo, ["cpp:array"] DoubleVec values);

	};

};

#endif