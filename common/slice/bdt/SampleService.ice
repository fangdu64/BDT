#ifndef SAMPLE_SERVICE_ICE
#define SAMPLE_SERVICE_ICE

#include "BasicSliceDefine.ice"
#include "FCDCentralService.ice"

module iBS
{
	struct BamToBinCountSampleInfo
	{
		string      SampleName;
		string		BamFile;
		string		Cell;
		string      Treatment;
		string      Description;
		GenomeEnum	Genome;
		//name of reference sequence for creating bins, in specified order
		StringVec	RefNames;
		int         BinWidth;		//base pairs in each bin
	};
	sequence<BamToBinCountSampleInfo> BamToBinCountSampleInfoVec;

	enum BatchSampleOrganizeEnum
	{
		BatchSampleOrganizeSeparate = 0,
		BatchSampleOrganizeGroup = 1
	};

	struct BamToBinCountInfo
	{
		string		BamFile;
		StringVec	RefNames;
		int         BinWidth;
	};

	interface SeqSampleService
	{
		["amd"]
		int CreateBamToBinCountSamples(BamToBinCountSampleInfoVec samples, out IntVec sampleIDs, out long taskID)
			throws ArgumentException;

		BamToBinCountSampleInfo GetBlankBamToBinCountSample()
			throws ArgumentException;

		int GetRefDataFromBamFile(string bamFile, out StringVec	refNames, out LongVec refLengths)
			throws ArgumentException;

		//refNames should be the same for building the bins
		int GetRefBinRanges(BamToBinCountInfo bbci, out LongVec refBinFroms, out LongVec refBinTos)
			throws ArgumentException;

		BamToBinCountInfo GetBlankBamToBinCountInfo()
			throws ArgumentException;

		["amd"]
		int GetHighCountBins(BamToBinCountInfo bbci, int cutoff, out LongVec binIdxs, out LongVec binCounts)
			throws ArgumentException;

		["amd"]
		int GetBasePairCountsInBins(BamToBinCountInfo bbci, LongVec binIdxs, out IntVecVec bpcounts)
			throws ArgumentException;
	};
};

#endif