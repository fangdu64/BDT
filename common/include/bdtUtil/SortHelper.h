
#ifndef __SortHelper_h__
#define __SortHelper_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <IceUtil/Time.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>

class CSortHelper
{
public:
	CSortHelper()
	{
	}
	static void GetSortedValuesAndIdxs(
		const ::iBS::LongVec& values, 
		::iBS::LongVec& sortedValues, ::iBS::LongVec& originalIdxs);

	static void GetSortedIdxs(
		const ::iBS::DoubleVec& values,::iBS::LongVec& originalIdxs, bool desc=false);
	static void GetSortedIdxs(
		const ::Ice::Double* values, Ice::Long cnt, ::iBS::LongVec& originalIdxs, bool desc = false);
};

#endif
