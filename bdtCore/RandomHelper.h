
#ifndef __RandomHelper_h__
#define __RandomHelper_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/Time.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>

#include <algorithm>
#include <functional>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

// unction object returning a randomly chosen value of type convertible to std::iterator_traits<RandomIt>::difference_type in the interval [0,n) if invoked as r(n)
struct RandomShuffleFunc : std::unary_function<int, int>
{
	boost::random::mt19937& m_state;
	RandomShuffleFunc(boost::random::mt19937& state)
		: m_state(state) {
	}

	int operator()(int i) {
		// Contrary to common C++ usage uniform_int_distribution does not take a half-open range. Instead it takes a closed range.
		boost::random::uniform_int_distribution<> dist(0, i - 1);
		return dist(m_state);
	}
};

class CIndexPermutation
{
public:
	CIndexPermutation(boost::random::mt19937& state, int len);

	const iBS::IntVec& Permutate();

	const iBS::IntVec& GetIdxs() { return m_idxs; }

private:
	RandomShuffleFunc m_shuffleFunc;
	iBS::IntVec m_idxs;
};

class CGroupedIndexPermutation
{
public:
    CGroupedIndexPermutation(boost::random::mt19937& state, const iBS::IntVecVec& groupedIdxs);

    const iBS::IntVec& Permutate();

    const iBS::IntVec& GetIdxs() { return m_idxs; }

private:
    RandomShuffleFunc m_shuffleFunc;
    iBS::IntVec m_groupSizes;
    iBS::IntVec m_idxs;
};

#endif
