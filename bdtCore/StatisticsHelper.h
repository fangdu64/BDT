
#ifndef __StatisticsHelper_h__
#define __StatisticsHelper_h__


#include <IceUtil/IceUtil.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <IceUtil/Time.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>

class CStatisticsHelper
{
public:
	CStatisticsHelper()
	{
	}
	
	static Ice::Double GetCriticalFStatistics(Ice::Double bgDF, Ice::Double wgDF, Ice::Double pvalue);

	//multiple rows
	static bool GetOneWayANOVA(const ::Ice::Double* Y, Ice::Long colCnt, Ice::Long featureIdxFrom, Ice::Long featureIdxTo, 
		const iBS::IntVecVec& conditionSampleIdxs, ::Ice::Double* FStatistics);

	static Ice::Double GetOneWayANOVA(const ::Ice::Double* Y, const iBS::IntVecVec& groupColIdxs);


};

#endif
