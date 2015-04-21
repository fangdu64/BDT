library(parallel)
library(latticeExtra)
rm(list=ls())
setwd("__RSCRIPT_DIR__")
source("Common.R")
source("BigClust_Plot_KMeansResult_Config.R")

data_dir="__DATA_DIR__"
out_dir="__OUT_DIR__"
distance="__DISTANCE__"

colnames_fn=paste(data_dir,"/kcentroids_colnames.txt",sep="")
colNames=read.table(colnames_fn,stringsAsFactors =FALSE,
		sep="\t",header= FALSE)
colNames=colNames[,1]

matrix_info_fn=paste(data_dir,"/kcentroids_mat.txt",sep="")
matTbl=read.table(matrix_info_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)
kcnts_mat_fn=paste(data_dir,"/kcnts_mat.txt",sep="")
kcntsTbl=read.table(kcnts_mat_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

##
## hclust for K-Centroids
##
K=0
if(TRUE){
	i=1
	colCnt = matTbl[i,"ColCnt"]
	rowCnt = matTbl[i,"RowCnt"]
	K=rowCnt
	dataFile = matTbl[i,"DataFile"]
	kcntsVec=readBigMatrixInt(1,rowCnt,kcntsTbl[i,"DataFile"])
	A=readBigMatrix(colCnt,rowCnt,dataFile)
	colnames(A)=colNames
	#rownames(A)=paste0("(",c(1:rowCnt),",",kcntsVec[,1],")",sep="")
	rownames(A)=paste0(kcntsVec[,1],sep="")
	if(distance=="KMeansDistCorrelation")
	{
		cord=(1 - cor(t(A)))/2
		cord[is.na(cord)]=0
		rd=as.dist(cord)
	}
	else{
		rd = dist(A)
	}
	
	dd.row <- as.dendrogram(hclust(rd))
	row.ord <- order.dendrogram(dd.row)
	
	if(distance=="KMeansDistCorrelation")
	{
		cord=(1 - cor(A))/2
		cord[is.na(cord)]=0
		cd=as.dist(cord)
	}
	else{
		cd = dist(t(A))
	}
	dd.col <- as.dendrogram(hclust(cd))
	col.ord <- order.dendrogram(dd.col)
	
	A=A[row.ord,col.ord]
	A=t(A)
	
	pdf(file = paste(out_dir,"/kcentroids_hclust_k",K,".pdf",sep=""),
		width=KCentroids_Plot_Width,height=KCentroids_Plot_Height)
	print(levelplot(A,main="K-Centroids",
			  scales = list(x = list(rot = 90)),
			  colorkey = list(space = "left"),
			  at=HeatmapColor.at,
			  col.regions = HeatmapColor.regions,
			  xlab="",
			  ylab="",
			  legend =
			  list(right =
				   list(fun = dendrogramGrob,
						args =
						list(x = dd.row,
							 side = "right")),
				   top =
				   list(fun = dendrogramGrob,
						args =
						list(x = dd.col, ord = col.ord,
							 side = "top"))))
	)
	dev.off()
}



##
## hclust for K-seeds
##
matrix_info_fn=paste(data_dir,"/kseeds_mat.txt",sep="")
matTbl=read.table(matrix_info_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)
if(TRUE){
	i=1
	colCnt = matTbl[i,"ColCnt"]
	rowCnt = matTbl[i,"RowCnt"]
	dataFile = matTbl[i,"DataFile"]
	A=readBigMatrix(colCnt,rowCnt,dataFile)
	colnames(A)=colNames
	rownames(A)=as.character(c(1:rowCnt))
	if(distance=="KMeansDistCorrelation")
	{
		cord=(1 - cor(t(A)))/2
		cord[is.na(cord)]=0
		rd=as.dist(cord)
	}
	else{
		rd = dist(A)
	}
	dd.row <- as.dendrogram(hclust(rd))
	row.ord <- order.dendrogram(dd.row)
	if(distance=="KMeansDistCorrelation")
	{
		cord=(1 - cor(A))/2
		cord[is.na(cord)]=0
		cd=as.dist(cord)
	}
	else{
		cd = dist(t(A))
	}
	dd.col <- as.dendrogram(hclust(cd))
	col.ord <- order.dendrogram(dd.col)
	
	A=A[row.ord,col.ord]
	A=t(A)
	
	pdf(file = paste(out_dir,"/kseeds_hclust_k",K,".pdf",sep=""),
		width=KSeeds_Plot_Width,height=KSeeds_Plot_Height)
	print(levelplot(A,main="K-Seeds",
			  scales = list(x = list(rot = 90)),
			  colorkey = list(space = "left"),
			  at= HeatmapColor.at,
			  col.regions = HeatmapColor.regions,
			  xlab="",
			  ylab="",
			  legend =
			  list(right =
				   list(fun = dendrogramGrob,
						args =
						list(x = dd.row,
							 side = "right")),
				   top =
				   list(fun = dendrogramGrob,
						args =
						list(x = dd.col, ord = col.ord,
							 side = "top"))))
	)
	dev.off()
	
	A=readBigMatrix(colCnt,rowCnt,dataFile)
	colnames(A)=colNames
	rownames(A)=as.character(c(1:rowCnt))
	row.ord = dim(A)[1]:1
	col.ord = 1:dim(A)[2]
	A=A[row.ord,col.ord]
	A=t(A)

	pdf(file = paste(out_dir,"/kseeds_mat_k",K,".pdf",sep=""),
		width=KSeeds_Plot_Width,height=KSeeds_Plot_Height)
	print(
	levelplot(A,
			scales = list(x = list(rot = 90)),
			colorkey = list(space = "left"),
			at=HeatmapColor.at,
			col.regions = HeatmapColor.regions,
			xlab="",
			ylab=""
			)
		)
	dev.off()
}
