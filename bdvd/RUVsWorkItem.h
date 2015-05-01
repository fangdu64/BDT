#ifndef __RUVsWorkItem_h__
#define __RUVsWorkItem_h__

#include <IceUtil/Mutex.h>
#include <IceUtil/ScopedArray.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
#include <RUVsCommonDefine.h>
#include <armadillo>
#include <RandomHelper.h>

class CRUVBuilder;
class CRUVVarDecmWorker;

class RUVsWorkItemBase: public IceUtil::Shared
{
public:
	RUVsWorkItemBase(){};
	virtual ~RUVsWorkItemBase(){};
    virtual void DoWork() = 0;
	virtual void CancelWork() = 0;
};


class CRUVsComputeABC :  public  RUVsWorkItemBase
{

public:
		CRUVsComputeABC(
			CRUVBuilder& ruvBuilder, int workerIdx, 
			::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, 
			::Ice::Double *Y, const iBS::ByteVec& controlFeatureFlags,
			::arma::mat& A, ::arma::mat& B, ::arma::mat& C, bool AequalB)
			:m_ruvBuilder(ruvBuilder), m_workerIdx(workerIdx),
			m_featureIdxFrom(featureIdxFrom),m_featureIdxTo(featureIdxTo),
			m_Y(Y), m_controlFeatureFlags(controlFeatureFlags), 
			m_A(A), m_B(B),m_C(C),
			m_AequalB(AequalB)
		{
		}

		virtual ~CRUVsComputeABC(){};
	    virtual void DoWork();
		virtual void CancelWork();
public:
	CRUVBuilder& m_ruvBuilder;
	int m_workerIdx;
	::Ice::Long m_featureIdxFrom;
	::Ice::Long m_featureIdxTo;
	::Ice::Double *m_Y;
	const iBS::ByteVec& m_controlFeatureFlags;
	//YcsYcsT
	::arma::mat& m_A;
	//YcscfYcscfT
	::arma::mat& m_B;
	//YcfYcscfT
	::arma::mat& m_C;

	bool m_AequalB;
};

class CRUVComputeRowANOVA :  public  RUVsWorkItemBase
{

public:
		CRUVComputeRowANOVA(
			CRUVBuilder& ruvBuilder, int workerIdx, 
			::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, 
			::Ice::Double *Y, ::Ice::Double *FStatistics)
			:m_ruvBuilder(ruvBuilder), m_workerIdx(workerIdx),
			m_featureIdxFrom(featureIdxFrom),m_featureIdxTo(featureIdxTo),
			m_Y(Y),m_FStatistics(FStatistics)
		{
		}

		virtual ~CRUVComputeRowANOVA(){};
	    virtual void DoWork();
		virtual void CancelWork();
public:
	CRUVBuilder& m_ruvBuilder;
	int m_workerIdx;
	::Ice::Long m_featureIdxFrom;
	::Ice::Long m_featureIdxTo;
	::Ice::Double *m_Y;
	::Ice::Double *m_FStatistics;
};

class CRUVVarDecompose : public  RUVsWorkItemBase
{

public:
	CRUVVarDecompose(
		CRUVBuilder& ruvBuilder, 
		CRUVVarDecmWorker *pVarDecmWorker,
		int workerIdx,
		RUVVarDecomposeStageEnum stage,
		::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo,
		::Ice::Double *Y, ::Ice::Double *rowMeans)
		:m_ruvBuilder(ruvBuilder), m_pVarDecmWorker(pVarDecmWorker), m_workerIdx(workerIdx),
		m_stage(stage),
		m_featureIdxFrom(featureIdxFrom), m_featureIdxTo(featureIdxTo),
		m_Y(Y), m_rowMeans(rowMeans)
	{
	}

	virtual ~CRUVVarDecompose(){};
	virtual void DoWork();
	virtual void CancelWork();
public:
	CRUVBuilder& m_ruvBuilder;
	CRUVVarDecmWorker* m_pVarDecmWorker;
	int m_workerIdx;
	RUVVarDecomposeStageEnum m_stage;
	::Ice::Long m_featureIdxFrom;
	::Ice::Long m_featureIdxTo;
	::Ice::Double *m_Y;
	::Ice::Double *m_rowMeans;
	
};

///////////////////////////////////////////////////
class CRUVgComputeA :  public  RUVsWorkItemBase
{

public:
		CRUVgComputeA(
			CRUVBuilder& ruvBuilder, int workerIdx, 
			::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, 
			::Ice::Double *Y, const iBS::ByteVec& controlFeatureFlags,
			::arma::mat& A,
			std::vector<::arma::mat>& As,
			CIndexPermutation& colIdxPermutation)
			:m_ruvBuilder(ruvBuilder), m_workerIdx(workerIdx),
			m_featureIdxFrom(featureIdxFrom),m_featureIdxTo(featureIdxTo),
			m_Y(Y), m_controlFeatureFlags(controlFeatureFlags), 
			m_A(A),
			m_As(As),
			m_colIdxPermutation(colIdxPermutation)
		{
		}

		virtual ~CRUVgComputeA(){};
	    virtual void DoWork();
		virtual void CancelWork();
public:
	CRUVBuilder& m_ruvBuilder;
	int m_workerIdx;
	::Ice::Long m_featureIdxFrom;
	::Ice::Long m_featureIdxTo;
	::Ice::Double *m_Y;
	const iBS::ByteVec& m_controlFeatureFlags;
	//YcfYcfT
	::arma::mat& m_A;
	std::vector<::arma::mat>& m_As;
	CIndexPermutation& m_colIdxPermutation;

};

///////////////////////////////////////////////////
class CRUVGetOutput : public  RUVsWorkItemBase
{
public:
	CRUVGetOutput(
		CRUVBuilder& ruvBuilder, int workerIdx,
		Ice::Long rowCnt, Ice::Double *Y, Ice::Double *rowMeans)
		:m_ruvBuilder(ruvBuilder), m_workerIdx(workerIdx),
		m_rowCnt(rowCnt), m_Y(Y), m_rowMeans(rowMeans)
	{
	}

	virtual ~CRUVGetOutput(){};
	virtual void DoWork();
	virtual void CancelWork();
public:
	CRUVBuilder& m_ruvBuilder;
	int m_workerIdx;
	Ice::Long m_rowCnt;
	Ice::Double *m_Y;
	Ice::Double *m_rowMeans;
};

#endif

