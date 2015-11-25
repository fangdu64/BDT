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
SignalMate1="__SignalMate1__"
SignalMate2="__SignalMate2__"
NoiseMate1="__NoiseMate1__"
NoiseMate2="__NoiseMate2__"

KsMate1=c(__KsMate1__)
NsMate1=c(__NsMate1__)
KsMate2=c(__KsMate2__)
NsMate2=c(__NsMate2__)

OnewayConfig=__OnewayConfig__ 

H1H0Ratio=__H1H0Ratio__
MAX_FDR=__MAX_FDR__

matTblSignalMate1=read.table(paste(SignalMate1,"/matrix_info.txt",sep=""),stringsAsFactors =FALSE,
		sep="\t",header= TRUE)
matTblSignalMate2=read.table(paste(SignalMate2,"/matrix_info.txt",sep=""),stringsAsFactors =FALSE,
		sep="\t",header= TRUE)
matTblNoiseMate1=read.table(paste(NoiseMate1,"/matrix_info.txt",sep=""),stringsAsFactors =FALSE,
		sep="\t",header= TRUE)
matTblNoiseMate2=read.table(paste(NoiseMate2,"/matrix_info.txt",sep=""),stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

rowIDs_s1=readVectorFromTxt(paste(SignalMate1,"/dataRowIDs.txt",sep=""))
rowIDs_s2=readVectorFromTxt(paste(SignalMate2,"/dataRowIDs.txt",sep=""))
rowIDs_n1=readVectorFromTxt(paste(NoiseMate1,"/dataRowIDs.txt",sep=""))
rowIDs_n2=readVectorFromTxt(paste(NoiseMate2,"/dataRowIDs.txt",sep=""))
rowIDs=1:length(rowIDs_s1)

KsCntMate1=nrow(matTblSignalMate1)
KsCntMate2=nrow(matTblSignalMate2)
N=length(KsMate1)*length(KsMate2)
if(OnewayConfig){
	N=length(KsMate1)
}

corSignals= vector(mode="list", length=N)
corNoises= vector(mode="list", length=N)

runInfos=data.frame(
	name=rep("",N),
	k1=rep(0,N),
	n1=rep(0,N),
	k2=rep(0,N),
	n2=rep(0,N),
	stringsAsFactors=FALSE)
##
## compute correlations for signal pairs
##
n=0
print("compute correlations for signal pairs")
for(i in 1:KsCntMate1){
	k_1 = matTblSignalMate1[i,"k"]
	extW_1=matTblSignalMate1[i,"extW"]
	if(pairInConfiguration(k_1,extW_1,KsMate1,NsMate1)==0){
		next
	}
	colCnt1 = matTblSignalMate1[i,"ColCnt"]
	rowCnt1 = matTblSignalMate1[i,"RowCnt"]
	dataFile1 = matTblSignalMate1[i,"DataFile"]
	mat_s1= readBigMatrix(colCnt1,rowCnt1,dataFile1)
	for(j in 1:KsCntMate2){
		
		k_2 = matTblSignalMate2[j,"k"]
		extW_2=matTblSignalMate2[j,"extW"]
		
		cfg2=0
		if(OnewayConfig){
			cfg2=pairInTwoConfig(k_1,extW_1,KsMate1,NsMate1,k_2,extW_2,KsMate2,NsMate2)
		}
		else{
			cfg2=pairInConfiguration(k_2,extW_2,KsMate2,NsMate2)
		}
		if(cfg2==0){
			next
		}
		
		colCnt2 = matTblSignalMate2[j,"ColCnt"]
		rowCnt2 = matTblSignalMate2[j,"RowCnt"]
		dataFile2 = matTblSignalMate2[j,"DataFile"]
		mat_s2= readBigMatrix(colCnt2,rowCnt2,dataFile2)
		n=n+1
		runInfos[n,"k1"]=k_1
		runInfos[n,"n1"]=extW_1
		runInfos[n,"k2"]=k_2
		runInfos[n,"n2"]=extW_2
		runInfos[n,"name"]=paste(
			getRUVConfigText(k_1,extW_1),
			getRUVConfigText(k_2,extW_2),sep=",")
		print(runInfos[n,"name"])
		corSignals[[n]]=mclapply(rowIDs,twoMatRowCor,mat_s1,mat_s2,rowIDs_s1,rowIDs_s2,mc.cores=num_threads)
	}
}

##
## compute correlations for background pairs
##
n=0
print("compute correlations for background pairs")
for(i in 1:KsCntMate1){
	k_1 = matTblNoiseMate1[i,"k"]
	extW_1=matTblNoiseMate1[i,"extW"]
	if(pairInConfiguration(k_1,extW_1,KsMate1,NsMate1)==0){
		next
	}
	colCnt1 = matTblNoiseMate1[i,"ColCnt"]
	rowCnt1 = matTblNoiseMate1[i,"RowCnt"]
	dataFile1 = matTblNoiseMate1[i,"DataFile"]
	mat_s1= readBigMatrix(colCnt1,rowCnt1,dataFile1)
	for(j in 1:KsCntMate2){
		k_2 = matTblNoiseMate2[j,"k"]
		extW_2=matTblNoiseMate2[j,"extW"]
		
		if(OnewayConfig){
			cfg2=pairInTwoConfig(k_1,extW_1,KsMate1,NsMate1,k_2,extW_2,KsMate2,NsMate2)
		}
		else{
			cfg2=pairInConfiguration(k_2,extW_2,KsMate2,NsMate2)
		}
		if(cfg2==0){
			next
		}
		
		colCnt2 = matTblNoiseMate2[j,"ColCnt"]
		rowCnt2 = matTblNoiseMate2[j,"RowCnt"]
		dataFile2 = matTblNoiseMate2[j,"DataFile"]
		mat_s2= readBigMatrix(colCnt2,rowCnt2,dataFile2)
		n=n+1
		print(runInfos[n,"name"])
		corNoises[[n]]=mclapply(rowIDs,twoMatRowCor,mat_s1,mat_s2,rowIDs_s1,rowIDs_s2,mc.cores=num_threads)
	}
}

if(FALSE){
	save(runInfos , corSignals, corNoises, file = paste(out_dir,"/ruv_correlation.rda",sep=""))
	rm(list=ls())
	out_dir="__OUT_DIR__"
	load(paste(out_dir,"/ruv_correlation.rda",sep=""))
	N=nrow(runInfos)
}

##
## AUC Table
##
runRowCnt=getRUVConfigCnt(runInfos[,"k1"],runInfos[,"n1"])
runColCnt=getRUVConfigCnt(runInfos[,"k2"],runInfos[,"n2"])
runAUCs=matrix(0,runRowCnt,runColCnt)

#max TPR within given FDR level
runTPRs=matrix(0,runRowCnt,runColCnt)

#max TPR within given FDR level
runSensitivity=matrix(0,runRowCnt,runColCnt)

FullRowCnt=length(corSignals[[1]])
TopCnt=min(50000,FullRowCnt)

##
## Signal and Noise density
##

for(n in 1:N)
{
	signalScores=unlist(corSignals[[n]])
	str(signalScores)
	noiseScores=unlist(corNoises[[n]])

	pdf(file = paste(out_dir,"/sn_density_",runInfos[n,"name"],".pdf",sep=""))
	plot(density(noiseScores,na.rm=TRUE,bw = 0.01),lwd=3, col="deepskyblue", xlim=c(-1,1),ylim=c(0,3))
	lines(density(signalScores,na.rm=TRUE),lwd=3, col="red")
	dev.off()
}
#q(save="no")

##
## Sensitivity plot
##
pdf(file = paste(out_dir,"/accuracy.pdf",sep=""))
plot(c(0,TopCnt), c(80,100), type="n", xlab="top # of pairs",
 ylab="% of signal pairs", xlim=c(0,TopCnt))
colors =c("salmon4", "red2", "dodgerblue3", "darkorange1", "green2", "black")
colors=rep(colors,as.integer(N/length(colors))+1)
linetype <- rep(1,N) 
plotchar <- rep(19,N)
sens=rep(0,N)
legendTxts=rep("",N)
for(i in 1:N)
{
	scores=c(unlist(corSignals[[i]]),unlist(corNoises[[i]]))
	lbs=c(rep(1,FullRowCnt),rep(0,FullRowCnt))
	
	oRowIDs = order(scores,decreasing = TRUE)
	scores=scores[oRowIDs[1:TopCnt]]
	lbs=lbs[oRowIDs[1:TopCnt]]
	xs=seq(from=100,to=TopCnt,by=100)
	ys=rep(1,length(xs))
	for( j in 1:length(xs))
	{
		ys[j]=sum(lbs[1:xs[j]])/xs[j]
	}
	print(xs)
	print(ys)
	sens[i]=ys[length(xs)]
	lines(xs, ys*100, type="l", lwd=2, lty=linetype[i], col=colors[i], pch=plotchar[i])
	legendTxts[i]=paste("[",runInfos[i,"name"],"], accuracy ",sprintf("%.3f",sens[i]),sep="")
}
# add a legend 
legend(200,94, legend=legendTxts,
 cex=1, col=colors, pch=plotchar, lty=linetype, bty ="n")
dev.off()

q(save="no")

##
## AUC Table
##
print(N)
for(n in 1:N)
{
	scores=c(unlist(corSignals[[n]]),unlist(corNoises[[n]]))
	lbs=c(rep(1,FullRowCnt),rep(0,FullRowCnt))
	oRowIDs = order(scores,decreasing = TRUE)
	scores=scores[oRowIDs[1:TopCnt]]
	lbs=lbs[oRowIDs[1:TopCnt]]
	pred <- prediction( scores, lbs)
	perf <- performance(pred,"auc")
	runRowID=getRUVConfigOrder(
		runInfos[,"k1"],runInfos[,"n1"],
		runInfos[n,"k1"],runInfos[n,"n1"])
	runColID=getRUVConfigOrder(
		runInfos[,"k2"],runInfos[,"n2"],
		runInfos[n,"k2"],runInfos[n,"n2"])
	runAUCs[runRowID,runColID]=as.numeric(perf@y.values)
	
	perf <- performance(pred,"tpr","fpr")
	xs=as.numeric(unlist(perf@x.values)) #fpr
	ys=as.numeric(unlist(perf@y.values)) #tpr
	fdr=xs/(xs+ys*H1H0Ratio)
	runTPRs[runRowID,runColID]=max(ys[fdr<MAX_FDR],na.rm = TRUE)
}



xlabls=getRUVConfigTexts(runInfos[,"k1"],runInfos[,"n1"])
xats=1:length(xlabls)
ylabls=getRUVConfigTexts(runInfos[,"k2"],runInfos[,"n2"])
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
## ROC plot
##
pdf(file = paste(out_dir,"/roc.pdf",sep=""))
plot(c(0,1), c(0,1), type="n", xlab="False positive rate",
 ylab="True positive rate", xlim=c(0,1))
abline(a=0,b=1, col="gray", lwd=1, lty = 2)
colors =c("salmon4", "red2", "dodgerblue3", "darkorange1", "green2", "black")
colors=rep(colors,as.integer(N/length(colors))+1)
linetype <- rep(1,N) 
plotchar <- rep(19,N)
aucs=rep(0,N)
legendTxts=rep("",N)
FullRowCnt=length(corSignals[[1]])
TopCnt=min(50000,FullRowCnt)
for(i in 1:N)
{
	scores=c(unlist(corSignals[[i]]),unlist(corNoises[[i]]))
	lbs=c(rep(1,FullRowCnt),rep(0,FullRowCnt))
	
	oRowIDs = order(scores,decreasing = TRUE)
	scores=scores[oRowIDs[1:TopCnt]]
	lbs=lbs[oRowIDs[1:TopCnt]]
	pred <- prediction( scores, lbs)
	perf <- performance(pred,"tpr","fpr")
	xs=as.numeric(unlist(perf@x.values))
	ys=as.numeric(unlist(perf@y.values))
	perf <- performance(pred,"auc")
	aucs[i]=perf@y.values
	lines(xs, ys, type="l", lwd=2, lty=linetype[i], col=colors[i], pch=plotchar[i])
	legendTxts[i]=paste("[",runInfos[i,"name"],"], auc ",sprintf("%.2f",aucs[i]),sep="")
}
# add a legend 
legend(0.6,0.6, legend=legendTxts,
 cex=1, col=colors, pch=plotchar, lty=linetype, bty ="n")
dev.off()

##
## FDR plot
##


pdf(file = paste(out_dir,"/fdr.pdf",sep=""))
plot(c(0,1), c(0,1), type="n", xlab="False discovery rate",
 ylab="True positive rate", xlim=c(0,1))
abline(a=0,b=1, col="gray", lwd=1, lty = 2)
#colors <- 1:N
linetype <- rep(1,N) 
plotchar <- rep(19,N)
aucs=rep(0,N)
legendTxts=rep("",N)
for(i in 1:N)
{
	scores=c(unlist(corSignals[[i]]),unlist(corNoises[[i]]))
	lbs=c(rep(1,FullRowCnt),rep(0,FullRowCnt))
	oRowIDs = order(scores,decreasing = TRUE)
	scores=scores[oRowIDs[1:TopCnt]]
	lbs=lbs[oRowIDs[1:TopCnt]]
	pred <- prediction( scores, lbs)
	perf <- performance(pred,"tpr","fpr")
	xs=as.numeric(unlist(perf@x.values)) #fpr
	ys=as.numeric(unlist(perf@y.values)) #tpr
	fdr=xs/(xs+ys*H1H0Ratio)
	maxTPR=max(ys[fdr<MAX_FDR],na.rm = TRUE)
	aucs[i]=maxTPR
	xs=fdr
	#perf <- performance(pred,"auc")
	#aucs[i]=perf@y.values
	lines(xs, ys, type="l", lwd=2, lty=linetype[i], col=colors[i], pch=plotchar[i])
	legendTxts[i]=paste("[",runInfos[i,"name"],"], TPR ",sprintf("%.2f",aucs[i]),sep="")
}
# add a legend 
legend(0.6,1.05, legend=legendTxts,
 cex=1, col=colors, pch=plotchar, lty=linetype, bty ="n")
dev.off()