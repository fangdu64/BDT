#ifndef __FeatureValueRAM_h__
#define __FeatureValueRAM_h__

#include <IceUtil/Mutex.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>

class CGlobalVars;

//manage feature values could be fit in as continues RAM
//for observer(and observer group) with dataSize>256M*12=3G
class CFeatureValueRAMDouble
{
public:
	CFeatureValueRAMDouble(CGlobalVars& );
	~CFeatureValueRAMDouble();

public:

	void SetFeatureValuesInRAM(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values);

	void GetFeatureValuesLoadAllToRAM(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values);

	void UpdateFeatureValuesIfLoaded(
			iBS::FeatureObserverSimpleInfoPtr& foi,
			::Ice::Int observerID,
			::Ice::Long featureIdxFrom,
			::Ice::Long featureIdxTo,
			const ::std::pair<const ::Ice::Double*, const ::Ice::Double*>& values);

	::Ice::Double* GetValuesInRAM(::Ice::Int observerID);
	::Ice::Double* GetValuesInRAM(const iBS::FeatureObserverSimpleInfoPtr& foi);

private:
	CGlobalVars&	m_gv;

	typedef ::Ice::Double*	  DoubleArrayPtr_T;
	typedef ::std::vector<DoubleArrayPtr_T> DoubleArrayPtrList_T;

	DoubleArrayPtrList_T m_doubleArrays;
};


#endif

