#ifndef __BamToBinCountCreator_h__
#define __BamToBinCountCreator_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <IceUtil/Time.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <SampleService.h>
#include <api/BamReader.h>
#include <api/BamAux.h>
#include <bdtUtil/GenomeHelper.h>
using namespace BamTools;
class CBamToBinCountCreator
{
public:
	CBamToBinCountCreator(const iBS::BamToBinCountSampleInfo& sample, const CGenomeBinMap& binMap)
		:m_sample(sample), m_binMap(binMap)
	{
	}

public:
	void ReadBinValues(Ice::Double* binValues);
private:
	void ProcessBamAlignment(const BamAlignment &bam, const RefVector &refs, Ice::Double* binValues);
private:
	const iBS::BamToBinCountSampleInfo& m_sample;
	const CGenomeBinMap& m_binMap;
	Ice::Long m_readCnt;
	Ice::Long m_mappedCnt;
	Ice::Long m_readInBinCnt;
};

class CBamToBinBasePairCounts
{
public:
	CBamToBinBasePairCounts(const iBS::BamToBinCountSampleInfo& sample, const CGenomeBinMap& binMap, 
		const ::iBS::LongVec& binIdxs)
		:m_sample(sample), m_binMap(binMap), m_binIdxs(binIdxs)
	{
		m_binWidth = (int)m_binMap.GetBinWidth();
	}

public:
	void ReadBasePairCounts(iBS::IntVecVec& bpCounts);
private:
	void ProcessBamAlignment(const BamAlignment &bam, const RefVector &refs, iBS::IntVecVec& bpCounts);
private:
	const iBS::BamToBinCountSampleInfo& m_sample;
	const CGenomeBinMap& m_binMap;
	::iBS::LongVec m_binIdxs;
	Ice::Long m_readCnt;
	Ice::Long m_mappedCnt;
	Ice::Long m_readInBinCnt;
	typedef std::map< Ice::Long, int> Long2Int_T;
	Long2Int_T m_binIdx2QueryIdx;
	iBS::ByteVec m_binFlags;
	int m_binWidth;
	int m_maxReadLen;
	int m_minReadLen;
};


#endif
