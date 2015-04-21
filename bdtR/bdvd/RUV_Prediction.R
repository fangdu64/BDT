# CONSTANTS
# RSCRIPT_DIR	
# OUT_DIR	
#library(scales)
library(parallel)
library("ROCR")
library(latticeExtra)
rm(list=ls())
setwd("__RSCRIPT_DIR__")
source("Common.R")

out_dir="__OUT_DIR__"
num_threads=__NUM_THREADS__
min_predictorCnt=__MIN_PREDICTOR_CNT__
#data dirs
YDataDir="__YDataDir__"
XDataDir="__XDataDir__"

KsY=c(__KsY__)
NsY=c(__NsY__)
KsX=c(__KsX__)
NsX=c(__NsX__)

colIDs_train=c(__ColIDs_Train__)
colIDs_test=c(__ColIDs_Test__)

OnewayConfig=__OnewayConfig__ 

matTblY=read.table(paste(YDataDir,"/matrix_info.txt",sep=""),stringsAsFactors =FALSE,
		sep="\t",header= TRUE)
matTblX=read.table(paste(XDataDir,"/matrix_info.txt",sep=""),stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

rowIDs_Y=readVectorFromTxt(paste(YDataDir,"/dataRowIDs.txt",sep="")) # 1-based
rowIDs_X=readVectorFromTxt(paste(XDataDir,"/dataRowIDs.txt",sep="")) # 1-based

KsCntY=nrow(matTblY)
KsCntX=nrow(matTblX)
N=length(KsY)*length(KsX)
if(OnewayConfig){
	N=length(KsY)
}

##
## Establish response (row) and predictors (rows)
##

pmResponseRowIDs=unique(rowIDs_Y)
# number of predictive models
pmCnt=length(pmResponseRowIDs)

# for each predictive model, we need its predictors
pmPredictorRowIDs=vector(mode="list", length=pmCnt)
for(i in 1:pmCnt)
{
	rowID_Y=pmResponseRowIDs[i]
	pmPredictorRowIDs[[i]]=rowIDs_X[which(rowIDs_Y==rowID_Y)]
}

pmIDs=which(lapply(pmPredictorRowIDs,length)>=min_predictorCnt)

print(paste("Total # models:",pmCnt,", selected models:",length(pmIDs)))

RMSE.test= vector(mode="list", length=N)
AbsErrors.test= vector(mode="list", length=N)

runInfos=data.frame(
	name=rep("",N),
	kx=rep(0,N),
	nx=rep(0,N),
	ky=rep(0,N),
	ny=rep(0,N),
	stringsAsFactors=FALSE)
##
## compute correlations for signal pairs
##
n_noruv=0
n=0

for(i in 1:KsCntX){
	k_x = matTblX[i,"k"]
	extW_x=matTblX[i,"extW"]
	if(pairInConfiguration(k_x,extW_x,KsX,NsX)==0){
		next
	}
	
	colCntX = matTblX[i,"ColCnt"]
	rowCntX = matTblX[i,"RowCnt"]
	dataFileX = matTblX[i,"DataFile"]
	mat_X= readBigMatrix(colCntX,rowCntX,dataFileX)
	for(j in 1:KsCntY){
		k_y = matTblY[j,"k"]
		extW_y=matTblY[j,"extW"]
		
		cfg2=0
		if(OnewayConfig){
			cfg2=pairInTwoConfig(k_x,extW_x,KsX,NsX,k_y,extW_y,KsY,NsY)
		}
		else{
			cfg2=pairInConfiguration(k_y,extW_y,KsY,NsY)
		}
		if(cfg2==0){
			next
		}
		
		colCntY = matTblY[j,"ColCnt"]
		rowCntY = matTblY[j,"RowCnt"]
		dataFileY = matTblY[j,"DataFile"]
		mat_Y= readBigMatrix(colCntY,rowCntY,dataFileY)
		n=n+1
		if(k_x==0 && extW_x==0){
			n_noruv=n
		}
		runInfos[n,"kx"]=k_x
		runInfos[n,"nx"]=extW_x
		runInfos[n,"ky"]=k_y
		runInfos[n,"ny"]=extW_y
		runInfos[n,"name"]=paste(
			getRUVConfigText(k_x,extW_x),
			getRUVConfigText(k_y,extW_y),sep=",")
		print(runInfos[n,"name"])
		
		AbsErrors.test[[n]]=mclapply(pmIDs,twoMatPredictionTestErrors,mat_X, mat_Y, pmPredictorRowIDs, pmResponseRowIDs, colIDs_train, colIDs_test,mc.cores=num_threads)
		
		RMSE.test[[n]] =lapply(AbsErrors.test[[n]],mean)
	}
}


absError_0=unlist(AbsErrors.test[[n_noruv]],use.names = FALSE)
pairedErrors=lapply(AbsErrors.test,function(x){
		pe=unlist(x,use.names = FALSE)-absError_0
		return (pe)
	}
	)
rm(absError_0,AbsErrors.test)

RMSE.test=lapply(RMSE.test,unlist,use.names = FALSE)

oidxs=getRUVConfigOrders(runInfos[,"kx"],runInfos[,"nx"])
RMSE.test=RMSE.test[oidxs]
names(RMSE.test)=getRUVConfigTexts(runInfos[,"kx"],runInfos[,"nx"])

pairedErrors=pairedErrors[oidxs]
names(pairedErrors)=getRUVConfigTexts(runInfos[,"kx"],runInfos[,"nx"])
##
## RMSE.test box plot
##

#dev.new()
pdf(file = paste(out_dir,"/rmse_test_boxplot.pdf",sep=""))
plotdata <- boxplot(RMSE.test, ylab = "RMSE", outline = FALSE)
dev.off()


##
## paired test errors box plot
##

pdf(file = paste(out_dir,"/paired_testerrors_boxplot.pdf",sep=""))
plotdata <- boxplot(pairedErrors[2:length(pairedErrors)], ylab = "Relative Errors", outline = FALSE)
abline(h=0, col="pink", lwd=2,lty=3)
dev.off()