
#ifndef __RowAdjustHelper_h__
#define __RowAdjustHelper_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <IceUtil/Time.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>

class CRowAdjustHelper
{
public:
	CRowAdjustHelper()
	{
	}
	static void Adjust(Ice::Double *values, Ice::Long rowCnt, Ice::Long colCnt, 
		iBS::RowAdjustEnum rowAdjust);

	static void AdjustByZeroMeanAndLength(Ice::Double *values,
		Ice::Long rowCnt, Ice::Long colCnt, Ice::Double constVal, Ice::Double len_square);

	static void ConvertFromLogToRawCnt(Ice::Double *values,
		Ice::Long rowCnt, Ice::Long colCnt);

	static void AddRowMeansBackToY(Ice::Long rowCnt, Ice::Long colCnt, 
		Ice::Double *Y, Ice::Double *rowMeans);

	static void ReArrangeByColIdxs(Ice::Double *values,
		Ice::Long rowCnt, Ice::Long colCnt, const iBS::IntVec& colIdxs, Ice::Double *retValues);

	static Ice::Long RowSelectedFlags(Ice::Double *values,
		Ice::Long rowCnt, Ice::Long colCnt, const iBS::RowSelection& selection, Ice::Byte *rowFlags);

	static Ice::Long RowSelectedFlags_RowMaxLargerThan(Ice::Double *values,
		Ice::Long rowCnt, Ice::Long colCnt, Ice::Double threshold, Ice::Byte *rowFlags);
};

#endif
