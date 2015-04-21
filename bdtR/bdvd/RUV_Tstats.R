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
num_threads=__NUM_THREADS__

colnames_fn=paste(data_dir,"/colnames.txt",sep="")
sampleNames=read.table(colnames_fn,stringsAsFactors =FALSE,
		sep="\t",header= FALSE)
sampleNames=sampleNames[,1]

matrix_info_fn=paste(data_dir,"/matrix_info.txt",sep="")
groupinfo_fn=paste(data_dir,"/groupinfo.txt",sep="")
groupTbl=read.table(groupinfo_fn,
		sep="\t",header= TRUE)
groupCnt= nrow(groupTbl)
groupPair1=vector(mode="list", length=groupCnt)
groupPair2=vector(mode="list", length=groupCnt)
groupNames=rep("",groupCnt)
for(i in 1:groupCnt)
{
	groupPair1[[i]]=groupTbl[i,"g1ColIDFrom"]:groupTbl[i,"g1ColIDTo"]
	groupPair2[[i]]=groupTbl[i,"g2ColIDFrom"]:groupTbl[i,"g2ColIDTo"]
	groupNames[i]=sampleNames[groupTbl[i,"g1ColIDFrom"]]
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
	mat= as.list(data.frame(t(mat)))
	Tstats[[i]]=mclapply(mat,rowMultiTTest,groupPair1,groupPair2,mc.cores=num_threads)
}

oidxs=getRUVConfigOrders(matTbl[,"k"],matTbl[,"extW"])
Tstats=Tstats[oidxs]
matTbl=matTbl[oidxs,]
names(Tstats)=getRUVConfigTexts(matTbl[,"k"],matTbl[,"extW"])

#save(Tstats, matTbl, groupNames,groupPair1, groupPair2, file = paste(out_dir,"/tstats.rda",sep=""))
#rm(list=ls())
#out_dir="__OUT_DIR__"
#load(paste(out_dir,"/tstats.rda",sep=""))
#KsCnt=length(Tstats)
#groupCnt=length(groupPair1)

for(g in 1:groupCnt)
{
	##
	## T-stats box plot
	##
	T=lapply(Tstats,function(Tk,g) {
		unlist(
			lapply(Tk,function(Tki,g){
				Tki$tstats[g]
				},g=g),
			use.names = FALSE)
	},g=g)
	pdf(file = paste(out_dir,"/t_boxplot_g",g,".pdf",sep=""))
	plotdata <- boxplot(T, ylab = "T-statistic", outline = FALSE, main=groupNames[g])
	abline(h=0, col="pink", lwd=1)
	dev.off()
	
	##
	## MSE  plot
	##

	T.MSE=lapply(T,function(x){
		sum(x^2,na.rm = TRUE)/sum(!is.na(x))
		})
	pdf(file = paste(out_dir,"/t_mse_plot_g",g,".pdf",sep=""))
	#plot bars first
	colIdxs=c(1:KsCnt)
	plot(colIdxs,T.MSE[colIdxs], type="h", xlab="",col="gray",lty=2,
		 ylab="T-stats MSE", bty = "n",xaxt='n',xlim=c(0.8, KsCnt+0.2),main=groupNames[g])
	colIdxs=c(1:KsCnt)
	lines(colIdxs, T.MSE[colIdxs], type="o", lwd=2,
		 lty=1, col="deepskyblue", pch=19)
	axis(side=1, at=colIdxs, labels=names(T)[colIdxs])
	#axis(side=2, at=c(1.4,1.5,1.6))
	#abline(h=means[2], col="pink", lwd=1,lty=3)
	dev.off()
	
	T.MAE=lapply(T,function(x){
		sum(abs(x),na.rm = TRUE)/sum(!is.na(x))
		})
	pdf(file = paste(out_dir,"/t_mae_plot_g",g,".pdf",sep=""))
	#plot bars first
	colIdxs=c(1:KsCnt)
	plot(colIdxs,T.MAE[colIdxs], type="h", xlab="",col="gray",lty=2,
		 ylab="T-stats MAE", bty = "n",xaxt='n',xlim=c(0.8, KsCnt+0.2),main=groupNames[g])
	colIdxs=c(1:KsCnt)
	lines(colIdxs, T.MAE[colIdxs], type="o", lwd=2,
		 lty=1, col="deepskyblue", pch=19)
	axis(side=1, at=colIdxs, labels=names(T)[colIdxs])
	#axis(side=2, at=c(1.4,1.5,1.6))
	#abline(h=means[2], col="pink", lwd=1,lty=3)
	dev.off()
}

##
## Median MAE against different Ks
##



runIDs=c(1,4,2,3)
print(runIDs)
for(i in 1:(length(runIDs)-1))
{
	for(j in (i+1):length(runIDs))
	{
		run_A=runIDs[i]
		run_B=runIDs[j]
		name_A=names(Tstats)[run_A]
		name_B=names(Tstats)[run_B]
		
		TMAEs_A=rep(0,groupCnt)
		TMAEs_B=rep(0,groupCnt)

		for(g in 1:groupCnt)
		{
			T=lapply(Tstats,function(Tk,g) {
				unlist(
					lapply(Tk,function(Tki,g){
						Tki$tstats[g]
						},g=g),
					use.names = FALSE)
			},g=g)

			T.MAE=lapply(T,function(x){
				sum(abs(x),na.rm = TRUE)/sum(!is.na(x))
				})
			TMAEs_A[g]=T.MAE[[run_A]]
			TMAEs_B[g]=T.MAE[[run_B]]
		}
		print(TMAEs_A)
		print(TMAEs_B)
		max_v=max(c(TMAEs_A,TMAEs_B))*1.1
		min_v=min(c(TMAEs_A,TMAEs_B))*1.1
		lessCnt=sum(TMAEs_B<TMAEs_A)
		btest=binom.test(lessCnt, length(TMAEs_A), p = 0.5, alternative="two.sided")
		pVal=btest$p.value
		pdf(file = paste(out_dir,"/t_mae_x_",name_A,"_y_",name_B,".pdf",sep=""))
		plot(c(min_v,max_v), c(min_v,max_v), type="n", xlab=name_A, ylab=name_B,
			main=paste("p-val ",pVal,sep=""))
		abline(a=0,b=1, col="gray", lwd=1, lty = 2)
		points(TMAEs_A,TMAEs_B,col=adjustcolor("deepskyblue",1),pch=20,cex=2.5)
		dev.off()
	}
}


