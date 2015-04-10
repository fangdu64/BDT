#ifndef __GenomeHelper_h__
#define __GenomeHelper_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <IceUtil/Time.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <SampleService.h>

class CGenomeBinMap
{
public:
	CGenomeBinMap(){}
	CGenomeBinMap(const iBS::IntVec& refIDs, const iBS::StringVec& refNames,
		const iBS::LongVec& refLengths, Ice::Long binWidth);


public:
	void Reset(const iBS::IntVec& refIDs, const iBS::StringVec& refNames,
		const iBS::LongVec& refLengths, Ice::Long binWidth);
	Ice::Long GetBinIdxByRefIDPos(int refID, Ice::Long pos) const;
	int GetRefIDByRefName(const std::string& refName) const;
	Ice::Long GetBinIdxByRefNamePos(const std::string& refName, Ice::Long pos) const;
	Ice::Long GetTotalBinCount() const{ return m_totalBinCount; }
	Ice::Long GetBinWidth() const{ return m_binWidth; }
	bool IsValid() { return !m_refIDs.empty(); }

	Ice::Long GetBinIdxAndOffsetByRefIDPos(int refID, Ice::Long pos, Ice::Long& offset) const;
private:
	void Initialize();
	Ice::Long GetBinIdxByLocalRefIdxPos(int refIdx, Ice::Long pos) const;
	Ice::Long GetBinIdxAndOffsetByLocalRefIdxPos(int refIdx, Ice::Long pos, Ice::Long& offset) const;
private:
	iBS::IntVec		m_refIDs;	//refIDs in alignment file such as BAM
	iBS::StringVec	m_refNames;
	iBS::LongVec	m_refLengths;
	Ice::Long		m_binWidth;
	iBS::LongVec	m_refStartBinIdx;
	Ice::Long		m_totalBinCount;
	iBS::IntVec		m_refID2LocalRefIdx;
	typedef std::map< std::string, int> Str2IntMap_T;
	Str2IntMap_T	m_refName2LocalRefIdx;
	int m_maxRefID;
};

class CGenomeHelper
{
public:
	CGenomeHelper()
	{
	}
public:
	static bool GetRefDataFromBamFile(const ::std::string& bamFile,
		::iBS::IntVec& refIDs,
		::iBS::StringVec& refNames,
		::iBS::LongVec& refLengths, ::std::string& exreason);

	static bool GetBinMapFromSampleInfo(const iBS::BamToBinCountSampleInfo& sample, 
		CGenomeBinMap& binMap, ::std::string& exreason);
};

#endif
