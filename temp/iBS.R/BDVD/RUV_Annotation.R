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
#data dirs
YDataDir="__YDataDir__"
XDataDir="__XDataDir__"

KsY=c(__KsY__)
NsY=c(__NsY__)
KsX=c(__KsX__)
NsX=c(__NsX__)

OnewayConfig=__OnewayConfig__ 
nearbyCnt=10

H1H0Ratio=1

matTblY=read.table(paste(YDataDir,"/matrix_info.txt",sep=""),stringsAsFactors =FALSE,
		sep="\t",header= TRUE)
matTblX=read.table(paste(XDataDir,"/matrix_info.txt",sep=""),stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

rowIDs_Y=readVectorFromTxt(paste(YDataDir,"/dataRowIDs.txt",sep="")) # 1-based
rowIDs_X=readVectorFromTxt(paste(XDataDir,"/dataRowIDs.txt",sep="")) # 1-based

locationTbl_Y=read.table(paste(YDataDir,"/locationTblFile.txt",sep=""),stringsAsFactors =FALSE,
		sep="\t",header= TRUE)
locationRowIDs_Y=readVectorFromTxt(paste(YDataDir,"/dataLocationTblRowIDs.txt",sep="")) # 1-based

#rowIDs=1:length(rowIDs_X)
rowIDs=sample(length(rowIDs_X),250000)

KsCntY=nrow(matTblY)
KsCntX=nrow(matTblX)
N=length(KsY)*length(KsX)
if(OnewayConfig){
	N=length(KsY)
}

##
## rowIDs_Y establishes X's (e.g., DNase) correct corresponding rowIDs in Y (here Exon)
##

scoreLabels= vector(mode="list", length=N)

runInfos=data.frame(
	name=rep("",N),
	kx=rep(0,N),
	nx=rep(0,N),
	ky=rep(0,N),
	ny=rep(0,N),
	stringsAsFactors=FALSE)
##
## identify Y rows by correlation
##
n=0
print("compute correlations")

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
		runInfos[n,"kx"]=k_x
		runInfos[n,"nx"]=extW_x
		runInfos[n,"ky"]=k_y
		runInfos[n,"ny"]=extW_y
		runInfos[n,"name"]=paste(
			getRUVConfigText(k_x,extW_x),
			getRUVConfigText(k_y,extW_y),sep=",")
		print(runInfos[n,"name"])
		
		scoreLabels[[n]]=mclapply(rowIDs,twoMatAnnotation,mat_X, mat_Y, rowIDs_X, rowIDs_Y,
			locationTbl_Y, locationRowIDs_Y,nearbyCnt,mc.cores=num_threads)
	}
}

##
## AUC Table
##
runRowCnt=getRUVConfigCnt(runInfos[,"kx"],runInfos[,"nx"])
runColCnt=getRUVConfigCnt(runInfos[,"ky"],runInfos[,"ny"])
runAUCs=matrix(0,runRowCnt,runColCnt)

#max TPR within given FDR level
runTPRs=matrix(0,runRowCnt,runColCnt)
MAX_FDR=0.1
FullRowCnt=length(rowIDs_X)
TopCnt=min(50000,FullRowCnt)
print(N)

##
## Accuracy plot
##
pdf(file = paste(out_dir,"/accuracy.pdf",sep=""))
plot(c(0,TopCnt), c(0,100), type="n", xlab="top # of annotations",
 ylab="% of correct annotations", xlim=c(0,TopCnt))
colors =c("salmon4", "red2", "dodgerblue3", "darkorange1", "green2", "black")
colors=rep(colors,as.integer(N/length(colors))+1)
linetype <- rep(1,N) 
plotchar <- rep(19,N)
sens=rep(0,N)
legendTxts=rep("",N)
for(i in 1:N)
{
	scores=unlist(lapply(scoreLabels[[i]],function(x) {
		x[1]
		})
	)
	lbs=unlist(lapply(scoreLabels[[i]],function(x) {
		x[2]
		})
	)
	
	oRowIDs = order(scores,decreasing = TRUE)
	scores=scores[oRowIDs[1:TopCnt]]
	lbs=lbs[oRowIDs[1:TopCnt]]
	xs=seq(from=100,to=TopCnt,by=100)
	ys=rep(1,length(xs))
	for( j in 1:length(xs))
	{
		ys[j]=sum(lbs[1:xs[j]])/xs[j]
	}
	sens[i]=ys[length(xs)]
	lines(xs, ys*100, type="l", lwd=2, lty=linetype[i], col=colors[i], pch=plotchar[i])
	legendTxts[i]=paste("[",runInfos[i,"name"],"], accuracy ",sprintf("%.3f",sens[i]),sep="")
}
# add a legend 
legend(200,94, legend=legendTxts,
 cex=1, col=colors, pch=plotchar, lty=linetype, bty ="n")
dev.off()

q(save="no")


for(n in 1:N)
{
	scores=unlist(lapply(scoreLabels[[n]],function(x) {
		x[1]
		})
	)
	lbs=unlist(lapply(scoreLabels[[n]],function(x) {
		x[2]
		})
	)
	
	oRowIDs = order(scores,decreasing = TRUE)
	scores=scores[oRowIDs[1:TopCnt]]
	lbs=lbs[oRowIDs[1:TopCnt]]
	pred <- prediction( scores, lbs)
	perf <- performance(pred,"auc")
	runRowID=getRUVConfigOrder(
		runInfos[,"kx"],runInfos[,"nx"],
		runInfos[n,"kx"],runInfos[n,"nx"])
	runColID=getRUVConfigOrder(
		runInfos[,"ky"],runInfos[,"ny"],
		runInfos[n,"ky"],runInfos[n,"ny"])
	runAUCs[runRowID,runColID]=as.numeric(perf@y.values)
	
	perf <- performance(pred,"tpr","fpr")
	xs=as.numeric(unlist(perf@x.values)) #fpr
	ys=as.numeric(unlist(perf@y.values)) #tpr
	fdr=xs/(xs+ys*H1H0Ratio)
	runTPRs[runRowID,runColID]=max(ys[fdr<MAX_FDR],na.rm = TRUE)
}

xlabls=getRUVConfigTexts(runInfos[,"kx"],runInfos[,"nx"])
xats=1:length(xlabls)
ylabls=getRUVConfigTexts(runInfos[,"ky"],runInfos[,"ny"])
yats=1:length(ylabls)

##
## AUC matrix plot
##
minAUC=min(runAUCs)
maxAUC=max(runAUCs)
pdf(file = paste(out_dir,"/aucs.pdf",sep=""))
levelplot(runAUCs,
	scales = list(x = list(at=xats, labels=xlabls), y = list(at=yats, labels=ylabls),tck = c(1,0)),
	main="AUC",
	colorkey = FALSE,
	xlab="DNase",
	ylab="Exon",
	at=unique(c(seq(minAUC-0.01, maxAUC+0.01,length=100))),
	col.regions = colorRampPalette(c("white", "red"))(1e2),
	panel=function(x,y,z,...) {
		panel.levelplot(x,y,z,...)
		panel.text(x, y, round(z,2))}
	)
dev.off()

##
## TPR matrix plot
##
minTPR=min(runTPRs)
maxTPR=max(runTPRs)
pdf(file = paste(out_dir,"/tprs.pdf",sep=""))
levelplot(runTPRs,
	scales = list(x = list(at=xats, labels=xlabls), y = list(at=yats, labels=ylabls),tck = c(1,0)),
	main=paste("Max TPR with FDR<",MAX_FDR,sep=""),
	colorkey = FALSE,
	xlab="DNase",
	ylab="Exon",
	at=unique(c(seq(minTPR-0.01, maxTPR+0.01,length=100))),
	col.regions = colorRampPalette(c("white", "red"))(1e2),
	panel=function(x,y,z,...) {
		panel.levelplot(x,y,z,...)
		panel.text(x, y, round(z,2))}
	)
dev.off()

##
## FDR plot
##

pdf(file = paste(out_dir,"/fdr.pdf",sep=""))
plot(c(0,1), c(0,1), type="n", xlab="False discovery rate",
 ylab="True positive rate", xlim=c(0,1))
abline(a=0,b=1, col="gray", lwd=1, lty = 2)
colors =c("salmon4", "red2", "dodgerblue3", "darkorange1", "green2", "black")
colors=rep(colors,as.integer(N/length(colors))+1)
linetype <- rep(1,N) 
plotchar <- rep(19,N)
aucs=rep(0,N)
legendTxts=rep("",N)
FullRowCnt=length(scoreLabels[[1]])
TopCnt=min(50000,FullRowCnt)
for(i in 1:N)
{
	scores=unlist(lapply(scoreLabels[[i]],function(x) {
		x[1]
		})
	)
	lbs=unlist(lapply(scoreLabels[[i]],function(x) {
		x[2]
		})
	)
	print(unique(lbs))
	if(length(unique(lbs))==1){
		next
	}
	
	oRowIDs = order(scores,decreasing = TRUE)
	scores=scores[oRowIDs[1:TopCnt]]
	lbs=lbs[oRowIDs[1:TopCnt]]
	pred <- prediction( scores, lbs)
	perf <- performance(pred,"tpr","fpr")
	xs=as.numeric(unlist(perf@x.values)) #fpr
	ys=as.numeric(unlist(perf@y.values)) #tpr
	fdr=xs/(xs+ys*H1H0Ratio)
	#maxTPR=max(ys[fdr<0.1],na.rm = TRUE)
	aucs[i]=sum(lbs)
	xs=fdr
	lines(xs, ys, type="l", lwd=2, lty=linetype[i], col=colors[i], pch=plotchar[i])
	legendTxts[i]=paste("[",runInfos[i,"name"],"], TPR ",sprintf("%.2f",aucs[i]),sep="")
}
# add a legend 
legend(0.6,1.05, legend=legendTxts,
 cex=1, col=colors, pch=plotchar, lty=linetype, bty ="n")
dev.off()

