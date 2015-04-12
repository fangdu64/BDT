#ifndef __FeatureValueWorkerItemInt_h__
#define __FeatureValueWorkerItemInt_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <FCDCentralService.h>
#include <FeatureValueWorker.h>

class CSetRAMIntsColumnVector : public  FeatureValueWorkItemBase
{

public:
		CSetRAMIntsColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetIntsColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Int*, const ::Ice::Int*>& values);

		virtual ~CSetRAMIntsColumnVector();

	    virtual void DoWork();
		virtual void CancelWork();

private:
	iBS::FeatureObserverSimpleInfoPtr m_foi;
	::iBS::AMD_FcdcReadWriteService_SetIntsColumnVectorPtr m_cb;
	::Ice::Int  m_observerID;
    ::Ice::Long m_featureIdxFrom;
    ::Ice::Long m_featureIdxTo;
	::IceUtil::ScopedArray<Ice::Double>  m_values;
};


class CSetToStoreIntsColumnVector : public  FeatureValueWorkItemBase
{

public:
		CSetToStoreIntsColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetIntsColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Int*, const ::Ice::Int*>& values);

		virtual ~CSetToStoreIntsColumnVector();

	    virtual void DoWork();
		virtual void CancelWork();

private:
	iBS::FeatureObserverSimpleInfoPtr m_foi;
	::iBS::AMD_FcdcReadWriteService_SetIntsColumnVectorPtr m_cb;
	::Ice::Int  m_observerID;
    ::Ice::Long m_featureIdxFrom;
    ::Ice::Long m_featureIdxTo;
	::IceUtil::ScopedArray<Ice::Double>  m_values;
};


class CSetToStoreBytesColumnVector : public  FeatureValueWorkItemBase
{

public:
		CSetToStoreBytesColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetBytesColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*>& bytes,
			::iBS::ByteArrayContentEnum content,::iBS::ByteArrayEndianEnum endian);

		virtual ~CSetToStoreBytesColumnVector();

	    virtual void DoWork();
		virtual void CancelWork();

		bool IsDataCorrect() {return m_dataCorrect;}
private:
	iBS::FeatureObserverSimpleInfoPtr m_foi;
	::iBS::AMD_FcdcReadWriteService_SetBytesColumnVectorPtr m_cb;
	::Ice::Int  m_observerID;
    ::Ice::Long m_featureIdxFrom;
    ::Ice::Long m_featureIdxTo;
	::iBS::ByteArrayContentEnum m_content;
	::iBS::ByteArrayEndianEnum m_endian;

	::IceUtil::ScopedArray<Ice::Double>  m_values;
	bool m_dataCorrect;
};

class CSetRAMBytesColumnVector : public  FeatureValueWorkItemBase
{

public:
		CSetRAMBytesColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetBytesColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Byte*, const ::Ice::Byte*>& bytes,
			::iBS::ByteArrayContentEnum content,::iBS::ByteArrayEndianEnum endian);

		virtual ~CSetRAMBytesColumnVector();

	    virtual void DoWork();
		virtual void CancelWork();

		bool IsDataCorrect() {return m_dataCorrect;}
private:
	iBS::FeatureObserverSimpleInfoPtr m_foi;
	::iBS::AMD_FcdcReadWriteService_SetBytesColumnVectorPtr m_cb;
	::Ice::Int  m_observerID;
    ::Ice::Long m_featureIdxFrom;
    ::Ice::Long m_featureIdxTo;
	::iBS::ByteArrayContentEnum m_content;
	::iBS::ByteArrayEndianEnum m_endian;

	::IceUtil::ScopedArray<Ice::Double>  m_values;
	bool m_dataCorrect;
};

class CRecalculateObserverStats : public  FeatureValueWorkItemBase
{

public:
		CRecalculateObserverStats(
			iBS::FeatureObserverSimpleInfoPtr& foi)
			:m_foi(foi)
		{
		}

		virtual ~CRecalculateObserverStats();
	    virtual void DoWork();
		virtual void CancelWork();
private:
	void DoWorkFromStore();

private:
	iBS::FeatureObserverSimpleInfoPtr m_foi; //original foi, not converted to group observer
	
};

namespace Original
{

class CRemoveFeatureValues : public  FeatureValueWorkItemBase
{
public:
	CRemoveFeatureValues(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::IntVec& observerIDs,
		const ::iBS::AMD_FcdcAdminService_RemoveFeatureObserversPtr& cb)
		:m_fois(fois), m_observerIDs(observerIDs)
	{
	}

	virtual ~CRemoveFeatureValues();
	virtual void DoWork();
	virtual void CancelWork();

protected:
	::iBS::FeatureObserverSimpleInfoVec m_fois;
	::iBS::IntVec	m_observerIDs;
	::iBS::AMD_FcdcAdminService_RemoveFeatureObserversPtr m_cb;
};

class CGetJoinedRowMatrix : public  FeatureValueWorkItemBase
{
	friend class CGetRowMatrix;
	friend class ::CRUVBuilder;
	
public:
	CGetJoinedRowMatrix(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::IntVec& observerIDs,
        ::Ice::Long featureIdxFrom,
        ::Ice::Long featureIdxTo)
		:m_fois(fois), m_observerIDs(observerIDs),
		m_featureIdxFrom(featureIdxFrom),m_featureIdxTo(featureIdxTo)
	{
	}

	virtual ~CGetJoinedRowMatrix();
	virtual void DoWork();
	virtual void CancelWork();

protected:
	void getRetValues(::IceUtil::ScopedArray<Ice::Double>&  retValues);
	void getRetValues(Ice::Double*  retValues);
protected:
	::iBS::FeatureObserverSimpleInfoVec m_fois;
	::iBS::IntVec	m_observerIDs;
    ::Ice::Long		m_featureIdxFrom;
    ::Ice::Long		m_featureIdxTo;
};



class CGetRowMatrix : public  FeatureValueWorkItemBase
{
	friend class ::CRUVBuilder;
	friend class ::CFeatureValueHelper;

public:
	CGetRowMatrix(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::AMD_FcdcReadService_GetRowMatrixPtr& cb,
		const ::iBS::IntVec& observerIDs,
        ::Ice::Long featureIdxFrom,
        ::Ice::Long featureIdxTo,
		iBS::RowAdjustEnum rowAdjust = iBS::RowAdjustNone)
		:m_fois(fois), m_cb(cb), m_observerIDs(observerIDs),
		m_featureIdxFrom(featureIdxFrom), m_featureIdxTo(featureIdxTo), m_rowAdjust(rowAdjust)
	{
	}

	virtual ~CGetRowMatrix();
	virtual void DoWork();
	virtual void CancelWork();

public:
	void getRetValues(Ice::Double*  retValues);

protected:
	::iBS::FeatureObserverSimpleInfoVec m_fois;
	::iBS::AMD_FcdcReadService_GetRowMatrixPtr m_cb;
	::iBS::IntVec	m_observerIDs;
    ::Ice::Long		m_featureIdxFrom;
    ::Ice::Long		m_featureIdxTo;
	iBS::RowAdjustEnum m_rowAdjust;
};


class CSampleJoinedRowMatrix : public  FeatureValueWorkItemBase
{

public:
	CSampleJoinedRowMatrix(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::IntVec& observerIDs,
        const ::std::pair<const ::Ice::Long*, const ::Ice::Long*>& featureIdxs);

	virtual ~CSampleJoinedRowMatrix();
	virtual void DoWork();
	virtual void CancelWork();
protected:
	void getJoinedRowMatrixByRange(Ice::Double *pRetValues);

protected:
	::iBS::FeatureObserverSimpleInfoVec m_fois;
	::iBS::IntVec	m_observerIDs;
	::IceUtil::ScopedArray<Ice::Long>  m_featureIdxs;
	::Ice::Long m_rowCnt;

	::Ice::Long		m_featureIdxFrom;
    ::Ice::Long		m_featureIdxTo;
};


class CSampleRowMatrix : public  FeatureValueWorkItemBase
{
	friend class ::CRUVBuilder;
public:
	CSampleRowMatrix(
		const iBS::FeatureObserverSimpleInfoVec& fois,
		const ::iBS::AMD_FcdcReadService_SampleRowMatrixPtr& cb,
		const ::iBS::IntVec& observerIDs,
		const ::iBS::LongVec& featureIdxs,
		iBS::RowAdjustEnum rowAdjust = iBS::RowAdjustNone);

	virtual ~CSampleRowMatrix();
	virtual void DoWork();
	virtual void CancelWork();

protected:
	void getRetValues(Ice::Double*  retValues);

	void getRetValuesFromObserverGroup(Ice::Double*  retValues);

protected:
	::iBS::FeatureObserverSimpleInfoVec m_fois;
	::iBS::AMD_FcdcReadService_SampleRowMatrixPtr m_cb;
	::iBS::IntVec	m_observerIDs;
	::iBS::LongVec  m_featureIdxs;
	iBS::RowAdjustEnum m_rowAdjust;
};

}

////////////////////////////////////////////////////////////////////////////
class CObserverIndexIntValueIntKeyJob;
class CRecalculateObserverIndexIntValueIntKey : public  FeatureValueWorkItemBase
{

public:
		CRecalculateObserverIndexIntValueIntKey(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcAdminService_RecalculateObserverIndexPtr& cb,
			bool saveIndexFile)
			:m_foi(foi),m_cb(cb),m_saveIndexFile(saveIndexFile)
		{
		}

		virtual ~CRecalculateObserverIndexIntValueIntKey();
	    virtual void DoWork();
		virtual void CancelWork();
private:
	bool oneRoundWork(CObserverIndexIntValueIntKeyJob& job);
	void DoWorkFromStore(CObserverIndexIntValueIntKeyJob& job);
	void SaveObserverIndex(CObserverIndexIntValueIntKeyJob& job);

private:
	iBS::FeatureObserverSimpleInfoPtr m_foi; //original foi, not converted to group observer
	::iBS::AMD_FcdcAdminService_RecalculateObserverIndexPtr m_cb;
	bool m_saveIndexFile;
};

/////////////////////////////////////////////////////////////////////////////////////////

class CGetFeatureIdxsByIntKeys : public  FeatureValueWorkItemBase
{

public:
		CGetFeatureIdxsByIntKeys(
			iBS::FeatureObserverSimpleInfoPtr& idxfoi,
			const ::iBS::AMD_FcdcReadService_GetFeatureIdxsByIntKeysPtr& cb,
			::Ice::Int observerID,
			::Ice::Long maxFeatureCnt,
			const ::iBS::ObserverIndexInfo& oii,
			const iBS::IntVec& keyIdxs)
			:m_idxfoi(idxfoi),m_cb(cb),
			m_observerID(observerID),m_maxFeatureCnt(maxFeatureCnt),
			m_oii(oii), m_keyIdxs(keyIdxs)
		{
		}

		virtual ~CGetFeatureIdxsByIntKeys();
	    virtual void DoWork();
		virtual void CancelWork();

protected:
	::iBS::FeatureObserverSimpleInfoPtr m_idxfoi;
	::iBS::AMD_FcdcReadService_GetFeatureIdxsByIntKeysPtr m_cb;
	::Ice::Int  m_observerID;
    ::Ice::Long m_maxFeatureCnt;
    ::iBS::ObserverIndexInfo m_oii;
	::iBS::IntVec m_keyIdxs;
};


#endif
