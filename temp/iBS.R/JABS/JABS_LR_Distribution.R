# CONSTANTS
# RSCRIPT_DIR	
# DATA_DIR	
# OUT_DIR	
# NUM_THREADS
#library(scales)
library(parallel)
library(latticeExtra)
rm(list=ls())
setwd("E:/iBS/trunk/core/src/iBS.R")
source("BDVD/Common.R")


data_dir="E:/iBS/trunk/analysis/iBS.Projects/JABS/Hg19/10CpG/GM12878/s03-lrdistribution"
out_dir="E:/iBS/trunk/analysis/iBS.Projects/JABS/Hg19/10CpG/GM12878/s01-amrfinder"
num_threads=4

matrix_info_fn=paste(data_dir,"/matrix_info.txt",sep="")
matTbl=read.table(matrix_info_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

matTbl[,2] = gsub("/dcs01/gcode/fdu1/projects/JABS/Hg19/10CpG/GM12878/s01-amrfinder/fcdcentral/FeatureValueStore",
	"E:/iBS/trunk/analysis/iBS.Projects/JABS/Hg19/10CpG/GM12878/s01-amrfinder", matTbl[,2])

bfvFilePrefix = matTbl[1,"DataFile"]
rowCnt = matTbl[1,"RowCnt"]
colCnt = matTbl[1,"ColCnt"]
h0=readBigMatrixAuto(colCnt,rowCnt,bfvFilePrefix)

bfvFilePrefix = matTbl[2,"DataFile"]
rowCnt = matTbl[2,"RowCnt"]
colCnt = matTbl[2,"ColCnt"]
h1=readBigMatrixAuto(colCnt,rowCnt,bfvFilePrefix)

lr=as.numeric(2*(h1-h0))
rm(h0,h1)
#plot(density(lr, na.rm=TRUE),xlim=c(0,30))



bfvFilePrefix = matTbl[3,"DataFile"]
rowCnt = matTbl[3,"RowCnt"]
colCnt = matTbl[3,"ColCnt"]
stats=readBigMatrixAuto(colCnt,rowCnt,bfvFilePrefix)
colnames(stats)=c("readCnt","cpgCnt","cpgPerRead","em_iter")
cpgCnts = as.integer(stats[,2])
#plot(density(stats[,"readCnt"],na.rm=TRUE))
rm(stats)
#hist(cpgCnts,breaks=0:10, xlab="number of CpGs", ylab="Frequency", main="CpG sites for sliding windows")

lr10=lr[cpgCnts==10]
plot(density(lr10,na.rm=TRUE),xlim=c(0,30),lwd=3, col="deepskyblue")

###############################################################
data_dir="E:/iBS/trunk/analysis/iBS.Projects/JABS/Hg19/10CpG/GM12878/s04-lrdistribution-h0simu"
matrix_info_fn=paste(data_dir,"/matrix_info.txt",sep="")
matTbl=read.table(matrix_info_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

matTbl[,2] = gsub("/dcs01/gcode/fdu1/projects/JABS/Hg19/10CpG/GM12878/s02-amrfinder-h0simu/fcdcentral/FeatureValueStore",
	"E:/iBS/trunk/analysis/iBS.Projects/JABS/Hg19/10CpG/GM12878/s02-amrfinder-h0simu", matTbl[,2])

bfvFilePrefix = matTbl[1,"DataFile"]
rowCnt = matTbl[1,"RowCnt"]
colCnt = matTbl[1,"ColCnt"]
h0=readBigMatrixAuto(colCnt,rowCnt,bfvFilePrefix)

bfvFilePrefix = matTbl[2,"DataFile"]
rowCnt = matTbl[2,"RowCnt"]
colCnt = matTbl[2,"ColCnt"]
h1=readBigMatrixAuto(colCnt,rowCnt,bfvFilePrefix)

lr_simu=as.numeric(2*(h1-h0))
rm(h0,h1)
#plot(density(lr_simu, na.rm=TRUE),xlim=c(0,30))


bfvFilePrefix = matTbl[3,"DataFile"]
rowCnt = matTbl[3,"RowCnt"]
colCnt = matTbl[3,"ColCnt"]
stats_simu=readBigMatrixAuto(colCnt,rowCnt,bfvFilePrefix)
cpgCnts_simu = as.integer(stats_simu[,2])
#plot(density(stats[,"readCnt"],na.rm=TRUE))
rm(stats_simu)
#hist(cpgCnts,breaks=0:10, xlab="number of CpGs", ylab="Frequency", main="CpG sites for sliding windows")

lr10_simu=lr_simu[cpgCnts_simu==10]
lines(density(lr10_simu,na.rm=TRUE),xlim=c(0,30),lwd=3, col="deepskyblue")