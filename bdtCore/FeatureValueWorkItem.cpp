#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <FeatureValueWorker.h>
#include <FeatureValueWorkItem.h>
#include <algorithm>    // std::copy
#include <GlobalVars.h>
#include <FeatureValueRAM.h>
#include <FeatureValueStoreMgr.h>
#include <FeatureObserverDB.h>
#include <ObserverStatsDB.h>
#include <CommonHelper.h>
#include <ObserverIndexDB.h>
#include <bdtUtil/RowAdjustHelper.h>
//======================================================================

CSetRAMIntsColumnVector::CSetRAMIntsColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetIntsColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Int*, const ::Ice::Int*>& values)
			:m_foi(foi),m_cb(cb),
			m_observerID(observerID),m_featureIdxFrom(featureIdxFrom),
			m_featureIdxTo(featureIdxTo)
{
	//values are owned by caller, need to copy
	m_values.reset(new ::Ice::Double[featureIdxTo-featureIdxFrom]);
	std::copy(values.first, values.second, m_values.get());
}

CSetRAMIntsColumnVector::~CSetRAMIntsColumnVector()
{

}

void
CSetRAMIntsColumnVector::DoWork()

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
CSetRAMIntsColumnVector::CancelWork()

{
	m_cb->ice_exception();
}

//============================================


CSetToStoreIntsColumnVector::CSetToStoreIntsColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetIntsColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Int*, const ::Ice::Int*>& values)
			:m_foi(foi),m_cb(cb),
			m_observerID(observerID),m_featureIdxFrom(featureIdxFrom),
			m_featureIdxTo(featureIdxTo)
{
	//values are owned by caller, need to copy
	m_values.reset(new ::Ice::Double[featureIdxTo-featureIdxFrom]);
	std::copy(values.first, values.second, m_values.get());
}

CSetToStoreIntsColumnVector::~CSetToStoreIntsColumnVector()
{

}

void
CSetToStoreIntsColumnVector::DoWork()

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
CSetToStoreIntsColumnVector::CancelWork()

{
	m_cb->ice_exception();
}

//============================================================================

//======================================================================

CSetRAMBytesColumnVector::CSetRAMBytesColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetBytesColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*>& bytes,
			::iBS::ByteArrayContentEnum content,::iBS::ByteArrayEndianEnum endian)
			:m_foi(foi),m_cb(cb),
			m_observerID(observerID),m_featureIdxFrom(featureIdxFrom),
			m_featureIdxTo(featureIdxTo),
		    m_content(content), m_endian(endian)
{
	//convert bytes to int
	m_dataCorrect=true;
	if(m_content==iBS::ByteArrayContentUINT16)
	{
		::Ice::Long bytesCnt=bytes.second-bytes.first;
		Ice::Long valueCnt=featureIdxTo-featureIdxFrom;
		if(bytesCnt/2!=valueCnt)
		{
			m_dataCorrect=false;
			return;
		}
		m_values.reset(new ::Ice::Double[valueCnt]);
		const ::Ice::Byte* bt=bytes.first;
		for(int i=0;i<valueCnt;i++)
		{
			double val=0;
			if(m_endian==iBS::ByteArrayLittleEndian)
			{
				
				val=bt[i*2]+bt[i*2+1]*16;
			}
			else
			{
				val=bt[i*2]*16+bt[i*2+1];
			}
			m_values[i]=val;
		}

	}
	else
	{
		m_dataCorrect=false;
		return;
	}

}

CSetRAMBytesColumnVector::~CSetRAMBytesColumnVector()
{

}

void
CSetRAMBytesColumnVector::DoWork()

{
	if(!m_dataCorrect)
	{
		::iBS::ArgumentException ex;
		ex.reason = "data not correct";
		m_cb->ice_exception(ex);
		return;
	}
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
CSetRAMBytesColumnVector::CancelWork()

{
	m_cb->ice_exception();
}


CSetToStoreBytesColumnVector::CSetToStoreBytesColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetBytesColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*>& bytes,
			::iBS::ByteArrayContentEnum content,::iBS::ByteArrayEndianEnum endian)
			:m_foi(foi),m_cb(cb),
			m_observerID(observerID),m_featureIdxFrom(featureIdxFrom),
			m_featureIdxTo(featureIdxTo),
		    m_content(content), m_endian(endian)
{
	//convert bytes to int
	m_dataCorrect=true;
	if(m_content==iBS::ByteArrayContentUINT16)
	{
		::Ice::Long bytesCnt=bytes.second-bytes.first;
		Ice::Long valueCnt=featureIdxTo-featureIdxFrom;
		if(bytesCnt/2!=valueCnt)
		{
			m_dataCorrect=false;
			return;
		}
		m_values.reset(new ::Ice::Double[valueCnt]);
		const ::Ice::Byte* bt=bytes.first;
		for(int i=0;i<valueCnt;i++)
		{
			double val=0;
			if(m_endian==iBS::ByteArrayLittleEndian)
			{
				val=bt[i*2]+bt[i*2+1]*256;
			}
			else
			{
				val=bt[i*2]*256+bt[i*2+1];
			}
			m_values[i]=val;
		}

	}
	else
	{
		m_dataCorrect=false;
		return;
	}

}

CSetToStoreBytesColumnVector::~CSetToStoreBytesColumnVector()
{

}

void
CSetToStoreBytesColumnVector::DoWork()

{
	if(!m_dataCorrect)
	{
		::iBS::ArgumentException ex;
		ex.reason = "data not correct";
		m_cb->ice_exception(ex);
		return;
	}

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
CSetToStoreBytesColumnVector::CancelWork()

{
	m_cb->ice_exception();
}



////////////////////////////////////////////////////////////////////////////////////////

CRecalculateObserverStats::~CRecalculateObserverStats()
{

}

void
CRecalculateObserverStats::DoWork()
{
	Ice::Int foiObserverID=m_foi->ObserverID;
	Ice::Int foiStoreObserverID=m_foi->ObserverGroupSize>1? m_foi->ObserverGroupID:foiObserverID;
	Ice::Long foiStoreDomainSize = m_foi->ObserverGroupSize*m_foi->DomainSize;

	if(foiStoreDomainSize <= CGlobalVars::get()->theMaxFeatureValueCntDouble)
	{
		CFeatureValueRAMDouble* pRAM=CGlobalVars::get()->theFeatureValueRAMDobule;
		
		//using oid for storage
		::Ice::Double* values = pRAM->GetValuesInRAM(foiStoreObserverID);

		if(values)
		{
			//all in RAM
			CObserverStatsBasicJob job;
			job.m_totalValueCnt=foiStoreDomainSize;
			job.m_processedValueCnt=0;
			job.m_handleNaN=true;

			job.m_colCnt=m_foi->ObserverGroupSize;
			job.m_colIdx=m_foi->IdxInObserverGroup;

			//already handled gapped data
			CObserverStatsHelper::BasicJobProcessNextBatch(job,values,job.m_totalValueCnt);

			::iBS::ObserverStatsInfo osi;
			osi.ObserverID=m_foi->ObserverID;
			osi.UpdateDT=IceUtil::Time::now().toMilliSeconds();
			osi.Version=0;
			CObserverStatsHelper::BasicJobToStats(job,osi);
			
			CGlobalVars::get()->theObserverStatsDB->SetObserverStatsInfo(m_foi->ObserverID,osi);
		}
		else
		{
			DoWorkFromStore();
		}
	}
	else
	{
		///not in RAM, calculate from file store
		DoWorkFromStore();
	}
}

void CRecalculateObserverStats::DoWorkFromStore()
{

	Ice::Int foiObserverID=m_foi->ObserverID;
	Ice::Int foiStoreObserverID=m_foi->ObserverGroupSize>1? m_foi->ObserverGroupID:foiObserverID;
	Ice::Long foiStoreDomainSize = m_foi->ObserverGroupSize*m_foi->DomainSize;
	Ice::Long foiDomainSize = m_foi->DomainSize;


	::Ice::Long batchValueCnt=1024*1024*32;//256M for double
	if(batchValueCnt%m_foi->ObserverGroupSize!=0)
	{
		batchValueCnt-=(batchValueCnt%m_foi->ObserverGroupSize); //data row not spread in two batch files
	}

	cout<<IceUtil::Time::now().toDateTime()<< " CRecalculateObserverStats::DoWorkFromStore begin ..."<<endl; 

	CObserverStatsBasicJob job;
	job.m_totalValueCnt=foiStoreDomainSize;
	job.m_processedValueCnt=0;
	job.m_handleNaN=true;
	job.m_colCnt=m_foi->ObserverGroupSize;
	job.m_colIdx=m_foi->IdxInObserverGroup;

	::Ice::Long s_featureIdxFrom = 0;
	::Ice::Long remainCnt=foiStoreDomainSize;

	//in RAM or in Store, all index are rolled out,
	m_foi->DomainSize=foiStoreDomainSize; //convert to store size
	m_foi->ObserverID=foiStoreObserverID;
	while(remainCnt>0)
	{
		if(remainCnt>batchValueCnt)
		{
			::IceUtil::ScopedArray<Ice::Double>  values(
				CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
					m_foi,s_featureIdxFrom,s_featureIdxFrom+batchValueCnt));

			CObserverStatsHelper::BasicJobProcessNextBatch(job,values.get(),batchValueCnt);

			s_featureIdxFrom+=batchValueCnt;
			remainCnt-=batchValueCnt;
		}
		else
		{
			::IceUtil::ScopedArray<Ice::Double>  values(
				CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
					m_foi,s_featureIdxFrom,s_featureIdxFrom+remainCnt));

			CObserverStatsHelper::BasicJobProcessNextBatch(job,values.get(),remainCnt);

			remainCnt=0;
		}
		
	}
	m_foi->ObserverID=foiObserverID; 
	m_foi->DomainSize=foiDomainSize; //convert back

	::iBS::ObserverStatsInfo osi;
	osi.ObserverID=m_foi->ObserverID;
	osi.UpdateDT=IceUtil::Time::now().toMilliSeconds();
	CObserverStatsHelper::BasicJobToStats(job,osi);
	CGlobalVars::get()->theObserverStatsDB->SetObserverStatsInfo(m_foi->ObserverID,osi);

	cout<<IceUtil::Time::now().toDateTime()<< " CRecalculateObserverStats::DoWorkFromStore End"<<endl; 

}

void
CRecalculateObserverStats::CancelWork()
{
}

//////////////////////////////////////////////////////////////////////////////////////

Original::CGetJoinedRowMatrix::~CGetJoinedRowMatrix()
{

}


void
Original::CGetJoinedRowMatrix::getRetValues(::IceUtil::ScopedArray<Ice::Double>&  retValues)
{
	getRetValues(retValues.get());
}
void
Original::CGetJoinedRowMatrix::getRetValues(Ice::Double*  retValues)
{
	//all values should already be in RAM
	Ice::Long colCnt= m_observerIDs.size();
	Ice::Long rowCnt=m_featureIdxTo-m_featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	//copy by each column
	for(int i=0;i<colCnt;i++)
	{
		iBS::FeatureObserverSimpleInfoPtr& foi = m_fois[i];
		//int observerID = foi->ObserverID;
		
		int destColIdx=i;
		::Ice::Double* destValues=retValues+destColIdx; // begin of the column data
		
		Ice::Long destColCnt=colCnt;
		Ice::Long copyCnt=rowCnt;
		Ice::Long srcColCnt = foi->ObserverGroupSize;
		Ice::Long srcColIdx= foi->IdxInObserverGroup;

		Ice::Int foiObserverID=foi->ObserverID;
		Ice::Int foiStoreObserverID=foi->ObserverGroupSize>1? foi->ObserverGroupID:foiObserverID;
		Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
		Ice::Long foiDomainSize = foi->DomainSize;
		Ice::Long s_featureIdxFrom = m_featureIdxFrom * foi->ObserverGroupSize; //index in store
		Ice::Long s_featureIdxTo = m_featureIdxTo * foi->ObserverGroupSize;     //index in store

		if(foiStoreDomainSize <= CGlobalVars::get()->theMaxFeatureValueCntDouble)
		{
			CFeatureValueRAMDouble* pRAM=CGlobalVars::get()->theFeatureValueRAMDobule;
			::Ice::Double* values = pRAM->GetValuesInRAM(foi); //already handled group, idx starting from zero
			if(values)
			{
				//in RAM
				//consider offset from zero
				//in RAM or in Store, all index are rolled out,
				Ice::Long srcOffsetIdx=s_featureIdxFrom+srcColIdx;

				const Ice::Double *srcValues = values+ srcOffsetIdx;
				CFeatureValueHelper::GappedCopy(srcValues,srcColCnt,
					destValues,destColCnt,copyCnt);
			}
			else
			{
				//not in RAM
				//in RAM or in Store, all index are rolled out,
				
				foi->DomainSize=foiStoreDomainSize; //convert to store size
				foi->ObserverID=foiStoreObserverID; 
				::IceUtil::ScopedArray<Ice::Double>  values(
					CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
						foi,s_featureIdxFrom,s_featureIdxTo));
				foi->ObserverID=foiObserverID; 
				foi->DomainSize=foiDomainSize; //convert back

				const Ice::Double *srcValues = values.get()+ srcColIdx;
				CFeatureValueHelper::GappedCopy(srcValues,srcColCnt,
					destValues,destColCnt,copyCnt);

			}
		}
		else
		{
			//use batched RAM

			//not in RAM, load involved batches and save to RAM
			foi->DomainSize=foiStoreDomainSize; //convert to store size
			foi->ObserverID=foiStoreObserverID; 
			::IceUtil::ScopedArray<Ice::Double>  values(
				CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
					foi,s_featureIdxFrom,s_featureIdxTo));
			foi->ObserverID=foiObserverID; 
			foi->DomainSize=foiDomainSize; //convert back

			const Ice::Double *srcValues = values.get()+ srcColIdx;
			CFeatureValueHelper::GappedCopy(srcValues,srcColCnt,
				destValues,destColCnt,copyCnt);
		}
	}
	
}

void
Original::CGetJoinedRowMatrix::DoWork()
{
	
}

void
Original::CGetJoinedRowMatrix::CancelWork()
{
	
}


//////////////////////////////////////////////////////////////////////////////////////

Original::CRemoveFeatureValues::~CRemoveFeatureValues()
{

}

void
Original::CRemoveFeatureValues::DoWork()
{
	//all values should already be in RAM
	Ice::Long colCnt = m_observerIDs.size();
	//copy by each column
	for (int i = 0; i<colCnt; i++)
	{
		iBS::FeatureObserverSimpleInfoPtr& foi = m_fois[i];
		if (foi->ObserverGroupSize>1 && foi->IdxInObserverGroup > 0)
		{
			continue;
		}

		Ice::Int foiObserverID = foi->ObserverID;
		Ice::Int foiStoreObserverID = foiObserverID;
		Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
		Ice::Long foiDomainSize = foi->DomainSize;

		foi->DomainSize = foiStoreDomainSize; //convert to store size
		foi->ObserverID = foiStoreObserverID;
		CGlobalVars::get()->theFeatureValueStoreMgr->RemoveFeatureValueStore(foi);
		foi->ObserverID = foiObserverID;
		foi->DomainSize = foiDomainSize; //convert back
	}

	m_cb->ice_response(1);
}

void
Original::CRemoveFeatureValues::CancelWork()
{

}
//////////////////////////////////////////////////////////////////////////////////////

Original::CGetRowMatrix::~CGetRowMatrix()
{

}

void
Original::CGetRowMatrix::getRetValues(Ice::Double*  retValues)
{
	//
	bool bGetGroupRowMatrix=false;
	if(m_fois[0]->ObserverGroupSize>0 && m_fois[0]->IdxInObserverGroup==0
		&& m_fois[0]->ObserverGroupSize==(int)m_fois.size())
	{
		int groupSize=m_fois[0]->ObserverGroupSize;
		bGetGroupRowMatrix=true;
		for(int i=1;i<groupSize;i++)
		{
			if(m_fois[i]->ObserverID!=m_fois[0]->ObserverID+i)
			{
				bGetGroupRowMatrix=false;
				break;
			}
		}
	}

	bool bGetPartialRowMatrix = false;
	iBS::IntVec colIdxs;
	if (bGetGroupRowMatrix == false && m_fois[0]->ObserverGroupSize > 0)
	{
		
		int groupSize = m_fois[0]->ObserverGroupSize;
		bGetPartialRowMatrix = true;
		int groupObserverID = m_fois[0]->ObserverGroupID;
		int maxID = groupObserverID + groupSize - 1;
		int minID = groupObserverID;
		int colCnt = (int)m_fois.size();
		colIdxs.resize(colCnt, 0);
		for (int i = 0; i<colCnt; i++)
		{
			if (m_fois[i]->ObserverID > maxID || m_fois[i]->ObserverID<minID)
			{
				bGetPartialRowMatrix = false;
				break;
			}
			colIdxs[i] = m_fois[i]->ObserverID - minID;
		}
	}


	if(bGetGroupRowMatrix)
	{
		::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr nullcb;
		::Original::CGetDoublesRowMatrix wi(
			m_fois[0],nullcb,m_observerIDs[0],m_featureIdxFrom,m_featureIdxTo);
		wi.getRetValues(retValues);
	}
	else if (bGetPartialRowMatrix)
	{
		Ice::Long fullColCnt = m_fois[0]->ObserverGroupSize;
		Ice::Long rowCnt = m_featureIdxTo - m_featureIdxFrom;
		Ice::Long fullValueCnt = fullColCnt * rowCnt;
		::IceUtil::ScopedArray<Ice::Double>  fullValues(new Ice::Double[fullValueCnt]);
		if (!fullValues.get())
		{
			return;
		}
		::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr nullcb;
		
		iBS::FeatureObserverSimpleInfoPtr goi 
			= CGlobalVars::get()->theObserversDB.GetFeatureObserver(m_fois[0]->ObserverGroupID);
		::Original::CGetDoublesRowMatrix wi(
			goi, nullcb, goi->ObserverID, m_featureIdxFrom, m_featureIdxTo);
		wi.getRetValues(fullValues.get());

		CRowAdjustHelper::ReArrangeByColIdxs(
			fullValues.get(), rowCnt, fullColCnt, colIdxs, retValues);
		
	}
	else
	{
		::Original::CGetJoinedRowMatrix wi(
			m_fois,m_observerIDs,m_featureIdxFrom,m_featureIdxTo);
		wi.getRetValues(retValues);
	}
}

void
Original::CGetRowMatrix::DoWork()
{
	Ice::Long colCnt = m_observerIDs.size();
	Ice::Long rowCnt = m_featureIdxTo - m_featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	::IceUtil::ScopedArray<Ice::Double>  retValues(new ::Ice::Double[totalValueCnt]);
	if (!retValues.get())
	{
		::iBS::ArgumentException ex;
		ex.reason = "no mem available";
		m_cb->ice_exception(ex);
		return;
	}

	getRetValues(retValues.get());

	CRowAdjustHelper::Adjust(retValues.get(), rowCnt, colCnt, m_rowAdjust);

	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);
	ret.first = retValues.get();
	ret.second = retValues.get() + totalValueCnt;
	m_cb->ice_response(1, ret);
}

void
Original::CGetRowMatrix::CancelWork()
{
	m_cb->ice_exception();
}

//////////////////////////////////////////////////////////////////////////////////
Original::CSampleRowMatrix::CSampleRowMatrix(
	const iBS::FeatureObserverSimpleInfoVec& fois,
	const ::iBS::AMD_FcdcReadService_SampleRowMatrixPtr& cb,
	const ::iBS::IntVec& observerIDs,
	const ::iBS::LongVec& featureIdxs,
	iBS::RowAdjustEnum rowAdjust)
	:m_fois(fois), m_cb(cb), m_observerIDs(observerIDs), m_featureIdxs(featureIdxs), m_rowAdjust(rowAdjust)
{
}

Original::CSampleRowMatrix::~CSampleRowMatrix()
{

}

void
Original::CSampleRowMatrix::DoWork()
{
	//all values should already be in RAM
	Ice::Long colCnt = m_observerIDs.size();
	Ice::Long rowCnt = m_featureIdxs.size();
	Ice::Long totalValueCnt = rowCnt*colCnt;

	::IceUtil::ScopedArray<Ice::Double>  retValues(new ::Ice::Double[totalValueCnt]);
	if (!retValues.get())
	{
		::iBS::ArgumentException ex;
		ex.reason = "no mem";
		m_cb->ice_exception(ex);
		return;
	}

	getRetValues(retValues.get());

	CRowAdjustHelper::Adjust(retValues.get(), rowCnt, colCnt, m_rowAdjust);

	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);
	ret.first = retValues.get();
	ret.second = retValues.get() + totalValueCnt;
	m_cb->ice_response(1, ret);
}

void
Original::CSampleRowMatrix::getRetValues(Ice::Double*  retValues)
{
	//
	bool bGetGroupRowMatrix = false;
	if (m_fois[0]->ObserverGroupSize>0 && m_fois[0]->IdxInObserverGroup == 0
		&& m_fois[0]->ObserverGroupSize == (int)m_fois.size())
	{
		int groupSize = m_fois[0]->ObserverGroupSize;
		bGetGroupRowMatrix = true;
		for (int i = 1; i<groupSize; i++)
		{
			if (m_fois[i]->ObserverID != m_fois[0]->ObserverID + i)
			{
				bGetGroupRowMatrix = false;
				break;
			}
		}
	}

	if (bGetGroupRowMatrix)
	{
		getRetValuesFromObserverGroup(retValues);
	}
	else
	{

	}
}

void
Original::CSampleRowMatrix::getRetValuesFromObserverGroup(Ice::Double*  retValues)
{
	::iBS::FeatureObserverSimpleInfoPtr& foi = m_fois[0];

	Ice::Long colCnt = m_observerIDs.size();
	Ice::Long rowCnt = m_featureIdxs.size();
	Ice::Long totalValueCnt = rowCnt*colCnt;

	Ice::Int foiObserverID = foi->ObserverID;
	Ice::Int foiStoreObserverID = foi->ObserverGroupSize>1 ? foi->ObserverGroupID : foiObserverID;
	Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
	Ice::Long foiDomainSize = foi->DomainSize;

	iBS::LongVec s_featureIdxFroms(rowCnt);
	iBS::LongVec s_featureIdxTos(rowCnt);
	for (Ice::Long i = 0; i < rowCnt; i++)
	{
		s_featureIdxFroms[i] = m_featureIdxs[i] * foi->ObserverGroupSize;  //index in store
		s_featureIdxTos[i] = (m_featureIdxs[i]+1) * foi->ObserverGroupSize; //index in store
	}

	if (foiStoreDomainSize <= CGlobalVars::get()->theMaxFeatureValueCntDouble)
	{
		CFeatureValueRAMDouble* pRAM = CGlobalVars::get()->theFeatureValueRAMDobule;
		::Ice::Double* values = pRAM->GetValuesInRAM(foiStoreObserverID);
		if (values)
		{
			//already in RAM, continues, ready to send to wire
		}
		else if (foi->GetPolicy == iBS::FeatureValueGetPolicyGetFromRAM)
		{
			//need to load all values into RAM first
			
			//ret already pointed to right address, continues,ready to send to wire
		}
		else if (foi->GetPolicy == iBS::FeatureValueGetPolicyGetForOneTimeRead)
		{
			//not in RAM, one time read, load required only
			foi->DomainSize = foiStoreDomainSize; //convert to store size
			foi->ObserverID = foiStoreObserverID;

			CGlobalVars::get()->theFeatureValueStoreMgr->LoadNonOverlapRangesFromStore(
				foi, s_featureIdxFroms, s_featureIdxTos, retValues);
			foi->ObserverID = foiObserverID;
			foi->DomainSize = foiDomainSize; //convert back
			
		}

	}
	else
	{
		//not in RAM, load involved batches
		//not in RAM, one time read, load required only
		foi->DomainSize = foiStoreDomainSize; //convert to store size
		foi->ObserverID = foiStoreObserverID;

		CGlobalVars::get()->theFeatureValueStoreMgr->LoadNonOverlapRangesFromStore(
			foi, s_featureIdxFroms, s_featureIdxTos, retValues);
		foi->ObserverID = foiObserverID;
		foi->DomainSize = foiDomainSize; //convert back
	}
}

void
Original::CSampleRowMatrix::CancelWork()
{
	m_cb->ice_exception();
}

//////////////////////////////////////////////////////////////////////////////////


Original::CSampleJoinedRowMatrix::CSampleJoinedRowMatrix(
			const iBS::FeatureObserverSimpleInfoVec& fois,
			const ::iBS::IntVec& observerIDs,
            const ::std::pair<const ::Ice::Long*, const ::Ice::Long*>& featureIdxs)
			:m_fois(fois), m_observerIDs(observerIDs)
{
	m_rowCnt=featureIdxs.second-featureIdxs.first;
	m_featureIdxs.reset(new ::Ice::Long[m_rowCnt]);
	std::copy(featureIdxs.first, featureIdxs.second, m_featureIdxs.get());
	
}

Original::CSampleJoinedRowMatrix::~CSampleJoinedRowMatrix()
{

}

void
Original::CSampleJoinedRowMatrix::DoWork()
{
	
}

void
Original::CSampleJoinedRowMatrix::getJoinedRowMatrixByRange(Ice::Double *pRetValues)
{
	Ice::Long colCnt= m_observerIDs.size();
	Ice::Long rowCnt=m_featureIdxTo-m_featureIdxFrom;
	//Ice::Long totalValueCnt = rowCnt*colCnt;

	for(int i=0;i<colCnt;i++)
	{
		iBS::FeatureObserverSimpleInfoPtr& foi = m_fois[i];
	
		Ice::Long destColIdx=i;
		::Ice::Double* destValues=pRetValues+destColIdx; // begin of the column data
		Ice::Long destColCnt=colCnt;
		Ice::Long copyCnt=rowCnt;
		Ice::Long srcColCnt = foi->ObserverGroupSize;
		Ice::Long srcColIdx= foi->IdxInObserverGroup;

		Ice::Int foiObserverID=foi->ObserverID;
		Ice::Int foiStoreObserverID=foi->ObserverGroupSize>1? foi->ObserverGroupID:foiObserverID;

		Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
		Ice::Long foiDomainSize = foi->DomainSize;
		Ice::Long s_featureIdxFrom = m_featureIdxFrom * foi->ObserverGroupSize; //index in store
		Ice::Long s_featureIdxTo = m_featureIdxTo * foi->ObserverGroupSize;     //index in store

		if(foiStoreDomainSize <= CGlobalVars::get()->theMaxFeatureValueCntDouble)
		{
			CFeatureValueRAMDouble* pRAM=CGlobalVars::get()->theFeatureValueRAMDobule;
			::Ice::Double* values = pRAM->GetValuesInRAM(foiStoreObserverID);
			if(values)
			{
				//in RAM
				//consider offset from zero
				//in RAM or in Store, all index are rolled out,
				Ice::Long srcOffsetIdx=s_featureIdxFrom+srcColIdx;

				const Ice::Double *srcValues = values+ srcOffsetIdx;
				CFeatureValueHelper::GappedCopy(srcValues,srcColCnt,
					destValues,destColCnt,copyCnt);
			}
			else
			{
				//not in RAM
				//in RAM or in Store, all index are rolled out,
				
				foi->DomainSize=foiStoreDomainSize; //convert to store size
				foi->ObserverID=foiStoreObserverID; 
				::IceUtil::ScopedArray<Ice::Double>  values(
					CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
						foi,s_featureIdxFrom,s_featureIdxTo));
				foi->ObserverID=foiObserverID; 
				foi->DomainSize=foiDomainSize; //convert back

				const Ice::Double *srcValues = values.get()+ srcColIdx;
				CFeatureValueHelper::GappedCopy(srcValues,srcColCnt,
					destValues,destColCnt,copyCnt);

			}
		}
		else
		{
			foi->DomainSize=foiStoreDomainSize; //convert to store size
			foi->ObserverID=foiStoreObserverID;

			::IceUtil::ScopedArray<Ice::Double>  values(
				CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
					foi,s_featureIdxFrom,s_featureIdxTo));
			foi->ObserverID=foiObserverID; 
			foi->DomainSize=foiDomainSize; //convert back

			const Ice::Double *srcValues = values.get()+ srcColIdx;
			CFeatureValueHelper::GappedCopy(srcValues,srcColCnt,
				destValues,destColCnt,copyCnt);
		}
	}
	
}

void
Original::CSampleJoinedRowMatrix::CancelWork()
{
}

//////////////////////////////////////////////////////////////////////////////////

CRecalculateObserverIndexIntValueIntKey::~CRecalculateObserverIndexIntValueIntKey()
{

}

void
CRecalculateObserverIndexIntValueIntKey::DoWork()
{
	iBS::ObserverIndexInfo oii;
	CGlobalVars::get()->theObserverIndexDB->SetObserverIndexInfoBlank(oii);
	CObserverIndexIntValueIntKeyJob job(oii);
	oii.MakeIndexFile=m_saveIndexFile;
	job.m_handleNaN=false;
	job.m_colCnt=m_foi->ObserverGroupSize;
	job.m_colIdx=m_foi->IdxInObserverGroup;
	//give response to caller
	m_cb->ice_response(1);

	oneRoundWork(job);
	CObserverIndexHelper::SyncKeyIdx2RowIdxListStartIdx(oii);
	if(m_saveIndexFile)
	{
		job.m_round=1;
		oneRoundWork(job);
	}

	SaveObserverIndex(job);
}

bool
CRecalculateObserverIndexIntValueIntKey::oneRoundWork(CObserverIndexIntValueIntKeyJob& job)
{
	Ice::Int foiObserverID=m_foi->ObserverID;
	Ice::Int foiStoreObserverID=m_foi->ObserverGroupSize>1? m_foi->ObserverGroupID:foiObserverID;
	Ice::Long foiStoreDomainSize = m_foi->ObserverGroupSize*m_foi->DomainSize;

	if(foiStoreDomainSize <= CGlobalVars::get()->theMaxFeatureValueCntDouble)
	{
		CFeatureValueRAMDouble* pRAM=CGlobalVars::get()->theFeatureValueRAMDobule;
		
		//using oid for storage
		::Ice::Double* values = pRAM->GetValuesInRAM(foiStoreObserverID);
		if(values)
		{
			//all in RAM
			//already handled gapped data
			CObserverIndexHelper::IntValueIntKeyProcessNextBatch(job,0,values,foiStoreDomainSize);
		}
		else
		{
			DoWorkFromStore(job);
		}
	}
	else
	{
		///not in RAM, calculate from file store
		DoWorkFromStore(job);
	}

	return true;
}


void CRecalculateObserverIndexIntValueIntKey::DoWorkFromStore(CObserverIndexIntValueIntKeyJob& job)
{

	Ice::Int foiObserverID=m_foi->ObserverID;
	Ice::Int foiStoreObserverID=m_foi->ObserverGroupSize>1? m_foi->ObserverGroupID:foiObserverID;
	Ice::Long foiStoreDomainSize = m_foi->ObserverGroupSize*m_foi->DomainSize;
	Ice::Long foiDomainSize = m_foi->DomainSize;

	::Ice::Long batchValueCnt=1024*1024*32;//256M for double
	if(batchValueCnt%m_foi->ObserverGroupSize!=0)
	{
		batchValueCnt-=(batchValueCnt%m_foi->ObserverGroupSize); //data row not spread in two batch files
	}

	cout<<IceUtil::Time::now().toDateTime()<< " CRecalculateObserverIndexIntValueIntKey::DoWorkFromStore begin ..."<<endl; 

	::Ice::Long s_featureIdxFrom = 0;
	::Ice::Long remainCnt=foiStoreDomainSize;

	//in RAM or in Store, all index are rolled out,
	m_foi->DomainSize=foiStoreDomainSize; //convert to store size
	m_foi->ObserverID=foiStoreObserverID;
	::Ice::Long thisBatchCnt=0;
	while(remainCnt>0)
	{
		if(remainCnt>batchValueCnt){
			thisBatchCnt=batchValueCnt;
		}
		else{
			thisBatchCnt=remainCnt;
		}

		::IceUtil::ScopedArray<Ice::Double>  values(
			CGlobalVars::get()->theFeatureValueStoreMgr->LoadFeatureValuesFromStore(
				m_foi,s_featureIdxFrom,s_featureIdxFrom+thisBatchCnt));

		::Ice::Long fidx=s_featureIdxFrom/m_foi->ObserverGroupSize;
		CObserverIndexHelper::IntValueIntKeyProcessNextBatch(job,fidx,values.get(),thisBatchCnt);

		s_featureIdxFrom+=thisBatchCnt;
		remainCnt-=thisBatchCnt;
	}

	m_foi->ObserverID=foiObserverID; 
	m_foi->DomainSize=foiDomainSize; //convert back
	
	cout<<IceUtil::Time::now().toDateTime()<< " CRecalculateObserverIndexIntValueIntKey::DoWorkFromStore End"<<endl; 

}

void CRecalculateObserverIndexIntValueIntKey::SaveObserverIndex(CObserverIndexIntValueIntKeyJob& job)
{
	iBS::ObserverIndexInfo& oii=job.m_oii;
	oii.ObserverID=m_foi->ObserverID;
	oii.UpdateDT=IceUtil::Time::now().toMilliSeconds();

	
	if(!oii.MakeIndexFile)
	{
		CGlobalVars::get()->theObserverIndexDB->SetObserverIndexInfo(m_foi->ObserverID,oii);
		return;
	}

	cout<<IceUtil::Time::now().toDateTime()<< " SaveObserverIndex::Save2File begin ..."<<endl; 


	int indexObserverID=CGlobalVars::get()->theObserverIndexDB->GetObserverIndexObserverID(oii.ObserverID);
	if(indexObserverID==0)
	{
		//not existed, need to allocate new feature observer for storing indexes
		CGlobalVars::get()->theObserversDB.RqstNewFeatureObserverID(indexObserverID,false);
		iBS::FeatureObserverInfoVec indexFois;
		CGlobalVars::get()->theObserversDB.GetFeatureObservers(iBS::IntVec(1,indexObserverID),indexFois);
		iBS::FeatureObserverInfo& indexFoi=indexFois[0];

		indexFoi.ParentObserverID=m_foi->ObserverID;
		indexFoi.ThreadRandomIdx=m_foi->ThreadRandomIdx;
		indexFoi.DomainSize=oii.TotalRowCnt;
		indexFoi.SetPolicy=iBS::FeatureValueSetPolicyDoNothing; //readonly
		indexFoi.GetPolicy=iBS::FeatureValueGetPolicyGetForOneTimeRead;
		CGlobalVars::get()->theObserversDB.SetFeatureObservers(indexFois);
	}
	iBS::FeatureObserverSimpleInfoPtr idxfoi
		=CGlobalVars::get()->theObserversDB.GetFeatureObserver(indexObserverID);

	std::pair<const Ice::Double*, const Ice::Double*> values(
			job.m_featureIdxs.get(), job.m_featureIdxs.get()+oii.TotalRowCnt);

	CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
		idxfoi,0,oii.TotalRowCnt,values);
	oii.IndexObserverID=indexObserverID;
	CGlobalVars::get()->theObserverIndexDB->SetObserverIndexInfo(m_foi->ObserverID,oii);
	cout<<IceUtil::Time::now().toDateTime()<< " SaveObserverIndex::Save2File end"<<endl; 
}


void
CRecalculateObserverIndexIntValueIntKey::CancelWork()
{
	m_cb->ice_exception();
}

///////////////////////////////////////////////////////////////////////////////////

CGetFeatureIdxsByIntKeys::~CGetFeatureIdxsByIntKeys()
{

}

void
CGetFeatureIdxsByIntKeys::DoWork()
{

	size_t keyCnt = m_keyIdxs.size();
	Ice::Long totalValueCnt=0;
	for(int i=0; i<keyCnt;i++)
	{
		int keyIdx=m_keyIdxs[i];
		totalValueCnt += m_oii.KeyIdx2RowCnt[keyIdx];
	}

	if(totalValueCnt>m_maxFeatureCnt)
	{
		totalValueCnt=m_maxFeatureCnt;
	}

	::IceUtil::ScopedArray<Ice::Long>  retFeatureIdxs(new ::Ice::Long[totalValueCnt]);
	if(!retFeatureIdxs.get())
	{
		::iBS::ArgumentException ex;
		ex.reason = "no mem";
		m_cb->ice_exception(ex);
		return;
	}

	Ice::Long remainCnt=totalValueCnt;
	Ice::Long readCnt=0;

	for(int i=0;i<keyCnt; i++)
	{
		int keyIdx=m_keyIdxs[i];
		Ice::Long featureIdxFrom=m_oii.KeyIdx2RowIdxListStartIdx[keyIdx];
		Ice::Long featureIdxTo=featureIdxFrom+m_oii.KeyIdx2RowCnt[keyIdx];

		::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr nullcb;
		::Original::CGetDoublesColumnVector wi(
			m_idxfoi,nullcb,m_idxfoi->ObserverID,featureIdxFrom,featureIdxTo);

		//ret always pointing to continues RAM that is ready to send to wire
		std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);

		//retValues manage newly allocated mem (if retValues.get() is not null), exclusively for this call,
		//therefore, can be in-place editing without effecting original values
		::IceUtil::ScopedArray<Ice::Double>  retValues;
		wi.getRetValues(ret, retValues);

		Ice::Long thisBatchCnt=ret.second-ret.first;
		if(remainCnt<thisBatchCnt)
		{
			thisBatchCnt=remainCnt;
		}

		for(Ice::Long j=0;j<thisBatchCnt;j++)
		{
			retFeatureIdxs.get()[readCnt]=(Ice::Long)ret.first[j];
			readCnt++;
			remainCnt--;
		}
	}
	
	std::pair<const Ice::Long*, const Ice::Long*> lret(0, 0);
	lret.first =retFeatureIdxs.get();
	lret.second = retFeatureIdxs.get()+totalValueCnt;

	m_cb->ice_response(1,lret);
}

void CGetFeatureIdxsByIntKeys::CancelWork()
{
	m_cb->ice_exception();
}
