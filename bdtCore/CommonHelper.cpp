#include <bdt/bdtBase.h>
#include <IceUtil/Random.h>
#include <CommonHelper.h>
#include <GlobalVars.h>
#include <algorithm>     
#include <limits>
#include <FeatureValueWorker.h>
#include <FeatureValueStoreMgr.h>
#include <FeatureObserverDB.h>
#include <FeatureValueWorkItem.h>

void CFeatureValueHelper::GappedCopy(
		const Ice::Double *srcValues, Ice::Long srcColCnt, 
		Ice::Double *destValues, Ice::Long destColCnt, Ice::Long copyCnt)
{
	if(srcColCnt!=1||destColCnt!=1)
	{
		for(Ice::Long i=0;i<copyCnt;i++)
		{
			destValues[i*destColCnt]=srcValues[i*srcColCnt];
		}
	}
	else
	{
		//both continues RAM
		std::copy(srcValues,srcValues+copyCnt,destValues);
	}

	
}

void CFeatureValueHelper::GappedCopyWithMultiplyFactor(
		const Ice::Double *srcValues, Ice::Long srcColCnt, 
		Ice::Double *destValues, Ice::Long destColCnt, Ice::Long copyCnt, Ice::Double factor)
{
	for(Ice::Long i=0;i<copyCnt;i++)
	{
		destValues[i*destColCnt]=srcValues[i*srcColCnt]*factor;
	}
}

void CFeatureValueHelper::GetDoubleVecInRAM(
	::iBS::FeatureObserverSimpleInfoPtr foi,
	Ice::Double* values, Ice::Long batchMb)
{
	::Ice::Long ramMb = batchMb;
	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = 1;

	Ice::Long rowCnt = foi->DomainSize;

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;


	::Ice::Long batchCnt = rowCnt / batchRowCnt + 1;

	int batchIdx = 0;
	::Ice::Long remainCnt = rowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom =0;
	::Ice::Long featureIdxTo = 0;
	iBS::FeatureObserverSimpleInfoVec fois(1, foi);
	::iBS::IntVec oids(1, foi->ObserverID);
	Ice::Long fetchedRowCnt = 0;
	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		::iBS::AMD_FcdcReadService_GetRowMatrixPtr nullcb;
		::Original::CGetRowMatrix wi(
			fois, nullcb, oids, featureIdxFrom, featureIdxTo);
		wi.getRetValues(values + fetchedRowCnt*colCnt);

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
		fetchedRowCnt += thisBatchRowCnt;
	}
}

void CFeatureValueHelper::ExportMatrix(
	Ice::Double* values,
	Ice::Long rowCnt, Ice::Long colCnt,
	const std::string& outdir, int outID)
{
	Ice::Long theMaxFeatureValueFileSize = 1024 * 1024 * 1024;

	iBS::FeatureObserverSimpleInfoPtr foi
		= new ::iBS::FeatureObserverSimpleInfo();
	foi->ObserverID = outID;
	foi->DomainID = 0;
	foi->DomainSize = rowCnt;
	foi->ValueType = iBS::FeatureValueDouble;
	foi->StorePolicy = iBS::FeatureValueStorePolicyBinaryFilesObserverGroup;
	foi->Status = ::iBS::NodeStatusIDOnly;
	foi->GetPolicy = ::iBS::FeatureValueGetPolicyGetForOneTimeRead;
	foi->SetPolicy = ::iBS::FeatureValueSetPolicyNoRAMImmediatelyToStore;
	foi->ThreadRandomIdx = 0;
	foi->ObserverGroupID = foi->ObserverID;
	foi->ObserverGroupSize = (int)colCnt;
	foi->IdxInObserverGroup = 0;

	CFeatureValueStoreMgr fileStore(theMaxFeatureValueFileSize, outdir);

	std::pair<const Ice::Double*, const Ice::Double*> vals(
		values, values + rowCnt*colCnt);

	Ice::Int foiObserverID = foi->ObserverID;
	Ice::Int foiStoreObserverID = foi->ObserverID;
	Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
	Ice::Long foiDomainSize = foi->DomainSize;
	Ice::Long s_featureIdxFrom = 0 * foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = rowCnt * foi->ObserverGroupSize;  //index in store

	foi->DomainSize = foiStoreDomainSize; //convert to store size
	foi->ObserverID = foiStoreObserverID;
	fileStore.SaveFeatureValueToStore(
		foi, s_featureIdxFrom, s_featureIdxTo, vals);
	foi->ObserverID = foiObserverID;
	foi->DomainSize = foiDomainSize; //convert back
}