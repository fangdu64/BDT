#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <ComputeBuilder.h>
#include <algorithm>    // std::copy
#include <GlobalVars.h>
#include <ComputeWorkerMgr.h>
#include <ComputeWorkItem.h>
#include <ComputeWorker.h>
#include <math.h>
#include <bdtUtil/RowAdjustHelper.h>
#include <bdtUtil/SortHelper.h>
#include <armadillo>
#include <FeatureValueStoreMgr.h>
#include <StatisticsHelper.h>
#include <FeatureValueWorkerMgr.h>
#include <FeatureValueWorker.h>
#include <FeatureValueWorkItem.h>

/////////////////////////////////////////////////////////////////
CMultiThreadBuilder::CMultiThreadBuilder()
{
	m_shutdownRequested = false;
	m_needNotify = false;
}


CMultiThreadBuilder::~CMultiThreadBuilder()
{
}

void CMultiThreadBuilder::NotifyWorkerBecomesFree(int workerIdx)
{
	IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);

	m_freeWorkerIdxs.push_back(workerIdx);

	if (m_needNotify)
	{
		m_monitor.notify();
	}
}

//////////////////////////////////////////////////////////////////
CPearsonCorrelationBuilder::CPearsonCorrelationBuilder(const ::iBS::DistMatrixTask& task, const std::string& outFile)
:m_task(task), m_colMeans(task.SampleIDs.size(), 0), m_outFile(outFile)
{

}


CPearsonCorrelationBuilder::~CPearsonCorrelationBuilder()
{

}

bool CPearsonCorrelationBuilder::CalcualteColumnMeans(::Ice::Long ramMb)
{
	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(
		m_task.TaskID, "DistMatrix.Correlation ColumnMeans", batchCnt);

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	Ice::Double validRowCnt = 0;
	bool needRowFlag = m_task.Subset.Enable;
	iBS::ByteVec rowFlags;
	if (needRowFlag)
	{
		rowFlags.resize(batchRowCnt, 0);
		m_task.Subset.SelectedCnt =0;
		m_task.Subset.UnselectedCnt =0;
	}
	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " CalcualteColumnMeans batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, IceUtil::None, Y);

		if (needRowFlag)
		{
			Ice::Double *pY = reinterpret_cast<Ice::Double*>(&Y[0]);
			Ice::Byte *pRowFlags = reinterpret_cast<Ice::Byte*>(&rowFlags[0]);
			Ice::Long selectedCnt=
				CRowAdjustHelper::RowSelectedFlags(pY, thisBatchRowCnt, colCnt, m_task.Subset, pRowFlags);
			m_task.Subset.SelectedCnt += selectedCnt;
			m_task.Subset.UnselectedCnt += (thisBatchRowCnt-selectedCnt);
		}

		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			if (needRowFlag && rowFlags[i] == 0)
			{
				continue;
			}
			validRowCnt++;
			for (Ice::Long j = 0; j<colCnt; j++)
			{
				m_colMeans[j] += Y[i*colCnt + j];
			}
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_task.TaskID, 1);
	}

	if (validRowCnt < 1)
	{
		validRowCnt = 1;
	}

	for (Ice::Long j = 0; j<colCnt; j++)
	{
		m_colMeans[j] /= validRowCnt;
	}

	cout << IceUtil::Time::now().toDateTime() << " CalcualteColumnMeans RowCnt " << validRowCnt << ", " << m_task.Subset.UnselectedCnt << endl;

	return true;
}

bool CPearsonCorrelationBuilder::MultithreadPC(::Ice::Int  threadCnt, ::Ice::Long ramMb)
{
	Ice::Long colCnt = m_task.SampleIDs.size();

	ramMb /= sizeof(Ice::Double);
	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	CPearsonCorrelationWorkerMgr workerMgr(threadCnt);
	//each worker will allocate Y with size of batchValueCnt 
	if (workerMgr.Initilize(batchValueCnt, colCnt) == false)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
		return false;
	}
	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;
	::Ice::Long batchCnt = remainCnt / batchRowCnt + 1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(
		m_task.TaskID, "DistMatrix.Correlation MultithreadPC", batchCnt);

	bool bContinue = true;
	bool bNeedExit = false;
	m_needNotify = false;

	int batchIdx = 0;
	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();

	for (int i = 0; i<threadCnt; i++)
	{
		m_freeWorkerIdxs.push_back(i);
	}

	while (!bNeedExit)
	{

		{
			//entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while (!m_shutdownRequested)
			{
				//merge freeworker that has not been lauched due to remainCnt==0
				if (!m_processingWorkerIdxs.empty())
				{
					std::copy(m_processingWorkerIdxs.begin(), m_processingWorkerIdxs.end(),
						std::back_inserter(m_freeWorkerIdxs));
					m_processingWorkerIdxs.clear();
				}

				//have to wait if 1: remain count>0, but no free worker; 
				//or 2. remain count==0, worker not finished yet
				if ((remainCnt>0 && m_freeWorkerIdxs.empty())
					|| (remainCnt == 0 && m_freeWorkerIdxs.size()<threadCnt))
				{
					m_needNotify = true;
					m_monitor.wait();
				}

				//free worker need to finish left items
				if (remainCnt>0 && !m_freeWorkerIdxs.empty())
				{
					std::copy(m_freeWorkerIdxs.begin(), m_freeWorkerIdxs.end(),
						std::back_inserter(m_processingWorkerIdxs));
					m_freeWorkerIdxs.clear();
					break;
				}
				else if (remainCnt == 0 && m_freeWorkerIdxs.size() == threadCnt)
				{
					bNeedExit = true;
					break;

				}
			}

			//if control request shutdown
			if (m_shutdownRequested)
			{
				bContinue = false;
				bNeedExit = true;
				cout << "CPearsonCorrelationBuilder m_shutdownRequested..." << endl;
			}

			//leaving critical region
			m_needNotify = false;
		}

		//these workers are free, need to use them to process
		while (!m_processingWorkerIdxs.empty() && remainCnt>0)
		{
			int workerIdx = m_processingWorkerIdxs.front();

			if (remainCnt>batchRowCnt){
				thisBatchRowCnt = batchRowCnt;
			}
			else{
				thisBatchRowCnt = remainCnt;
			}
			batchIdx++;

			cout << IceUtil::Time::now().toDateTime() << " MultithreadPC batch " << batchIdx << "/" << batchCnt << " begin" << endl;

			featureIdxTo = featureIdxFrom + thisBatchRowCnt;

			ComputeWorkerPtr worker = workerMgr.GetComputeWorker(workerIdx);
			CPearsonCorrelationWorker *pcWorker = static_cast<CPearsonCorrelationWorker*>(worker.get());

			m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, IceUtil::None, pcWorker->GetBatchY());

			ComputeWorkItemPtr wi = new CPearsonCorrelation(
				*this, workerIdx, m_task.Subset, pcWorker->GetBatchY(), thisBatchRowCnt, colCnt, m_colMeans,
				pcWorker->A, pcWorker->ColSumSquares);

			worker->AddWorkItem(wi);

			m_processingWorkerIdxs.pop_front();
			featureIdxFrom += thisBatchRowCnt;
			remainCnt -= thisBatchRowCnt;
			CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_task.TaskID, 1);
		}

	}

	if (!bContinue)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
		return false;
	}

	cout << IceUtil::Time::now().toDateTime() << " MultithreadPC batch " << batchIdx << "/" << batchCnt << " end" << endl;

	::arma::mat A((int)colCnt, (int)colCnt, arma::fill::zeros);
	::arma::mat B((int)colCnt, 1, arma::fill::zeros);
	for (int i = 0; i<threadCnt; i++)
	{
		ComputeWorkerPtr worker = workerMgr.GetComputeWorker(i);
		CPearsonCorrelationWorker *pcWorker = static_cast<CPearsonCorrelationWorker*>(worker.get());
		A += pcWorker->A;
		B += pcWorker->ColSumSquares;
	}

	workerMgr.RequestShutdownAllWorkers();
	workerMgr.UnInitilize();

	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();

	//update the lower left triagle of A
	for (int i = 0; i<colCnt; i++)
	{
		A(i, i) = 1.0;
		for (int j = i + 1; j<colCnt; j++)
		{
			Ice::Double d = sqrt(B[i])*sqrt(B[j]);
			A(i, j) /= d;
			A(j, i) = A(i, j);
		}
	}

	std::ofstream ofs(m_outFile.c_str(), std::ofstream::out);
	for (int i = 0; i<colCnt; i++)
	{
		for (int j = 0; j<colCnt; j++)
		{
			ofs << A(i, j);
			if (j == colCnt - 1)
			{
				ofs << endl;
			}
			else
			{
				ofs << '\t';
			}
		}
	}
	ofs.close();

	return true;
}

void CPearsonCorrelationBuilder::Calculate(::Ice::Int  threadCnt, ::Ice::Long ramMb)
{
	CalcualteColumnMeans(250);
	MultithreadPC(threadCnt, ramMb);
}

/////////////////////////////////////////////////////////////////
CEuclideanDistMatrixBuilder::CEuclideanDistMatrixBuilder(const ::iBS::DistMatrixTask& task, const std::string& outFile)
:m_task(task), m_outFile(outFile)
{

}

CEuclideanDistMatrixBuilder::~CEuclideanDistMatrixBuilder()
{

}

void CEuclideanDistMatrixBuilder::Calculate(::Ice::Int  threadCnt, ::Ice::Long ramMb)
{
	Ice::Long colCnt = m_task.SampleIDs.size();

	ramMb /= sizeof(Ice::Double);
	ramMb /= threadCnt;

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	CEuclideanDistMatrixWorkerMgr workerMgr(threadCnt);
	//each worker will allocate Y with size of batchValueCnt 
	if (workerMgr.Initilize(batchValueCnt, colCnt) == false)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
		return;
	}
	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;
	::Ice::Long batchCnt = remainCnt / batchRowCnt + 1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(
		m_task.TaskID, "DistMatrix.Euclidean", batchCnt);

	bool bContinue = true;
	bool bNeedExit = false;
	m_needNotify = false;

	int batchIdx = 0;
	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();

	for (int i = 0; i<threadCnt; i++)
	{
		m_freeWorkerIdxs.push_back(i);
	}

	while (!bNeedExit)
	{

		{
			//entering critical region
			IceUtil::Monitor<IceUtil::Mutex>::Lock lock(m_monitor);
			while (!m_shutdownRequested)
			{
				//merge freeworker that has not been lauched due to remainCnt==0
				if (!m_processingWorkerIdxs.empty())
				{
					std::copy(m_processingWorkerIdxs.begin(), m_processingWorkerIdxs.end(),
						std::back_inserter(m_freeWorkerIdxs));
					m_processingWorkerIdxs.clear();
				}

				//have to wait if 1: remain count>0, but no free worker; 
				//or 2. remain count==0, worker not finished yet
				if ((remainCnt>0 && m_freeWorkerIdxs.empty())
					|| (remainCnt == 0 && m_freeWorkerIdxs.size()<threadCnt))
				{
					m_needNotify = true;
					m_monitor.wait();
				}

				//free worker need to finish left items
				if (remainCnt>0 && !m_freeWorkerIdxs.empty())
				{
					std::copy(m_freeWorkerIdxs.begin(), m_freeWorkerIdxs.end(),
						std::back_inserter(m_processingWorkerIdxs));
					m_freeWorkerIdxs.clear();
					break;
				}
				else if (remainCnt == 0 && m_freeWorkerIdxs.size() == threadCnt)
				{
					bNeedExit = true;
					break;

				}
			}

			//if control request shutdown
			if (m_shutdownRequested)
			{
				bContinue = false;
				bNeedExit = true;
				cout << "CEuclideanDistMatrixBuilder m_shutdownRequested..." << endl;
			}

			//leaving critical region
			m_needNotify = false;
		}

		//these workers are free, need to use them to process
		while (!m_processingWorkerIdxs.empty() && remainCnt>0)
		{
			int workerIdx = m_processingWorkerIdxs.front();

			if (remainCnt>batchRowCnt){
				thisBatchRowCnt = batchRowCnt;
			}
			else{
				thisBatchRowCnt = remainCnt;
			}
			batchIdx++;

			cout << IceUtil::Time::now().toDateTime() << " EuclideanDistMatrix batch " << batchIdx << "/" << batchCnt << " begin" << endl;

			featureIdxTo = featureIdxFrom + thisBatchRowCnt;

			ComputeWorkerPtr worker = workerMgr.GetComputeWorker(workerIdx);
			CEuclideanDistMatrixWorker *pcWorker = static_cast<CEuclideanDistMatrixWorker*>(worker.get());

			m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, IceUtil::None, pcWorker->GetBatchY());

			ComputeWorkItemPtr wi = new CEuclideanDist(
				*this, workerIdx, m_task.Subset, pcWorker->GetBatchY(), thisBatchRowCnt, colCnt,
				pcWorker->A);

			worker->AddWorkItem(wi);

			m_processingWorkerIdxs.pop_front();
			featureIdxFrom += thisBatchRowCnt;
			remainCnt -= thisBatchRowCnt;
			CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_task.TaskID, 1);
		}

	}

	if (!bContinue)
	{
		workerMgr.RequestShutdownAllWorkers();
		workerMgr.UnInitilize();
	}

	cout << IceUtil::Time::now().toDateTime() << " EuclideanDistMatrix batch " << batchIdx << "/" << batchCnt << " end" << endl;

	::arma::mat A((int)colCnt, (int)colCnt, arma::fill::zeros);
	::arma::mat B((int)colCnt, 1, arma::fill::zeros);
	for (int i = 0; i<threadCnt; i++)
	{
		ComputeWorkerPtr worker = workerMgr.GetComputeWorker(i);
		CEuclideanDistMatrixWorker *pcWorker = static_cast<CEuclideanDistMatrixWorker*>(worker.get());
		A += pcWorker->A;
	}

	workerMgr.RequestShutdownAllWorkers();
	workerMgr.UnInitilize();

	m_freeWorkerIdxs.clear();
	m_processingWorkerIdxs.clear();

	//update the lower left triagle of A
	for (int i = 0; i<colCnt; i++)
	{
		A(i, i) = 0;
		for (int j = i + 1; j<colCnt; j++)
		{
			A(i, j) = sqrt(A(i, j));
			A(j, i) = A(i, j);
		}
	}

	std::ofstream ofs(m_outFile.c_str(), std::ofstream::out);
	for (int i = 0; i<colCnt; i++)
	{
		for (int j = 0; j<colCnt; j++)
		{
			ofs << A(i, j);
			if (j == colCnt - 1)
			{
				ofs << endl;
			}
			else
			{
				ofs << '\t';
			}
		}
	}
	ofs.close();

}

//////////////////////////////////////////////////////////////////
CDegreeDistriBuilder::CDegreeDistriBuilder(const iBS::FeatureObserverSimpleInfoVec& fois,
	const::iBS::DegreeDistriTask& task)
	:m_fois(fois), m_task(task), m_lowK(100000)
{

}


CDegreeDistriBuilder::~CDegreeDistriBuilder()
{

}

void CDegreeDistriBuilder::Calculate(::Ice::Long ramMb)
{
	if (m_task.FeatureIdxTo == 0)
	{
		m_task.FeatureIdxTo = m_fois[0]->DomainSize;
	}
	Ice::Long colCnt = m_task.SampleIDs.size();
	Ice::Long totalValuesLowK = m_lowK*colCnt;
	m_sampleLowPKs.reset(new ::Ice::Int[totalValuesLowK]);
	if (!m_sampleLowPKs.get()){
		return;
	}
	std::fill(m_sampleLowPKs.get(), m_sampleLowPKs.get() + totalValuesLowK, 0);
	m_sampleHighPKmaps.resize(colCnt);

	m_lowKRowSum.resize(m_lowK, 0);
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
		CalculateByFeatureMajor(ramMb);
	}
	else
	{
		CalculateBySampleMajor(ramMb);
	}

	OutputMatrix();

	cout << IceUtil::Time::now().toDateTime() << " CDegreeDistriBuilder::Calculate [end]" << endl;

}

void CDegreeDistriBuilder::OutputMatrix()
{
	Ice::Long sampleCnt = m_task.SampleIDs.size();
	//output
	iBS::LongVec Ks;
	Ks.reserve(m_lowK);
	for (int i = 0; i < m_lowK; i++)
	{
		if (m_lowKRowSum[i]>0)
		{
			Ks.push_back(i);
		}
	}
	Int2Int_T highKs;

	for (int i = 0; i < sampleCnt; i++)
	{
		Int2Int_T& pkmap = m_sampleHighPKmaps[i];
		for (Int2Int_T::iterator it = pkmap.begin(); it != pkmap.end(); it++)
		{
			int k = it->first;
			if (highKs.find(k) == highKs.end())
			{
				highKs[k] = 1;
				Ks.push_back(k);
			}
		}
	}

	iBS::LongVec sortedKs(Ks.size());
	iBS::LongVec originalIdxs(Ks.size());

	CSortHelper::GetSortedValuesAndIdxs(Ks, sortedKs, originalIdxs);
	int kCnt = (int)sortedKs.size();
	::arma::umat A(kCnt, (int)(sampleCnt + 1), ::arma::fill::zeros);

	for (int i = 0; i < kCnt; i++)
	{
		int k = (int)sortedKs[i];
		A(i, 0) = k;
		if (k < m_lowK)
		{
			for (int j = 0; j < sampleCnt; j++)
			{
				A(i,j+1)=m_sampleLowPKs[k*sampleCnt + j];
			}
		}
		else
		{
			for (int j = 0; j < sampleCnt; j++)
			{
				Int2Int_T& pkmap = m_sampleHighPKmaps[j];
				if (pkmap.find(k) != pkmap.end())
				{
					A(i, j + 1) = pkmap[k];
				}
				
			}
		}
	}

	A.save("PKs.csv", arma::csv_ascii);
	cout << IceUtil::Time::now().toDateTime() << " DegreeDistriTask done!" << endl;
}

void CDegreeDistriBuilder::CalculateByFeatureMajor(::Ice::Long ramMb)
{
	if (ramMb > 250)
	{
		ramMb = 250;
	}

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	
	Ice::Int y = 0;
	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " Calculate batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, IceUtil::None, Y);


		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			for (Ice::Long j = 0; j<colCnt; j++)
			{
				y = (Ice::Int)Y[i*colCnt + j];
				if (y < 0)
				{
					cout << "Error: negative read counts, sampleID=" << m_task.SampleIDs[j] << ", featureIdx=" << featureIdxFrom + i << endl;
				}
				else if (y < m_lowK)
				{
					m_sampleLowPKs[y*colCnt + j]++;
					m_lowKRowSum[y]++;
				}
				else
				{
					Int2Int_T::iterator it = m_sampleHighPKmaps[j].find(y);
					if (it == m_sampleHighPKmaps[j].end())
					{
						m_sampleHighPKmaps[j][y] = 1;
					}
					else
					{
						m_sampleHighPKmaps[j][y]++;
					}
				}
			}
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
	}

	return;
}

void CDegreeDistriBuilder::CalculateBySampleMajor(::Ice::Long ramMb)
{
	Ice::Long colCnt = m_task.SampleIDs.size();
	for (int i = 0; i<colCnt; i++)
	{
		CalculateSingleSample(i, ramMb);
	}
}
void CDegreeDistriBuilder::CalculateSingleSample(int sampleIdx, ::Ice::Long ramMb)
{
	if (ramMb > 250)
	{
		ramMb = 250;
	}

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt =1;

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	Ice::Int y = 0;
	int sampleID = m_task.SampleIDs[sampleIdx];
	Ice::Long sampleCnt = m_task.SampleIDs.size();
	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " Calculate sampleID=" << sampleID << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetDoublesColumnVector(sampleID, featureIdxFrom, featureIdxTo, Y);


		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			y = (Ice::Int)Y[i];
			if (y < 0)
			{
				cout << "Error: negative read counts, sampleID=" << sampleID << ", featureIdx=" << featureIdxFrom + i << endl;
			}
			else if (y < m_lowK)
			{
				m_sampleLowPKs[y*sampleCnt + sampleIdx]++;
				m_lowKRowSum[y]++;
			}
			else
			{
				Int2Int_T::iterator it = m_sampleHighPKmaps[sampleIdx].find(y);
				if (it == m_sampleHighPKmaps[sampleIdx].end())
				{
					m_sampleHighPKmaps[sampleIdx][y] = 1;
				}
				else
				{
					m_sampleHighPKmaps[sampleIdx][y]++;
				}
			}
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
	}

	return;
}

//////////////////////////////////////////////////////////////////
CQuantileBuilder::CQuantileBuilder(const iBS::FeatureObserverSimpleInfoVec& fois,
	const::iBS::QuantileTask& task, const ::iBS::AMD_ComputeService_QuantilePtr& cb)
	:m_fois(fois), m_task(task), m_cb(cb)
{

}


CQuantileBuilder::~CQuantileBuilder()
{

}

void CQuantileBuilder::Calculate(::Ice::Long ramMb)
{
	if (m_task.FeatureIdxTo == 0)
	{
		m_task.FeatureIdxTo = m_fois[0]->DomainSize;
	}
	Ice::Long colCnt = m_task.SampleIDs.size();
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

	::iBS::LongVecVec qFeatureIdxs(colCnt);
	::iBS::DoubleVecVec qValues(colCnt);
	Ice::Long qCnt = m_task.Quantiles.size();
	for (int i = 0; i < colCnt; i++)
	{
		qFeatureIdxs[i].resize(qCnt, 0);
		qValues[i].resize(qCnt, 0);
	}

	if (bGetGroupRowMatrix)
	{
		CalculateByFeatureMajor(ramMb, qFeatureIdxs, qValues);
	}
	else
	{
		CalculateBySampleMajor(ramMb, qFeatureIdxs, qValues);
	}

	cout << IceUtil::Time::now().toDateTime() << " CQuantileBuilder::Calculate [end]" << endl;
	m_cb->ice_response(1, qFeatureIdxs, qValues);
}

void CQuantileBuilder::CalculateByFeatureMajor(::Ice::Long ramMb, ::iBS::LongVecVec& qFeatureIdxs, ::iBS::DoubleVecVec& qValues)
{
	return;
}

void CQuantileBuilder::CalculateBySampleMajor(::Ice::Long ramMb, ::iBS::LongVecVec& qFeatureIdxs, ::iBS::DoubleVecVec& qValues)
{
	Ice::Long colCnt = m_task.SampleIDs.size();
	for (int i = 0; i<colCnt; i++)
	{
		CalculateSingleSample(i, ramMb, qFeatureIdxs, qValues);
	}
}
void CQuantileBuilder::CalculateSingleSample(int sampleIdx, ::Ice::Long ramMb, ::iBS::LongVecVec& qFeatureIdxs, ::iBS::DoubleVecVec& qValues)
{
	if (ramMb > 250)
	{
		ramMb = 250;
	}

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = 1;

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	iBS::DoubleVec  allY(TotalRowCnt);

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	int sampleID = m_task.SampleIDs[sampleIdx];
	Ice::Long sampleCnt = m_task.SampleIDs.size();
	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " Calculate sampleID=" << sampleID << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetDoublesColumnVector(sampleID, featureIdxFrom, featureIdxTo, Y);

		Ice::Double *pSource = reinterpret_cast<Ice::Double*>(&Y[0]);
		Ice::Double *pDest = reinterpret_cast<Ice::Double*>(&allY[0]) + featureIdxFrom - m_task.FeatureIdxFrom;
		std::copy(pSource, pSource + thisBatchRowCnt, pDest);

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
	}

	iBS::LongVec originalIdxs(TotalRowCnt);

	CSortHelper::GetSortedIdxs(allY, originalIdxs);
	Ice::Long qCnt = m_task.Quantiles.size();
	for (int i = 0; i < qCnt; i++)
	{
		Ice::Long qIdx = originalIdxs[(Ice::Long)((TotalRowCnt - 1)*m_task.Quantiles[i])];
		Ice::Double qVal = allY[qIdx];
		qFeatureIdxs[sampleIdx][i] = qIdx + m_task.FeatureIdxFrom;
		qValues[sampleIdx][i] = qVal;
	}

	return;
}

//////////////////////////////////////////////////////////////////
CHighValueFeaturesBuilder::CHighValueFeaturesBuilder(const iBS::FeatureObserverSimpleInfoVec& fois,
	const ::iBS::HighValueFeaturesTask& task,
	const ::iBS::AMD_ComputeService_HighValueFeaturesPtr& cb)
	:m_fois(fois), m_task(task), m_cb(cb)
{

}

CHighValueFeaturesBuilder::~CHighValueFeaturesBuilder()
{

}

void CHighValueFeaturesBuilder::Calculate(::Ice::Long ramMb)
{
	if (m_task.FeatureIdxTo == 0)
	{
		m_task.FeatureIdxTo = m_fois[0]->DomainSize;
	}
	Ice::Long colCnt = m_task.SampleIDs.size();
	
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
		CalculateByFeatureMajor(ramMb);
	}
	else
	{
		CalculateBySampleMajor(ramMb);
	}

	Ice::Long featureCnt = m_hvFeatureIdx2SampleIDs.size();
	iBS::LongVec featureIdxs(featureCnt);
	iBS::LongVec sortedFeatureIdxs(featureCnt);
	iBS::LongVec originalIdxs(featureCnt);

	Ice::Long idx = 0;
	for (Long2IntVec_T::iterator it = m_hvFeatureIdx2SampleIDs.begin(); it != m_hvFeatureIdx2SampleIDs.end(); it++)
	{
		featureIdxs[idx++] = it->first;
	}
	CSortHelper::GetSortedValuesAndIdxs(featureIdxs, sortedFeatureIdxs, originalIdxs);

	iBS::IntVecVec featureSampleIDs(featureCnt);
	for (int i = 0; i < featureCnt; i++)
	{
		Ice::Long featureIdx = sortedFeatureIdxs[i];

		featureSampleIDs[i] = m_hvFeatureIdx2SampleIDs[featureIdx];
	}

	m_cb->ice_response(1, sortedFeatureIdxs, featureSampleIDs);
}

void CHighValueFeaturesBuilder::CalculateByFeatureMajor(::Ice::Long ramMb)
{
	if (ramMb > 250)
	{
		ramMb = 250;
	}

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;


	Ice::Double y = 0;
	Ice::Double upperLimit = 0;
	int sampleID=0;
	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " Calculate batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, IceUtil::None, Y);


		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			for (Ice::Long j = 0; j<colCnt; j++)
			{
				upperLimit = m_task.UpperLimits[j];
				sampleID = m_task.SampleIDs[j];
				y = Y[i*colCnt + j];
				if (y > upperLimit)
				{
					Ice::Long featureIdx = i + featureIdxFrom;
					Long2IntVec_T::iterator it = m_hvFeatureIdx2SampleIDs.find(featureIdx);
					if (it == m_hvFeatureIdx2SampleIDs.end())
					{
						iBS::IntVec sampleIDs(1, sampleID);
						m_hvFeatureIdx2SampleIDs.insert(Long2IntVec_T::value_type(featureIdx, sampleIDs));
					}
					else
					{
						m_hvFeatureIdx2SampleIDs[featureIdx].push_back(sampleID);
					}
				}
			}
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
	}

	return;
}

void CHighValueFeaturesBuilder::CalculateBySampleMajor(::Ice::Long ramMb)
{
	Ice::Long colCnt = m_task.SampleIDs.size();
	for (int i = 0; i<colCnt; i++)
	{
		CalculateSingleSample(i, ramMb);
	}
}
void CHighValueFeaturesBuilder::CalculateSingleSample(int sampleIdx, ::Ice::Long ramMb)
{
	if (ramMb > 250)
	{
		ramMb = 250;
	}

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = 1;

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	Ice::Double y = 0;
	Ice::Double upperLimit = m_task.UpperLimits[sampleIdx];
	int sampleID = m_task.SampleIDs[sampleIdx];
	Ice::Long sampleCnt = m_task.SampleIDs.size();
	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " Calculate sampleID=" << sampleID << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetDoublesColumnVector(sampleID, featureIdxFrom, featureIdxTo, Y);


		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			y = Y[i];
			if (y > upperLimit)
			{
				Ice::Long featureIdx = i + featureIdxFrom;
				Long2IntVec_T::iterator it = m_hvFeatureIdx2SampleIDs.find(featureIdx);
				if (it == m_hvFeatureIdx2SampleIDs.end())
				{
					iBS::IntVec sampleIDs(1, sampleID);
					m_hvFeatureIdx2SampleIDs.insert(Long2IntVec_T::value_type(featureIdx, sampleIDs));
				}
				else
				{
					m_hvFeatureIdx2SampleIDs[featureIdx].push_back(sampleID);
				}
			}
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
	}

	return;
}

//////////////////////////////////////////////////////////////////
CFeatureRowAdjustBuilder::CFeatureRowAdjustBuilder(const iBS::FeatureObserverSimpleInfoVec& fois,
	const ::iBS::FeatureRowAdjustTask& task,
	const ::iBS::AMD_ComputeService_FeatureRowAdjustPtr& cb)
	:m_fois(fois), m_task(task), m_cb(cb)
{

}

CFeatureRowAdjustBuilder::~CFeatureRowAdjustBuilder()
{

}

void CFeatureRowAdjustBuilder::Adjust(::Ice::Long ramMb)
{
	Ice::Long domainSize = m_fois[0]->DomainSize;
	
	m_featureFlags.resize(domainSize, 0);
	Ice::Long featureCnt = m_task.FeatureIdxs.size();

	m_featureIdxFrom = m_task.FeatureIdxs[0];
	m_featureIdxTo = m_task.FeatureIdxs[0];
	for (int i = 0; i < featureCnt; i++)
	{
		Ice::Long featureIdx = m_task.FeatureIdxs[i];
		m_featureFlags[featureIdx] = 1;
		if (featureIdx < m_featureIdxFrom)
		{
			m_featureIdxFrom = featureIdx;
		}
		if (featureIdx>m_featureIdxTo)
		{
			m_featureIdxTo = featureIdx;
		}
	}

	m_featureIdxTo += 1;

	Ice::Long colCnt = m_task.SampleIDs.size();

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
		AdjustByFeatureMajor(ramMb);
	}
	else
	{
		AdjustBySampleMajor(ramMb);
	}
	
	if (m_task.RecalculateStats && !bGetGroupRowMatrix)
	{
		m_task.admin->RecalculateObserverStats(m_task.SampleIDs);
	}

	m_cb->ice_response(1);
}

void CFeatureRowAdjustBuilder::AdjustByFeatureMajor(::Ice::Long ramMb)
{
	if (ramMb > 250)
	{
		ramMb = 250;
	}

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::Ice::Long TotalRowCnt = m_featureIdxTo - m_featureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_featureIdxFrom;
	::Ice::Long featureIdxTo = 0;


	::Ice::Double adjustedVal = 0; 
	::Ice::Long udpateFeatureIdxFrom = 0;
	::Ice::Long udpateFeatureIdxTo = 0;
	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " AdjustByFeatureMajor batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.writer->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, IceUtil::None, Y);

		bool needWrite = false;
		udpateFeatureIdxFrom = -1;
		udpateFeatureIdxTo = 0;
		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			Ice::Long featureIdx = i + featureIdxFrom;
			if (m_featureFlags[featureIdx] == 0)
			{
				continue;
			}
			needWrite = true;
			if (udpateFeatureIdxFrom < 0)
			{
				udpateFeatureIdxFrom = featureIdx;
			}
			udpateFeatureIdxTo = featureIdx + 1;
			for (Ice::Long j = 0; j<colCnt; j++)
			{
				adjustedVal = m_task.AdjustedValues[j];
				Y[i*colCnt + j] = adjustedVal;
			}
		}

		if (needWrite)
		{
			Ice::Double* pY_begin = reinterpret_cast<Ice::Double*>(&Y[0]) + (udpateFeatureIdxFrom - featureIdxFrom)*colCnt;
			Ice::Double* pY_end = pY_begin + (udpateFeatureIdxTo - udpateFeatureIdxFrom)*colCnt;
			std::pair<Ice::Double*, Ice::Double*> updatedY(pY_begin, pY_end);
			m_task.writer->SetDoublesRowMatrix(m_fois[0]->ObserverID, udpateFeatureIdxFrom, udpateFeatureIdxTo, updatedY);
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
	}

	return;
}

void CFeatureRowAdjustBuilder::AdjustBySampleMajor(::Ice::Long ramMb)
{
	Ice::Long colCnt = m_task.SampleIDs.size();
	for (int i = 0; i<colCnt; i++)
	{
		AdjustSingleSample(i, ramMb);
	}
}
void CFeatureRowAdjustBuilder::AdjustSingleSample(int sampleIdx, ::Ice::Long ramMb)
{
	if (ramMb > 250)
	{
		ramMb = 250;
	}

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = 1;

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::Ice::Long TotalRowCnt = m_featureIdxTo - m_featureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_featureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	Ice::Double y = 0;
	::Ice::Double adjustedVal = m_task.AdjustedValues[sampleIdx];
	::Ice::Long udpateFeatureIdxFrom = 0;
	::Ice::Long udpateFeatureIdxTo = 0;
	int sampleID = m_task.SampleIDs[sampleIdx];
	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " Adjust sampleID=" << sampleID << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.writer->GetDoublesColumnVector(sampleID, featureIdxFrom, featureIdxTo, Y);

		bool needWrite = false;
		udpateFeatureIdxFrom = -1;
		udpateFeatureIdxTo = 0;
		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			Ice::Long featureIdx = i + featureIdxFrom;
			if (m_featureFlags[featureIdx] == 0)
			{
				continue;
			}
			needWrite = true;
			if (udpateFeatureIdxFrom < 0)
			{
				udpateFeatureIdxFrom = featureIdx;
			}
			udpateFeatureIdxTo = featureIdx + 1;
			Y[i] = adjustedVal;
		}

		if (needWrite)
		{
			Ice::Double* pY_begin = reinterpret_cast<Ice::Double*>(&Y[0]) + (udpateFeatureIdxFrom - featureIdxFrom);
			Ice::Double* pY_end = pY_begin + (udpateFeatureIdxTo - udpateFeatureIdxFrom);
			std::pair<Ice::Double*, Ice::Double*> updatedY(pY_begin, pY_end);
			m_task.writer->SetDoublesColumnVector(sampleID, udpateFeatureIdxFrom, udpateFeatureIdxTo, updatedY);
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
	}

	return;
}

//////////////////////////////////////////////////////////////////
CExportRowMatrixBuilder::CExportRowMatrixBuilder(
	const ::iBS::ExportRowMatrixTask& task)
	: m_task(task)
{

}


CExportRowMatrixBuilder::~CExportRowMatrixBuilder()
{

}

iBS::FeatureObserverSimpleInfoPtr CExportRowMatrixBuilder::GetOutputFOI()
{
	::iBS::FeatureObserverSimpleInfoPtr foiPtr
		= new ::iBS::FeatureObserverSimpleInfo();
	foiPtr->ObserverID = m_task.OutID;
	foiPtr->DomainID = 0;
	foiPtr->DomainSize = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	foiPtr->ValueType = m_task.ConvertToType;
	foiPtr->StorePolicy = iBS::FeatureValueStorePolicyBinaryFilesObserverGroup;
	foiPtr->Status = ::iBS::NodeStatusIDOnly;
	foiPtr->GetPolicy = ::iBS::FeatureValueGetPolicyGetForOneTimeRead;
	foiPtr->SetPolicy = ::iBS::FeatureValueSetPolicyNoRAMImmediatelyToStore;
	foiPtr->ThreadRandomIdx = 0;
	foiPtr->ObserverGroupID = foiPtr->ObserverID;
	foiPtr->ObserverGroupSize = (int)m_task.SampleIDs.size();
	foiPtr->IdxInObserverGroup = 0;

	if (!m_task.OutFile.empty())
	{
		foiPtr->StoreLocation = iBS::FeatureValueStoreLocationSpecified;
		foiPtr->SpecifiedPathPrefix = m_task.OutFile;
	}

	return foiPtr;
}
void CExportRowMatrixBuilder::Export(::Ice::Long ramMb)
{
	Ice::Long theMaxFeatureValueFileSize = m_task.FileSizeLimitInMBytes * 1024 * 1024;

	iBS::FeatureObserverSimpleInfoPtr foi = GetOutputFOI();
	CFeatureValueStoreMgr fileStore(theMaxFeatureValueFileSize,m_task.OutPath);

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;
	if (batchRowCnt > TotalRowCnt)
	{
		batchRowCnt = TotalRowCnt + 1;
	}
	

	iBS::DoubleVec  Y(batchValueCnt);

	
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_task.TaskID, 0, batchCnt);

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	::Ice::Long exportedFeatureIdxFrom = 0;
	::Ice::Long exportedFeatureIdxTo = 0;
	::Ice::Long thisBatchExportedRowCnt = 0;

	Ice::Double validRowCnt = 0;
	
	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " Export batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, m_task.RowAdjust, Y);

		thisBatchExportedRowCnt = thisBatchRowCnt;
		exportedFeatureIdxTo = exportedFeatureIdxFrom + thisBatchExportedRowCnt;
		if (thisBatchExportedRowCnt>0)
		{
			Ice::Double *pY = reinterpret_cast<Ice::Double*>(&Y[0]);
			std::pair<const Ice::Double*, const Ice::Double*> values(
				pY, pY + thisBatchExportedRowCnt*colCnt);

			Ice::Int foiObserverID = foi->ObserverID;
			Ice::Int foiStoreObserverID = foi->ObserverID;
			Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
			Ice::Long foiDomainSize = foi->DomainSize;
			Ice::Long s_featureIdxFrom = exportedFeatureIdxFrom * foi->ObserverGroupSize; //index in store
			Ice::Long s_featureIdxTo = exportedFeatureIdxTo * foi->ObserverGroupSize;  //index in store

			foi->DomainSize = foiStoreDomainSize; //convert to store size
			foi->ObserverID = foiStoreObserverID;
			fileStore.SaveFeatureValueToStore(
				foi, s_featureIdxFrom, s_featureIdxTo, values);
			foi->ObserverID = foiObserverID;
			foi->DomainSize = foiDomainSize; //convert back
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
		exportedFeatureIdxFrom += thisBatchExportedRowCnt;

		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_task.TaskID, 1);
	}

	cout << IceUtil::Time::now().toDateTime() << " Export done. ExportedRowCnt = " << TotalRowCnt << endl;
	CGlobalVars::get()->theFeatureValueWorkerMgr->SetAMDTaskDone(m_task.TaskID);
}

void
CExportRowMatrixBuilder::ReadRowMatrix(Ice::Long featureIdxFrom, Ice::Long featureIdxTo,
std::pair<const Ice::Double*, const Ice::Double*>& ret,
::IceUtil::ScopedArray<Ice::Double>&  retValues)
{
	Ice::Long theMaxFeatureValueFileSize = m_task.FileSizeLimitInMBytes * 1024 * 1024;

	iBS::FeatureObserverSimpleInfoPtr foi = GetOutputFOI();
	CFeatureValueStoreMgr fileStore(theMaxFeatureValueFileSize, m_task.OutPath);

	Ice::Long colCnt = foi->ObserverGroupSize;
	Ice::Long rowCnt = featureIdxTo - featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	Ice::Int foiObserverID = foi->ObserverID;
	Ice::Int foiStoreObserverID = foi->ObserverGroupSize>1 ? foi->ObserverGroupID : foiObserverID;
	Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
	Ice::Long foiDomainSize = foi->DomainSize;
	Ice::Long s_featureIdxFrom = featureIdxFrom * foi->ObserverGroupSize; //index in store
	Ice::Long s_featureIdxTo = featureIdxTo * foi->ObserverGroupSize;     //index in store

	//not in RAM, load involved batches
	//currentlly the same as above
	foi->DomainSize = foiStoreDomainSize; //convert to store size
	foi->ObserverID = foiStoreObserverID;
	//retValues will hold a writable copy, ready to send to wire
	retValues.reset(
		fileStore.LoadFeatureValuesFromStore(
		foi, s_featureIdxFrom, s_featureIdxTo));
	foi->ObserverID = foiObserverID;
	foi->DomainSize = foiDomainSize; //convert back
	if (retValues.get())
	{
		ret.first = retValues.get();
		ret.second = retValues.get() + totalValueCnt;
	}
}


//////////////////////////////////////////////////////////////////
CVectors2MatrixBuilder::CVectors2MatrixBuilder(
	const ::iBS::Vectors2MatrixTask& task,
	const ::iBS::FeatureObserverSimpleInfoVec& inFOIs,
	const ::iBS::FeatureObserverSimpleInfoVec& outFOIs)
	: m_task(task), m_inFOIs(inFOIs), m_outFOIs(outFOIs)
{

}


CVectors2MatrixBuilder::~CVectors2MatrixBuilder()
{

}

void CVectors2MatrixBuilder::DoWork(::Ice::Long ramMb)
{
	Ice::Long colCnt = m_task.InOIDs.size();

	ramMb /= sizeof(Ice::Double);

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::IceUtil::ScopedArray<Ice::Double>  preFilterRawY(new ::Ice::Double[batchValueCnt]);
	if (!preFilterRawY.get()){
		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_task.TaskID, 0, iBS::AMDTaskStatusFailure);
		return;
	}

	::Ice::Long J = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;
	::Ice::Long batchCnt = J / batchRowCnt + 1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_task.TaskID, 0, batchCnt);

	int batchIdx = 0;
	::Ice::Long remainCnt = J;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	::Ice::Long keptFeatureIdxFrom = 0;
	::Ice::Long keptFeatureIdxTo = 0;
	::Ice::Long thisBatchKeptRowCnt = 0;
	iBS::FeatureObserverSimpleInfoPtr filtered_foi = m_outFOIs[0];
	while (remainCnt>0)
	{
		if (remainCnt>batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		
		std::cout << IceUtil::Time::now().toDateTime() << " Vectors2Matrix batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		GetInRawY(featureIdxFrom, featureIdxTo, preFilterRawY.get());

		thisBatchKeptRowCnt = thisBatchRowCnt;
		keptFeatureIdxTo = keptFeatureIdxFrom + thisBatchKeptRowCnt;

		//save filtered to store
		if (thisBatchKeptRowCnt>0)
		{
			std::pair<const Ice::Double*, const Ice::Double*> values(
				preFilterRawY.get(), preFilterRawY.get() + thisBatchKeptRowCnt*colCnt);

			Ice::Int foiObserverID = filtered_foi->ObserverID;
			Ice::Int foiStoreObserverID = filtered_foi->ObserverID;

			Ice::Long foiStoreDomainSize = filtered_foi->ObserverGroupSize*filtered_foi->DomainSize;
			Ice::Long foiDomainSize = filtered_foi->DomainSize;
			Ice::Long s_featureIdxFrom = keptFeatureIdxFrom * filtered_foi->ObserverGroupSize; //index in store
			Ice::Long s_featureIdxTo = keptFeatureIdxTo * filtered_foi->ObserverGroupSize;  //index in store

			filtered_foi->DomainSize = foiStoreDomainSize; //convert to store size
			filtered_foi->ObserverID = foiStoreObserverID;
			CGlobalVars::get()->theFeatureValueStoreMgr->SaveFeatureValueToStore(
				filtered_foi, s_featureIdxFrom, s_featureIdxTo, values);
			filtered_foi->ObserverID = foiObserverID;
			filtered_foi->DomainSize = foiDomainSize; //convert back
		}

		keptFeatureIdxFrom += thisBatchKeptRowCnt;

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;

		std::cout << IceUtil::Time::now().toDateTime() << " Vectors2Matrix batch " << batchIdx << "/" << batchCnt << " end" << endl;
		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_task.TaskID, 1);
	}

	std::cout << IceUtil::Time::now().toDateTime() << " Vectors2Matrix totoal kept rows " << J << endl;
	CGlobalVars::get()->theFeatureValueWorkerMgr->SetAMDTaskDone(m_task.TaskID);

}

bool CVectors2MatrixBuilder::GetInRawY(
	::Ice::Long featureIdxFrom, ::Ice::Long featureIdxTo, ::Ice::Double*  RawY)
{
	if (!RawY)
	{
		//should already allocated 
		return false;
	}

	Ice::Long colCnt = m_task.InOIDs.size();
	Ice::Long rowCnt = featureIdxTo - featureIdxFrom;
	Ice::Long totalValueCnt = rowCnt*colCnt;

	Ice::Double* rawCnts = RawY;

	::iBS::AMD_FcdcReadService_GetRowMatrixPtr nullcb;
	::Original::CGetRowMatrix wi(
		m_inFOIs, nullcb, m_task.InOIDs, featureIdxFrom, featureIdxTo);
	wi.getRetValues(rawCnts);

	return true;
}

//////////////////////////////////////////////////////////////////
CExportZeroOutBgRowMatrixBuilder::CExportZeroOutBgRowMatrixBuilder(
	const ::iBS::ExportZeroOutBgRowMatrixTask& task)
	: m_task(task)
{
	m_backYFromIdx = -1;
	m_backYToIdx = -1;
	m_frontYFromIdx = -1;
	m_frontYToIdx = -1;
}


CExportZeroOutBgRowMatrixBuilder::~CExportZeroOutBgRowMatrixBuilder()
{

}

iBS::FeatureObserverSimpleInfoPtr CExportZeroOutBgRowMatrixBuilder::GetOutputFOI()
{
	::iBS::FeatureObserverSimpleInfoPtr foiPtr
		= new ::iBS::FeatureObserverSimpleInfo();
	foiPtr->ObserverID = m_task.OutID;
	foiPtr->DomainID = 0;
	foiPtr->DomainSize = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	foiPtr->ValueType = m_task.ConvertToType;
	foiPtr->StorePolicy = iBS::FeatureValueStorePolicyBinaryFilesObserverGroup;
	foiPtr->Status = ::iBS::NodeStatusIDOnly;
	foiPtr->GetPolicy = ::iBS::FeatureValueGetPolicyGetForOneTimeRead;
	foiPtr->SetPolicy = ::iBS::FeatureValueSetPolicyNoRAMImmediatelyToStore;
	foiPtr->ThreadRandomIdx = 0;
	foiPtr->ObserverGroupID = foiPtr->ObserverID;
	foiPtr->ObserverGroupSize = (int)m_task.SampleIDs.size();
	foiPtr->IdxInObserverGroup = 0;

	return foiPtr;
}

void CExportZeroOutBgRowMatrixBuilder::GetFrontRowValue(
	Ice::Long featureIdx, iBS::DoubleVec& data)
{
	::Ice::Long ramMb = 250;
	::Ice::Long colCnt = m_task.SampleIDs.size();
	if (m_frontYFromIdx <0 || featureIdx >= m_frontYToIdx)
	{
		ramMb /= sizeof(Ice::Double);
		Ice::Long colCnt = m_task.SampleIDs.size();

		::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
		if (batchValueCnt%colCnt != 0){
			batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
		}

		::Ice::Long batchRowCnt = batchValueCnt / colCnt;

		if ((Ice::Long)m_frontY.size() < batchValueCnt)
		{
			m_frontY.resize(batchValueCnt);
		}
		

		::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - featureIdx;
		::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

		int batchIdx = 0;
		::Ice::Long remainCnt = TotalRowCnt;
		::Ice::Long thisBatchRowCnt = 0;
		::Ice::Long featureIdxFrom = featureIdx;
		::Ice::Long featureIdxTo = 0;

		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		//cout << IceUtil::Time::now().toDateTime() << " Export batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;
		if (thisBatchRowCnt > 0)
		{
			m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, IceUtil::None, m_frontY);
		}

		m_frontYFromIdx = featureIdxFrom;
		m_frontYToIdx = featureIdxTo;
	}

	for (Ice::Long j = 0; j < colCnt; j++)
	{
		Ice::Long idx = (featureIdx - m_frontYFromIdx)*colCnt + j;

		if (m_task.NeedLogFirst)
		{
			data[j] = log(m_frontY[idx]*m_task.LibrarySizeFactors[j] + 1);
		}
		else
		{
			data[j] = m_frontY[idx];
		}
	}

}

void CExportZeroOutBgRowMatrixBuilder::GetBackRowValue(
	Ice::Long featureIdx, iBS::DoubleVec& data)
{
	::Ice::Long ramMb = 250;
	::Ice::Long colCnt = m_task.SampleIDs.size();
	if (m_backYFromIdx <0 || featureIdx >= m_backYToIdx)
	{
		ramMb /= sizeof(Ice::Double);
		Ice::Long colCnt = m_task.SampleIDs.size();

		::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
		if (batchValueCnt%colCnt != 0){
			batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
		}

		::Ice::Long batchRowCnt = batchValueCnt / colCnt;

		if ((Ice::Long)m_backY.size() < batchValueCnt)
		{
			m_backY.resize(batchValueCnt);
		}


		::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - featureIdx;
		::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

		int batchIdx = 0;
		::Ice::Long remainCnt = TotalRowCnt;
		::Ice::Long thisBatchRowCnt = 0;
		::Ice::Long featureIdxFrom = featureIdx;
		::Ice::Long featureIdxTo = 0;

		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		//cout << IceUtil::Time::now().toDateTime() << " Export batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;
		if (thisBatchRowCnt > 0)
		{
			m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, IceUtil::None, m_backY);
		}

		m_backYFromIdx = featureIdxFrom;
		m_backYToIdx = featureIdxTo;
	}

	for (Ice::Long j = 0; j < colCnt; j++)
	{
		Ice::Long idx = (featureIdx - m_backYFromIdx)*colCnt + j;
		if (m_task.NeedLogFirst)
		{
			data[j] = log(m_backY[idx] * m_task.LibrarySizeFactors[j] + 1);
		}
		else
		{
			data[j] = m_backY[idx];
		}
		
	}
}

void CExportZeroOutBgRowMatrixBuilder::GetBgValue(
	Ice::Long featureIdx, Ice::Long windowHalfSize, iBS::DoubleVec& data)
{
	Ice::Long winFromIdx = featureIdx - windowHalfSize;
	Ice::Long winToIdx = featureIdx + windowHalfSize+1;
	if (winFromIdx < m_task.FeatureIdxFrom)
	{
		winFromIdx = m_task.FeatureIdxFrom;
	}
	if (winToIdx>m_task.FeatureIdxTo)
	{
		winToIdx = m_task.FeatureIdxTo;
	}
	Ice::Long colCnt = m_task.SampleIDs.size();

	//new font area
	for (Ice::Long i = m_bgWinToIdx; i < winToIdx; i++)
	{
		GetFrontRowValue(i, data);
		for (Ice::Long j = 0; j < colCnt; j++)
		{
			if (m_task.NeedLogFirst)
			{
				m_bgWindowColSums[j] += log(data[j] * m_task.LibrarySizeFactors[j] + 1);
			}
			else
			{
				m_bgWindowColSums[j] += data[j];
			}
		}

		m_bgWinToIdx = i + 1;
	}

	for (Ice::Long i = m_bgWinFromIdx; i < winFromIdx; i++)
	{
		GetBackRowValue(i, data);
		for (Ice::Long j = 0; j < colCnt; j++)
		{
			if (m_task.NeedLogFirst)
			{
				m_bgWindowColSums[j] -= log(data[j] * m_task.LibrarySizeFactors[j] + 1);
			}
			else
			{
				m_bgWindowColSums[j] -= data[j];
			}
		}
		m_bgWinFromIdx = i + 1;
	}

	Ice::Double winSize = (Ice::Double) (m_bgWinToIdx - m_bgWinFromIdx);

	for (Ice::Long j = 0; j < colCnt; j++)
	{
		data[j] = m_bgWindowColSums[j] / winSize;
	}

}

void CExportZeroOutBgRowMatrixBuilder::Export(::Ice::Long ramMb)
{
	Ice::Long theMaxFeatureValueFileSize = m_task.FileSizeLimitInMBytes * 1024 * 1024;

	iBS::FeatureObserverSimpleInfoPtr foi = GetOutputFOI();
	CFeatureValueStoreMgr fileStore(theMaxFeatureValueFileSize, m_task.OutPath);

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::iBS::DoubleVec bgValues(colCnt, 0);
	::iBS::LongVec  signalFeatureCnts(colCnt, 0);
	::iBS::LongVec  zeroOutFeatureCnts(colCnt, 0);
	m_bgWindowColSums.resize(colCnt, 0);
	m_bgWinFromIdx = m_task.FeatureIdxFrom;
	m_bgWinToIdx = m_task.FeatureIdxFrom;

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	::iBS::DoubleVec  Y(batchValueCnt);
	

	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	::Ice::Long exportedFeatureIdxFrom = 0;
	::Ice::Long exportedFeatureIdxTo = 0;
	::Ice::Long thisBatchExportedRowCnt = 0;

	Ice::Double validRowCnt = 0;

	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " Export batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, IceUtil::None, Y);

		Ice::Long idx = 0;
		Ice::Double diff = 0;
		for (Ice::Long i = 0; i < thisBatchRowCnt; i++)
		{
			Ice::Long featureIdx = i + featureIdxFrom;
			GetBgValue(featureIdx, m_task.BgWindowRadius, bgValues);
			for (Ice::Long j = 0; j < colCnt; j++)
			{
				idx = i*colCnt + j;
				if (m_task.NeedLogFirst)
				{
					diff = log(Y[idx] * m_task.LibrarySizeFactors[j] + 1) - bgValues[j];
				}
				else
				{
					diff = Y[idx] - bgValues[j];
				}

				if (diff < m_task.BgSignalDifference)
				{
					if (Y[idx]>0)
					{
						zeroOutFeatureCnts[j]++;
					}

					Y[idx] = 0;
				}
				else if (m_task.NeedLogFirst)
				{
					Y[idx] -= (exp(bgValues[j]) - 1) / m_task.LibrarySizeFactors[j];
					signalFeatureCnts[j]++;
				}
				else
				{
					Y[idx] -= bgValues[j];
					signalFeatureCnts[j]++;
				}
			}
		}

		thisBatchExportedRowCnt = thisBatchRowCnt;
		exportedFeatureIdxTo = exportedFeatureIdxFrom + thisBatchExportedRowCnt;
		if (thisBatchExportedRowCnt>0)
		{
			Ice::Double *pY = reinterpret_cast<Ice::Double*>(&Y[0]);
			std::pair<const Ice::Double*, const Ice::Double*> values(
				pY, pY + thisBatchExportedRowCnt*colCnt);

			Ice::Int foiObserverID = foi->ObserverID;
			Ice::Int foiStoreObserverID = foi->ObserverID;
			Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
			Ice::Long foiDomainSize = foi->DomainSize;
			Ice::Long s_featureIdxFrom = exportedFeatureIdxFrom * foi->ObserverGroupSize; //index in store
			Ice::Long s_featureIdxTo = exportedFeatureIdxTo * foi->ObserverGroupSize;  //index in store

			foi->DomainSize = foiStoreDomainSize; //convert to store size
			foi->ObserverID = foiStoreObserverID;
			fileStore.SaveFeatureValueToStore(
				foi, s_featureIdxFrom, s_featureIdxTo, values);
			foi->ObserverID = foiObserverID;
			foi->DomainSize = foiDomainSize; //convert back
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
		exportedFeatureIdxFrom += thisBatchExportedRowCnt;
	}

	cout << IceUtil::Time::now().toDateTime() << " Export done. ExportedRowCnt = " << TotalRowCnt << endl;

	for (Ice::Long j = 0; j < colCnt; j++)
	{
		cout << " sample " << m_task.SampleIDs[j] << ": removed " << zeroOutFeatureCnts[j]
			 << ", kept "<<signalFeatureCnts[j]<< endl;
	}

}


//////////////////////////////////////////////////////////////////
CExportByRowIdxsBuilder::CExportByRowIdxsBuilder(
	const ::iBS::ExportByRowIdxsTask& task, Ice::Long taskID)
	: m_task(task), m_taskID(taskID)
{

}


CExportByRowIdxsBuilder::~CExportByRowIdxsBuilder()
{

}

iBS::FeatureObserverSimpleInfoPtr CExportByRowIdxsBuilder::GetOutputFOI()
{
	::iBS::FeatureObserverSimpleInfoPtr foiPtr
		= new ::iBS::FeatureObserverSimpleInfo();
	foiPtr->ObserverID = m_task.OutID;
	foiPtr->DomainID = 0;
	foiPtr->DomainSize = m_task.FeatureIdxs.size();
	foiPtr->ValueType = iBS::FeatureValueDouble;
	foiPtr->StorePolicy = iBS::FeatureValueStorePolicyBinaryFilesObserverGroup;
	foiPtr->Status = ::iBS::NodeStatusIDOnly;
	foiPtr->GetPolicy = ::iBS::FeatureValueGetPolicyGetForOneTimeRead;
	foiPtr->SetPolicy = ::iBS::FeatureValueSetPolicyNoRAMImmediatelyToStore;
	foiPtr->ThreadRandomIdx = 0;
	foiPtr->ObserverGroupID = foiPtr->ObserverID;
	foiPtr->ObserverGroupSize = (int)m_task.SampleIDs.size();
	foiPtr->IdxInObserverGroup = 0;

	return foiPtr;
}

void CExportByRowIdxsBuilder::Export(::Ice::Long ramMb)
{
	::Ice::Long TotalRowCnt = m_task.FeatureIdxs.size();
	if (TotalRowCnt < 2000000)
	{
		ExportBySampleRowMatrix(ramMb);
	}
	else
	{
		ExportByGetRowMatrix(ramMb);
	}

}

void CExportByRowIdxsBuilder::ExportByGetRowMatrix(::Ice::Long ramMb)
{
	Ice::Long theMaxFeatureValueFileSize = m_task.FileSizeLimitInMBytes * 1024 * 1024;

	iBS::FeatureObserverSimpleInfoPtr foi = GetOutputFOI();
	CFeatureValueStoreMgr fileStore(theMaxFeatureValueFileSize, m_task.OutPath);

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::IceUtil::ScopedArray<Ice::Double>  filteredY(new ::Ice::Double[batchValueCnt]);
	if (!filteredY.get()){
	}

	::Ice::Long TotalRowCnt = m_task.FeatureIdxs.size();
	

	//set up row selection flags, input featureIdx should alreay be in ascending order
	Ice::Long maxFeatureIdx = m_task.FeatureIdxs[TotalRowCnt-1];
	Ice::Long minFeatureIdx = m_task.FeatureIdxs[0];
	::Ice::Long batchCnt = (maxFeatureIdx - minFeatureIdx + 1) / batchRowCnt + 1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(m_taskID, m_task.TaskName, batchCnt);

	iBS::ByteVec featureIdxFlags(maxFeatureIdx + 1,0);

	Ice::Long needExportRowCnt = 0;
	for (Ice::Long i = 0; i < TotalRowCnt; i++)
	{
		featureIdxFlags[m_task.FeatureIdxs[i]] = 1;
	}

	int batchIdx = 0;
	::Ice::Long remainCnt = maxFeatureIdx - minFeatureIdx+1;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = minFeatureIdx;
	::Ice::Long featureIdxTo = 0;

	::Ice::Long exportedFeatureIdxFrom = 0;
	::Ice::Long exportedFeatureIdxTo = 0;
	::Ice::Long thisBatchExportedRowCnt = 0;


	Ice::Double validRowCnt = 0;

	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;
		thisBatchExportedRowCnt = 0;

		cout << IceUtil::Time::now().toDateTime() << " Export batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, m_task.RowAdjust, Y);

		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			::Ice::Long fidx = i + featureIdxFrom;
			if (featureIdxFlags[fidx]>0)
			{
				Ice::Double *pSource = reinterpret_cast<Ice::Double*>(&Y[0]) + i*colCnt;;
				Ice::Double *pDest = filteredY.get() + thisBatchExportedRowCnt*colCnt;
				std::copy(pSource, pSource + colCnt, pDest);
				thisBatchExportedRowCnt++;
			}
		}

		exportedFeatureIdxTo = exportedFeatureIdxFrom + thisBatchExportedRowCnt;
		if (thisBatchExportedRowCnt>0)
		{
			validRowCnt += thisBatchExportedRowCnt;

			Ice::Double *pY = reinterpret_cast<Ice::Double*>(&filteredY[0]);
			std::pair<const Ice::Double*, const Ice::Double*> values(
				pY, pY + thisBatchExportedRowCnt*colCnt);

			Ice::Int  foiObserverID = foi->ObserverID;
			Ice::Int  foiStoreObserverID = foi->ObserverID;
			Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
			Ice::Long foiDomainSize = foi->DomainSize;
			Ice::Long s_featureIdxFrom = exportedFeatureIdxFrom * foi->ObserverGroupSize; //index in store
			Ice::Long s_featureIdxTo = exportedFeatureIdxTo * foi->ObserverGroupSize;  //index in store

			foi->DomainSize = foiStoreDomainSize; //convert to store size
			foi->ObserverID = foiStoreObserverID;
			fileStore.SaveFeatureValueToStore(
				foi, s_featureIdxFrom, s_featureIdxTo, values);
			foi->ObserverID = foiObserverID;
			foi->DomainSize = foiDomainSize; //convert back
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
		exportedFeatureIdxFrom += thisBatchExportedRowCnt;
		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_taskID, 1);
	}

	cout << IceUtil::Time::now().toDateTime() << " Export done. ExportedRowCnt = " << validRowCnt << endl;

}

void CExportByRowIdxsBuilder::ExportBySampleRowMatrix(::Ice::Long ramMb)
{
	Ice::Long theMaxFeatureValueFileSize = m_task.FileSizeLimitInMBytes * 1024 * 1024;

	iBS::FeatureObserverSimpleInfoPtr foi = GetOutputFOI();
	CFeatureValueStoreMgr fileStore(theMaxFeatureValueFileSize, m_task.OutPath);

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::Ice::Long TotalRowCnt = m_task.FeatureIdxs.size();

	::Ice::Long batchCnt = (TotalRowCnt) / batchRowCnt + 1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(m_taskID, m_task.TaskName, batchCnt);

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = 0;
	::Ice::Long featureIdxTo = 0;

	Ice::Double validRowCnt = 0;

	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " Export batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;
		iBS::LongVec batchFeatureIdxs(thisBatchRowCnt, 0);
		
		Ice::Long *pSource = reinterpret_cast<Ice::Long*>(&m_task.FeatureIdxs[featureIdxFrom]);
		Ice::Long *pDest = reinterpret_cast<Ice::Long*>(&batchFeatureIdxs[0]);
		std::copy(pSource, pSource + thisBatchRowCnt, pDest);

		m_task.reader->SampleRowMatrix(m_task.SampleIDs, batchFeatureIdxs, m_task.RowAdjust, Y);

		if (thisBatchRowCnt>0)
		{
			Ice::Double *pY = reinterpret_cast<Ice::Double*>(&Y[0]);
			std::pair<const Ice::Double*, const Ice::Double*> values(
				pY, pY + thisBatchRowCnt*colCnt);

			Ice::Int  foiObserverID = foi->ObserverID;
			Ice::Int  foiStoreObserverID = foi->ObserverID;
			Ice::Long foiStoreDomainSize = foi->ObserverGroupSize*foi->DomainSize;
			Ice::Long foiDomainSize = foi->DomainSize;
			Ice::Long s_featureIdxFrom = featureIdxFrom * foi->ObserverGroupSize; //index in store
			Ice::Long s_featureIdxTo = featureIdxTo * foi->ObserverGroupSize;  //index in store

			foi->DomainSize = foiStoreDomainSize; //convert to store size
			foi->ObserverID = foiStoreObserverID;
			fileStore.SaveFeatureValueToStore(
				foi, s_featureIdxFrom, s_featureIdxTo, values);
			foi->ObserverID = foiObserverID;
			foi->DomainSize = foiDomainSize; //convert back
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_taskID, 1);
	}

	cout << IceUtil::Time::now().toDateTime() << " Export done. ExportedRowCnt = " << validRowCnt << endl;

}


//////////////////////////////////////////////////////////////////
CFeatureVariabilityBuilder::CFeatureVariabilityBuilder(
	const ::iBS::HighVariabilityFeaturesTask& task,
	const ::iBS::AMD_ComputeService_HighVariabilityFeaturesPtr& cb)
	:m_task(task), m_cb(cb)
{

}

CFeatureVariabilityBuilder::~CFeatureVariabilityBuilder()
{

}

void CFeatureVariabilityBuilder::Calculate(::Ice::Long ramMb)
{
	TestByFeatureMajor(ramMb);
}

void CFeatureVariabilityBuilder::TestByFeatureMajor(::Ice::Long ramMb)
{
	if (ramMb > 250)
	{
		ramMb = 250;
	}

	ramMb /= sizeof(Ice::Double);
	if (ramMb == 0)
	{
		ramMb = 1;
	}
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	
	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	Ice::Long resultFeatureCnt = TotalRowCnt;
	bool needSampling = false;
	int fraction = 0;
	if (m_task.SamplingFeatureCnt > 0)
	{
		needSampling = true;
		fraction = (int)(TotalRowCnt / m_task.SamplingFeatureCnt);
		resultFeatureCnt = m_task.SamplingFeatureCnt;
	}

	iBS::LongVec featureIdxs;
	featureIdxs.reserve(resultFeatureCnt);

	iBS::DoubleVec variabilities;
	variabilities.reserve(resultFeatureCnt);

	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " TestByFeatureMajor batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, m_task.RowAdjust, Y);

		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			Ice::Long featureIdx = i + featureIdxFrom;
			if (needSampling && CGlobalVars::get()->GetUniformInt(1, fraction)>1)
			{
				continue;
			}
			
			//row value adjust
			Ice::Double maxVal = Y[i*colCnt];
			Ice::Double meanVal = 0;
			Ice::Double sd = 0;
			for (Ice::Long j = 0; j<colCnt; j++)
			{
				meanVal += Y[i*colCnt + j];
				if (Y[i*colCnt + j]>maxVal)
				{
					maxVal = Y[i*colCnt + j];
				}
			}

			if (maxVal<m_task.FeatureFilterMaxCntLowThreshold)
			{
				continue;
			}

			meanVal = meanVal / colCnt;

			for (Ice::Long j = 0; j<colCnt; j++)
			{
				sd += (Y[i*colCnt + j] - meanVal)*(Y[i*colCnt + j] - meanVal);
			}
			sd = sqrt(sd / colCnt);

			Ice::Double variability = 0;
			

			if (m_task.VariabilityTest == iBS::VariabilityTestGCV)
			{
				//assuming the data is already natural log transformed
				variability = sqrt(exp(sd*sd) - 1.0);
			}
			else if (m_task.VariabilityTest == iBS::VariabilityTestCV)
			{
				variability = sd / meanVal;
			}
			if (variability > m_task.VariabilityCutoff)
			{
				featureIdxs.push_back(featureIdx);
				variabilities.push_back(variability);
			}
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
	}
	cout << IceUtil::Time::now().toDateTime() << " Export done. ResultRowCnt = " << featureIdxs.size() << endl;
	m_cb->ice_response(1, featureIdxs, variabilities);

	return;
}


//////////////////////////////////////////////////////////////////
CRowANOVABuilder::CRowANOVABuilder(
	const ::iBS::RowANOVATask& task,
	const ::iBS::AMD_ComputeService_RowANOVAPtr& cb)
	:m_task(task), m_cb(cb)
{

}

CRowANOVABuilder::~CRowANOVABuilder()
{

}

void CRowANOVABuilder::Calculate(::Ice::Long ramMb)
{
	int sampleCnt = (int)m_task.SampleIDs.size();
	for (int i = 0; i<sampleCnt; i++)
	{
		int sampleID = m_task.SampleIDs[i];
		m_sampleID2sampleIdx.insert(std::pair<int, int>(sampleID, i));
	}
	int groupCnt =(int) m_task.GroupSampleIDs.size();
	m_group2SampleIdxs.resize(groupCnt);
	for (int i = 0; i < groupCnt; i++)
	{
		int groupIdx = i;
		const iBS::IntVec& sampleIDs = m_task.GroupSampleIDs[groupIdx];
		m_group2SampleIdxs[groupIdx].resize(sampleIDs.size());

		for (int j = 0; j<sampleIDs.size(); j++)
		{

			int sampleIdx = m_sampleID2sampleIdx[sampleIDs[j]];
			m_group2SampleIdxs[groupIdx][j] = sampleIdx;
		}
	}

	ANOVAByFeatureMajor(ramMb);
}

void CRowANOVABuilder::ANOVAByFeatureMajor(::Ice::Long ramMb)
{
	if (ramMb > 250)
	{
		ramMb = 250;
	}

	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);


	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	Ice::Long resultFeatureCnt = TotalRowCnt;
	bool needSampling = false;
	int fraction = 0;
	if (m_task.SamplingFeatureCnt > 0)
	{
		needSampling = true;
		fraction = (int)(TotalRowCnt / m_task.SamplingFeatureCnt);
		resultFeatureCnt = m_task.SamplingFeatureCnt;
	}

	iBS::LongVec featureIdxs;
	featureIdxs.reserve(resultFeatureCnt);

	iBS::DoubleVec Fs;
	Fs.reserve(resultFeatureCnt);

	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " ANOVAByFeatureMajor batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, m_task.RowAdjust, Y);

		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			Ice::Long featureIdx = i + featureIdxFrom;
			if (needSampling && CGlobalVars::get()->GetUniformInt(1, fraction)>1)
			{
				continue;
			}

			//row value adjust
			if (m_task.FeatureFilterMaxCntLowThreshold > 0)
			{
				Ice::Double maxVal = Y[i*colCnt];
				for (Ice::Long j = 0; j<colCnt; j++)
				{
					if (Y[i*colCnt + j]>maxVal)
					{
						maxVal = Y[i*colCnt + j];
					}
				}

				if (maxVal < m_task.FeatureFilterMaxCntLowThreshold)
				{
					continue;
				}
			}

			
			Ice::Double *y = reinterpret_cast<Ice::Double*>(&Y[i*colCnt]);

			Ice::Double FStatistic = CStatisticsHelper::GetOneWayANOVA(y, m_group2SampleIdxs);

			if (FStatistic>0)
			{
				featureIdxs.push_back(featureIdx);
				Fs.push_back(FStatistic);
			}
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
	}
	cout << IceUtil::Time::now().toDateTime() << " RowANOVA done. ResultRowCnt = " << featureIdxs.size() << endl;
	m_cb->ice_response(1, featureIdxs, Fs);

	return;
}

//////////////////////////////////////////////////////////////////
CWithSignalFeaturesBuilder::CWithSignalFeaturesBuilder(
	const ::iBS::WithSignalFeaturesTask& task,
	const ::iBS::AMD_ComputeService_WithSignalFeaturesPtr& cb)
	:m_task(task), m_cb(cb)
{

}

CWithSignalFeaturesBuilder::~CWithSignalFeaturesBuilder()
{

}

void CWithSignalFeaturesBuilder::Calculate(::Ice::Long ramMb)
{
	TestByFeatureMajor(ramMb);
}

void CWithSignalFeaturesBuilder::TestByFeatureMajor(::Ice::Long ramMb)
{
	if (ramMb > 250)
	{
		ramMb = 250;
	}

	ramMb /= sizeof(Ice::Double);
	if (ramMb == 0)
	{
		ramMb = 1;
	}
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);


	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	Ice::Long resultFeatureCnt = TotalRowCnt;
	bool needSampling = false;
	int fraction = 0;
	if (m_task.SamplingFeatureCnt > 0)
	{
		needSampling = true;
		fraction = (int)(TotalRowCnt / m_task.SamplingFeatureCnt);
		resultFeatureCnt = m_task.SamplingFeatureCnt;
	}

	iBS::LongVec featureIdxs;
	featureIdxs.reserve(resultFeatureCnt);

	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " WithSignalFeatures batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, m_task.RowAdjust, Y);

		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			Ice::Long featureIdx = i + featureIdxFrom;
			if (needSampling && CGlobalVars::get()->GetUniformInt(1, fraction)>1)
			{
				continue;
			}

			int aboveThresholdCnt=0;
			for (Ice::Long j = 0; j<colCnt; j++)
			{
				if (Y[i*colCnt + j]>m_task.SampleCntAboveThreshold)
				{
					aboveThresholdCnt++;
					if (aboveThresholdCnt >= m_task.SampleCntAboveThreshold)
					{
						break;
					}
				}
			}

			if (aboveThresholdCnt<m_task.SampleCntAboveThreshold)
			{
				continue;
			}

			featureIdxs.push_back(featureIdx);
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;
	}
	cout << IceUtil::Time::now().toDateTime() << " WithSignalFeatures done. ResultRowCnt = " << featureIdxs.size() << endl;
	m_cb->ice_response(1, featureIdxs);

	return;
}


//////////////////////////////////////////////////////////////////
CRuvVdAnovaBuilder::CRuvVdAnovaBuilder(
	const ::iBS::VdAnovaTask& task, Ice::Long amdTaskID)
	: m_task(task), m_amdTaskID(amdTaskID)
{
}

CRuvVdAnovaBuilder::~CRuvVdAnovaBuilder()
{
}

void CRuvVdAnovaBuilder::DoWork(iBS::VdAnovaResult& ret, ::Ice::Long ramMb)
{
	Ice::Double grandMean = GetGrandMean(ramMb);
	DoVD(ramMb, grandMean, ret);
}

void CRuvVdAnovaBuilder::DoVD(::Ice::Long ramMb, Ice::Double grandMean, iBS::VdAnovaResult& ret)
{
	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(m_amdTaskID, m_task.TaskName, batchCnt);

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	Ice::Double totalVar = 0;

	//"explained variance" by locus, or "between-group variability
	Ice::Double bgVar = 0;
	//"unexplained variance", or "within-group variability"
	Ice::Double wgVar = 0;

	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " Export batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, IceUtil::None, Y);

		Ice::Double rowMean = 0;
		Ice::Double colNum = (Ice::Double)colCnt;
		Ice::Double y = 0;
		for (Ice::Long i = 0; i<thisBatchRowCnt; i++)
		{
			rowMean = 0;
			for (Ice::Long j = 0; j < colCnt; j++){
				rowMean += Y[i*colCnt + j];
			}
			rowMean /= colNum;

			bgVar += (colNum*(rowMean - grandMean)*(rowMean - grandMean));

			for (Ice::Long j = 0; j<colCnt; j++){
				y = Y[i*colCnt + j];
				wgVar += (rowMean - y)*(rowMean - y);
				totalVar += (grandMean - y)*(grandMean - y);
			}
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;

		cout << "totalVar: " << totalVar << ", between locus " << bgVar << "(" << (bgVar * 100 / totalVar)
			<< "%), within locus " << wgVar << "(" << (wgVar * 100 / totalVar)
			<< "%)" << endl;

		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_amdTaskID, 1);
	}

	ret.totalVar = totalVar;
	ret.bgVar = bgVar;
	ret.wgVar = wgVar;

	cout << IceUtil::Time::now().toDateTime() << " DoVD done, RowCnt = " << TotalRowCnt << endl;
}

Ice::Double CRuvVdAnovaBuilder::GetGrandMean(::Ice::Long ramMb)
{
	ramMb /= sizeof(Ice::Double);
	Ice::Long colCnt = m_task.SampleIDs.size();

	::Ice::Long batchValueCnt = 1024 * 1024 * ramMb;
	if (batchValueCnt%colCnt != 0){
		batchValueCnt -= (batchValueCnt%colCnt); //data row not spread in two batch files
	}

	::Ice::Long batchRowCnt = batchValueCnt / colCnt;

	iBS::DoubleVec  Y(batchValueCnt);

	::Ice::Long TotalRowCnt = m_task.FeatureIdxTo - m_task.FeatureIdxFrom;
	::Ice::Long batchCnt = TotalRowCnt / batchRowCnt + 1;

	std::string taskName = m_task.TaskName + " GrandMean";
	CGlobalVars::get()->theFeatureValueWorkerMgr->InitAMDSubTask(m_amdTaskID, taskName, batchCnt);

	int batchIdx = 0;
	::Ice::Long remainCnt = TotalRowCnt;
	::Ice::Long thisBatchRowCnt = 0;
	::Ice::Long featureIdxFrom = m_task.FeatureIdxFrom;
	::Ice::Long featureIdxTo = 0;

	Ice::Double sum = 0;

	while (remainCnt > 0)
	{
		if (remainCnt > batchRowCnt){
			thisBatchRowCnt = batchRowCnt;
		}
		else{
			thisBatchRowCnt = remainCnt;
		}
		batchIdx++;

		cout << IceUtil::Time::now().toDateTime() << " Export batch " << batchIdx << "/" << batchCnt << " begin" << endl;

		featureIdxTo = featureIdxFrom + thisBatchRowCnt;

		m_task.reader->GetRowMatrix(m_task.SampleIDs, featureIdxFrom, featureIdxTo, IceUtil::None, Y);

		Ice::Long thisBatchTotalValueCnt = thisBatchRowCnt*colCnt;

		for (Ice::Long i = 0; i<thisBatchTotalValueCnt; i++)
		{
			sum += Y[i];
		}

		featureIdxFrom += thisBatchRowCnt;
		remainCnt -= thisBatchRowCnt;

		CGlobalVars::get()->theFeatureValueWorkerMgr->UpdateAMDTaskProgress(m_amdTaskID, 1);
	}

	Ice::Double grandMean = sum / ((Ice::Double) TotalRowCnt*colCnt);

	cout << IceUtil::Time::now().toDateTime() << " Export done. ExportedRowCnt = " << TotalRowCnt << endl;

	return grandMean;
}

