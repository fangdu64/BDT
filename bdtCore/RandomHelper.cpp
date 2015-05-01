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