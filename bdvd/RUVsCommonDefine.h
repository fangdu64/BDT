#ifndef __JointAMRCommonDefine_h__
#define __JointAMRCommonDefine_h__

#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>

class RUVsWorkItemBase;
typedef IceUtil::Handle<RUVsWorkItemBase> RUVsWorkItemPtr;

class CRUVBuilder;

enum RUVVarDecomposeStageEnum
{
	RUVVarDecomposeStageTopLevelOnly = 0,
	RUVVarDecomposeStageAfterRUVGrandMean = 1,
	RUVVarDecomposeStageToplevelAndLocus = 2
};

struct RUVVarDecomposeBatchParams
{
	RUVVarDecomposeStageEnum stage;
	::Ice::Long featureIdxFrom;
	::Ice::Long featureIdxTo;
	::Ice::Double	*rowMeans;
	::Ice::Double *Y;
	::iBS::DoubleVec& ssGrandMean;
	::iBS::DoubleVec& ssXb;
	::iBS::DoubleVec& ssWa;
	::iBS::DoubleVec& ssXbWa;

	::iBS::DoubleVec& xbGrandMean;
	::iBS::DoubleVec& ssXbTotalVar;
	::iBS::DoubleVec& ssXbBgVar;
	::iBS::DoubleVec& ssXbWgVar;
};


#endif

