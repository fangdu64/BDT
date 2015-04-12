
#ifndef __GenomeBpBitset_h__
#define __GenomeBpBitset_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <IceUtil/Time.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <bitset>

const Ice::Long Hg19_BpCnt = 3200000000;

class CGenomeBitSet
{
public:
	CGenomeBitSet(iBS::GenomeEnum genome=iBS::GenomeHG19);
	virtual ~CGenomeBitSet();
public:
	void Reset();
	bool Get(int refIdx, Ice::Long bpIdx);
	void Set(int refIdx, Ice::Long bpIdx, bool flag);
	void Set(int refIdx, Ice::Long bpIdxFrom, Ice::Long bpIdxTo, bool flag);
	Ice::Long GetCnt(int refIdx, Ice::Long bpIdxFrom, Ice::Long bpIdxTo, bool flag);
private:
	std::bitset<Hg19_BpCnt> *m_pFlags;
	iBS::LongVec m_refBpStartIdxs;
};

#endif
