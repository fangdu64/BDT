#include <bdt/bdtBase.h>
#include <IceUtil/Config.h>
#include <IceUtil/Time.h>
#include <GlobalVars.h>
#include <algorithm>    // std::copy
#include <KMeanWorkItem.h>
#include <KMeanContract.h>
#include <math.h>

void
CEuclideanUpdateKCntsAndKSums::DoWork()
{
	const KMeanContractInfoPtr& contractPtr=m_kmeanL2Ptr->m_contractPtr;
	Ice::Long colCnt=contractPtr->ObserverIDs.size();
	Ice::Long rowCnt=contractPtr->K;

	Ice::Double *pKCnts=m_kmeanL2Ptr->m_workerKCnts[m_workerIdx];
	Ice::Double	*pKSums=m_kmeanL2Ptr->m_workerKSums[m_workerIdx];
	m_kmeanL2Ptr->m_workerClusterChangeCnts[m_workerIdx] = 0;

	if(m_resetKCntsKSumsFirst)
	{
		std::fill(pKCnts, pKCnts+rowCnt, 0.0);
		std::fill(pKSums, pKSums+(rowCnt*colCnt), 0.0);
	}

	Ice::Double distorsionSum = 0;
	//for each data row assign it to the nearest center kMin
	Ice::Long dataRowCnt=m_workerFeatureIdxTo - m_workerFeatureIdxFrom;
	Ice::Long K=contractPtr->K;

	for(Ice::Long i=0;i<dataRowCnt;i++)
	{
		Ice::Long rowIdxInContractor = m_workerFeatureIdxFrom - contractPtr->FeatureIdxFrom +i;

		//dataRow
		Ice::Double *dr = m_kmeanL2Ptr->m_data.get()+(rowIdxInContractor*colCnt);

		Ice::Double distMin=0;
		Ice::Long kMin=0;

		//clusterRow
		Ice::Double *cr=0;
		for(Ice::Long k=0;k<K;k++)
		{
			cr = &m_kmeanL2Ptr->m_KClusters[k*colCnt];
			
			//compute distance
			Ice::Double dist=0;
			for(int j=0;j<colCnt;j++)
			{
				dist+=(cr[j]-dr[j])*(cr[j]-dr[j]);
			}

			if(k==0||dist<distMin)
			{
				distMin=dist;
				kMin=k;
			}
		}

		//now kMin is the cluster closest to data row
		//update KCnts
		pKCnts[kMin]++;

		cr=&m_kmeanL2Ptr->m_KClusters[kMin*colCnt];
		//update KSums
		Ice::Double	*theKSums=pKSums+(kMin*colCnt);

		for(int j=0;j<colCnt;j++)
		{
			//theKSums[j]+=(dr[j]-cr[j]);
			theKSums[j]+=dr[j]; //just the sum of new data points
		}

		//update datarow's cluster idx
		if (m_kmeanL2Ptr->m_localFeatureIdx2ClusterIdx[rowIdxInContractor] != kMin)
		{
			m_kmeanL2Ptr->m_workerClusterChangeCnts[m_workerIdx]++;
		}
		m_kmeanL2Ptr->m_localFeatureIdx2ClusterIdx[rowIdxInContractor]=kMin;

		//update distorsion
		distorsionSum += distMin;
		if(i%100000==0)
		{
			cout<<IceUtil::Time::now().toDateTime()<< " UpdateKCntsAndKSums, workerIdx="<<m_workerIdx
				<<" processedRowIdx="<<i<<"/ "<<dataRowCnt<<endl;
		}
		
	}
	m_kmeanL2Ptr->m_workerDistortion[m_workerIdx] = distorsionSum;
}


///////////////////////////////////////////////////////////////////////
void
CCorrelationUpdateKCntsAndKSums::DoWork()
{
	const KMeanContractInfoPtr& contractPtr = m_kmeanL2Ptr->m_contractPtr;
	Ice::Long colCnt = contractPtr->ObserverIDs.size();
	Ice::Long rowCnt = contractPtr->K;

	Ice::Double *pKCnts = m_kmeanL2Ptr->m_workerKCnts[m_workerIdx];
	Ice::Double	*pKSums = m_kmeanL2Ptr->m_workerKSums[m_workerIdx];
	m_kmeanL2Ptr->m_workerClusterChangeCnts[m_workerIdx] = 0;

	if (m_resetKCntsKSumsFirst)
	{
		std::fill(pKCnts, pKCnts + rowCnt, 0.0);
		std::fill(pKSums, pKSums + (rowCnt*colCnt), 0.0);
	}

	Ice::Double distorsionSum = 0;
	//for each data row assign it to the nearest center kMin
	Ice::Long dataRowCnt = m_workerFeatureIdxTo - m_workerFeatureIdxFrom;
	Ice::Long K = contractPtr->K;

	for (Ice::Long i = 0; i<dataRowCnt; i++)
	{
		Ice::Long rowIdxInContractor = m_workerFeatureIdxFrom - contractPtr->FeatureIdxFrom + i;

		//dataRow
		Ice::Double *dr = m_kmeanL2Ptr->m_data.get() + (rowIdxInContractor*colCnt);

		Ice::Double distMin = 0;
		Ice::Long kMin = 0;

		//clusterRow
		Ice::Double *cr = 0;
		for (Ice::Long k = 0; k<K; k++)
		{
			cr = &m_kmeanL2Ptr->m_KClusters[k*colCnt];

			//compute distance
			//correlation distance = One minus the sample correlation between points 
			//both centroid and datarow should already be with zero mean and unit standard deviation
			Ice::Double dist = 1.0;
			for (int j = 0; j<colCnt; j++)
			{
				dist -= cr[j] * dr[j]; 
			}

			if (k == 0 || dist<distMin)
			{
				distMin = dist;
				kMin = k;
			}
		}

		//now kMin is the cluster closest to data row
		//update KCnts
		pKCnts[kMin]++;

		cr = &m_kmeanL2Ptr->m_KClusters[kMin*colCnt];
		//update KSums
		Ice::Double	*theKSums = pKSums + (kMin*colCnt);

		for (int j = 0; j<colCnt; j++)
		{
			theKSums[j] += dr[j]; //just the sum of new data points
		}

		//update datarow's cluster idx
		if (m_kmeanL2Ptr->m_localFeatureIdx2ClusterIdx[rowIdxInContractor] != kMin)
		{
			m_kmeanL2Ptr->m_workerClusterChangeCnts[m_workerIdx]++;
		}
		m_kmeanL2Ptr->m_localFeatureIdx2ClusterIdx[rowIdxInContractor] = kMin;

		//update distorsion
		distorsionSum += distMin;

		if (i % 100000 == 0)
		{
			cout << IceUtil::Time::now().toDateTime() << " UpdateKCntsAndKSums, workerIdx=" << m_workerIdx
				<< " processedRowIdx=" << i << "/ " << dataRowCnt << endl;
		}

	}
	m_kmeanL2Ptr->m_workerDistortion[m_workerIdx] = distorsionSum;
}

//////////////////////////////////////////////////////////////////////
//	KMeans++ Seeds
//////////////////////////////////////////////////////////////////////
void
CEuclideanPPSeedComputeMinDistance::DoWork()
{
	const KMeanContractInfoPtr& contractPtr = m_kmeanL2Ptr->m_contractPtr;
	Ice::Long colCnt = contractPtr->ObserverIDs.size();

	Ice::Long dataRowCnt = m_workerFeatureIdxTo - m_workerFeatureIdxFrom;
	const Ice::Long k = m_kmeanL2Ptr->m_it -1;

	Ice::Double distSum = 0;
	//seed Row
	Ice::Double *cr = &m_kmeanL2Ptr->m_KClusters[k*colCnt];;

	for (Ice::Long i = 0; i<dataRowCnt; i++)
	{
		Ice::Long rowIdxInContractor = m_workerFeatureIdxFrom - contractPtr->FeatureIdxFrom + i;

		//dataRow
		Ice::Double *dr = m_kmeanL2Ptr->m_data.get() + (rowIdxInContractor*colCnt);

		//compute distance
		Ice::Double dist = 0;
		for (int j = 0; j<colCnt; j++)
		{
			dist += (cr[j] - dr[j])*(cr[j] - dr[j]);
		}

		//update datarow's cluster idx
		if (m_kmeanL2Ptr->m_localFeatureIdx2MinDist[rowIdxInContractor]>dist)
		{
			m_kmeanL2Ptr->m_localFeatureIdx2MinDist[rowIdxInContractor] = dist;
		}
		distSum += m_kmeanL2Ptr->m_localFeatureIdx2MinDist[rowIdxInContractor];
	}
	m_kmeanL2Ptr->m_workerDistSum[m_workerIdx] = distSum;
}


void
CCorrelationPPSeedComputeMinDistance::DoWork()
{
	const KMeanContractInfoPtr& contractPtr = m_kmeanL2Ptr->m_contractPtr;
	Ice::Long colCnt = contractPtr->ObserverIDs.size();

	Ice::Long dataRowCnt = m_workerFeatureIdxTo - m_workerFeatureIdxFrom;
	const Ice::Long k = m_kmeanL2Ptr->m_it - 1;

	Ice::Double distSum = 0;
	//seed Row
	Ice::Double *cr = &m_kmeanL2Ptr->m_KClusters[k*colCnt];;

	for (Ice::Long i = 0; i<dataRowCnt; i++)
	{
		Ice::Long rowIdxInContractor = m_workerFeatureIdxFrom - contractPtr->FeatureIdxFrom + i;

		//dataRow
		Ice::Double *dr = m_kmeanL2Ptr->m_data.get() + (rowIdxInContractor*colCnt);

		//compute distance
		//correlation distance = One minus the sample correlation between points 
		//both centroid and datarow should already be with zero mean and unit standard deviation
		Ice::Double dist = 1.0;
		for (int j = 0; j<colCnt; j++)
		{
			dist -= cr[j] * dr[j];
		}
		if (dist<0)
		{
			dist = 0;
			cout << IceUtil::Time::now().toDateTime() 
				<< " CCorrelationPPSeedComputeMinDistance, dist " << dist << endl;
		}

		//update datarow's cluster idx
		if (m_kmeanL2Ptr->m_localFeatureIdx2MinDist[rowIdxInContractor]>dist)
		{
			m_kmeanL2Ptr->m_localFeatureIdx2MinDist[rowIdxInContractor] = dist;
		}
		distSum += m_kmeanL2Ptr->m_localFeatureIdx2MinDist[rowIdxInContractor];
	}
	m_kmeanL2Ptr->m_workerDistSum[m_workerIdx] = distSum;

	cout << IceUtil::Time::now().toDateTime()
		<< " CCorrelationPPSeedComputeMinDistance, workerIdx  " << m_workerIdx 
		<< " distSum" << distSum << endl;
}

/////////////////////////////////////////////////////////////////////////
void
CUniformPPSeedComputeMinDistance::DoWork()
{
	const KMeanContractInfoPtr& contractPtr = m_kmeanL2Ptr->m_contractPtr;
	Ice::Long dataRowCnt = m_workerFeatureIdxTo - m_workerFeatureIdxFrom;
	Ice::Double distSum = 0;
	for (Ice::Long i = 0; i<dataRowCnt; i++)
	{
		Ice::Long rowIdxInContractor = m_workerFeatureIdxFrom - contractPtr->FeatureIdxFrom + i;

		Ice::Double dist = 1.0;
		m_kmeanL2Ptr->m_localFeatureIdx2MinDist[rowIdxInContractor] = dist;
		distSum += m_kmeanL2Ptr->m_localFeatureIdx2MinDist[rowIdxInContractor];
	}
	m_kmeanL2Ptr->m_workerDistSum[m_workerIdx] = distSum;
	cout << IceUtil::Time::now().toDateTime()
		<< " CUniformPPSeedComputeMinDistance, workerIdx  " << m_workerIdx
		<< " distSum" << distSum << endl;
}