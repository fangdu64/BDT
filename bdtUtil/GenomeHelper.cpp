#include <bdt/bdtBase.h>
#include <IceUtil/Random.h>
#include <bdtUtil/GenomeHelper.h>
#include <algorithm>     
#include <limits>
#include <api/BamReader.h>
#include <api/BamAux.h>


CGenomeBinMap::CGenomeBinMap(const iBS::IntVec& refIDs, const iBS::StringVec& refNames,
	const iBS::LongVec& refLengths, Ice::Long binWidth)
	:m_refIDs(refIDs), m_refNames(refNames), m_refLengths(refLengths), m_binWidth(binWidth)
{
	Initialize();
}

void CGenomeBinMap::Reset(const iBS::IntVec& refIDs, const iBS::StringVec& refNames,
	const iBS::LongVec& refLengths, Ice::Long binWidth)
{
	m_refIDs = refIDs;
	m_refNames = refNames;
	m_refLengths = refLengths;
	m_binWidth = binWidth;

	m_refStartBinIdx.clear();
	m_totalBinCount=0;
	m_refID2LocalRefIdx.clear();
	m_refName2LocalRefIdx.clear();
	m_maxRefID = 0;

	Initialize();
}

void CGenomeBinMap::Initialize()
{
	size_t refCnt = m_refIDs.size();
	m_maxRefID = m_refIDs[0];

	for (int i = 0; i < refCnt; i++)
	{
		if (m_maxRefID < m_refIDs[i])
		{
			m_maxRefID = m_refIDs[i];
		}
	}

	m_refID2LocalRefIdx.resize(m_maxRefID+1, -1);
	m_refStartBinIdx.resize(refCnt, 0);
	m_totalBinCount = 0;
	for (int i = 0; i < refCnt; i++)
	{
		int refID = m_refIDs[i];
		m_refID2LocalRefIdx[refID] = i;
		m_refName2LocalRefIdx.insert(std::pair<std::string, int>(m_refNames[i], i));
		m_refStartBinIdx[i] = m_totalBinCount;
		Ice::Long refLen = m_refLengths[i];
		Ice::Long binCnt = 0;
		if (refLen%m_binWidth == 0)
		{
			binCnt = refLen / m_binWidth;
		}
		else
		{
			binCnt = refLen / m_binWidth + 1;
		}
		m_totalBinCount += binCnt;
	}
}

Ice::Long CGenomeBinMap::GetBinIdxAndOffsetByRefIDPos(int refID, Ice::Long pos, Ice::Long& offset) const
{
	if (refID > m_maxRefID)
	{
		offset = -1;
		return -1;
	}
	else
	{
		int refIdx = m_refID2LocalRefIdx[refID];
		if (refIdx < 0)
		{
			offset = -1;
			return -1;
		}
		return GetBinIdxAndOffsetByLocalRefIdxPos(refIdx, pos, offset);
	}
}

Ice::Long CGenomeBinMap::GetBinIdxByRefIDPos(int refID, Ice::Long pos) const
{
	if (refID > m_maxRefID)
	{
		return -1;
	}
	else
	{
		int refIdx = m_refID2LocalRefIdx[refID];
		if (refIdx < 0)
			return -1;
		return GetBinIdxByLocalRefIdxPos(refIdx, pos);
	}
}

Ice::Long CGenomeBinMap::GetBinIdxByLocalRefIdxPos(int refIdx, Ice::Long pos) const
{
	if (pos >= m_refLengths[refIdx])
	{
		//should not reach here
		return -1;
	}

	//pos is 0 based
	Ice::Long binIdx = pos / m_binWidth + m_refStartBinIdx[refIdx];
	return binIdx;
}

Ice::Long CGenomeBinMap::GetBinIdxAndOffsetByLocalRefIdxPos(int refIdx, Ice::Long pos, Ice::Long& offset) const
{
	if (pos >= m_refLengths[refIdx])
	{
		//should not reach here
		offset = -1;
		return -1;
	}

	//pos is 0 based
	Ice::Long binIdx = pos / m_binWidth + m_refStartBinIdx[refIdx];
	offset = pos % m_binWidth;
	return binIdx;
}

Ice::Long CGenomeBinMap::GetBinIdxByRefNamePos(const std::string& refName, Ice::Long pos) const
{
	Str2IntMap_T::const_iterator p = m_refName2LocalRefIdx.find(refName);
	if (p == m_refName2LocalRefIdx.end())
		return -1;
	int refIdx = p->second;
	return GetBinIdxByLocalRefIdxPos(refIdx, pos);
}

int CGenomeBinMap::GetRefIDByRefName(const std::string& refName) const
{
	Str2IntMap_T::const_iterator p = m_refName2LocalRefIdx.find(refName);
	if (p == m_refName2LocalRefIdx.end())
		return -1;
	int refIdx = p->second;
	return m_refIDs[refIdx];
}

////////////////////////////////////////////////////////////////////////////////////////
bool CGenomeHelper::GetRefDataFromBamFile(const ::std::string& bamFile,
	::iBS::IntVec& refIDs,
	::iBS::StringVec& refNames,
	::iBS::LongVec& refLengths, ::std::string& exreason)
{
	using namespace BamTools;

	// open the BAM file
	BamReader reader;
	if (!reader.Open(bamFile)) {
		exreason = "Failed to open BAM file: " + bamFile;
		return false;
	}

	RefVector refs = reader.GetReferenceData();
	refIDs.reserve(refs.size());
	for (int i = 0; i < refs.size(); i++)
	{
		refIDs.push_back(i);
		refNames.push_back(refs[i].RefName);
		refLengths.push_back(refs[i].RefLength);
	}

	reader.Close();

	return true;
}


bool 
CGenomeHelper::GetBinMapFromSampleInfo(const iBS::BamToBinCountSampleInfo& sample, 
	CGenomeBinMap& binMap, ::std::string& exreason)
{
	::iBS::IntVec fullRefIDs;
	::iBS::StringVec fullRefNames;
	::iBS::LongVec fullRefLengths;
	bool ret = GetRefDataFromBamFile(sample.BamFile, fullRefIDs, fullRefNames, fullRefLengths, exreason);
	if (!ret)
	{
		return false;
	}

	CGenomeBinMap fullMap(fullRefIDs, fullRefNames, fullRefLengths, sample.BinWidth);
	if (sample.RefNames.empty())
	{
		binMap = fullMap;
		return true;
	}
	
	::iBS::IntVec refIDs;
	::iBS::StringVec refNames;
	::iBS::LongVec refLengths;
	refIDs.reserve(sample.RefNames.size());
	refNames.reserve(sample.RefNames.size());
	refLengths.reserve(sample.RefNames.size());

	for (int i = 0; i < sample.RefNames.size(); i++)
	{
		const std::string& refName = sample.RefNames[i];
		int refID = fullMap.GetRefIDByRefName(refName);
		if (refID < 0)
		{
			exreason = refName + " not found in bam file";
			return false;
		}
		refIDs.push_back(refID);
		refNames.push_back(refName);
		refLengths.push_back(fullRefLengths[refID]);
	}

	binMap.Reset(refIDs, refNames, refLengths, sample.BinWidth);
	return true;
}