# CONSTANTS
# RSCRIPT_DIR	
# DATA_DIR	
# OUT_DIR	
#library(scales)
rm(list=ls())
library(latticeExtra)
setwd("__RSCRIPT_DIR__")
source("Common.R")

data_dir="__DATA_DIR__"
out_dir="__OUT_DIR__"
subColIDs=c(__ColIDs__)

colnames_fn=paste(data_dir,"/colnames.txt",sep="")
matrix_info_fn=paste(data_dir,"/model_mats.txt",sep="")

tbl = read.table(colnames_fn,stringsAsFactors =FALSE,
		sep="\t",header= FALSE)
colNames=tbl[,1]
colGroupIDs=tbl[,2]

matTbl=read.table(matrix_info_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

for(i in 1:nrow(matTbl))
{
	colCnt = matTbl[i,"ColCnt"]
	rowCnt = matTbl[i,"RowCnt"]
	dataFile = matTbl[i,"DataFile"]
	matName = matTbl[i,"MatName"]
	mat= readBigMatrix(colCnt,rowCnt,dataFile)
	colnames(mat)=colNames
	A=mat[,subColIDs]
	print(sum(A))
	row.ord = dim(A)[1]:1
	col.ord = 1:dim(A)[2]
	A=A[row.ord,col.ord]
	A=t(A)

	pdf(file = paste(out_dir,"/",matName,"_plot.pdf",sep=""),width=16,height=7)
	print(
	levelplot(A,
			scales = list(x = list(draw=F),y = list(draw=F)),
			main=NA,
			at=unique(c(seq(-2, -1,length=4),
				seq(-1, 8,length=100),
				seq(8, 10,length=4))),
			colorkey=FALSE,
			col.regions = colorRampPalette(c("blue", "white", "red"))(1e3),
			xlab="",
			ylab=""
			)
		)
	dev.off()
	
	if(matName=="Xb")
	{
		A=mat[,subColIDs]
	
		BL=matrix(0,nrow(A),ncol(A))
		for(i in 1:nrow(A))
		{
			BL[i,]=mean(A[i,])
		}
		WL=A-BL
		
		row.ord = dim(BL)[1]:1
		col.ord = 1:dim(BL)[2]
		BL=BL[row.ord,col.ord]
		BL=t(BL)
	
		
		pdf(file = paste(out_dir,"/",matName,"_BL_plot.pdf",sep=""),width=16,height=7)
		print(
		levelplot(BL,
				scales = list(x = list(draw=F),y = list(draw=F)),
				main=NA,
				at=unique(c(seq(-2, -1,length=4),
					seq(-1, 8,length=100),
					seq(8, 10,length=4))),
				colorkey=FALSE,
				col.regions = colorRampPalette(c("blue", "white", "red"))(1e3),
				xlab="",
				ylab=""
				)
			)
		dev.off()
		
		row.ord = dim(WL)[1]:1
		col.ord = 1:dim(WL)[2]
		WL=WL[row.ord,col.ord]
		WL=t(WL)
		pdf(file = paste(out_dir,"/",matName,"_WL_plot.pdf",sep=""),width=16,height=7)
		print(
		levelplot(WL,
				scales = list(x = list(draw=F),y = list(draw=F)),
				main=NA,
				at=unique(c(seq(-2, -1,length=4),
					seq(-1, 8,length=100),
					seq(8, 10,length=4))),
				colorkey=FALSE,
				col.regions = colorRampPalette(c("blue", "white", "red"))(1e3),
				xlab="",
				ylab=""
				)
			)
		dev.off()
	}
}

