#include <bdt/bdtBase.h>
#include <IceUtil/Random.h>
#include <bdtUtil/SortHelper.h>
#include <algorithm>     
#include <limits>

typedef std::pair<Ice::Long, Ice::Long> LongLongPair_T;
typedef std::vector<LongLongPair_T> LongLongPairVec_T;
bool LongLongComparator(const LongLongPair_T& l, const LongLongPair_T& r)
{
	return l.first < r.first;
}

typedef std::pair<Ice::Double, Ice::Long> DoubleLongPair_T;
typedef std::vector<DoubleLongPair_T> DoubleLongPairVec_T;
bool DoubleLongComparator(const DoubleLongPair_T& l, const DoubleLongPair_T& r)
{
	return l.first < r.first;
}

bool DoubleLongComparatorDesc(const DoubleLongPair_T& l, const DoubleLongPair_T& r)
{
	return l.first > r.first;
}

void CSortHelper::GetSortedValuesAndIdxs(
	const ::iBS::LongVec& values,
	::iBS::LongVec& sortedValues, ::iBS::LongVec& originalIdxs)
{
	Ice::Long cnt = (Ice::Long)values.size();
	LongLongPairVec_T pairs(cnt);
	for (Ice::Long i = 0; i < cnt; i++)
	{
		pairs[i].first = values[i];
		pairs[i].second = i;
	}
	std::sort(pairs.begin(), pairs.end(), LongLongComparator);

	for (Ice::Long i = 0; i < cnt; i++)
	{
		originalIdxs[i]=pairs[i].second;
		sortedValues[i] = pairs[i].first;
	}
}

void CSortHelper::GetSortedIdxs(
	const ::iBS::DoubleVec& values, ::iBS::LongVec& originalIdxs, bool desc)
{
	Ice::Long cnt = (Ice::Long)values.size();
	DoubleLongPairVec_T pairs(cnt);
	for (Ice::Long i = 0; i < cnt; i++)
	{
		pairs[i].first = values[i];
		pairs[i].second = i;
	}
	
	if (!desc)
	{
		std::sort(pairs.begin(), pairs.end(), DoubleLongComparator);
	}
	else
	{
		std::sort(pairs.begin(), pairs.end(), DoubleLongComparatorDesc);
	}

	for (Ice::Long i = 0; i < cnt; i++)
	{
		originalIdxs[i] = pairs[i].second;
	}
}

void CSortHelper::GetSortedIdxs(
	const ::Ice::Double* values, Ice::Long cnt, ::iBS::LongVec& originalIdxs, bool desc)
{
	DoubleLongPairVec_T pairs(cnt);
	for (Ice::Long i = 0; i < cnt; i++)
	{
		pairs[i].first = values[i];
		pairs[i].second = i;
	}

	if (!desc)
	{
		std::sort(pairs.begin(), pairs.end(), DoubleLongComparator);
	}
	else
	{
		std::sort(pairs.begin(), pairs.end(), DoubleLongComparatorDesc);
	}

	for (Ice::Long i = 0; i < cnt; i++)
	{
		originalIdxs[i] = pairs[i].second;
	}
}