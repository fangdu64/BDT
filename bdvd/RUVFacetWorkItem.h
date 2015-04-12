#ifndef __RUVFacetWorkItem_h__
#define __RUVFacetWorkItem_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <FCDCentralService.h>
#include <FeatureValueWorker.h>
#include <FeatureValueWorkItem.h>

class CRUVBuilder;

namespace RUVs
{

class CRebuildRUVsModel : public  FeatureValueWorkItemBase
{
public:
	CRebuildRUVsModel(
		CRUVBuilder& ruvBuilder, ::Ice::Int  threadCnt, ::Ice::Long ramMb, Ice::Long taskID)
		:m_ruvBuilder(ruvBuilder), m_threadCnt(threadCnt), m_ramMb(ramMb), m_taskID(taskID)
	{
	}

	virtual ~CRebuildRUVsModel();
	virtual void DoWork();
	virtual void CancelWork();
private:
	CRUVBuilder& m_ruvBuilder;
	::Ice::Int  m_threadCnt;
    ::Ice::Long m_ramMb;
	Ice::Long m_taskID;
};

class CSetActiveK : public  FeatureValueWorkItemBase
{
public:
	CSetActiveK(const ::iBS::AMD_FcdcRUVService_SetActiveKPtr& cb, ::Ice::Int k,
		::Ice::Int extW,
		CRUVBuilder& ruvBuilder)
		:m_cb(cb), m_k(k), m_extW(extW), m_ruvBuilder(ruvBuilder)
	{
	}

	virtual ~CSetActiveK();
	virtual void DoWork();
	virtual void CancelWork();
private:
	::iBS::AMD_FcdcRUVService_SetActiveKPtr m_cb;
	::Ice::Int m_k;
	::Ice::Int m_extW;
	CRUVBuilder& m_ruvBuilder;
};

class CDecomposeVariance : public  FeatureValueWorkItemBase
{
public:
	CDecomposeVariance(
		const ::iBS::AMD_FcdcRUVService_DecomposeVariancePtr& cb,
		CRUVBuilder& ruvBuilder, const ::iBS::IntVec&  ks,
		const ::iBS::IntVec& extWs,
		const ::iBS::IntVecVec& wtVecIdxs,
		::Ice::Long featureIdxFrom,
		::Ice::Long featureIdxTo,
		::Ice::Int  threadCnt, ::Ice::Long ramMb, const ::std::string& outfile )
		:m_cb(cb), m_ruvBuilder(ruvBuilder), m_ks(ks), m_extWs(extWs), m_wtVecIdxs(wtVecIdxs),
		m_featureIdxFrom(featureIdxFrom), m_featureIdxTo(featureIdxTo), 
		m_threadCnt(threadCnt), m_ramMb(ramMb), m_outfile(outfile)
	{
	}

	virtual ~CDecomposeVariance();
	virtual void DoWork();
	virtual void CancelWork();
private:
	::iBS::AMD_FcdcRUVService_DecomposeVariancePtr m_cb;
	CRUVBuilder& m_ruvBuilder;
	::iBS::IntVec  m_ks;
	::iBS::IntVec  m_extWs;
	::iBS::IntVecVec m_wtVecIdxs;
	::Ice::Long m_featureIdxFrom;
	::Ice::Long m_featureIdxTo;
	::Ice::Int  m_threadCnt;
	::Ice::Long m_ramMb;
	::std::string m_outfile;

};


class CGetDoublesColumnVector : public ::Original::CGetDoublesColumnVector
{

public:
		CGetDoublesColumnVector(
			::iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			CRUVBuilder& ruvBuilder)
			: ::Original::CGetDoublesColumnVector(foi, cb, observerID, featureIdxFrom, featureIdxTo),
			m_ruvBuilder(ruvBuilder)
		{
		}

		virtual ~CGetDoublesColumnVector();
	    virtual void DoWork();

private:
	CRUVBuilder& m_ruvBuilder;
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
			CRUVBuilder& ruvBuilder)
			: ::Original::CGetDoublesRowMatrix(foi, cb,observerID,featureIdxFrom,featureIdxTo),
			m_ruvBuilder(ruvBuilder)
		{
		}

		virtual ~CGetDoublesRowMatrix();
	    virtual void DoWork();
private:
	CRUVBuilder& m_ruvBuilder;
};


class CGetRowMatrix : public  ::Original::CGetRowMatrix
{

public:
	CGetRowMatrix(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::AMD_FcdcReadService_GetRowMatrixPtr& cb,
		const ::iBS::IntVec& observerIDs,
		::Ice::Long featureIdxFrom,
		::Ice::Long featureIdxTo,
		CRUVBuilder& ruvBuilder,
		iBS::RowAdjustEnum rowAdjust = iBS::RowAdjustNone)
		: ::Original::CGetRowMatrix(fois, cb, observerIDs, featureIdxFrom, featureIdxTo, rowAdjust),
		m_ruvBuilder(ruvBuilder)
	{
		
	}

	virtual ~CGetRowMatrix();
	virtual void DoWork();
private:
	CRUVBuilder& m_ruvBuilder;
};

class CSampleRowMatrix : public  ::Original::CSampleRowMatrix
{

public:
	CSampleRowMatrix(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::AMD_FcdcReadService_SampleRowMatrixPtr& cb,
		const ::iBS::IntVec& observerIDs,
		const ::iBS::LongVec& featureIdxs,
		CRUVBuilder& ruvBuilder,
		iBS::RowAdjustEnum rowAdjust = iBS::RowAdjustNone)
		: ::Original::CSampleRowMatrix(fois, cb, observerIDs, featureIdxs, rowAdjust),
		m_ruvBuilder(ruvBuilder)
	{}

	virtual ~CSampleRowMatrix();
	virtual void DoWork();
private:
	CRUVBuilder& m_ruvBuilder;
};


}

#endif
