#include <bdt/bdtBase.h>

#include <bdtUtil/BSEvidenceReader.h>
CBSEvidenceReader::CBSEvidenceReader(const iBS::StringVec& evTblFiles,
	const iBS::StringVec& refs, bool mergeWatsonCrick, int minMAPQ, int maxReadLen)
	:m_evTblFiles(evTblFiles),
	m_fileCnt((int)evTblFiles.size()),
	m_refs(refs),
	m_mergeWatsonCrick(mergeWatsonCrick),
	m_minMAPQ(minMAPQ),
	m_maxReadLen(maxReadLen),
	m_currFileIdx(0),
	m_pStream(NULL),
	m_lastRefIdx(0)
{

}

void CBSEvidenceReader::Initilize()
{
	if (m_pStream != NULL)
	{
		delete m_pStream;
		m_pStream = NULL;
	}

	m_lastRefIdx = 0;
	m_currFileIdx = -1;
	m_wfSeqcycleFlags.resize(m_maxReadLen, 0);
	m_wrSeqcycleFlags.resize(m_maxReadLen, 0);
	m_cfSeqcycleFlags.resize(m_maxReadLen, 0);
	m_crSeqcycleFlags.resize(m_maxReadLen, 0);

	m_hasLeftOver = false;
	m_preRefOffset = -1;
}
void CBSEvidenceReader::UnInitilize()
{
	if (m_pStream != NULL)
	{
		delete m_pStream;
		m_pStream = NULL;
	}
}

void CBSEvidenceReader::GetStudyIDReadID(
	const std::string& readname, int& studyID, Ice::Long& readID, Ice::Byte& mateID)
{
	studyID = 0;
	readID = 0;
	mateID = 0;
	//SRR568016.696288195.1_HWI-ST926:82:D0DN9ACXX:6:1103:17559:191549_length=45
	size_t pos_begin = 3;
	size_t found = readname.find_first_of('.', pos_begin);
	std::string strField;
	if (found == string::npos){
		return;
	}
	strField = readname.substr(pos_begin, found - pos_begin);
	studyID = atoi(strField.c_str());

	pos_begin = found + 1;
	found = readname.find_first_of('.', pos_begin);
	if (found == string::npos){
		return;
	}
	strField = readname.substr(pos_begin, found - pos_begin);
	readID = atol(strField.c_str());


	pos_begin = found + 1;
	found = readname.find_first_of('_', pos_begin);
	if (found == string::npos || (found - pos_begin) != 1){
		return;
	}
	strField = readname.substr(pos_begin, found - pos_begin);
	mateID = atoi(strField.c_str());
}

int CBSEvidenceReader::ReadEvidences(
	Ice::Long maxCnt, Ice::Long evIdxFrom, iBS::BSEvidenceVec& evs,
	Ice::Long& validCnt, Ice::Long& mapqFilteredCnt, Ice::Long& uniqueFilteredCnt)
{
	validCnt = 0;
	mapqFilteredCnt = 0;
	uniqueFilteredCnt = 0;
	int ret = 0; // processed all files
	if (m_hasLeftOver)
	{
		evs[evIdxFrom] = m_evLeftOver;
		validCnt++;
		m_hasLeftOver = false;
	}

	if (m_pStream == NULL && m_currFileIdx < m_fileCnt)
	{
		m_preRefOffset = -1;
		m_currFileIdx++;
		m_pStream = new std::ifstream(m_evTblFiles[m_currFileIdx].c_str());
	}
	else if (m_pStream == NULL && m_currFileIdx >= m_fileCnt)
	{
		return ret; //processed all files
	}
	
	std::string line;
	std::string refname;
	int refoffset;
	std::string readname;
	char allele;
	char waston;
	char isforawrd;
	int flag;
	char qual;
	int qual2;
	int readpos;
	int readlen;
	int alignscore;
	int mapq;
	
	ret = 1;// reached end of one file
	bool startOfNextSite = false;
	bool startOfNextRef = false;
	while (getline((*m_pStream), line))
	{
		std::istringstream iss(line);
		
		if (!(iss >> refname
			>> refoffset
			>> readname
			>> allele
			>> waston
			>> isforawrd
			>> flag
			>> qual
			>> qual2
			>> readpos
			>> readlen
			>> alignscore
			>> mapq))
		{
			//read error
		}

		if (refname == m_lastRef){
			m_ev.RefIdx = m_lastRefIdx;
		}
		else{
			m_ev.RefIdx= GetRefIdx(refname);
			if (m_ev.RefIdx < 0)
			{
				cout << IceUtil::Time::now().toDateTime() << " Unknown Ref: " << refname << endl;
			}
			m_lastRef = refname;
			if (m_lastRefIdx != m_ev.RefIdx)
			{
				m_lastRefIdx = m_ev.RefIdx;
				startOfNextRef = true;
			}
		}

		//if m_mergeWatsonCrick, before doing anything, convert the evidence to watson
		//the net result is that the reads are all from watson
		if (m_mergeWatsonCrick && (waston == '0'))
		{
			waston = '1';
			refoffset = refoffset - 1;
			if (allele == 'G'){
				allele = 'C';
			}
			else if (allele == 'A'){
				allele = 'T';
			}
			else{
				allele = 'N';
			}
		}

		refoffset = refoffset - 1; // 0 -based
		m_ev.RefOffset = refoffset; //still keep watson/crick
		
		m_ev.IsWatson = (waston == '1');
		m_ev.IsForward = (isforawrd == '1');
		m_ev.Qual = (Ice::Byte)qual;
		m_ev.SeqCycle = readpos;
		m_ev.MapQ = (Ice::Byte)mapq;

		//Methylation state
		if (m_ev.IsWatson && allele == 'C')
		{
			m_ev.MethyState = iBS::MethyStateM;
		}
		else if (m_ev.IsWatson && allele == 'T')
		{
			m_ev.MethyState = iBS::MethyStateU;
		}
		else if (m_ev.IsWatson == 0 && allele == 'G')
		{
			m_ev.MethyState = iBS::MethyStateM;
		}
		else if (m_ev.IsWatson == 0 && allele == 'A')
		{
			m_ev.MethyState = iBS::MethyStateU;
		}
		else
		{
			m_ev.MethyState = iBS::MethyStateUnknown;
		}

		if (!m_ev.IsWatson)
		{
			refoffset = refoffset - 1; //treat cpg site on watson and crick the same
		}
		//next cpg site
		if (m_preRefOffset > 0 && refoffset != m_preRefOffset)
		{
			//clear seqcycle flags
			std::fill(m_wfSeqcycleFlags.begin(), m_wfSeqcycleFlags.end(), 0);
			std::fill(m_wrSeqcycleFlags.begin(), m_wrSeqcycleFlags.end(), 0);
			std::fill(m_cfSeqcycleFlags.begin(), m_cfSeqcycleFlags.end(), 0);
			std::fill(m_crSeqcycleFlags.begin(), m_crSeqcycleFlags.end(), 0);
			startOfNextSite = true;
		}
		m_preRefOffset = refoffset;
		//unique reads
		if (m_ev.IsWatson && m_ev.IsForward)
		{
			if (m_wfSeqcycleFlags[m_ev.SeqCycle])
			{
				//read aligned to the same position
				uniqueFilteredCnt++;
				continue;
			}
			else
			{
				m_wfSeqcycleFlags[m_ev.SeqCycle] = 1;
			}
		}
		else if (m_ev.IsWatson && (!m_ev.IsForward))
		{
			if (m_wrSeqcycleFlags[m_ev.SeqCycle])
			{
				uniqueFilteredCnt++;
				continue;
			}
			else
			{
				m_wrSeqcycleFlags[m_ev.SeqCycle] = 1;
			}
		}
		else if ((!m_ev.IsWatson) && m_ev.IsForward)
		{
			if (m_cfSeqcycleFlags[m_ev.SeqCycle])
			{
				uniqueFilteredCnt++;
				continue;
			}
			else
			{
				m_cfSeqcycleFlags[m_ev.SeqCycle] = 1;
			}
		}
		else if ((!m_ev.IsWatson) && (!m_ev.IsForward))
		{
			if (m_crSeqcycleFlags[m_ev.SeqCycle])
			{
				uniqueFilteredCnt++;
				continue;
			}
			else
			{
				m_crSeqcycleFlags[m_ev.SeqCycle] = 1;
			}
		}

		if (m_minMAPQ > 0 && mapq < m_minMAPQ)
		{
			mapqFilteredCnt++;
			continue;
		}

		GetStudyIDReadID(readname, m_ev.StudyID, m_ev.ReadID, m_ev.MateID);
		if (startOfNextRef)
		{
			m_hasLeftOver = true;
			m_evLeftOver = m_ev;
			ret = 2; // reach this ref's end
			break;
		}
		else if (startOfNextSite && validCnt >= maxCnt)
		{
			m_hasLeftOver = true;
			m_evLeftOver = m_ev;
			ret = 3; // reach this batch's maxCnt
			break;
		}
		else
		{
			evs[evIdxFrom + validCnt] = m_ev;
			validCnt++;
		}
		
	}

	if (ret == 1)
	{
		delete m_pStream;
		m_pStream = NULL;
	}

	return ret;

}

int CBSEvidenceReader::GetRefIdx(const std::string& ref)
{
	for (int i = 0; i < m_refs.size(); i++)
	{
		if (ref == m_refs[i])
		{
			return i;
		}
	}
	return -1;
}