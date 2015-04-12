#ifndef __BigMatrixFacetWorkItem_h__
#define __BigMatrixFacetWorkItem_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <FCDCentralService.h>
#include <FeatureValueWorker.h>
#include <FeatureValueWorkItem.h>

namespace BigMatrix
{

class CGetRowMatrix : public  ::Original::CGetRowMatrix
{

public:
	CGetRowMatrix(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::AMD_FcdcReadService_GetRowMatrixPtr& cb,
		const ::iBS::IntVec& observerIDs,
		::Ice::Long featureIdxFrom,
		::Ice::Long featureIdxTo,
		iBS::RowAdjustEnum rowAdjust = iBS::RowAdjustNone)
		: ::Original::CGetRowMatrix(fois, cb, observerIDs, featureIdxFrom, featureIdxTo, rowAdjust)
	{
		
	}

	virtual ~CGetRowMatrix();
	virtual void DoWork();
private:

};

class CSampleRowMatrix : public  ::Original::CSampleRowMatrix
{

public:
	CSampleRowMatrix(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::AMD_FcdcReadService_SampleRowMatrixPtr& cb,
		const ::iBS::IntVec& observerIDs,
		const ::iBS::LongVec& featureIdxs,
		iBS::RowAdjustEnum rowAdjust = iBS::RowAdjustNone)
		: ::Original::CSampleRowMatrix(fois, cb, observerIDs, featureIdxs, rowAdjust)
	{}

	virtual ~CSampleRowMatrix();

	virtual void DoWork();
private:

};

class CRecalculateObserverStats : public  FeatureValueWorkItemBase
{

public:
	CRecalculateObserverStats(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::IntVec& observerIDs,
		::Ice::Long ramMb,
		Ice::Long taskID)
		:m_fois(fois), m_observerIDs(observerIDs), m_ramMb(ramMb), m_taskID(taskID)
		{

		}

	virtual ~CRecalculateObserverStats();
	virtual void DoWork();
	virtual void CancelWork(){}

private:
	bool GetY(::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Double*  Y);

private:
	::iBS::FeatureObserverSimpleInfoVec m_fois;
	::iBS::IntVec	m_observerIDs;
	::Ice::Long m_ramMb;
	Ice::Long m_taskID;
};


}

#endif
