#include <bdt/bdtBase.h>
#include <IceUtil/Random.h>
#include <StatisticsHelper.h>
#include <GlobalVars.h>
#include <algorithm>     
#include <limits>
#include <boost/math/distributions/fisher_f.hpp>

Ice::Double CStatisticsHelper::GetCriticalFStatistics(
	Ice::Double bgDF, Ice::Double wgDF, Ice::Double pvalue)
{
	using boost::math::fisher_f;
	fisher_f dist(bgDF,wgDF);
	Ice::Double criticalVal=quantile(dist,1.0-pvalue);
	Ice::Double p=cdf(dist,criticalVal);
	return criticalVal;
}


Ice::Double CStatisticsHelper::GetOneWayANOVA(const ::Ice::Double* Y, const iBS::IntVecVec& groupColIdxs)
{
	int P = (int)groupColIdxs.size();
	int colCnt = 0;
	for (int i = 0; i<P; i++)
	{
		colCnt += (int)groupColIdxs[i].size();
	}

	iBS::DoubleVec groupMeans(P, 0);
	Ice::Double grandMean = 0;
	Ice::Double epsilon = std::numeric_limits<Ice::Double>::epsilon();
	//"explained variance", or "between-group variability
	Ice::Double bgVar = 0;
	Ice::Double bgDF = P - 1;

	//"unexplained variance", or "within-group variability"
	Ice::Double wgVar = 0;
	Ice::Double wgDF = (Ice::Double)colCnt - P;

	using boost::math::fisher_f;
	fisher_f dist(bgDF, wgDF);


	for (int j = 0; j<P; j++)
	{
		int groupSize = (int)groupColIdxs[j].size();
		groupMeans[j] = 0;
		for (int k = 0; k<groupSize; k++)
		{
			int colIdx = groupColIdxs[j][k];
			groupMeans[j] += Y[colIdx];
		}
		grandMean += groupMeans[j];
		groupMeans[j] /= groupSize;
	}
	grandMean /= colCnt;

	for (int j = 0; j<P; j++)
	{
		int groupSize = (int)groupColIdxs[j].size();

		bgVar += groupSize*(groupMeans[j] - grandMean)*(groupMeans[j] - grandMean);

		for (int k = 0; k<groupSize; k++)
		{
			int colIdx = groupColIdxs[j][k];
			wgVar += (Y[colIdx] - groupMeans[j])*(Y[colIdx] - groupMeans[j]);
		}
	}
	bgVar /= bgDF;
	wgVar /= wgDF;
	::Ice::Double F = 0;
	if (wgVar>epsilon)
	{
		F = bgVar / wgVar;
	}
	else
	{
		//all columns are equal, discard this row
		F = -1.0;
	}

	return F;
}

bool CStatisticsHelper::GetOneWayANOVA(const ::Ice::Double* Y, Ice::Long colCnt, 
	Ice::Long featureIdxFrom, Ice::Long featureIdxTo, 
	const iBS::IntVecVec& conditionSampleIdxs, ::Ice::Double* FStatistics)
{
	int P=(int)conditionSampleIdxs.size();
	iBS::DoubleVecVec groupValues(P);
	iBS::DoubleVec groupMeans(P,0);

	Ice::Double grandMean=0;
	for(int i=0;i<P;i++)
	{
		groupValues[i].resize(conditionSampleIdxs[i].size(),0);
	}
	
	Ice::Long rowCnt=featureIdxTo-featureIdxFrom;
	Ice::Double epsilon = std::numeric_limits<Ice::Double>::epsilon();
	//"explained variance", or "between-group variability
	Ice::Double bgVar=0;
	Ice::Double bgDF=P-1;

	//"unexplained variance", or "within-group variability"
	Ice::Double wgVar=0;
	Ice::Double wgDF=(Ice::Double)colCnt-P;

	using boost::math::fisher_f;
	fisher_f dist(bgDF,wgDF);

	
	for(int i=0;i<rowCnt;i++)
	{
		//reorganize according to conditions
		grandMean=0;  
		for(int j=0;j<P;j++)
		{
			int groupSize=(int)conditionSampleIdxs[j].size();
			groupMeans[j]=0;
			for(int k=0;k<groupSize;k++)
			{
				int colIdx=conditionSampleIdxs[j][k];
				groupValues[j][k]=Y[i*colCnt+colIdx];
				groupMeans[j]+=Y[i*colCnt+colIdx];
			}
			grandMean+=groupMeans[j];
			groupMeans[j]/=groupSize;
		}
		grandMean/=colCnt;

		bgVar=0;
		wgVar=0;

		for(int j=0;j<P;j++)
		{
			int groupSize=(int)conditionSampleIdxs[j].size();

			bgVar+=groupSize*(groupMeans[j]-grandMean)*(groupMeans[j]-grandMean);

			for(int k=0;k<groupSize;k++)
			{
				wgVar+=(groupValues[j][k]-groupMeans[j])*(groupValues[j][k]-groupMeans[j]);
			}
		}
		bgVar/=bgDF;
		wgVar/=wgDF;
		if(wgVar>epsilon)
		{
			::Ice::Double F=bgVar/wgVar;

			//F=cdf(dist, F);
			//FStatistics[i]=1.0-cdf(dist, F);

			FStatistics[i]=F;
		}
		else
		{
			//all columns are equal, discard this row
			FStatistics[i]=100.0;
		}

		
		
	}

	return true;
}
