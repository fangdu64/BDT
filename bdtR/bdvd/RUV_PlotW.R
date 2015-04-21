# CONSTANTS
# RSCRIPT_DIR	
# DATA_DIR	
# OUT_DIR	
#library(scales)
rm(list=ls())
setwd("__RSCRIPT_DIR__")
source("Common.R")

data_dir="__DATA_DIR__"
out_dir="__OUT_DIR__"

colnames_fn=paste(data_dir,"/colnames.txt",sep="")
matrix_info_fn=paste(data_dir,"/matrix_info.txt",sep="")

tbl = read.table(colnames_fn,stringsAsFactors =FALSE,
		sep="\t",header= FALSE)
colNames=tbl[,1]
colGroupIDs=tbl[,2]

matTbl=read.table(matrix_info_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)


i=1
colCnt = matTbl[i,"ColCnt"]
rowCnt = matTbl[i,"RowCnt"]
dataFile = matTbl[i,"DataFile"]
Wt= readBigMatrix(colCnt,rowCnt,dataFile)
print(c(ncol(Wt),nrow(Wt)))
#colnames(Wt)=colNames
colnames(Wt)=as.character(1:colCnt)
##
## Wt barplot
##
minY0=min(Wt[1:3,])
maxY0=max(Wt[1:3,])
minY=minY0-0.1*(maxY0-minY0)
maxY=maxY0+0.1*(maxY0-minY0)

pdf(file = paste(out_dir,"/wt_barplot.pdf",sep=""),width=colCnt*0.1,height=36)
par(mfrow=c(rowCnt,1),mar=c(2,2,2,1),oma=c(1,0,0,0))
for(i in 1:rowCnt)
{
	barplot(Wt[i,],col=colGroupIDs, border=NA,xlab=NULL,ylab=NULL,ylim=c(minY,maxY),las = 3)
	abline(h=0, col="gray", lwd=1)
}
dev.off()


colnames(Wt)=NULL

for(i in 1:rowCnt)
{
	pdf(file = paste(out_dir,"/wt_",i,".pdf",sep=""),width=16,height=2)
	par(mfrow=c(1,1),mar=c(0,0,0,0),oma=c(0,0,0,0))
	barplot(Wt[i,],col=colGroupIDs, border=NA,xlab=NULL,ylab=NULL, ylim=c(minY,maxY), yaxt='n', las = 3)
	abline(h=0, col="gray", lwd=1)
	dev.off()
}
