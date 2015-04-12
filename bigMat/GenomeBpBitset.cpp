#include <bdt/bdtBase.h>
#include <IceUtil/Random.h>
#include <GenomeBpBitset.h>
#include <GlobalVars.h>
#include <algorithm>     
#include <limits>
#include <FeatureValueWorker.h>
#include <FeatureValueStoreMgr.h>
#include <FeatureObserverDB.h>
#include <FeatureValueWorkItem.h>

CGenomeBitSet::CGenomeBitSet(iBS::GenomeEnum genome)
{
	m_pFlags = NULL;
	//chr1 -> chr22,chrX, chrY, chrM
	Ice::Long refLens[] = { 
		249250621,
		243199373,
		198022430,
		191154276,
		180915260,
		171115067,
		159138663,
		146364022,
		141213431,
		135534747,
		135006516,
		133851895,
		115169878,
		107349540,
		102531392,
		90354753,
		81195210,
		78077248,
		59128983,
		63025520,
		48129895,
		51304566,
		155270560,
		59373566,
		16571 };
	m_refBpStartIdxs.resize(25);
	Ice::Long sum = 0;
	for (size_t i = 0; i < m_refBpStartIdxs.size(); i++)
	{
		m_refBpStartIdxs[i] = sum;
		sum += refLens[i];
	}

	m_pFlags = new std::bitset<Hg19_BpCnt>();
}

CGenomeBitSet::~CGenomeBitSet()
{
	if (m_pFlags != NULL)
	{
		delete m_pFlags;
	}
}

bool CGenomeBitSet::Get(int refIdx, Ice::Long bpIdx)
{
	Ice::Long idx = m_refBpStartIdxs[refIdx] + bpIdx;
	return (*m_pFlags)[idx];
}

void CGenomeBitSet::Set(int refIdx, Ice::Long bpIdx, bool flag)
{
	Ice::Long idx = m_refBpStartIdxs[refIdx] + bpIdx;
	(*m_pFlags)[idx] = flag;
}

void CGenomeBitSet::Reset()
{
	(*m_pFlags).reset();
}

Ice::Long CGenomeBitSet::GetCnt(int refIdx, Ice::Long bpIdxFrom, Ice::Long bpIdxTo, bool flag)
{
	Ice::Long cnt = 0;
	Ice::Long idxFrom = m_refBpStartIdxs[refIdx] + bpIdxFrom;
	Ice::Long idxTo = m_refBpStartIdxs[refIdx] + bpIdxTo;
	for (Ice::Long i = idxFrom; i < idxTo; i++)
	{
		if ((*m_pFlags)[i] == flag)
		{
			cnt++;
		}
	}
	return cnt;
}

void CGenomeBitSet::Set(int refIdx, Ice::Long bpIdxFrom, Ice::Long bpIdxTo, bool flag)
{
	Ice::Long idxFrom = m_refBpStartIdxs[refIdx] + bpIdxFrom;
	Ice::Long idxTo = m_refBpStartIdxs[refIdx] + bpIdxTo;
	for (Ice::Long i = idxFrom; i < idxTo; i++)
	{
		(*m_pFlags)[i] = flag;
	}
}