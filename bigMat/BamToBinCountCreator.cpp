#include <bdt/bdtBase.h>
#include <IceUtil/Random.h>
#include <BamToBinCountCreator.h>
#include <GlobalVars.h>
#include <algorithm>     
#include <limits>

using namespace BamTools;

void CBamToBinCountCreator::ReadBinValues(Ice::Double* binValues)
{
	// open the BAM file
	BamReader reader;
	if (!reader.Open(m_sample.BamFile)) {
		cout << "Failed to open BAM file " << m_sample.BamFile << endl;
		return;
	}

	cout << IceUtil::Time::now().toDateTime() << " ReadBinValues " <<
		m_sample.SampleName << "[begin]"<< endl;

	// get header & reference information
	string header = reader.GetHeaderText();
	RefVector refs = reader.GetReferenceData();

	m_readCnt = 0;
	m_mappedCnt = 0;
	m_readInBinCnt = 0;
	// rip through the BAM file and convert each mapped entry to BED
	BamAlignment bam;
	while (reader.GetNextAlignment(bam)) {
		if (bam.IsMapped() == true) {
			m_mappedCnt++;
			ProcessBamAlignment(bam, refs, binValues);
		}
		m_readCnt++;
		if (m_readCnt % 10000000 == 0)
		{
			//report once per million alignments
			cout << IceUtil::Time::now().toDateTime() << " ReadBinValues " <<
				m_sample.SampleName << ": in bin " << m_readInBinCnt / 1000000 << "M, mapped " << 
				m_mappedCnt / 1000000 << "M, reads " << m_readCnt/1000000<<"M"<< endl;
		}
	}
	cout << IceUtil::Time::now().toDateTime() << " ReadBinValues [end] " <<
		m_sample.SampleName << ": in bin " << m_readInBinCnt<< ", mapped " <<
		m_mappedCnt<< ", reads " << m_readCnt<< endl;

	reader.Close();
}

void CBamToBinCountCreator::ProcessBamAlignment(
	const BamAlignment &bam, const RefVector &refs, Ice::Double* binValues)
{
	// get the unpadded (parm = false) end position based on the CIGAR
	unsigned int alignmentEnd = bam.GetEndPosition(false, false);
	//std::string chromName = refs.at(bam.RefID).RefName;
	Ice::Long midPos = bam.Position + alignmentEnd;
	midPos /= 2;
	Ice::Long binIdx = m_binMap.GetBinIdxByRefIDPos(bam.RefID, midPos);
	if (binIdx < 0)
	{
		return;
	}
	m_readInBinCnt++;
	binValues[binIdx]++;

	/*if (binValues[binIdx]>1000000)
	{
		int jjj = 0;
	}*/
}

////////////////////////////////////////////////////////////////////////

void CBamToBinBasePairCounts::ReadBasePairCounts(iBS::IntVecVec& bpCounts)
{
	m_maxReadLen=0;
	m_minReadLen=100000;
	Ice::Long totalBinCnt = m_binMap.GetTotalBinCount();
	m_binFlags.resize(totalBinCnt, 0);

	bpCounts.resize(m_binIdxs.size());
	
	for (int i = 0; i < m_binIdxs.size(); i++)
	{
		Ice::Long binIdx = m_binIdxs[i];
		m_binIdx2QueryIdx[binIdx] = i;
		m_binFlags[binIdx] = 1;
		bpCounts[i].resize(m_binWidth, 0);
	}

	// open the BAM file
	BamReader reader;
	if (!reader.Open(m_sample.BamFile)) {
		cout << "Failed to open BAM file " << m_sample.BamFile << endl;
		return;
	}

	cout << IceUtil::Time::now().toDateTime() << " ReadBinValues " <<
		m_sample.SampleName << "[begin]" << endl;

	// get header & reference information
	string header = reader.GetHeaderText();
	RefVector refs = reader.GetReferenceData();

	m_readCnt = 0;
	m_mappedCnt = 0;
	m_readInBinCnt = 0;
	// rip through the BAM file and convert each mapped entry to BED
	BamAlignment bam;
	while (reader.GetNextAlignment(bam)) {
		if (bam.IsMapped() == true) {
			m_mappedCnt++;
			ProcessBamAlignment(bam, refs, bpCounts);
		}
		m_readCnt++;
		if (m_readCnt % 10000000 == 0)
		{
			//report once per million alignments
			cout << IceUtil::Time::now().toDateTime() << " ReadBinValues " <<
				m_sample.SampleName << ": in bin " << m_readInBinCnt / 1000000 << "M, mapped " <<
				m_mappedCnt / 1000000 << "M, reads " << m_readCnt / 1000000 << "M" << endl;
		}
	}

	cout << IceUtil::Time::now().toDateTime() << " min read len = " <<
		m_minReadLen << ", max len=" << m_maxReadLen<<endl;

	cout << IceUtil::Time::now().toDateTime() << " ReadBinValues [end] " <<
		m_sample.SampleName << ": in bin " << m_readInBinCnt << ", mapped " <<
		m_mappedCnt << ", reads " << m_readCnt << endl;

	

	reader.Close();
}

void CBamToBinBasePairCounts::ProcessBamAlignment(
	const BamAlignment &bam, const RefVector &refs, iBS::IntVecVec& bpCounts)
{
	// get the unpadded (parm = false) end position based on the CIGAR
	unsigned int alignmentEnd = bam.GetEndPosition(false, false); //half open interval
	Ice::Long beginPos=bam.Position;
	Ice::Long endPos = alignmentEnd;
	

	Ice::Long bp_offset1 = 0;
	Ice::Long binIdx1 = m_binMap.GetBinIdxAndOffsetByRefIDPos(bam.RefID, beginPos, bp_offset1);
	Ice::Long bp_offset2 = 0;
	Ice::Long binIdx2 = m_binMap.GetBinIdxAndOffsetByRefIDPos(bam.RefID, endPos-1, bp_offset2);
	if (binIdx1 < 0 || binIdx2<0)
	{
		return;
	}
	bool notInBin = true;
	for (Ice::Long i = binIdx1; i <= binIdx2; i++)
	{
		if (m_binFlags[i] == 1)
		{
			notInBin = false;
			break;
		}
	}

	if (notInBin)
	{
		//not fall in any bin
		return;
	}
	m_readInBinCnt++;

	int readLen = (int)(endPos - beginPos);
	
	if (readLen < m_minReadLen)
	{
		m_minReadLen = readLen;
	}
	if (readLen > m_maxReadLen)
	{
		m_maxReadLen = readLen;
	}

	for (Ice::Long pos = beginPos; pos <endPos; pos++)
	{
		Ice::Long bp_offset = 0;
		Ice::Long binIdx = m_binMap.GetBinIdxAndOffsetByRefIDPos(bam.RefID, pos, bp_offset);
		if (m_binFlags[binIdx]>0)
		{
			int queryIdx = m_binIdx2QueryIdx[binIdx];
			iBS::IntVec& bps = bpCounts[queryIdx];
			bps[bp_offset]++;
		}
	}

}