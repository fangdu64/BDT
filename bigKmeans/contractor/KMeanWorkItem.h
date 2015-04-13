#ifndef __KMeanWorkItem_h__
#define __KMeanWorkItem_h__

#include <KMeanCommonDefine.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <KMeanService.h>
#include <KMeanContract.h>

using namespace iBS;



class KMeansWorkItemBase : public IceUtil::Shared
{
	friend class CKMeanWorker;
public:
	KMeansWorkItemBase(
		const CKMeanContractL2Ptr& kmeanL2Ptr,
		int workerIdx,
		::Ice::Long workerFeatureIdxFrom, //global feature idx range for this worker
		::Ice::Long workerFeatureIdxTo,
		bool resetKCntsKSumsFirst)
		:m_kmeanL2Ptr(kmeanL2Ptr),
		m_workerIdx(workerIdx),
		m_workerFeatureIdxFrom(workerFeatureIdxFrom),
		m_workerFeatureIdxTo(workerFeatureIdxTo),
		m_resetKCntsKSumsFirst(resetKCntsKSumsFirst)
	{
	}
	virtual ~KMeansWorkItemBase(){};
    virtual void DoWork() = 0;
	virtual void CancelWork() {};

protected:
	CKMeanContractL2Ptr m_kmeanL2Ptr;
	int m_workerIdx;
	::Ice::Long m_workerFeatureIdxFrom; //global feature idx
	::Ice::Long m_workerFeatureIdxTo;   //global feature idx
	bool m_resetKCntsKSumsFirst;
};


class CEuclideanUpdateKCntsAndKSums : public  KMeansWorkItemBase
{

public:
		CEuclideanUpdateKCntsAndKSums(
			const CKMeanContractL2Ptr& kmeanL2Ptr,
			int workerIdx,
			::Ice::Long workerFeatureIdxFrom, //global feature idx range for this worker
			::Ice::Long workerFeatureIdxTo,
			bool resetKCntsKSumsFirst)
			: KMeansWorkItemBase(kmeanL2Ptr, workerIdx, workerFeatureIdxFrom, workerFeatureIdxTo, resetKCntsKSumsFirst)
		{
		}

		virtual ~CEuclideanUpdateKCntsAndKSums(){};
	    virtual void DoWork();
};


class CCorrelationUpdateKCntsAndKSums : public  KMeansWorkItemBase
{

public:
		CCorrelationUpdateKCntsAndKSums(
		const CKMeanContractL2Ptr& kmeanL2Ptr,
		int workerIdx,
		::Ice::Long workerFeatureIdxFrom, //global feature idx range for this worker
		::Ice::Long workerFeatureIdxTo,
		bool resetKCntsKSumsFirst)
		: KMeansWorkItemBase(kmeanL2Ptr, workerIdx, workerFeatureIdxFrom, workerFeatureIdxTo, resetKCntsKSumsFirst)
	{
	}

	virtual ~CCorrelationUpdateKCntsAndKSums(){};
	virtual void DoWork();
};

//////////////////////////////////////////////////////////////////////
//	KMeans++ Seeds
//////////////////////////////////////////////////////////////////////

class CEuclideanPPSeedComputeMinDistance : public  KMeansWorkItemBase
{

public:
	CEuclideanPPSeedComputeMinDistance(
		const CKMeanContractL2Ptr& kmeanL2Ptr,
		int workerIdx,
		::Ice::Long workerFeatureIdxFrom, //global feature idx range for this worker
		::Ice::Long workerFeatureIdxTo)
		: KMeansWorkItemBase(kmeanL2Ptr, workerIdx, workerFeatureIdxFrom, workerFeatureIdxTo, false)
	{
	}

	virtual ~CEuclideanPPSeedComputeMinDistance(){};
	virtual void DoWork();
};


class CCorrelationPPSeedComputeMinDistance : public  KMeansWorkItemBase
{

public:
	CCorrelationPPSeedComputeMinDistance(
		const CKMeanContractL2Ptr& kmeanL2Ptr,
		int workerIdx,
		::Ice::Long workerFeatureIdxFrom, //global feature idx range for this worker
		::Ice::Long workerFeatureIdxTo)
		: KMeansWorkItemBase(kmeanL2Ptr, workerIdx, workerFeatureIdxFrom, workerFeatureIdxTo, false)
	{
	}

	virtual ~CCorrelationPPSeedComputeMinDistance(){};
	virtual void DoWork();
};

class CUniformPPSeedComputeMinDistance : public  KMeansWorkItemBase
{

public:
	CUniformPPSeedComputeMinDistance(
		const CKMeanContractL2Ptr& kmeanL2Ptr,
		int workerIdx,
		::Ice::Long workerFeatureIdxFrom, //global feature idx range for this worker
		::Ice::Long workerFeatureIdxTo)
		: KMeansWorkItemBase(kmeanL2Ptr, workerIdx, workerFeatureIdxFrom, workerFeatureIdxTo, false)
	{
	}

	virtual ~CUniformPPSeedComputeMinDistance(){};
	virtual void DoWork();
};

#endif

