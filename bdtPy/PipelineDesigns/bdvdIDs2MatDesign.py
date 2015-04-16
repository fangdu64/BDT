import os
import pickle
import iBSDefines

desing_params=os.path.dirname(os.path.abspath(__file__))+"/design_params.pickle"
desing_params=desing_params.replace("/script/design_params.pickle","-script/design_params.pickle")

SampleNames,ColCnt,RowCnt,DataFile,FieldSep,StartingRowIdx,AddValue,CalcStatistics=iBSDefines.loadPickle(desing_params)
# ==================================================================
# calculate row count
# ==================================================================
def getRowCount():
    rowCnt = 0;
    lineNum=0
    for line in open(DataFile):
        lineNum = lineNum+1
        if lineNum < (StartingRowIdx+1):
            continue
        if line.strip():
            rowCnt = rowCnt + 1;
    return rowCnt

# ==================================================================
# upload data
# ==================================================================
def uploadData(fcdcPrx, matID, batchRowCnt):
    lineNum=0
    dataValues=[]
    thisBatchRowCnt=0
    rowIdxFrom=0
    batch=0
    for line in open(DataFile):
        lineNum = lineNum+1
        if lineNum < (StartingRowIdx+1):
            continue
        
        fields=line.rstrip('\n').split(FieldSep)
        for j in range(ColCnt):
            #skip first column
            dataValues.append(float(fields[j+0])+AddValue)
        
        #===================================
        # No need to change below this line
        #===================================
        thisBatchRowCnt=thisBatchRowCnt+1
        if thisBatchRowCnt>=batchRowCnt:
            batch=batch+1
            print("uploading batch {0}, rows {1} - {2} ...".format(batch, rowIdxFrom, rowIdxFrom+thisBatchRowCnt))
            fcdcPrx.SetDoublesRowMatrix(matID,rowIdxFrom,rowIdxFrom+thisBatchRowCnt,dataValues)
            rowIdxFrom = rowIdxFrom +thisBatchRowCnt
            dataValues=[]
            thisBatchRowCnt=0
    if thisBatchRowCnt>0:
            batch=batch+1
            print("uploading batch {0}, rows {1} - {2} ...".format(batch, rowIdxFrom, rowIdxFrom+thisBatchRowCnt))
            fcdcPrx.SetDoublesRowMatrix(matID,rowIdxFrom,rowIdxFrom+thisBatchRowCnt,dataValues)
            rowIdxFrom = rowIdxFrom +thisBatchRowCnt
            dataValues=[]
            thisBatchRowCnt=0
