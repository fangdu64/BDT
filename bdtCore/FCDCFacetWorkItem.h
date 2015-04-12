#ifndef __FCDCFacetWorkItem_h__
#define __FCDCFacetWorkItem_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <FCDCentralService.h>
#include <FeatureValueWorker.h>
#include <FeatureValueWorkItem.h>


namespace DivideByColumnSum
{

class CGetDoublesColumnVector : public ::Original::CGetDoublesColumnVector
{

public:
		CGetDoublesColumnVector(
			::iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			::Ice::Double columnSum)
			: ::Original::CGetDoublesColumnVector(foi, cb, observerID, featureIdxFrom, featureIdxTo),
			m_columnSum(columnSum)
		{
		}

		virtual ~CGetDoublesColumnVector();
	    virtual void DoWork();

private:
	::Ice::Double m_columnSum;
};


class CGetDoublesRowMatrix : public  ::Original::CGetDoublesRowMatrix
{

public:
		CGetDoublesRowMatrix(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const iBS::DoubleVec&  columnSums)
			: ::Original::CGetDoublesRowMatrix(foi, cb,observerID,featureIdxFrom,featureIdxTo),
			m_columnSums(columnSums)
		{
		}

		virtual ~CGetDoublesRowMatrix();
	    virtual void DoWork();
private:
	iBS::DoubleVec  m_columnSums;
};

}

#endif
