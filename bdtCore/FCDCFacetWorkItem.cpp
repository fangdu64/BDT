#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <FeatureValueWorker.h>
#include <FCDCFacetWorkItem.h>
#include <algorithm>    // std::copy
#include <GlobalVars.h>
#include <FeatureValueRAM.h>
#include <FeatureValueStoreMgr.h>
#include <FeatureObserverDB.h>
#include <ObserverStatsDB.h>
#include <CommonHelper.h>


//==========================================================
DivideByColumnSum::CGetDoublesColumnVector::~CGetDoublesColumnVector()
{

}

void
DivideByColumnSum::CGetDoublesColumnVector::DoWork()
{
	//ret always pointing to continues RAM that is ready to send to wire
	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);

	//retValues manage newly allocated mem (if retValues.get() is not null), exclusively for this call,
	//therefore, can be in-place editing without effecting original value
	::IceUtil::ScopedArray<Ice::Double>  retValues;
	
	getRetValues(ret,retValues);

	if(ret.first==ret.second)
	{
		m_cb->ice_exception();
		return;
	}

	Ice::Long colCnt=1;
	Ice::Long rowCnt=m_featureIdxTo-m_featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	if(retValues.get()==0)
	{
		//meaning ret pointed to original values, should not overwrite them
		retValues.reset(new ::Ice::Double[totalValueCnt]);
		if(!retValues.get())
		{
			::iBS::ArgumentException ex;
			ex.reason = "no mem available";
			m_cb->ice_exception(ex);
			return;
		}

		//copy to new array
		std::copy(ret.first,ret.second,retValues.get());
		ret.first = retValues.get();
		ret.second= retValues.get()+totalValueCnt;
	}

	Ice::Double factor=1.0/m_columnSum;
	const Ice::Double *srcValues = retValues.get();
	//source and dest are with the same address
	CFeatureValueHelper::GappedCopyWithMultiplyFactor(srcValues,1,
		retValues.get(),1,totalValueCnt,factor);

	m_cb->ice_response(1,ret);
}


///////////////////////////////////////////////////////////////////////


DivideByColumnSum::CGetDoublesRowMatrix::~CGetDoublesRowMatrix()
{

}

void
DivideByColumnSum::CGetDoublesRowMatrix::DoWork()
{
	//ret always pointing to continues RAM that is ready to send to wire
	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);

	//retValues manage newly allocated mem (if retValues.get() is not null), exclusively for this call,
	//therefore, can be in-place editing without effecting original value
	::IceUtil::ScopedArray<Ice::Double>  retValues;
	
	getRetValues(ret,retValues);

	if(ret.first==ret.second)
	{
		m_cb->ice_exception();
		return;
	}

	Ice::Long colCnt= m_foi->ObserverGroupSize;
	Ice::Long rowCnt=m_featureIdxTo-m_featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	if(retValues.get()==0)
	{
		//meaning ret pointed to original values, should not overwrite them

		retValues.reset(new ::Ice::Double[totalValueCnt]);
		if(!retValues.get())
		{
			::iBS::ArgumentException ex;
			ex.reason = "no mem available";
			m_cb->ice_exception(ex);
			return;
		}

		//copy to new array
		std::copy(ret.first,ret.second,retValues.get());
		ret.first = retValues.get();
		ret.second= retValues.get()+totalValueCnt;

	}

	//now ret should point to writable values

	//multiply by each column
	for(int i=0;i<colCnt;i++)
	{
		int destColIdx=i;
		::Ice::Double* destValues=const_cast< ::Ice::Double*>(ret.first)+destColIdx; // begin of the column data
		
		Ice::Long destColCnt=colCnt;
		Ice::Long copyCnt=rowCnt;
		Ice::Long srcColCnt = colCnt;
		Ice::Long srcColIdx= i;

		Ice::Double factor=1.0/m_columnSums[i];
		Ice::Long srcOffsetIdx=srcColIdx; //actually the same as destColIdx

		const Ice::Double *srcValues = ret.first+ srcOffsetIdx;
		//source and dest are with the same address
		CFeatureValueHelper::GappedCopyWithMultiplyFactor(srcValues,srcColCnt,
			destValues,destColCnt,copyCnt,factor);
	}

	m_cb->ice_response(1,ret); 
}
