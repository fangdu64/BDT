#ifndef __FeatureValueStoreMgr_h__
#define __FeatureValueStoreMgr_h__

#include <IceUtil/IceUtil.h>
#include <Ice/Ice.h>
#include <FCDCentralService.h>
using namespace iBS;

class CGlobalVars;
class CFeatureValueStoreMgr
{
public:
	CFeatureValueStoreMgr( Ice::Long maxFeatureValueFileSize, const string& rootdir);
	~CFeatureValueStoreMgr();

public:

	void Initilize();
	void UnInitilize();
	bool RemoveFeatureValueStore(const iBS::FeatureObserverSimpleInfoPtr& foi);
	//save continues RAM to store
	bool SaveFeatureValueToStore(const iBS::FeatureObserverSimpleInfoPtr& foi,
		::Ice::Long featureIdxFrom,
		::Ice::Long featureIdxTo,
		const std::pair<const Ice::Double*, const Ice::Double*>& values);

	//load to continues RAM, caller need to delete, if no error, allocate NaN values for no data
	::Ice::Double*  LoadFeatureValuesFromStore(const iBS::FeatureObserverSimpleInfoPtr& foi);


	//load to continues RAM, caller need to delete, if no error, allocate NaN values for no data
	::Ice::Double* LoadFeatureValuesFromStore(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		::Ice::Long featureIdxFrom,
		::Ice::Long featureIdxTo);

	bool LoadFeatureValuesFromStore(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		::Ice::Long featureIdxFrom,
		::Ice::Long featureIdxTo, ::Ice::Double* values);

	//load to continues RAM, caller need to delete, if no error, allocate NaN values for no data
	::Ice::Double* LoadNonOverlapRangesFromStore(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		const ::iBS::LongVec& featureIdxFroms,
		const ::iBS::LongVec& featureIdxTos);

	 bool LoadNonOverlapRangesFromStore(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		const ::iBS::LongVec& featureIdxFroms,
		const ::iBS::LongVec& featureIdxTos,
		::Ice::Double* values);

	 string GetRootDir();

	 string GetStoreFilePathPrefix(const iBS::FeatureObserverSimpleInfoPtr& foi);


private:

	//for foi can fit within one file i.e. <=1G
	bool StoreFileExist(const iBS::FeatureObserverSimpleInfoPtr& foi);
	//for foi can fit within one file
	string GetStoreFilePath(const iBS::FeatureObserverSimpleInfoPtr& foi);

	//for foi  >1G
	bool StoreFileExist(const iBS::FeatureObserverSimpleInfoPtr& foi, 
			Ice::Long featureIdxFrom, Ice::Long featureIdxTo);

	//for foi  >1G
	int GetStoreFilePath(const iBS::FeatureObserverSimpleInfoPtr& foi, 
		Ice::Long featureIdxFrom, Ice::Long featureIdxTo, 
		Ice::Long& featureCntPerBatch,
		StringVec& files, LongVec& batchFileIdxs, LongVec& posFroms, LongVec& posTos);

	string GetBatchStoreFilePath(const iBS::FeatureObserverSimpleInfoPtr& foi, Ice::Long batchIdx);


	bool SaveFeatureValueToStoreDouble(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		::Ice::Long localFeatureIdxFrom,
		::Ice::Long localFeatureIdxTo,
		::Ice::Double *values,
		const string& batchFileName, Ice::Long featureCntPerBatch);

	bool SaveFeatureValueToStoreInt32(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		::Ice::Long localFeatureIdxFrom,
		::Ice::Long localFeatureIdxTo,
		::Ice::Double *values,
		const string& batchFileName, Ice::Long featureCntPerBatch);

	int LoadBatchFeatureValuesDouble(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		::Ice::Long localFeatureIdxFrom,
		::Ice::Long localFeatureIdxTo,
		::Ice::Double *values,
		const string& batchFileName);


	bool LoadNonOverlapSortedRangesFromStore(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		const ::iBS::LongVec& featureIdxFroms,
		const ::iBS::LongVec& featureIdxTos, 
		::Ice::Long totalValueCnt, ::Ice::Double *values);

	//load multiple discontinuous ranges
	int multipleLoadBatchFeatureValuesDouble(
		const iBS::FeatureObserverSimpleInfoPtr& foi,
		const ::iBS::LongVec& localFeatureIdxFroms,
		const ::iBS::LongVec& localFeatureIdxTos,
		::Ice::Double *values,
		const string& batchFileName);

	//these ranges should be non-overlapping
	Ice::Long getValueCntByMultipleRanges(
		const ::iBS::LongVec& featureIdxFroms,
		const ::iBS::LongVec& featureIdxTos);


	size_t GetSizeOfValueType(iBS::FeatureValueEnum valueType);

	Ice::Long GetMaxFeatureValueCntPerStoreFile(iBS::FeatureValueEnum valueStoredType);
private:
	const string m_rootDir;
	const Ice::Long m_theMaxFeatureValueFileSize; //in byte
	Ice::Long m_theMaxFeatureValueCntDouble;
	Ice::Long m_theMaxFeatureValueCntInt32;

};
#endif

