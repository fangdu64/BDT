# CONSTANTS
# ANNOTATION_DATA_DIR

readBigMatrix<-function(colCnt,rowCnt,bfvFile)
{
	totalValueCnt = rowCnt*colCnt
	con=file(bfvFile, "rb")
	Y = readBin(con, double(),n=totalValueCnt,endian = "little")
	Y=matrix(data=Y,nrow=rowCnt,ncol=colCnt,byrow=TRUE)
	close(con)
	return (Y)
}

readBigMatrixAuto<-function(colCnt,rowCnt,bfvFilePrefix, batchFileSizeMB=1024)
{
	szValueType=8
	batchFileSize=batchFileSizeMB*1024*1024
	rowBytesSize=colCnt*szValueType
	if(batchFileSize%%rowBytesSize!=0){
		#data row will not split in two batch files
		batchFileSize = batchFileSize-(batchFileSize%%rowBytesSize)
	}
	valueCntPerBatchFile = as.integer(batchFileSize / szValueType)
	rowCntPerBatchFile = as.integer(valueCntPerBatchFile / colCnt)
	batchFileIdxFrom=0
	batchFileIdxTo= as.integer((rowCnt-1)/rowCntPerBatchFile)
	if(batchFileIdxTo==batchFileIdxFrom)
	{
		bfvFile=paste(bfvFilePrefix,".bfv",sep="")
		return (readBigMatrix(colCnt,rowCnt,bfvFile))
	}
	else
	{
		return (readBigMatrixBatches(colCnt,rowCnt,bfvFile,batchFileSizeMB))
	}

}

readBigMatrixBatches<-function(colCnt,rowCnt,bfvFilePrefix, batchFileSizeMB=1024)
{
	szValueType=8
	batchFileSize=batchFileSizeMB*1024*1024
	rowBytesSize=colCnt*szValueType
	if(batchFileSize%%rowBytesSize!=0){
		#data row will not split in two batch files
		batchFileSize = batchFileSize-(batchFileSize%%rowBytesSize)
	}
	valueCntPerBatchFile = as.integer(batchFileSize / szValueType)
	rowCntPerBatchFile = as.integer(valueCntPerBatchFile / colCnt)
	batchFileIdxFrom=0
	batchFileIdxTo= as.integer((rowCnt-1)/rowCntPerBatchFile)
	Y=matrix(NA,nrow=rowCnt,ncol=colCnt)
	for(i in batchFileIdxFrom:batchFileIdxTo)
	{
		bfvFile=paste(bfvFilePrefix,"_",i,".bfv",sep="")
		rowIDFrom = i*rowCntPerBatchFile
		if(i==batchFileIdxTo){
			thisBatchRowCnt = rowCnt%%rowCntPerBatchFile
		}
		else{
			thisBatchRowCnt = rowCntPerBatchFile
		}

		y=readBigMatrix(colCnt,thisBatchRowCnt,bfvFile)
		batchRowIDs=(rowIDFrom+1):(rowIDFrom+thisBatchRowCnt)
		Y[batchRowIDs,]=y
	}
	return (Y)
}

readBigMatrixInt<-function(colCnt,rowCnt,bfvFile)
{
	totalValueCnt = rowCnt*colCnt
	con=file(bfvFile, "rb")
	Y = readBin(con, integer(),size=4,n=totalValueCnt,endian = "little")
	Y=matrix(data=Y,nrow=rowCnt,ncol=colCnt,byrow=TRUE)
	close(con)
	return (Y)
}

readVectorFromTxt<-function(txtFile)
{
	vec=read.table(txtFile, sep="\t")
	vec=vec[,1]
	return (vec)
}
