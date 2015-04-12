#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/ScopedArray.h>
#include <GlobalVars.h>
#include <SampleServiceImpl.h>
#include <FeatureObserverDB.h>
#include <FeatureValueRAM.h>
#include <FeatureValueWorker.h>
#include <FeatureValueWorkerMgr.h>
#include <ObserverStatsDB.h>
#include <SampleWorkItem.h>
#include <bdtUtil/GenomeHelper.h>
#include <BamToBinCountCreator.h>
#include <bdtUtil/SortHelper.h>

void
CSampleServiceImpl::CreateBamToBinCountSamples_async(
	const ::iBS::AMD_SeqSampleService_CreateBamToBinCountSamplesPtr& cb,
	const ::iBS::BamToBinCountSampleInfoVec& samples,
	const Ice::Current& current)
{
	int sampleCnt = (int)samples.size();
	if (sampleCnt==0)
	{
		::iBS::ArgumentException ex("empty sample");
		cb->ice_exception(ex);
		return;
	}
	
	std::vector<CGenomeBinMap> binMaps;
	binMaps.reserve(sampleCnt);
	Ice::Long totalBinCnt = 0;
	for (int i = 0; i < sampleCnt; i++)
	{
		std::string reason;
		CGenomeBinMap binMap;
		bool ret = CGenomeHelper::GetBinMapFromSampleInfo(samples[i], binMap, reason);
		if (!ret)
		{
			::iBS::ArgumentException ex(reason);
			cb->ice_exception(ex);
			return;
		}
		if (totalBinCnt == 0)
		{
			totalBinCnt = binMap.GetTotalBinCount();
		}
		if (totalBinCnt != binMap.GetTotalBinCount())
		{
			::iBS::ArgumentException ex("found different total bin count in sample");
			cb->ice_exception(ex);
			return;
		}

		binMaps.push_back(binMap);
	}

	if (totalBinCnt == 0)
	{
		::iBS::ArgumentException ex("total bin count is 0");
		cb->ice_exception(ex);
		return;
	}

	iBS::IntVec observerIDs;
	observerIDs.reserve(sampleCnt);
	for (int i = 0; i < sampleCnt; i++)
	{
		int observerID =0;
		CGlobalVars::get()->theObserversDB.RqstNewFeatureObserverID(observerID);
		observerIDs.push_back(observerID);
	}
	

	iBS::FeatureObserverInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs, fois);
	for (int i = 0; i < sampleCnt; i++)
	{
		iBS::FeatureObserverInfo& foi = fois[i];
		const iBS::BamToBinCountSampleInfo& bsi = samples[i];
		foi.ObserverName = bsi.SampleName;
		foi.ContextName = bsi.Cell;
		foi.Description = bsi.Description;
		foi.DomainSize = totalBinCnt;
	}
	CGlobalVars::get()->theObserversDB.SetFeatureObservers(fois);
	
	Ice::Long taskID = CGlobalVars::get()->theFeatureValueWorkerMgr->RegisterAMDTask(
		"CreateBamToBinCountSamples", sampleCnt);
	cb->ice_response(1, observerIDs, taskID);

	for (int i = 0; i < sampleCnt; i++)
	{
		int observerID = observerIDs[i];
		iBS::FeatureObserverSimpleInfoPtr foi
			= CGlobalVars::get()->theObserversDB.GetFeatureObserver(observerID);

		const iBS::BamToBinCountSampleInfo& bsi = samples[i];
		FeatureValueWorkItemPtr wi = new CCreateBamToBinSample(foi, bsi, binMaps[i], taskID);

		CGlobalVars::get()->theFeatureValueWorkerMgr->AssignItemToWorker(foi, wi);
	}
}

::iBS::BamToBinCountSampleInfo
CSampleServiceImpl::GetBlankBamToBinCountSample(const Ice::Current& current)
{
	::iBS::BamToBinCountSampleInfo sample;
	sample.BinWidth = 100;
	sample.Genome = iBS::GenomeHG19;
	return sample;
}

::Ice::Int
CSampleServiceImpl::GetRefDataFromBamFile(const ::std::string& bamFile,
::iBS::StringVec& refNames,
::iBS::LongVec& refLengths,
const Ice::Current& current)
{
	std::string reason;
	::iBS::IntVec refIDs;
	if (CGenomeHelper::GetRefDataFromBamFile(bamFile, refIDs, refNames, refLengths, reason))
	{
		return 1;
	}
	else
	{
		::iBS::ArgumentException ex(reason);
		throw ex;
	}
}

::Ice::Int
CSampleServiceImpl::GetRefBinRanges(const ::iBS::BamToBinCountInfo& bbci,
::iBS::LongVec& refBinFroms,
::iBS::LongVec& refBinTos,
const Ice::Current& current)
{
	iBS::BamToBinCountSampleInfo sample;
	sample.BamFile = bbci.BamFile;
	sample.BinWidth = bbci.BinWidth;
	sample.RefNames = bbci.RefNames;

	std::string reason;
	CGenomeBinMap binMap;
	bool ret = CGenomeHelper::GetBinMapFromSampleInfo(sample, binMap, reason);
	if (!ret)
	{
		::iBS::ArgumentException ex(reason);
		throw ex;
	}

	int refCnt = (int)bbci.RefNames.size();
	refBinFroms.resize(refCnt, -1);
	refBinTos.resize(refCnt, -1);

	for (int i = 0; i < refCnt; i++)
	{
		Ice::Long binIdx = binMap.GetBinIdxByRefNamePos(bbci.RefNames[i], 0);
		refBinFroms[i] = binIdx;
	}
	for (int i = 0; i < refCnt-1; i++)
	{
		refBinTos[i] = refBinFroms[i+1];
	}
	refBinTos[refCnt - 1] = binMap.GetTotalBinCount();

	return 1;
}


::iBS::BamToBinCountInfo
CSampleServiceImpl::GetBlankBamToBinCountInfo(const Ice::Current& current)
{
	::iBS::BamToBinCountInfo sample;
	sample.BinWidth = 100;
	return sample;
}

void
CSampleServiceImpl::GetHighCountBins_async(const ::iBS::AMD_SeqSampleService_GetHighCountBinsPtr& cb,
const ::iBS::BamToBinCountInfo& bbci,
::Ice::Int cutoff,
const Ice::Current& current)
{
	iBS::BamToBinCountSampleInfo sample;
	sample.BamFile = bbci.BamFile;
	sample.BinWidth = bbci.BinWidth;
	sample.RefNames = bbci.RefNames;
	std::string reason;
	CGenomeBinMap binMap;
	bool ret = CGenomeHelper::GetBinMapFromSampleInfo(sample, binMap, reason);
	if (!ret)
	{
		::iBS::ArgumentException ex(reason);
		cb->ice_exception(ex);
		return;
	}

	Ice::Long totalValueCnt = binMap.GetTotalBinCount();
	::IceUtil::ScopedArray<Ice::Double>  binValues(new ::Ice::Double[totalValueCnt]);
	if (!binValues.get()){
		::iBS::ArgumentException ex("memory failed");
		cb->ice_exception(ex);
		return;
	}

	std::fill(binValues.get(), binValues.get() + totalValueCnt, 0);

	CBamToBinCountCreator bc(sample, binMap);
	bc.ReadBinValues(binValues.get());
	iBS::LongVec binIdxs;
	iBS::LongVec binCounts;
	binIdxs.reserve(10000);
	binCounts.reserve(10000);
	Ice::Double cutoff_d = (Ice::Double)cutoff;
	for (Ice::Long i = 0; i < totalValueCnt; i++)
	{
		if (binValues[i] >= cutoff_d)
		{
			binIdxs.push_back(i);
			binCounts.push_back((Ice::Long)binValues[i]);
		}
	}

	iBS::LongVec sortedBinCounts(binIdxs.size());
	iBS::LongVec originalIdxs(binIdxs.size());
	
	CSortHelper::GetSortedValuesAndIdxs(binCounts, sortedBinCounts, originalIdxs);
	iBS::LongVec sortedBinIdxs(binIdxs.size());
	for (int i = 0; i < binIdxs.size(); i++)
	{
		sortedBinIdxs[i] = binIdxs[originalIdxs[i]];
	}

	cb->ice_response(1, sortedBinIdxs, sortedBinCounts);
}

void
CSampleServiceImpl::GetBasePairCountsInBins_async(const ::iBS::AMD_SeqSampleService_GetBasePairCountsInBinsPtr& cb,
const ::iBS::BamToBinCountInfo& bbci,
const ::iBS::LongVec& binIdxs,
const Ice::Current& current)
{
	iBS::BamToBinCountSampleInfo sample;
	sample.BamFile = bbci.BamFile;
	sample.BinWidth = bbci.BinWidth;
	sample.RefNames = bbci.RefNames;
	std::string reason;
	CGenomeBinMap binMap;
	bool ret = CGenomeHelper::GetBinMapFromSampleInfo(sample, binMap, reason);
	if (!ret)
	{
		::iBS::ArgumentException ex(reason);
		cb->ice_exception(ex);
		return;
	}

	CBamToBinBasePairCounts bc(sample, binMap, binIdxs);
	iBS::IntVecVec bpCounts;
	bc.ReadBasePairCounts(bpCounts);
	cb->ice_response(1, bpCounts);
}