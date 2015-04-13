#ifndef __KMeanCommonDefine_h__
#define __KMeanCommonDefine_h__

#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <Ice/Ice.h>

class KMeansWorkItemBase;
typedef IceUtil::Handle<KMeansWorkItemBase> KMeansWorkItemPtr;

class CKMeanContractL2;
typedef IceUtil::Handle<CKMeanContractL2> CKMeanContractL2Ptr;

#endif

