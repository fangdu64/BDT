#'
#' read output from bigMat
#' @export
#'
readMat <- function(matInfo) {
    mat = readBigMatrixAuto(matInfo$colCnt, matInfo$rowCnt, matInfo$storePathPrefix)
    return (mat)
}

#'
#' read output from bigMat
#' @export
#'
readVec <- function(vecInfo) {
    mat = readBigMatrixAuto(1, vecInfo$rowCnt, vecInfo$storePathPrefix)
    return (mat)
}

#'
#' read output from bigMat
#' @export
#'
readIntMat <- function(matInfo) {
    storePath = paste0(matInfo$storePathPrefix,".bfv")
    mat = readBigMatrixInt(matInfo$colCnt, matInfo$rowCnt, storePath)
    return (mat)
}

#'
#' read output from bigMat
#' @export
#'
readIntVec <- function(vecInfo) {
    storePath = paste0(vecInfo$storePathPrefix,".bfv")
    mat = readBigMatrixInt(1, vecInfo$rowCnt, storePath)
    return (mat)
}

#'
#' get script dir
#' @export
#'
getScriptDir <- function() {
    args = commandArgs()
    m <- regexpr("(?<=^--file=).+", args, perl=TRUE)
    scriptDir <- dirname(regmatches(args, m))
    if(length(scriptDir) == 0) stop("can't determine script dir: please call the script with Rscript")
    if(length(scriptDir) > 1) stop("can't determine script dir: more than one --file argument detected")
    return (scriptDir)
}

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

vecListToString<-function(vecList)
{
    outStr = c()
    for (v in vecList) {
        outStr <- append(outStr, paste0('[', paste0(v, collapse=","), ']'))
    }
    return (paste0(outStr, collapse=","))
}

