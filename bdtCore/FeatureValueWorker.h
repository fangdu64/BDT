#ifndef __FeatureValueWorker_h__
#define __FeatureValueWorker_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <FCDCentralService.h>

class CFeatureValueWorker;
typedef IceUtil::Handle<CFeatureValueWorker> FeatureValueWorkerPtr;

class FeatureValueWorkItemBase;
typedef IceUtil::Handle<FeatureValueWorkItemBase> FeatureValueWorkItemPtr;

class FeatureValueWorkItemBase: public IceUtil::Shared
{
public:
	FeatureValueWorkItemBase(){};
	virtual ~FeatureValueWorkItemBase(){};
    virtual void DoWork() = 0;
	virtual void CancelWork() = 0;
};


class CForceLoadInRAM : public  FeatureValueWorkItemBase
{

public:
		CForceLoadInRAM(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			::Ice::Int observerID, ::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo)
			:m_foi(foi),m_observerID(observerID),
			m_featureIdxFrom(featureIdxFrom),
			m_featureIdxTo(featureIdxTo)
		{

		}

		virtual ~CForceLoadInRAM();
	    virtual void DoWork();
		virtual void CancelWork();

private:
	iBS::FeatureObserverSimpleInfoPtr m_foi;
	::Ice::Int  m_observerID;
	::Ice::Long m_featureIdxFrom;
    ::Ice::Long m_featureIdxTo;
};

class CGetFeatureIdxsByIntKeys;
class CRUVBuilder;
class CFeatureValueHelper;
namespace Original
{

class CGetDoublesColumnVector : public  FeatureValueWorkItemBase
{

	friend class ::CGetFeatureIdxsByIntKeys;
	friend class ::CRUVBuilder;
public:
		CGetDoublesColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo)
			:m_foi(foi),m_cb(cb),
			m_observerID(observerID),m_featureIdxFrom(featureIdxFrom),
			m_featureIdxTo(featureIdxTo)
		{
		}

		virtual ~CGetDoublesColumnVector();
	    virtual void DoWork();
		virtual void CancelWork();
protected:
	void getRetValues( std::pair<const Ice::Double*, const Ice::Double*>& ret,
						::IceUtil::ScopedArray<Ice::Double>&  retValues);
	
protected:
	::iBS::FeatureObserverSimpleInfoPtr m_foi;
	::iBS::AMD_FcdcReadService_GetDoublesColumnVectorPtr m_cb;
	::Ice::Int  m_observerID;
    ::Ice::Long m_featureIdxFrom;
    ::Ice::Long m_featureIdxTo;
};

}


class CSetRAMDoublesColumnVector : public  FeatureValueWorkItemBase
{

public:
		CSetRAMDoublesColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetDoublesColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values);

		virtual ~CSetRAMDoublesColumnVector();

	    virtual void DoWork();
		virtual void CancelWork();

private:
	iBS::FeatureObserverSimpleInfoPtr m_foi;
	::iBS::AMD_FcdcReadWriteService_SetDoublesColumnVectorPtr m_cb;
	::Ice::Int  m_observerID;
    ::Ice::Long m_featureIdxFrom;
    ::Ice::Long m_featureIdxTo;
	::IceUtil::ScopedArray<Ice::Double>  m_values;
};


class CSetToStoreDoublesColumnVector : public  FeatureValueWorkItemBase
{

public:
		CSetToStoreDoublesColumnVector(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetDoublesColumnVectorPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values);

		virtual ~CSetToStoreDoublesColumnVector();

	    virtual void DoWork();
		virtual void CancelWork();

private:
	iBS::FeatureObserverSimpleInfoPtr m_foi;
	::iBS::AMD_FcdcReadWriteService_SetDoublesColumnVectorPtr m_cb;
	::Ice::Int  m_observerID;
    ::Ice::Long m_featureIdxFrom;
    ::Ice::Long m_featureIdxTo;
	::IceUtil::ScopedArray<Ice::Double>  m_values;
};


//////////////////////////////////////////////////////////////////////////////////////

namespace Original
{
	class CGetDoublesRowMatrix : public  FeatureValueWorkItemBase
	{
		friend class CGetRowMatrix;
	public:
			CGetDoublesRowMatrix(
				const iBS::FeatureObserverSimpleInfoPtr& foi,
				const ::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr& cb,
				::Ice::Int observerID,
				::Ice::Long featureIdxFrom,
				::Ice::Long featureIdxTo)
				:m_foi(foi),m_cb(cb),
				m_observerID(observerID),m_featureIdxFrom(featureIdxFrom),
				m_featureIdxTo(featureIdxTo)
			{
			}

			virtual ~CGetDoublesRowMatrix();
			virtual void DoWork();
			virtual void CancelWork();

	protected:
		void getRetValues( std::pair<const Ice::Double*, const Ice::Double*>& ret,
				::IceUtil::ScopedArray<Ice::Double>&  retValues);
		void getRetValues(Ice::Double*  retValues);
	protected:
		iBS::FeatureObserverSimpleInfoPtr m_foi;
		::iBS::AMD_FcdcReadService_GetDoublesRowMatrixPtr m_cb;
		::Ice::Int  m_observerID;
		::Ice::Long m_featureIdxFrom;
		::Ice::Long m_featureIdxTo;
	};

}

class CSetRAMDoublesRowMatrix : public  FeatureValueWorkItemBase
{

public:
		CSetRAMDoublesRowMatrix(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			const ::iBS::AMD_FcdcReadWriteService_SetDoublesRowMatrixPtr& cb,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values);

		virtual ~CSetRAMDoublesRowMatrix();

	    virtual void DoWork();
		virtual void CancelWork();

private:
	iBS::FeatureObserverSimpleInfoPtr m_foi;
	::iBS::AMD_FcdcReadWriteService_SetDoublesRowMatrixPtr m_cb;
	::Ice::Int  m_observerID;
    ::Ice::Long m_featureIdxFrom;
    ::Ice::Long m_featureIdxTo;
	::IceUtil::ScopedArray<Ice::Double>  m_values;
};

///////////////////////////////////////////////////////////////////////////////


class CFeatureValueWorker : public IceUtil::Thread
{
public:
	CFeatureValueWorker();
	~CFeatureValueWorker();

public:
	 virtual void run();

	 void AddWorkItem(const FeatureValueWorkItemPtr& item);
	 void RequestShutdown();

private:
     bool m_needNotify;
	 bool m_shutdownRequested;

	 IceUtil::Monitor<IceUtil::Mutex>	m_monitor;

	 typedef std::list<FeatureValueWorkItemPtr> FeatureValueWorkItemPtrLsit_T;
     FeatureValueWorkItemPtrLsit_T m_pendingItems;
	 FeatureValueWorkItemPtrLsit_T m_processingItems;

};


#endif
