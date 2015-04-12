#ifndef __SeqSampleServiceImpl_h__
#define __SeqSampleServiceImpl_h__

#include <FCDCentralService.h>
#include <SampleService.h>

class CSampleServiceImpl;
typedef IceUtil::Handle<CSampleServiceImpl> CSampleServiceImplPtr;

class CSampleServiceImpl : virtual public iBS::SeqSampleService
{
public:
	CSampleServiceImpl()
	{
	}

	virtual ~CSampleServiceImpl(){}

public:
	virtual void CreateBamToBinCountSamples_async(const ::iBS::AMD_SeqSampleService_CreateBamToBinCountSamplesPtr&,
		const ::iBS::BamToBinCountSampleInfoVec&,
		const Ice::Current&);

	virtual ::iBS::BamToBinCountSampleInfo GetBlankBamToBinCountSample(const Ice::Current&);

	virtual ::Ice::Int GetRefDataFromBamFile(const ::std::string&,
		::iBS::StringVec&,
		::iBS::LongVec&,
		const Ice::Current&);

	virtual ::Ice::Int GetRefBinRanges(const ::iBS::BamToBinCountInfo&,
		::iBS::LongVec&,
		::iBS::LongVec&,
		const Ice::Current&);

	virtual ::iBS::BamToBinCountInfo GetBlankBamToBinCountInfo(const Ice::Current&);

	virtual void GetHighCountBins_async(const ::iBS::AMD_SeqSampleService_GetHighCountBinsPtr&,
		const ::iBS::BamToBinCountInfo&,
		::Ice::Int,
		const Ice::Current&);

	virtual void GetBasePairCountsInBins_async(const ::iBS::AMD_SeqSampleService_GetBasePairCountsInBinsPtr&,
		const ::iBS::BamToBinCountInfo&,
		const ::iBS::LongVec&,
		const Ice::Current&);
};
#endif
