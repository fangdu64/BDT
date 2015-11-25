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
maxK=__MAX_K__
minFraction=__MIN_FRACTION__

matrix_info_fn=paste(data_dir,"/matrix_info.txt",sep="")

matTbl=read.table(matrix_info_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

i=1
colCnt = matTbl[i,"ColCnt"]
rowCnt = matTbl[i,"RowCnt"]
dataFile = matTbl[i,"DataFile"]
mat= readBigMatrix(colCnt,rowCnt,dataFile)
eigenVals=mat[1,]
fractions=mat[2,]
##
## eigen values - fraction of variance explained
##
pdf(file = paste(out_dir,"/k_fractions_plot.pdf",sep=""))
#plot bars first
ks=c(1:maxK)
plot(ks,fractions[ks], type="h", xlab="k (# unwanted factors)",col="gray",lty=2,
     ylab="Percentage of increase", bty = "n",xaxt='n',ylim=c(0,1),xlim=c(0.8, maxK+0.2))

lines(ks, fractions[ks], type="o", lwd=3,
     lty=1, col="deepskyblue", pch=19)
axis(side=1, at=ks, labels=as.character(ks))
abline(h=minFraction, col="pink", lwd=1,lty=3)
dev.off()
