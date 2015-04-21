#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/ScopedArray.h>
#include <GlobalVars.h>
#include <ComputeServiceImpl.h>
#include <FeatureObserverDB.h>
#include <ComputeBuilder.h>
#include <ObserverStatsDB.h>
#include <FeatureValueWorkerMgr.h>
#include <fstream>  

::iBS::DistMatrixTask
CComputeServiceImpl::GetBlankDistMatrixTask(const Ice::Current& current)
{
	::iBS::DistMatrixTask task;
	task.TaskID = 0;
	task.Distance = iBS::DistEuclidean;
	task.Subset.Max = 0;
	task.Subset.Min = 0;
	task.Subset.SelectedCnt = 0;
	task.Subset.UnselectedCnt = 0;
	task.Subset.Statistic = iBS::RowStatisticMax;
	task.Subset.Selector = iBS::RowSelectorHigherThanThreshold;
	task.Subset.Threshold = 0;
	task.Subset.Enable = false;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;
	task.RamMb = 250;
	task.ThreadCnt = 1;
	return task;
}

void
CComputeServiceImpl::DistMatrix_async(
const ::iBS::AMD_ComputeService_DistMatrixPtr& cb,
const ::iBS::DistMatrixTask& task,
const Ice::Current& current)
{
	Ice::Long taskID = CGlobalVars::get()->theFeatureValueWorkerMgr->RegisterAMDTask(
		"DistMatrix", 0);
	cb->ice_response(1, taskID);
	
	::iBS::DistMatrixTask task_d(task);
	task_d.TaskID = taskID;

	int threadCnt = task.ThreadCnt;
	if (threadCnt < 1)
	{
		threadCnt = 1;
	}
	
	if (task_d.Distance == iBS::DistPCC)
	{
		CPearsonCorrelationBuilder builder(task_d, task_d.OutPath);
		builder.Calculate(threadCnt, 250);
	}
	else if (task_d.Distance == iBS::DistEuclidean)
	{
		CEuclideanDistMatrixBuilder builder(task_d, task_d.OutPath);
		builder.Calculate(threadCnt, 250);
	}

	CGlobalVars::get()->theFeatureValueWorkerMgr->SetAMDTaskDone(task_d.TaskID);
}

::iBS::RUVDistMatrixBatchTask
CComputeServiceImpl::GetBlankRUVDistMatrixBatchTask(const Ice::Current& current)
{
	::iBS::RUVDistMatrixBatchTask task;
	task.TaskID = 0;
	return task;
}

void
CComputeServiceImpl::RUVDistMatrixBatch_async(const ::iBS::AMD_ComputeService_RUVDistMatrixBatchPtr& cb,
const ::iBS::RUVDistMatrixBatchTask& task,
const Ice::Current& current)
{
	if (task.ks.size() != task.extWs.size() || task.ks.size()!=task.Tasks.size())
	{
		cb->ice_exception();
		return;
	}

	::Ice::Int rt = 1;
	cb->ice_response(rt);
	int taskCnt = (int)task.ks.size();
	for (int i = 0; i < taskCnt; i++)
	{
		int k = task.ks[i];
		int extW = task.extWs[i];
		ostringstream os;
		os << "dist_K" <<k<<"_extW"<<extW<<".csv";
		std::string outFile = os.str();
		task.ruv->SetActiveK(k, extW);
		cout << outFile << " [begin]" << endl;
		const iBS::DistMatrixTask& cmtask = task.Tasks[i];
		
		int threadCnt = cmtask.ThreadCnt;
		if (threadCnt < 1)
		{
			threadCnt = 1;
		}
		Ice::Long ramMb = cmtask.RamMb;
		if (cmtask.RamMb>threadCnt * 250)
		{
			ramMb = threadCnt * 250;
		}

		if (cmtask.Distance == iBS::DistPCC)
		{
			CPearsonCorrelationBuilder builder(cmtask, outFile);
			builder.Calculate(threadCnt, ramMb);
		}
		else if (cmtask.Distance == iBS::DistEuclidean)
		{
			CEuclideanDistMatrixBuilder builder(cmtask, outFile);
			builder.Calculate(threadCnt, ramMb);
		}

		cout << outFile << " [end]" << endl;
	}

}

::iBS::DegreeDistriTask
CComputeServiceImpl::GetBlankDegreeDistriTask(const Ice::Current& current)
{
	::iBS::DegreeDistriTask task;
	task.TaskID = 0;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;
	return task;
}

void
CComputeServiceImpl::DegreeDistribution_async(const ::iBS::AMD_ComputeService_DegreeDistributionPtr& cb,
const ::iBS::DegreeDistriTask& task,
const Ice::Current& current)
{
	if (task.SampleIDs.empty())
	{
		::iBS::ArgumentException ex;
		ex.reason = "empty sampleIDs";
		cb->ice_exception(ex);
		return;
	}

	iBS::FeatureObserverSimpleInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(task.SampleIDs, fois);

	::Ice::Int rt = 1;
	cb->ice_response(rt);

	CDegreeDistriBuilder builder(fois,task);
	builder.Calculate(250);
}

::iBS::QuantileTask
CComputeServiceImpl::GetBlankQuantileTask(const Ice::Current& current)
{
	::iBS::QuantileTask task;
	task.TaskID = 0;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;

	int qCnt = 21;
	Ice::Double qStep = 1.0 / (qCnt-1);

	task.Quantiles = iBS::DoubleVec(qCnt, 0);
	for (int i = 0; i < qCnt; i++)
	{
		task.Quantiles[i] = qStep*i;
	}
	return task;
}

void
CComputeServiceImpl::Quantile_async(const ::iBS::AMD_ComputeService_QuantilePtr& cb,
const ::iBS::QuantileTask& task,
const Ice::Current& current)
{
	if (task.SampleIDs.empty())
	{
		::iBS::ArgumentException ex;
		ex.reason = "empty sampleIDs";
		cb->ice_exception(ex);
		return;
	}

	iBS::FeatureObserverSimpleInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(task.SampleIDs, fois);

	CQuantileBuilder builder(fois, task,cb);
	builder.Calculate(250);
}

::Ice::Int
CComputeServiceImpl::GetSampleValByRefVal(::Ice::Double refVal,
::Ice::Long refLibSize,
const ::iBS::IntVec& SampleIDs,
::iBS::DoubleVec& sampleVal,
const Ice::Current& current)
{
	//make sure observer stats is ready
	::iBS::ObserverStatsInfoVec observerStats;
	int rt = CGlobalVars::get()->theObserverStatsDB->GetObserversStats(SampleIDs, observerStats);
	if (rt != 1)
	{
		ostringstream os;
		os << " stats not ready, sample ID=" << SampleIDs[observerStats.size()];
		::iBS::ArgumentException ex(os.str());
		throw ex;
	}
	int sampleCnt = (int)SampleIDs.size();
	sampleVal.resize(sampleCnt, 0);
	for (int i = 0; i < sampleCnt; i++)
	{
		const iBS::ObserverStatsInfo& osi = observerStats[i];
		if (osi.Sum <= 0)
		{
			ostringstream os;
			os << " stats error, sample ID=" << osi.ObserverID;
			::iBS::ArgumentException ex(os.str());
			throw ex;
		}

		sampleVal[i] = refVal / (Ice::Double)(refLibSize);
		sampleVal[i] *= osi.Sum;
	}

	return 1;
}

::iBS::HighValueFeaturesTask
CComputeServiceImpl::GetBlankHighValueFeaturesTask(const Ice::Current& current)
{
	::iBS::HighValueFeaturesTask task;
	task.TaskID = 0;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;
	return task;
}

void
CComputeServiceImpl::HighValueFeatures_async(const ::iBS::AMD_ComputeService_HighValueFeaturesPtr& cb,
const ::iBS::HighValueFeaturesTask& task,
const Ice::Current& current)
{
	if (task.SampleIDs.empty())
	{
		::iBS::ArgumentException ex;
		ex.reason = "empty sampleIDs";
		cb->ice_exception(ex);
		return;
	}

	if (task.UpperLimits.size() != task.SampleIDs.size())
	{
		::iBS::ArgumentException ex;
		ex.reason = "number of library adjust factor and samples should be equal";
		cb->ice_exception(ex);
		return;
	}

	iBS::FeatureObserverSimpleInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(task.SampleIDs, fois);

	CHighValueFeaturesBuilder builder(fois, task,cb);
	builder.Calculate(250);
}

::iBS::FeatureRowAdjustTask
CComputeServiceImpl::GetBlankFeatureRowAdjustTask(const Ice::Current& current)
{
	::iBS::FeatureRowAdjustTask task;
	task.TaskID = 0;
	task.RecalculateStats = true;
	return task;
}

void
CComputeServiceImpl::FeatureRowAdjust_async(const ::iBS::AMD_ComputeService_FeatureRowAdjustPtr& cb,
const ::iBS::FeatureRowAdjustTask& task,
const Ice::Current& current)
{
	if (task.SampleIDs.empty())
	{
		::iBS::ArgumentException ex;
		ex.reason = "empty sampleIDs";
		cb->ice_exception(ex);
		return;
	}

	if (task.AdjustedValues.size() != task.SampleIDs.size())
	{
		::iBS::ArgumentException ex;
		ex.reason = "number of adjusted values and samples should be equal";
		cb->ice_exception(ex);
		return;
	}

	iBS::FeatureObserverSimpleInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(task.SampleIDs, fois);

	CFeatureRowAdjustBuilder builder(fois, task, cb);
	builder.Adjust();

	
}

::iBS::ExportRowMatrixTask
CComputeServiceImpl::GetBlankExportRowMatrixTask(const Ice::Current& current)
{
	::iBS::ExportRowMatrixTask task;
	task.TaskID = 0;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;
	task.FileSizeLimitInMBytes = 1024;
	task.OutPath = ".";
	task.OutID = 10000;
	task.ConvertToType = iBS::FeatureValueDouble;
	task.RowAdjust = ::iBS::RowAdjustNone;
	task.ValueAdjust = ::iBS::ValueAdjustNone;
	task.TaskName = "Export Matrix";
	return task;
}

void
CComputeServiceImpl::ExportRowMatrix_async(const ::iBS::AMD_ComputeService_ExportRowMatrixPtr& cb,
const ::iBS::ExportRowMatrixTask& task,
const Ice::Current& current)
{
	if (task.SampleIDs.empty())
	{
		::iBS::ArgumentException ex;
		ex.reason = "empty column IDs";
		cb->ice_exception(ex);
		return;
	}

	iBS::FeatureObserverSimpleInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(task.SampleIDs, fois);

	Ice::Long taskID = CGlobalVars::get()->theFeatureValueWorkerMgr->RegisterAMDTask(
		task.TaskName, 0);
	cb->ice_response(1,taskID);

	::iBS::ExportRowMatrixTask task_d(task);
	task_d.TaskID = taskID;
	CExportRowMatrixBuilder builder(task_d);
	builder.Export();

	
}

::iBS::Vectors2MatrixTask
CComputeServiceImpl::GetBlankVectors2MatrixTask(const Ice::Current& current)
{
	::iBS::Vectors2MatrixTask task;
	task.TaskID = 0;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;
	task.PerBatchInMBytes = 1024;
	return task;
}


void
CComputeServiceImpl::Vectors2Matrix_async(const ::iBS::AMD_ComputeService_Vectors2MatrixPtr& cb,
const ::iBS::Vectors2MatrixTask& task,
const Ice::Current& current)
{
	bool rt = false;

	size_t observerCnt = task.InOIDs.size();
	if (observerCnt == 0)
	{
		::iBS::ArgumentException ex("empty observer");
		cb->ice_exception(ex);
		return;
	}

	//test validity
	iBS::FeatureObserverSimpleInfoVec in_fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(task.InOIDs, in_fois);
	Ice::Long domainSize = 0;
	for (size_t i = 0; i < observerCnt; i++)
	{
		iBS::FeatureObserverSimpleInfoPtr foi = in_fois[i];
		if (!foi){
			::iBS::ArgumentException ex("illegal observer ID");
			cb->ice_exception(ex);
			return;
		}

		domainSize = foi->DomainSize;
		if (task.FeatureIdxFrom < 0 || task.FeatureIdxTo < 0 || task.FeatureIdxTo <= task.FeatureIdxFrom
			|| domainSize < task.FeatureIdxTo)
		{
			::iBS::ArgumentException ex("illegal feature idx range");
			cb->ice_exception(ex);
			return;
		}
	}

	// create observer group info
	iBS::FeatureObserverInfoVec in_FOIs;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(task.InOIDs, in_FOIs);

	iBS::IntVec observerIDs;
	Ice::Long colCnt = (Ice::Long)observerCnt;
	CGlobalVars::get()->theObserversDB.RqstNewFeatureObserversInGroup(
		(int)colCnt, observerIDs, false);

	iBS::FeatureObserverInfoVec out_FOIs;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(observerIDs, out_FOIs);
	domainSize = task.FeatureIdxTo - task.FeatureIdxFrom;
	for (int i = 0; i < colCnt; i++)
	{
		iBS::FeatureObserverInfo& foi = out_FOIs[i];
		foi.ObserverName = in_FOIs[i].ObserverName;
		foi.ParentObserverID = in_FOIs[i].ObserverID;
		foi.DomainSize = domainSize;
		foi.SetPolicy = iBS::FeatureValueSetPolicyDoNothing;	//readonly outside, interannly writalbe
		foi.GetPolicy = iBS::FeatureValueGetPolicyGetForOneTimeRead;
	}
	CGlobalVars::get()->theObserversDB.SetFeatureObservers(out_FOIs);

	::iBS::Vectors2MatrixTask task_d(task);
	task_d.OutOIDs = observerIDs;
	iBS::FeatureObserverSimpleInfoVec out_fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(task_d.OutOIDs, out_fois);

	Ice::Long taskID = CGlobalVars::get()->theFeatureValueWorkerMgr->RegisterAMDTask(
		"Vectors2MatrixTask", 100);
	cb->ice_response(1, task_d.OutOIDs, taskID);

	task_d.TaskID = taskID;
	CVectors2MatrixBuilder builder(task_d, in_fois, out_fois);
	builder.DoWork(task_d.PerBatchInMBytes);
}

::iBS::ExportZeroOutBgRowMatrixTask
CComputeServiceImpl::GetBlankExportZeroOutBgRowMatrixTask(const Ice::Current& current)
{
	::iBS::ExportZeroOutBgRowMatrixTask task;
	task.TaskID = 0;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;
	task.NeedLogFirst = true;
	task.BgSignalDifference =2;
	task.BgWindowRadius = 10000;
	task.FileSizeLimitInMBytes = 1024;
	task.OutPath = ".";
	task.OutID = 10000;
	task.ConvertToType = iBS::FeatureValueDouble;
	return task;
}

void
CComputeServiceImpl::ExportZeroOutBgRowMatrix_async(const ::iBS::AMD_ComputeService_ExportZeroOutBgRowMatrixPtr& cb,
const ::iBS::ExportZeroOutBgRowMatrixTask& task,
const Ice::Current& current)
{
	if (task.SampleIDs.empty())
	{
		::iBS::ArgumentException ex;
		ex.reason = "empty sampleIDs";
		cb->ice_exception(ex);
		return;
	}

	iBS::FeatureObserverSimpleInfoVec fois;
	CGlobalVars::get()->theObserversDB.GetFeatureObservers(task.SampleIDs, fois);
	cb->ice_response(1);

	CExportZeroOutBgRowMatrixBuilder builder(task);
	builder.Export();
}


void
CComputeServiceImpl::GetRowMatrix_async(const ::iBS::AMD_ComputeService_GetRowMatrixPtr& cb,
const ::iBS::ExportRowMatrixTask& task,
::Ice::Long featureIdxFrom,
::Ice::Long featureIdxTo,
const Ice::Current& current)
{
	if (task.SampleIDs.empty())
	{
		::iBS::ArgumentException ex;
		ex.reason = "empty sampleIDs";
		cb->ice_exception(ex);
		return;
	}

	//ret always pointing to continues RAM that is ready to send to wire
	std::pair<const Ice::Double*, const Ice::Double*> ret(0, 0);

	//retValues manage newly allocated mem (if retValues.get() is not null), exclusively for this call,
	//therefore, can be in-place editing without effecting original value
	::IceUtil::ScopedArray<Ice::Double>  values;

	CExportRowMatrixBuilder builder(task);
	builder.ReadRowMatrix(featureIdxFrom, featureIdxTo, ret, values);

	cb->ice_response(1, ret);
	
}

::iBS::ExportByRowIdxsTask
CComputeServiceImpl::GetBlankExportByRowIdxsTask(const Ice::Current& current)
{
	::iBS::ExportByRowIdxsTask task;
	task.TaskID = 0;
	task.FileSizeLimitInMBytes = 1024;
	task.OutPath = ".";
	task.OutID = 10000;
	task.PerRqstLimitInMBytes = 250;
	task.RowAdjust = ::iBS::RowAdjustNone;
	task.ValueAdjust = ::iBS::ValueAdjustNone;
	return task;
}

::iBS::RUVExportByRowIdxsBatchTask
CComputeServiceImpl::GetBlankRUVExportByRowIdxsBatchTask(const Ice::Current& current)
{
	return ::iBS::RUVExportByRowIdxsBatchTask();
}

void
CComputeServiceImpl::ExportByRowIdxs_async(const ::iBS::AMD_ComputeService_ExportByRowIdxsPtr& cb,
const ::iBS::ExportByRowIdxsTask& task,
const Ice::Current& current)
{
	Ice::Long taskID = CGlobalVars::get()->theFeatureValueWorkerMgr->RegisterAMDTask(
		"ExportByRowIdxs", 100);
	::Ice::Int rt = 1;
	cb->ice_response(rt, taskID);

	cout <<" ExportByRowIdxs [begin]" << endl;
	const iBS::ExportByRowIdxsTask& subTask = task;
	Ice::Long ramMb = subTask.PerRqstLimitInMBytes;
	CExportByRowIdxsBuilder builder(subTask, taskID);
	builder.Export(ramMb);
	cout <<" outID=" << subTask.OutID << " [end]" << endl;
	CGlobalVars::get()->theFeatureValueWorkerMgr->SetAMDTaskDone(taskID);
}

void
CComputeServiceImpl::RUVExportByRowIdxsBatch_async(
const ::iBS::AMD_ComputeService_RUVExportByRowIdxsBatchPtr& cb,
const ::iBS::RUVExportByRowIdxsBatchTask& task,
const Ice::Current& current)
{
	if (task.ks.size() != task.extWs.size() || task.ks.size() != task.Tasks.size())
	{
		cb->ice_exception();
		return;
	}

	Ice::Long taskID = CGlobalVars::get()->theFeatureValueWorkerMgr->RegisterAMDTask(
		"RUVExportByRowIdxsBatch", 0);

	::Ice::Int rt = 1;
	cb->ice_response(rt, taskID);
	int taskCnt = (int)task.ks.size();
	for (int i = 0; i < taskCnt; i++)
	{
		int k = task.ks[i];
		int extW = task.extWs[i];
		ostringstream os;
		os << "data_K" << k << "_extW" << extW;
		std::string outFile = os.str();
		task.ruv->SetActiveK(k, extW);
		
		cout << outFile << " [begin]" << endl;
		const iBS::ExportByRowIdxsTask& subTask = task.Tasks[i];
		Ice::Long ramMb = subTask.PerRqstLimitInMBytes;
		iBS::FeatureObserverInfoVec fois;
		iBS::IntVec oids;
		task.ruv->GetFeatureObservers(oids, fois);
		Ice::Double rqstColCnt = (Ice::Double)subTask.SampleIDs.size();
		Ice::Double fullColCnt = (Ice::Double)fois.size();
		Ice::Long  rqstLimitInCentral = (Ice::Long)(2000 * rqstColCnt / fullColCnt); //2G limit in FCDCentral
		if (ramMb > rqstLimitInCentral)
		{
			ramMb = rqstLimitInCentral;
		}

		CExportByRowIdxsBuilder builder(subTask, taskID);
		builder.Export(ramMb);

		cout << outFile << " outID=" << subTask.OutID << " [end]" << endl;
	}
	CGlobalVars::get()->theFeatureValueWorkerMgr->SetAMDTaskDone(taskID);
}

::iBS::HighVariabilityFeaturesTask
CComputeServiceImpl::GetBlankHighVariabilityFeaturesTask(const Ice::Current& current)
{
	::iBS::HighVariabilityFeaturesTask task;
	task.TaskID = 0;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;
	task.SamplingFeatureCnt = 0;
	task.RowAdjust = ::iBS::RowAdjustNone;
	task.ValueAdjust = ::iBS::ValueAdjustNone;
	task.VariabilityTest = ::iBS::VariabilityTestGCV;
	task.FeatureFilterMaxCntLowThreshold = 0;
	task.VariabilityCutoff = 0;
	return task;
}

void
CComputeServiceImpl::HighVariabilityFeatures_async(const ::iBS::AMD_ComputeService_HighVariabilityFeaturesPtr& cb,
const ::iBS::HighVariabilityFeaturesTask& task,
const Ice::Current& current)
{
	CFeatureVariabilityBuilder builder(task, cb);
	builder.Calculate(250);
}

::iBS::RowANOVATask
CComputeServiceImpl::GetBlankRowANOVATask(const Ice::Current& current)
{
	::iBS::RowANOVATask task;
	task.TaskID = 0;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;
	task.SamplingFeatureCnt = 0;
	task.ValueAdjust = ::iBS::ValueAdjustNone;
	task.RowAdjust = ::iBS::RowAdjustNone;
	return task;
}

void
CComputeServiceImpl::RowANOVA_async(const ::iBS::AMD_ComputeService_RowANOVAPtr& cb,
const ::iBS::RowANOVATask& task,
const Ice::Current& current)
{
	CRowANOVABuilder builder(task, cb);
	builder.Calculate(250);
}

::iBS::BetweenGroupTestTask
CComputeServiceImpl::GetBlankBetweenGroupTestTask(const Ice::Current& current)
{
	::iBS::BetweenGroupTestTask task;
	task.TaskID = 0;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;
	task.SamplingFeatureCnt = 0;
	task.ValueAdjust = ::iBS::ValueAdjustNone;
	task.RowAdjust = ::iBS::RowAdjustNone;
	task.Test = ::iBS::TTestUnequalVar;
	return task;
}

void
CComputeServiceImpl::BetweenGroupTest_async(const ::iBS::AMD_ComputeService_BetweenGroupTestPtr& cb,
const ::iBS::BetweenGroupTestTask& task,
const Ice::Current& current)
{
	
}

::iBS::WithSignalFeaturesTask
CComputeServiceImpl::GetBlankWithSignalFeaturesTask(const Ice::Current& current)
{
	::iBS::WithSignalFeaturesTask task;
	task.TaskID = 0;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;
	task.SignalThreshold = 0;
	task.SampleCntAboveThreshold = 1;
	task.SamplingFeatureCnt = 0;
	task.RowAdjust = ::iBS::RowAdjustNone;
	task.PerRqstLimitInMBytes = 250;
	return task;
}

void
CComputeServiceImpl::WithSignalFeatures_async(const ::iBS::AMD_ComputeService_WithSignalFeaturesPtr& cb,
const ::iBS::WithSignalFeaturesTask& task,
const Ice::Current& current)
{
	CWithSignalFeaturesBuilder builder(task, cb);
	builder.Calculate(task.PerRqstLimitInMBytes);
}

::iBS::VdAnovaTask
CComputeServiceImpl::GetBlankVdAnovaTask(const Ice::Current& current)
{
	::iBS::VdAnovaTask task;
	task.TaskID = 0;
	task.FeatureIdxFrom = 0;
	task.FeatureIdxTo = 0;
	task.PerRqstLimitInMBytes = 250;
	return task;
}

::iBS::RuvVdAnovaBatchTask
CComputeServiceImpl::GetBlankRuvVdAnovaBatchTask(const Ice::Current& current)
{
	::iBS::RuvVdAnovaBatchTask task;
	task.OutPath = "vdanova.txt";
	return task;
}

void
CComputeServiceImpl::RuvVdAnovaBatch_async(const ::iBS::AMD_ComputeService_RuvVdAnovaBatchPtr& cb,
const ::iBS::RuvVdAnovaBatchTask& task,
const Ice::Current& current)
{
	if (task.ks.size() != task.extWs.size() || task.ks.size() != task.Tasks.size())
	{
		cb->ice_exception();
		return;
	}

	Ice::Long taskID = CGlobalVars::get()->theFeatureValueWorkerMgr->RegisterAMDTask(
		"RuvVdAnovaBatch", 0);

	std::ofstream ofs(task.OutPath.c_str(), std::ofstream::out);
	::Ice::Int rt = 1;
	cb->ice_response(rt, taskID);
	int taskCnt = (int)task.ks.size();
	for (int i = 0; i < taskCnt; i++)
	{
		int k = task.ks[i];
		int extW = task.extWs[i];
		ostringstream os;
		os << "data_K" << k << "_extW" << extW;
		std::string outFile = os.str();
		task.ruv->SetActiveK(k, extW);

		cout << outFile << " [begin]" << endl;
		const iBS::VdAnovaTask& subTask = task.Tasks[i];
		Ice::Long ramMb = subTask.PerRqstLimitInMBytes;
		iBS::FeatureObserverInfoVec fois;
		iBS::IntVec oids;
		task.ruv->GetFeatureObservers(oids, fois);
		Ice::Double rqstColCnt = (Ice::Double)subTask.SampleIDs.size();
		Ice::Double fullColCnt = (Ice::Double)fois.size();
		Ice::Long  rqstLimitInCentral = (Ice::Long)(2000 * rqstColCnt / fullColCnt); //2G limit in FCDCentral
		if (ramMb > rqstLimitInCentral)
		{
			ramMb = rqstLimitInCentral;
		}
		iBS::VdAnovaResult ret;
		CRuvVdAnovaBuilder builder(subTask, taskID);
		builder.DoWork(ret, ramMb);
		ofs << (i + 1) << '\t'
			<< k << '\t'
			<< extW << '\t'
			<< ret.totalVar << '\t'
			<< ret.bgVar << '\t'
			<< ret.wgVar << endl;
	}
	ofs.close();
	CGlobalVars::get()->theFeatureValueWorkerMgr->SetAMDTaskDone(taskID);
}