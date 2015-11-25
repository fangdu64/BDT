# CONSTANTS
# RSCRIPT_DIR	
# DATA_DIR	
# OUT_DIR	
#library(scales)
library(parallel)
rm(list=ls())
setwd("__RSCRIPT_DIR__")
source("Common.R")

data_dir="__DATA_DIR__"
out_dir="__OUT_DIR__"

colnames_fn=paste(data_dir,"/colnames.txt",sep="")

matrix_info_fn=paste(data_dir,"/matrix_info.txt",sep="")
groupinfo_fn=paste(data_dir,"/groupinfo.txt",sep="")

groupTbl=read.table(groupinfo_fn,
		sep="\t",header= TRUE)
groupCnt= nrow(groupTbl)
groupPair1=vector(mode="list", length=groupCnt)
groupPair2=vector(mode="list", length=groupCnt)
for(i in 1:groupCnt)
{
	groupPair1[[i]]=groupTbl[i,"g1ColIDFrom"]:groupTbl[i,"g1ColIDTo"]
	groupPair2[[i]]=groupTbl[i,"g2ColIDFrom"]:groupTbl[i,"g2ColIDTo"]
}

matTbl=read.table(matrix_info_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

KsCnt=nrow(matTbl)
Tstats= vector(mode="list", length=KsCnt)

names(Tstats)=apply(matTbl,1, function(x) {
	paste("k=",x["k"],sep="")
	})

for(i in 1:KsCnt){
	colCnt = matTbl[i,"ColCnt"]
	rowCnt = matTbl[i,"RowCnt"]
	dataFile = matTbl[i,"DataFile"]
	k = matTbl[i,"k"]
	extW=matTbl[i,"extW"]
	print(paste("k=",k,",extW=",extW,sep=""))
	mat= readBigMatrix(colCnt,rowCnt,dataFile)
	mat_ts=matrix(0, nrow=nrow(mat),ncol=groupCnt)
	mat_pvals=matrix(0, nrow=nrow(mat),ncol=groupCnt)
	
	for(j in 1:nrow(mat))
	{
		for(g in 1:groupCnt)
		{
			g1Vals=mat[j,groupPair1[[g]]]
			g2Vals=mat[j,groupPair2[[g]]]
			if((max(g1Vals)==min(g1Vals)) && (max(g2Vals)==min(g2Vals)))
			{
				mat_ts[j,g]=NA
				mat_pvals[j,g]=NA
			}
			else
			{
				tt=t.test(g1Vals,g2Vals)
				mat_ts[j,g]=tt$statistic
				mat_pvals[j,g]=tt$p.value
			}
		}
	}
	
	Tstats[[i]]=list(mat_ts=mat_ts,mat_pvals=mat_pvals)

}

save(Tstats, matTbl, groupPair1, groupPair2, file = paste(out_dir,"/tstats.rda",sep=""))

rm(list=ls())
out_dir="__OUT_DIR__"
load(paste(out_dir,"/tstats.rda",sep=""))
KsCnt=length(Tstats)
groupCnt=length(groupPair1)
##
## T-stats box plot
##

for(i in 1:groupCnt)
{
	pdf(file = paste(out_dir,"/t_boxplot_g",i,".pdf",sep=""))
	T=do.call(cbind,
		lapply(Tstats,function(x) {
			x$mat_ts[,i]
		})
	)
	plotdata <- boxplot(T, ylab = "T-statistic", outline = FALSE)
	abline(h=0, col="pink", lwd=1)
	dev.off()
}
