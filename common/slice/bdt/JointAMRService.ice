#ifndef JOINTAMR_SERVICE_ICE
#define JOINTAMR_SERVICE_ICE
#include "BasicSliceDefine.ice"
#include "FCDCentralService.ice"

module iBS
{
	enum MethyStateEnum
	{
		MethyStateUnknown = 0,
		MethyStateU= 1,
		MethyStateM= 2
	};

	struct BSEvidence
	{
		int		RefIdx;		//0-based
		int		RefOffset;	//0-based
		MethyStateEnum MethyState;
		bool	IsWatson;
		bool	IsForward;
		byte	MateID;
		byte	MapQ;
		byte	Qual;
		int		SeqCycle;
		int		StudyID;
		long	ReadID;
	};
	sequence<BSEvidence> BSEvidenceVec;

	enum AMRSlidingWinEnum
	{
		AMRSlidingWinCenterBpCnt = 0, //each CpG site as center,[bpidx(c)-int(WinSize/2), bpidx(c)+WinSize-int(WinSize/2)-1]
		AMRSlidingWinCenterCpGCnt = 1 //each CpG site as center,[bpidx(c-int(WinSize/2)), bpidx(c+WinSize-int(WinSize/2)-1)]
	};

	struct AMRFinderResult
	{
		FcdcAdminService* fcdc;
		long WinCnt;
		int OIDH0LogLikelihood;
		int OIDH1LogLikelihood;
		int SaveProperty;
		int GIDProperty;			//ReadCnt, CpGSiteCnt,AvgSitePerRead,EM_iter
	};

	enum AMRFinderRunModeEnum
	{
		AMRFinderRunModeRealData,
		AMRFinderRunModeH0SimulatedData
	};

	struct AMRFinderTask
	{
		long		TaskID;
		string		TaskName;
		AMRFinderRunModeEnum RunMode;
		GenomeEnum	Genome;
		StringVec	Refs;
		StringVec	EvTblFiles;
		int			MaxReadLen;
		int			ThreadCnt;
		long		RamMb;
		long		BatchEVCnt;
		AMRSlidingWinEnum SlidingWin;
		int			WinSize;
		int			MinMAPQ;
		int			MinMUCntPerCpGSite; //min coverage a cpg site will be involved in the likelihood calculation
		int			MinEVCntPerRead;	//min cpg sites a read needs to have to be involved in the likelihood calculation
		int			MinReadsPerWindow;
		double		HighProb;
		double		LowProb;
		int			MaxEMIter;
		double		PseudoCnt;
		AMRFinderResult Result;
	};

	struct CpGSiteMapInfo
	{
		StringVec	Refs;
		LongVec		RefCpGIdxFroms;	//starting bin idx of the first CpG sites for each ref
		LongVec		RefCpGCnts;		//# of CpGs for each ref
		long		TotalCpGCnt;	//Total CpG cnt for watson
		int			OIDForCpGIdx2BpIdx; //each bin containing the bp idx (0-based) of a cpg site in a given ref
		GenomeEnum	Genome;
		bool		MergeWatsonCrick;  //paired cpg sites on watson and crick viewe as one site
	};

	struct JFixedPQModel
	{
		StringVec	SampleNames;
		IntVec		SingleH0H1LLOIDs;
		IntVec		JointH0H1LLOIDs;
		long		FeatureIdxFrom;
		long		FeatureIdxTo;
		int			MaxEMIter;
		double		EMErrTol;
	};

	struct JAMRFixedPQModelTask
	{
		long		TaskID;
		string		TaskName;
		int			ThreadCnt;
		long		RamMb;
		JFixedPQModel Model;
	};

	struct JEvalKnownAMRTask
	{
		BpRangeVec	KnownAMRs;
		StringVec	SampleNames;
		IntVec		SingleH0H1LLOIDs;
		long		TopCpGCnt;
		long		TopRegionCnt;
		long		MergeDistance; //in bp
		string		OutDir;
	};

	struct JEvalSingleH0H1BiasTask
	{
		StringVec	SampleNames;
		IntVec		SingleH0H1LLOIDs;
		long		SelectCpGCnt;	//sampling how many windows
		IntVec		OutIDs;
		string		OutDir;
	};

	interface JointAMRService
	{
		int GetBlankAMRFinderTask(bool saveproperty, out AMRFinderTask task)
			throws ArgumentException;

		["amd"]
		int RunAMRFinder(AMRFinderTask task, out long taskID)
			throws ArgumentException;

		int SetCpGSiteMap(CpGSiteMapInfo cpgMap)
			throws ArgumentException;

		CpGSiteMapInfo GetBlankCpGSiteMapInfo();
		
		//get cpg sites (on watson strand) in bp positions, 0-based
		int GetCpGSites(string ref, long bpIdxFrom, long bpIdxTo, bool watson, out LongVec cpgBpIdxs)
			throws ArgumentException;
		int GetCpGIdxs(string ref, LongVec bpIdxs, bool watson, out  LongVec cpgIdxs)
			throws ArgumentException;

		
		int GetBlankJAMRFixedPQModelTask(out JAMRFixedPQModelTask task)
			throws ArgumentException;

		["amd"]
		int RebuildJAMRFixedPQModel(JAMRFixedPQModelTask task, out long taskID)
			throws ArgumentException;


		int GetBlankJEvalKnownAMRTask(out JEvalKnownAMRTask task)
			throws ArgumentException;
		["amd"]
		int RunJEvalKnownAMR(JEvalKnownAMRTask task, out long taskID)
			throws ArgumentException;

		int GetBlankJEvalSingleH0H1BiasTask(out JEvalSingleH0H1BiasTask task)
			throws ArgumentException;
		["amd"]
		int RunJEvalH0H1Bias(JEvalSingleH0H1BiasTask task, out long taskID)
			throws ArgumentException;
	};

};

#endif