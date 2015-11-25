# CONSTANTS
# RSCRIPT_DIR	
# DATA_DIR	
# OUT_DIR	
# NUM_THREADS
#library(scales)
library(parallel)
rm(list=ls())
setwd("__RSCRIPT_DIR__")
source("Common.R")

data_dir="__DATA_DIR__"
out_dir="__OUT_DIR__"
num_threads=__NUM_THREADS__

colnames_fn=paste(data_dir,"/colnames.txt",sep="")

matrix_info_fn=paste(data_dir,"/matrix_info.txt",sep="")
groupinfo_fn=paste(data_dir,"/groupinfo.txt",sep="")

groupTbl=read.table(groupinfo_fn,
		sep="\t",header= TRUE)

colGroups=do.call(c,
	apply(groupTbl,1, function(x) {
	rep(x[1],x[3]-x[2]+1)
	})
	)
colGroups=as.vector(colGroups)
colGroups=as.factor(colGroups)
gcnt=nlevels(colGroups)


matTbl=read.table(matrix_info_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

KsCnt=nrow(matTbl)
Fstats= vector(mode="list", length=KsCnt)

names(Fstats)=apply(matTbl,1, function(x) {
	paste("k=",x["k"],sep="")
	})

for(i in 1:KsCnt){
	colCnt = matTbl[i,"ColCnt"]
	rowCnt = matTbl[i,"RowCnt"]
	dataFile = matTbl[i,"DataFile"]
	
	#dataFile = paste(data_dir,"/subtask_export/gid_",10000+i,".bfv",sep="")
	k = matTbl[i,"k"]
	extW=matTbl[i,"extW"]
	print(paste("k=",k,",extW=",extW,sep=""))
	#mat= readBigMatrix(colCnt,rowCnt,dataFile)
	mat= as.list(data.frame(t(readBigMatrix(colCnt,rowCnt,dataFile))))
	#Fstats[[i]]=apply(mat,1, function(x) {
	Fstats[[i]]=mclapply(mat,function(x) {
		n.i=tapply(x,colGroups,length)
		m.i=tapply(x,colGroups,mean)
		v.i=tapply(x,colGroups,var)
		n = sum(n.i)
		wgVar=(sum((n.i - 1) * v.i)/(n - gcnt))
		if(wgVar<0.000001)
		{
			return(NA)
		}
        f_stats = ((sum(n.i * (m.i - mean(x))^2)/(gcnt - 1))/wgVar)
		return (f_stats)
	},mc.cores=num_threads)
	
	#Fstats[[i]]=Fstats[[i]][Fstats[[i]]>=0]
}
Fstats=lapply(Fstats,unlist,use.names = FALSE)

oidxs=getRUVConfigOrders(matTbl[,"k"],matTbl[,"extW"])
Fstats=Fstats[oidxs]
matTbl=matTbl[oidxs,]
names(Fstats)=getRUVConfigTexts(matTbl[,"k"],matTbl[,"extW"])
save(Fstats, matTbl, file = paste(out_dir,"/fstats.rda",sep=""))

rm(list=ls())
out_dir="__OUT_DIR__"
load(paste(out_dir,"/fstats.rda",sep=""))

Fstats.mean=lapply(Fstats,mean,na.rm = TRUE)
Fstats.median=lapply(Fstats,median,na.rm = TRUE)
KsCnt=length(Fstats)
##
## F-stats box plot
##

#dev.new()
pdf(file = paste(out_dir,"/f_boxplot.pdf",sep=""))
#plotdata <- boxplot(Fstats, ylab = "F-statistic", outline = FALSE, na.action =na.omit )
plotdata <- boxplot(Fstats, ylab = "F-statistic", outline = FALSE)
dev.off()

##
## F-stats mean plot
##

#dev.new()
pdf(file = paste(out_dir,"/f_means_plot.pdf",sep=""))
#plot bars first
colIdxs=c(1:KsCnt)
plot(colIdxs,Fstats.mean[colIdxs], type="h", xlab="",col="gray",lty=2,
     ylab="F-stats Mean", bty = "n",xaxt='n',xlim=c(0.8, KsCnt+0.2))
colIdxs=c(1:KsCnt)
lines(colIdxs, Fstats.mean[colIdxs], type="o", lwd=2,
     lty=1, col="deepskyblue", pch=19)
axis(side=1, at=colIdxs, labels=names(Fstats)[colIdxs])
#axis(side=2, at=c(1.4,1.5,1.6))
#abline(h=means[2], col="pink", lwd=1,lty=3)
dev.off()

##
## F-stats median plot
##

#dev.new()
pdf(file = paste(out_dir,"/f_medians_plot.pdf",sep=""))
#plot bars first
colIdxs=c(1:KsCnt)
plot(colIdxs,Fstats.median[colIdxs], type="h", xlab="",col="gray",lty=2,
     ylab="F-stats Median", bty = "n",xaxt='n',xlim=c(0.8, KsCnt+0.2))
colIdxs=c(1:KsCnt)
lines(colIdxs, Fstats.median[colIdxs], type="o", lwd=2,
     lty=1, col="deepskyblue", pch=19)
axis(side=1, at=colIdxs, labels=names(Fstats)[colIdxs])
#axis(side=2, at=c(1.4,1.5,1.6))
#abline(h=means[2], col="pink", lwd=1,lty=3)
dev.off()