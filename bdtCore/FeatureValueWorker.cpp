#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <FeatureValueWorker.h>
#include <algorithm>    // std::copy
#include <GlobalVars.h>
#include <FeatureValueRAM.h>
#include <FeatureValueStoreMgr.h>
#include <FeatureObserverDB.h>
#include <ObserverStatsDB.h>
#include <CommonHelper.h>

CForceLoadInRAM::~CForceLoadInRAM()
{

}

void
CForceLoadInRAM::DoWork()
{
#if (defined(_MSC_VER) && (_MSC_VER >= 1600))
	std::pair<const Ice::Double*, const Ice::Double*> ret(static_cast<const Ice::Double*>(nullptr), static_cast<const Ice::Double*>(nullptr));
#else
	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);
#endif

	//m_foi not rolled out
	Ice::Long foiStoreDomainSize = m_foi->ObserverGroupSize*m_foi->DomainSize;
	Ice::Long foiDomainSize = m_foi->DomainSize;
	Ice::Int foiObserverID=m_observerID;
	Ice::Int foiStoreObserverID=m_foi->ObserverGroupSize>1? m_foi->ObserverGroupID:m_observerID;

	::Ice::Long s_featureIdxFrom = m_featureIdxFrom * m_foi->ObserverGroupSize; //index in store
	::Ice::Long s_featureIdxTo = m_featureIdxTo * m_foi->ObserverGroupSize;     //index in store
	
	if(foiStoreDomainSize <= CGlobalVars::get()->theMaxFeatureValueCntDouble)
	{
		CFeatureValueRAMDouble* pRAM=CGlobalVars::get()->theFeatureValueRAMDobule;

		//not in RAM
		//in RAM or in Store, all index are rolled out
		m_foi->DomainSize=foiStoreDomainSize; //convert to store size
		m_foi->ObserverID=foiStoreObserverID;
		pRAM->GetFeatureValuesLoadAllToRAM(
				m_foi, foiStoreObserverID, 
				s_featureIdxFrom, s_featureIdxTo, ret);
		m_foi->ObserverID=foiObserverID;
		m_foi->DomainSize=foiDomainSize; //convert back
	}
	else
	{
		//use batched RAM
	}
}

void
CForceLoadInRAM::CancelWork()
{
}

///////////////////////////////////////////////////////////////////////////////////

Original::CGetDoublesColumnVector::~CGetDoublesColumnVector()
{

}

void Original::CGetDoublesColumnVector::getRetValues(
	std::pair<const Ice::Double*, const Ice::Double*>& ret,
	::IceUtil::ScopedArray<Ice::Double>&  retValues)
{
	Ice::Long colCnt=1; //just one column
	Ice::Long rowCnt=m_featureIdxTo-m_featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	Ice::Long destColCnt=colCnt; //also 1
	Ice::Long copyCnt=rowCnt;
	Ice::Long srcColCnt = m_foi->ObserverGroupSize;
	Ice::Long srcColIdx= m_foi->IdxInObserverGroup;

	Ice::Int foiObserverID=m_foi->ObserverID;
	Ice::Int foiStoreObserverID=m_foi->ObserverGroupSize>1? m_foi->ObserverGroupID:foiObserverID;
	Ice::Long foiStoreDomainSize = m_foi->ObserverGroupSize*m_foi->DomainSize;
	Ice::Long foiDomainSize = m_foi->DomainSize;
	Ice::Long s_featureIdxFrom = m_featureIdxFrom * m_foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = m_featureIdxTo * m_foi->ObserverGroupSize;     //index in store

	
	if(foiStoreDomainSize <= CGlobalVars::get()->theMaxFeatureValueCntDouble)
	{
		//all values fit in continues RAM

		CFeatureValueRAMDouble* pRAM=CGlobalVars::get()->theFeatureValueRAMDobule;
		::Ice::Double* values = pRAM->GetValuesInRAM(foiStoreObserverID);
		if(values)
		{
			//already in RAM, values points to the first value, i.e., s_featureIdx=0
			//in RAM or in Store, all index are rolled out,
			Ice::Long srcOffsetIdx = s_featureIdxFrom+srcColIdx;
			Ice::Double *srcValues = values+ srcOffsetIdx;

			if(srcColCnt==1)
			{
				//column vector to column vector
				//directly return continues address
				ret.first = srcValues;
				ret.second= srcValues+totalValueCnt;
			}
			else
			{
				//src not in continues RAM, need allocate and copy to continues RAM
				retValues.reset(new Ice::Double[totalValueCnt]);
				if(retValues.get()==0)
				{
					return;
				}

				::Ice::Double* destValues=retValues.get();

				CFeatureValueHelper::GappedCopy(srcValues,srcColCnt,
					destValues,destColCnt,copyCnt);

				ret.first = retValues.get();
				ret.second= retValues.get()+totalValueCnt;
			}
		}
		else if(m_foi->GetPolicy==iBS::FeatureValueGetPolicyGetFromRAM)
		{
			//need to load all values into RAM first
			m_foi->DomainSize=foiStoreDomainSize; //convert to store size
			m_foi->ObserverID=foiStoreObserverID;
			pRAM->GetFeatureValuesLoadAllToRAM(
				m_foi, foiStoreObserverID, 
				s_featureIdxFrom, s_featureIdxTo, ret);
			m_foi->ObserverID=foiObserverID; 
			m_foi->DomainSize=foiDomainSize; //convert back

			if(srcColCnt==1)
			{
				//do nothing, ret already pointed to right address
				//not writable, therefore retValues has no associated RAM
			}
			else if(srcColCnt>1)
			{
				//ret pointing to the 1st column data, need to move to right column and do gapped copy
				//allocate new RAM
				retValues.reset(new Ice::Double[totalValueCnt]);
				if(retValues.get()==0)
				{
					return;
				}

				::Ice::Double *srcValues = const_cast<Ice::Double*>(ret.first) + srcColIdx;
				::Ice::Double *destValues=retValues.get();

				CFeatureValueHelper::GappedCopy(srcValues,srcColCnt,
					destValues,destColCnt,copyCnt);

				ret.first = retValues.get();
				ret.second= retValues.get()+totalValueCnt;
			}

		}
		else if(m_foi->GetPolicy==iBS::FeatureValueGetPolicyGetForOneTimeRead)
		{
			//anyway the retValues should be writalbe
			//not in RAM, one time read, load required only
			m_foi->DomainSize=foiStoreDomainSize; //convert to store size
			m_foi->ObserverID=foiStoreObserverID; 
			::IceUtil::ScopedArray<Ice::Double>  storeValues(
				CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
					m_foi,s_featureIdxFrom,s_featureIdxTo));
			m_foi->ObserverID=foiObserverID; 
			m_foi->DomainSize=foiDomainSize; //convert back

			if(storeValues.get()==0)
			{
				return;
			}

			if(srcColCnt==1)
			{
				//same size, ready to send to wire, let retValues to handle RAM delete
				retValues.swap(storeValues);
				ret.first = retValues.get();
				ret.second= retValues.get()+totalValueCnt;
			}
			else
			{
				//allocate new continues RAM, writable
				retValues.reset(new Ice::Double[totalValueCnt]);
				if(retValues.get()==0)
				{
					return;
				}
				::Ice::Double *srcValues = storeValues.get() + srcColIdx;
				::Ice::Double *destValues=retValues.get();

				CFeatureValueHelper::GappedCopy(srcValues,srcColCnt,
					destValues,destColCnt,copyCnt);

				ret.first = retValues.get();
				ret.second= retValues.get()+totalValueCnt;
			}

		}
		else
		{
			return;

		}
		
	}
	else
	{
		//anyway the retValues should be writalbe
		//not in RAM, one time read, load required only
		m_foi->DomainSize=foiStoreDomainSize; //convert to store size
		m_foi->ObserverID=foiStoreObserverID; 
		::IceUtil::ScopedArray<Ice::Double>  storeValues(
			CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
				m_foi,s_featureIdxFrom,s_featureIdxTo));
		m_foi->ObserverID=foiObserverID; 
		m_foi->DomainSize=foiDomainSize; //convert back

		if(storeValues.get()==0)
		{
			return;
		}

		if(srcColCnt==1)
		{
			//same size, ready to send to wire, let retValues to handle RAM delete
			retValues.swap(storeValues);
			ret.first = retValues.get();
			ret.second= retValues.get()+totalValueCnt;
		}
		else
		{
			//allocate new continues RAM, writable
			retValues.reset(new Ice::Double[totalValueCnt]);
			if(retValues.get()==0)
			{
				return;
			}
			::Ice::Double *srcValues = storeValues.get() + srcColIdx;
			::Ice::Double *destValues=retValues.get();

			CFeatureValueHelper::GappedCopy(srcValues,srcColCnt,
				destValues,destColCnt,copyCnt);

			ret.first = retValues.get();
			ret.second= retValues.get()+totalValueCnt;
		}
	}
}

void
Original::CGetDoublesColumnVector::DoWork()
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
	}
	else
	{
		m_cb->ice_response(1,ret); 
	}
}

void
Original::CGetDoublesColumnVector::CancelWork()
{
	m_cb->ice_exception();
}

//////////////////////////////////////////////////////////////////////////////

CSetRAMDoublesColumnVector::CSetRAMDoublesColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetDoublesColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values)
			:m_foi(foi),m_cb(cb),
			m_observerID(observerID),m_featureIdxFrom(featureIdxFrom),
			m_featureIdxTo(featureIdxTo)
{
	//values are owned by caller, need to copy
	m_values.reset(new ::Ice::Double[featureIdxTo-featureIdxFrom]);
	std::copy(values.first, values.second, m_values.get());
}

CSetRAMDoublesColumnVector::~CSetRAMDoublesColumnVector()
{

}

void
CSetRAMDoublesColumnVector::DoWork()

{
	if(m_foi->DomainSize <= CGlobalVars::get()->theMaxFeatureValueCntDouble)
	{
		CFeatureValueRAMDouble* pRAM=CGlobalVars::get()->theFeatureValueRAMDobule;

		std::pair<const Ice::Double*, const Ice::Double*> values(
			m_values.get(), m_values.get()+m_featureIdxTo-m_featureIdxFrom);

		pRAM->SetFeatureValuesInRAM(
				m_foi, m_observerID, 
				m_featureIdxFrom, m_featureIdxTo,  values);

		m_cb->ice_response(1);

		//now save updated content to the storage
		CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
			m_foi,m_featureIdxFrom,m_featureIdxTo,values);
	}
	else
	{
		//handling splited RAM

		std::pair<const Ice::Double*, const Ice::Double*> values(
			m_values.get(), m_values.get()+m_featureIdxTo-m_featureIdxFrom);

		//now save updated content to the storage
		CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
			m_foi,m_featureIdxFrom,m_featureIdxTo,values);

		m_cb->ice_response(0);
	}

}

void
CSetRAMDoublesColumnVector::CancelWork()

{
	m_cb->ice_exception();
}

//////////////////////////////////////////////////////////////////////////////////


CSetToStoreDoublesColumnVector::CSetToStoreDoublesColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetDoublesColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values)
			:m_foi(foi),m_cb(cb),
			m_observerID(observerID),m_featureIdxFrom(featureIdxFrom),
			m_featureIdxTo(featureIdxTo)
{
	//values are owned by caller, need to copy
	m_values.reset(new ::Ice::Double[featureIdxTo-featureIdxFrom]);
	std::copy(values.first, values.second, m_values.get());
}

CSetToStoreDoublesColumnVector::~CSetToStoreDoublesColumnVector()
{

}

void
CSetToStoreDoublesColumnVector::DoWork()

{
	if(m_foi->DomainSize <= CGlobalVars::get()->theMaxFeatureValueCntDouble)
	{
		CFeatureValueRAMDouble* pRAM=CGlobalVars::get()->theFeatureValueRAMDobule;

		std::pair<const Ice::Double*, const Ice::Double*> values(
			m_values.get(), m_values.get()+m_featureIdxTo-m_featureIdxFrom);

		pRAM->UpdateFeatureValuesIfLoaded(m_foi, m_observerID, 
				m_featureIdxFrom, m_featureIdxTo,  values);
		
		m_cb->ice_response(1);

		//now save updated content to the storage
		CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
			m_foi,m_featureIdxFrom,m_featureIdxTo,values);
	}
	else
	{
		//handling splited RAM
		m_cb->ice_response(0);
	}

}

void
CSetToStoreDoublesColumnVector::CancelWork()

{
	m_cb->ice_exception();
}

///////////////////////////////////////////////////////////////////////////

Original::CGetDoublesRowMatrix::~CGetDoublesRowMatrix()
{

}

void Original::CGetDoublesRowMatrix::getRetValues(Ice::Double*  retValues)
{
	//ret always pointing to continues RAM that is ready to send to wire
	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);

	//retValues manage newly allocated mem (if retValues.get() is not null), exclusively for this call,
	//therefore, can be in-place editing without effecting original value
	::IceUtil::ScopedArray<Ice::Double>  values;
	
	getRetValues(ret,values);
	//copy to new array
	std::copy(ret.first,ret.second,retValues);
}

void Original::CGetDoublesRowMatrix::getRetValues(
	std::pair<const Ice::Double*, const Ice::Double*>& ret,
	::IceUtil::ScopedArray<Ice::Double>&  retValues)
{
	Ice::Long colCnt=m_foi->ObserverGroupSize;
	Ice::Long rowCnt=m_featureIdxTo-m_featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	Ice::Int foiObserverID=m_foi->ObserverID;
	Ice::Int foiStoreObserverID=m_foi->ObserverGroupSize>1? m_foi->ObserverGroupID:foiObserverID;
	Ice::Long foiStoreDomainSize = m_foi->ObserverGroupSize*m_foi->DomainSize;
	Ice::Long foiDomainSize = m_foi->DomainSize;
	Ice::Long s_featureIdxFrom = m_featureIdxFrom * m_foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = m_featureIdxTo * m_foi->ObserverGroupSize;     //index in store

	if(foiStoreDomainSize<= CGlobalVars::get()->theMaxFeatureValueCntDouble)
	{
		CFeatureValueRAMDouble* pRAM=CGlobalVars::get()->theFeatureValueRAMDobule;
		::Ice::Double* values = pRAM->GetValuesInRAM(foiStoreObserverID);
		if(values)
		{
			//already in RAM, continues, ready to send to wire
			ret.first = values+s_featureIdxFrom;
			ret.second= values+s_featureIdxTo;
		}
		else if(m_foi->GetPolicy==iBS::FeatureValueGetPolicyGetFromRAM)
		{
			//need to load all values into RAM first
			m_foi->DomainSize=foiStoreDomainSize; //convert to store size
			m_foi->ObserverID=foiStoreObserverID;
			pRAM->GetFeatureValuesLoadAllToRAM(
				m_foi, foiStoreObserverID, 
				s_featureIdxFrom, s_featureIdxTo, ret);
			m_foi->ObserverID=foiObserverID; 
			m_foi->DomainSize=foiDomainSize; //convert back

			//ret already pointed to right address, continues,ready to send to wire
		}
		else if(m_foi->GetPolicy==iBS::FeatureValueGetPolicyGetForOneTimeRead)
		{
			//not in RAM, one time read, load required only
			m_foi->DomainSize=foiStoreDomainSize; //convert to store size
			m_foi->ObserverID=foiStoreObserverID; 

			//retValues will hold a writable copy, ready to send to wire
			retValues.reset(
				CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
					m_foi,s_featureIdxFrom,s_featureIdxTo));
			m_foi->ObserverID=foiObserverID; 
			m_foi->DomainSize=foiDomainSize; //convert back
			if(retValues.get())
			{
				ret.first = retValues.get();
				ret.second= retValues.get()+totalValueCnt;
			}
		}

	}
	else
	{
		//not in RAM, load involved batches
		//currentlly the same as above
		m_foi->DomainSize=foiStoreDomainSize; //convert to store size
		m_foi->ObserverID=foiStoreObserverID; 
		//retValues will hold a writable copy, ready to send to wire
		retValues.reset(
			CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
				m_foi,s_featureIdxFrom,s_featureIdxTo));
		m_foi->ObserverID=foiObserverID; 
		m_foi->DomainSize=foiDomainSize; //convert back
		if(retValues.get())
		{
			ret.first = retValues.get();
			ret.second= retValues.get()+totalValueCnt;
		}
	}
}

void
Original::CGetDoublesRowMatrix::DoWork()
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
	}
	else
	{
		m_cb->ice_response(1,ret); 
	}
	
}

void
Original::CGetDoublesRowMatrix::CancelWork()
{
	m_cb->ice_exception();
}

/////////////////////////////////////////////////////////////////////////////

CSetRAMDoublesRowMatrix::CSetRAMDoublesRowMatrix(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetDoublesRowMatrixPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values)
			:m_foi(foi),m_cb(cb),
			m_observerID(observerID),m_featureIdxFrom(featureIdxFrom),
			m_featureIdxTo(featureIdxTo)
{
	//values are owned by caller, need to copy
	m_values.reset(new ::Ice::Double[featureIdxTo-featureIdxFrom]);
	std::copy(values.first, values.second, m_values.get());
}

CSetRAMDoublesRowMatrix::~CSetRAMDoublesRowMatrix()
{

}

void
CSetRAMDoublesRowMatrix::DoWork()

{
	if(m_foi->DomainSize <= CGlobalVars::get()->theMaxFeatureValueCntDouble)
	{
		CFeatureValueRAMDouble* pRAM=CGlobalVars::get()->theFeatureValueRAMDobule;

		std::pair<const Ice::Double*, const Ice::Double*> values(
			m_values.get(), m_values.get()+m_featureIdxTo-m_featureIdxFrom);

		pRAM->SetFeatureValuesInRAM(
				m_foi, m_observerID, 
				m_featureIdxFrom, m_featureIdxTo,  values);

		m_cb->ice_response(1);
		//now save updated content to the storage
		CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
				m_foi,m_featureIdxFrom, m_featureIdxTo, values);
	}
	else
	{
		//no save to RAM
		m_cb->ice_response(1);

		std::pair<const Ice::Double*, const Ice::Double*> values(
			m_values.get(), m_values.get()+m_featureIdxTo-m_featureIdxFrom);

		//now save updated content to the storage
		CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
			m_foi,m_featureIdxFrom,m_featureIdxTo,values);
	}

}

void
CSetRAMDoublesRowMatrix::CancelWork()

{
	m_cb->ice_exception();
}

//////////////////////////////////////////////////////////////////////////////////////////
CFeatureValueWorker::CFeatureValueWorker()
	:m_needNotify(false), m_shutdownRequested(false)
{
}


CFeatureValueWorker::~CFeatureValueWorker()
{
}

void
CFeatureValueWorker::run()
{
	
	bool bNeedExit=false;

	while(!bNeedExit)
	{
		
		{
			//entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while(!m_shutdownRequested)
			{
				if(m_pendingItems.size() == 0)
				{
					m_needNotify = true;
					m_monitor.wait();
				}
				if(!m_pendingItems.empty())
				{
					std::copy(m_pendingItems.begin(),m_pendingItems.end(),
						std::back_inserter(m_processingItems));
					m_pendingItems.clear();
					break;
				}
			}
			//if control request shutdown
			if(m_shutdownRequested)
			{

				for(FeatureValueWorkItemPtrLsit_T::const_iterator it= m_pendingItems.begin(); 
					it!= m_pendingItems.end(); ++it)
				{
					//cancle any outstanding requests.
					(*it)->CancelWork();
				}
				bNeedExit=true;

				//
				cout<<"CFeatureValueWorker m_shutdownRequested==true ..."<<endl; 
			}

			//leaving critical region
			m_needNotify=false;
		}

		//these items are inserted before shutdown request, need to finish these items anyway
		while(!m_processingItems.empty())
		{
			FeatureValueWorkItemPtr wi = m_processingItems.front();
			wi->DoWork();
			m_processingItems.pop_front();
		}
	}


}

void CFeatureValueWorker::AddWorkItem(const FeatureValueWorkItemPtr& item)
{
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);

	if(!m_shutdownRequested)
    {
		m_pendingItems.push_back(item);
		if(m_needNotify)
		{
			m_monitor.notify();
		}
	}
	else
	{
		//control already issued shutdown request, cancel it
		item->CancelWork();
	}
}

void CFeatureValueWorker::RequestShutdown()
{
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);

	m_shutdownRequested = true;
	if(m_needNotify)
	{
		m_monitor.notify();
	}
}

