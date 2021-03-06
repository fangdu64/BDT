#include <bdt/bdtBase.h>
#include <RandomHelper.h>

CIndexPermutation::CIndexPermutation(boost::random::mt19937& state, int len)
	:m_shuffleFunc(state),
	m_idxs(len, 0)
{
	for (int i = 0; i < len; ++i) {
		m_idxs[i] = i;
	}
}

const iBS::IntVec& CIndexPermutation::Permutate() {
	std::random_shuffle(m_idxs.begin(), m_idxs.end(), m_shuffleFunc);
	return m_idxs;
}



CGroupedIndexPermutation::CGroupedIndexPermutation(boost::random::mt19937& state, const iBS::IntVecVec& groupedIdxs)
    :m_shuffleFunc(state), m_groupedIdxs(groupedIdxs)
{
    Ice::Long groupCnt = m_groupedIdxs.size();
    m_groupSizes.resize(groupCnt);

    for (int i = 0; i < groupCnt; ++i)
    {
        const iBS::IntVec& idxs = m_groupedIdxs[i];
        m_groupSizes[i] = (int) idxs.size();
        for (int j = 0; j < m_groupSizes[i]; j++)
        {
            m_idxs.push_back(idxs[j]);
        }
    }
}

const iBS::IntVec& CGroupedIndexPermutation::Permutate() {
    Ice::Long groupCnt = m_groupSizes.size();
    Ice::Long offset = 0;
    for (int i = 0; i < groupCnt; ++i)
    {
        const iBS::IntVec& idxs = m_groupedIdxs[i];
        for (int j = 0; j < m_groupSizes[i]; j++)
        {
            m_idxs[offset + j] = idxs[j];
        }

        std::random_shuffle(m_idxs.begin() + offset, m_idxs.begin() + offset + m_groupSizes[i], m_shuffleFunc);
        offset += m_groupSizes[i];
    }

    return m_idxs;
}