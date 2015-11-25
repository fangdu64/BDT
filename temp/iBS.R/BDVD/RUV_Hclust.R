# CONSTANTS
# RSCRIPT_DIR	
# DATA_DIR	
# OUT_DIR	
# NUM_THREADS
#library(scales)
library(parallel)
library(latticeExtra)
rm(list=ls())
setwd("__RSCRIPT_DIR__")
source("Common.R")

data_dir="__DATA_DIR__"
out_dir="__OUT_DIR__"
num_threads=__NUM_THREADS__
pdf_width=__PDF_WIDTH__
pdf_height=__PDF_HEIGHT__

colnames_fn=paste(data_dir,"/colnames.txt",sep="")
sampleNames=read.table(colnames_fn,stringsAsFactors =FALSE,
		sep="\t",header= FALSE)
sampleNames=sampleNames[,1]

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

unwantedGroupTbl=read.table(paste(data_dir,"/unwanted_groupinfo.txt",sep=""),
		sep="\t",header= TRUE)
		
matTbl=read.table(matrix_info_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

KsCnt=nrow(matTbl)
ClustScores= vector(mode="list", length=KsCnt)
for(i in 1:KsCnt){
	colCnt = matTbl[i,"ColCnt"]
	rowCnt = matTbl[i,"RowCnt"]
	dataFile = matTbl[i,"DataFile"]
	
	k = matTbl[i,"k"]
	extW=matTbl[i,"extW"]
	print(paste("k=",k,",extW=",extW,sep=""))
	mat=readBigMatrix(colCnt,rowCnt,dataFile)
	
	#compute correlation between columns (samples)
	A=cor(mat)
	colnames(A)=paste0(sampleNames,"_",unwantedGroupTbl[,"GroupName"],sep="")
	rownames(A)=colnames(A)
	d=(1 - A)/2
	#row distance
	rd=as.dist(d)
	cd=as.dist(t(d))
	
	hclust_rd=hclust(rd)
	mergeMemberFlags=getHClustMergeFlags(hclust_rd)
	cscores = rep(0,gcnt)
	for(j in 1:gcnt)
	{
		memberIDs=which(colGroups==j)
		nonmemberIDs=which(colGroups!=j)
		cscores[j]=getHClustScore(mergeMemberFlags,memberIDs,nonmemberIDs,method="strict")
	}
	ClustScores[[i]]=cscores
	
	dd.row <- as.dendrogram(hclust_rd)
	row.ord <- order.dendrogram(dd.row)
	dd.col <- as.dendrogram(hclust(cd))
	col.ord <- order.dendrogram(dd.col)
	
	pdf(file = paste(out_dir,"/hclust_corrmat_k",k,"_n",extW,".pdf",sep=""),
		width=pdf_width,height=pdf_height)
	print(levelplot(A[row.ord, col.ord],main="",
			  aspect = "fill",
			  scales = list(x = list(rot = 90,col=unwantedGroupTbl[col.ord,"GroupID"]), 
							y = list(col=unwantedGroupTbl[row.ord,"GroupID"])),
			  colorkey = list(space = "left"),
			  at=unique(c(seq(0, 0.2,length=4),
				seq(0.2, 0.8,length=100),
				seq(0.8, 1,length=4))),
			  col.regions = colorRampPalette(c("blue", "white", "red"))(1e3),
			  xlab="",
			  ylab="",
			  legend =
			  list(right =
				   list(fun = dendrogramGrob,
						args =
						list(x = dd.col, ord = col.ord,
							 side = "right")),
				   top =
				   list(fun = dendrogramGrob,
						args =
						list(x = dd.row, 
							 side = "top"))))
	)
	dev.off()
	
	pdf(file = paste(out_dir,"/hclust_labels_k",k,"_n",extW,".pdf",sep=""),
		width=pdf_width, height=7)
	plot(hclust_rd, hang = -1)
	dev.off()

}

sink(paste(out_dir,"/hclust_scores.txt",sep=""))
ClustScores=lapply(ClustScores,unlist,use.names = FALSE)
oidxs=getRUVConfigOrders(matTbl[,"k"],matTbl[,"extW"])
ClustScores=ClustScores[oidxs]
names(ClustScores)=getRUVConfigTexts(matTbl[,"k"],matTbl[,"extW"])
ClustScores.mean=lapply(ClustScores,mean,na.rm = TRUE)
ClustScores.correctNum=lapply(ClustScores,function(x){
	sum(x==1)})
print(ClustScores)
print(ClustScores.mean)
print(ClustScores.correctNum)
sink()
