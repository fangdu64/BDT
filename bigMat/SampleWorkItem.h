#ifndef __SampleWorkItem_h__
#define __SampleWorkItem_h__

#include <IceUtil/IceUtil.h>
#include <IceUtil/ScopedArray.h>
#include <FCDCentralService.h>
#include <SampleService.h>
#include <FeatureValueWorker.h>
#include <FeatureValueWorkItem.h>
#include <bdtUtil/GenomeHelper.h>

class CCreateBamToBinSample : public  FeatureValueWorkItemBase
{
public:
	CCreateBamToBinSample(
		iBS::FeatureObserverSimpleInfoPtr& foi,
		const iBS::BamToBinCountSampleInfo& sample,
		const CGenomeBinMap& binMap,
		Ice::Long taskID)
		:m_foi(foi), m_sample(sample), m_binMap(binMap), m_taskID(taskID)
	{
	}

	virtual ~CCreateBamToBinSample(){};
	virtual void DoWork();
	virtual void CancelWork();
private:
	iBS::FeatureObserverSimpleInfoPtr m_foi;
	const iBS::BamToBinCountSampleInfo m_sample;
	const CGenomeBinMap m_binMap;
	Ice::Long m_taskID;
};

#endif
