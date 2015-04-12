#ifndef __ComputeWorkItem_h__
#define __ComputeWorkItem_h__

#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <ComputeCommonDefine.h>
#include <armadillo>

class CMultiThreadBuilder;

class ComputeWorkItemBase: public IceUtil::Shared
{
public:
	ComputeWorkItemBase(CMultiThreadBuilder& builder, int workerIdx)
		:m_builder(builder), m_workerIdx(workerIdx){};
	virtual ~ComputeWorkItemBase(){};
    virtual void DoWork() = 0;
	virtual void CancelWork() = 0;

public:
	CMultiThreadBuilder& m_builder;
	int m_workerIdx;
};


class CPearsonCorrelation :  public  ComputeWorkItemBase
{

public:
	CPearsonCorrelation(CMultiThreadBuilder& builder, int workerIdx, const ::iBS::RowSelection& rowSelection,
			const ::iBS::DoubleVec& Y, ::Ice::Long rowCnt, ::Ice::Long colCnt,
			const iBS::DoubleVec& colMeans,
			::arma::mat& A, ::arma::mat& colSumSquares)
			:ComputeWorkItemBase(builder, workerIdx), m_rowSelection(rowSelection), m_rowCnt(rowCnt),
			m_colCnt(colCnt), m_colMeans(colMeans),
			m_Y(Y), m_A(A), m_colSumSquares(colSumSquares)
		{
		}

		virtual ~CPearsonCorrelation(){};
	    virtual void DoWork();
		virtual void CancelWork();
public:
	const ::iBS::DoubleVec& m_Y;
	::Ice::Long m_rowCnt;
	::Ice::Long m_colCnt;
	const iBS::DoubleVec& m_colMeans;
	::arma::mat& m_A;
	::arma::mat& m_colSumSquares;
	const ::iBS::RowSelection& m_rowSelection;
};

//////////////////////////////////////////////////////////////
class CEuclideanDist : public  ComputeWorkItemBase
{

public:
	CEuclideanDist(CMultiThreadBuilder& builder, int workerIdx, const ::iBS::RowSelection& rowSelection,
		const ::iBS::DoubleVec& Y, ::Ice::Long rowCnt, ::Ice::Long colCnt,
		::arma::mat& A)
		:ComputeWorkItemBase(builder, workerIdx), m_rowSelection(rowSelection), m_rowCnt(rowCnt),
		m_colCnt(colCnt), m_Y(Y), m_A(A)
	{
	}

	virtual ~CEuclideanDist(){};
	virtual void DoWork();
	virtual void CancelWork();
public:
	const ::iBS::DoubleVec& m_Y;
	::Ice::Long m_rowCnt;
	::Ice::Long m_colCnt;
	::arma::mat& m_A;
	const ::iBS::RowSelection& m_rowSelection;
};

#endif

