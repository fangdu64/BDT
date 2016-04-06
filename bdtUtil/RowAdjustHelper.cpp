#include <bdt/bdtBase.h>
#include <IceUtil/Random.h>
#include <bdtUtil/RowAdjustHelper.h>
#include <algorithm>     
#include <limits>
#include <math.h>

void CRowAdjustHelper::Adjust(Ice::Double *values,
	Ice::Long rowCnt, Ice::Long colCnt, iBS::RowAdjustEnum rowAdjust)
{
	
	if (rowAdjust == iBS::RowAdjustZeroMeanUnitSD)
	{
		Ice::Double constVal = 0;
		Ice::Double len_square = (Ice::Double)colCnt;
		AdjustByZeroMeanAndLength(values, rowCnt, colCnt, constVal, len_square);
	}
	else if (rowAdjust == iBS::RowAdjustZeroMeanUnitLength)
	{
		Ice::Double constVal = 1.0/sqrt((Ice::Double)colCnt);
		Ice::Double len_square = 1.0;
		AdjustByZeroMeanAndLength(values, rowCnt, colCnt, constVal, len_square);
	}
	else if (rowAdjust == iBS::RowAdjustZeroMeanUnitLengthConst0)
	{
		Ice::Double constVal = 0;
		Ice::Double len_square = 1.0;
		AdjustByZeroMeanAndLength(values, rowCnt, colCnt, constVal, len_square);
	}
	
}


void CRowAdjustHelper::AdjustByZeroMeanAndLength(Ice::Double *values,
	Ice::Long rowCnt, Ice::Long colCnt, Ice::Double constVal, Ice::Double len_square)
{
	Ice::Double rowMean = 0;
	Ice::Double squareSum = 0;
	Ice::Long idx = 0;
	Ice::Double d = (Ice::Double)colCnt;
	Ice::Double epsilon = std::numeric_limits<Ice::Double>::epsilon();

	for (Ice::Long i = 0; i < rowCnt; i++)
	{
		squareSum = 0;
		rowMean = 0;
		for (Ice::Long j = 0; j < colCnt; j++)
		{
			idx = i*colCnt + j;
			rowMean += values[idx];
		}
		rowMean /= d;
		for (Ice::Long j = 0; j < colCnt; j++)
		{
			idx = i*colCnt + j;
			values[idx] -= rowMean;
			squareSum += values[idx] * values[idx];
		}

		squareSum = sqrt(squareSum / len_square);
		if (squareSum < epsilon)
		{
			for (Ice::Long j = 0; j < colCnt; j++)
			{
				idx = i*colCnt + j;
				values[idx] = constVal;
			}
			continue;
		}

		for (Ice::Long j = 0; j < colCnt; j++)
		{
			idx = i*colCnt + j;
			values[idx] /= squareSum;
		}
	}
}

void CRowAdjustHelper::ConvertFromLogToRawCnt(Ice::Double *values,
	Ice::Long rowCnt, Ice::Long colCnt)
{
	Ice::Long idx = 0;
	for (Ice::Long i = 0; i < rowCnt; i++)
	{
		for (Ice::Long j = 0; j < colCnt; j++)
		{
			idx = i*colCnt + j;
			values[idx] = exp(values[idx])-1.0;
		}
	}
}

void CRowAdjustHelper::ConvertFromLog2ToRawCnt(Ice::Double *values,
    Ice::Long rowCnt, Ice::Long colCnt)
{
    Ice::Long idx = 0;
    for (Ice::Long i = 0; i < rowCnt; i++)
    {
        for (Ice::Long j = 0; j < colCnt; j++)
        {
            idx = i*colCnt + j;
            values[idx] = exp2(values[idx]) - 1.0;
        }
    }
}

void CRowAdjustHelper::AddRowMeansBackToY(Ice::Long rowCnt, Ice::Long colCnt,
	Ice::Double *Y, Ice::Double *rowMeans)
{
	Ice::Long idx = 0;
	for (Ice::Long i = 0; i < rowCnt; i++)
	{
		for (Ice::Long j = 0; j < colCnt; j++)
		{
			idx = i*colCnt + j;
			Y[idx] += rowMeans[i];
		}
	}
}

void CRowAdjustHelper::ReArrangeByColIdxs(Ice::Double *values,
	Ice::Long rowCnt, Ice::Long colCnt, const iBS::IntVec& colIdxs, Ice::Double *retValues)
{
	Ice::Long idx = 0;
	Ice::Long retColCnt = colIdxs.size();
	Ice::Long colIdx = 0;
	Ice::Double nan = std::numeric_limits<Ice::Double>::quiet_NaN();
	for (Ice::Long i = 0; i < rowCnt; i++)
	{
		for (Ice::Long j = 0; j < retColCnt; j++)
		{
			idx = i*retColCnt + j;
			colIdx = colIdxs[j];
			if (colIdx >= 0)
			{
				retValues[idx] = values[i*colCnt + colIdx];
			}
			else
			{
				retValues[idx] = nan;
			}
		}
	}
}

Ice::Long CRowAdjustHelper::RowSelectedFlags(Ice::Double *values,
	Ice::Long rowCnt, Ice::Long colCnt, const iBS::RowSelection& selection, Ice::Byte *rowFlags)
{
	if (selection.Statistic == iBS::RowStatisticMax
		&&selection.Selector==iBS::RowSelectorHigherThanThreshold)
	{
		return RowSelectedFlags_RowMaxLargerThan(values, rowCnt, colCnt, selection.Threshold, rowFlags);
	}
	else
	{
		return 0;
	}
}

Ice::Long CRowAdjustHelper::RowSelectedFlags_RowMaxLargerThan(Ice::Double *values,
	Ice::Long rowCnt, Ice::Long colCnt, Ice::Double threshold, Ice::Byte *rowFlags)
{
	Ice::Long selectedCnt = 0;
	Ice::Long idx = 0;
	for (Ice::Long i = 0; i < rowCnt; i++)
	{
		rowFlags[i] = 0;
		for (Ice::Long j = 0; j < colCnt; j++)
		{
			idx = i*colCnt + j;
			if (values[idx]>threshold)
			{
				rowFlags[i] = 1;
				selectedCnt++;
				break;
			}
		}
	}
	return selectedCnt;
}