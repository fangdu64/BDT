#ifndef __BSEvidenceReader_h__
#define __BSEvidenceReader_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <IceUtil/Time.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <JointAMRService.h>
#include <iostream>
#include <fstream>

class CBSEvidenceReader
{
public:
	CBSEvidenceReader(const iBS::StringVec& evTblFiles, 
		const iBS::StringVec& refs, bool mergeWatsonCrick, int minMAPQ, int maxReadLen=150);

public:
	void Initilize();
	void UnInitilize();
	//evs is already allocated with maxCnt elements, newly fethche evidence will be filled staring from evIdxFrom
	int ReadEvidences(Ice::Long maxCnt, Ice::Long evIdxFrom, iBS::BSEvidenceVec& evs,
		Ice::Long& validCnt, Ice::Long& mapqFilteredCnt, Ice::Long& uniqueFilteredCnt);
private:
	int GetRefIdx(const std::string& ref);
	void GetStudyIDReadID(const std::string& readname, int& studyID, Ice::Long& readID, Ice::Byte& mateID);
private:
	const iBS::StringVec m_evTblFiles;
	const int m_fileCnt;
	const iBS::StringVec m_refs;
	const bool m_mergeWatsonCrick;
	const int m_minMAPQ;
	const int m_maxReadLen;
	int m_currFileIdx;
	std::ifstream *m_pStream;
	std::string m_lastRef;
	int m_lastRefIdx;
	iBS::BSEvidence m_ev;
	iBS::ByteVec m_wfSeqcycleFlags; //watson, forward
	iBS::ByteVec m_wrSeqcycleFlags; //watson, reverse
	iBS::ByteVec m_cfSeqcycleFlags; //crick, forward
	iBS::ByteVec m_crSeqcycleFlags; //crick, reverse
	iBS::BSEvidence m_evLeftOver;
	bool m_hasLeftOver;
	int m_preRefOffset;
};

#endif
