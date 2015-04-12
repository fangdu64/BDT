
#ifndef __CommonHelper_h__
#define __CommonHelper_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <IceUtil/Time.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>

class CFeatureValueHelper
{
public:
	CFeatureValueHelper()
	{
	}
	static void GappedCopy(
		const Ice::Double *srcValues, Ice::Long srcColCnt, 
		Ice::Double *destValues, Ice::Long destColCnt, Ice::Long copyCnt);

	static void GappedCopyWithMultiplyFactor(
		const Ice::Double *srcValues, Ice::Long srcColCnt, 
		Ice::Double *destValues, Ice::Long destColCnt, Ice::Long copyCnt,
		Ice::Double factor);

	static void GetDoubleVecInRAM(
		::iBS::FeatureObserverSimpleInfoPtr foi, 
		Ice::Double*  values, Ice::Long batchMb = 512);

	static void ExportMatrix(
		Ice::Double* values,
		Ice::Long rowCnt, Ice::Long colCnt,
		const std::string& outdir, int outID);
};

#endif
