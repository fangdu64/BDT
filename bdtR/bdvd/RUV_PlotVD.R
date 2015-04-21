# CONSTANTS
# RSCRIPT_DIR	
# DATA_DIR	
# OUT_DIR	
#library(scales)

rm(list=ls())
#setwd("__RSCRIPT_DIR__")
setwd("E:/iBS/trunk/core/src/iBS.R/BDVD")
source("Common.R")

data_dir="__DATA_DIR__"
out_dir="__OUT_DIR__"

#data_dir="E:/iBS/trunk/analysis/iBS.Projects/BDVD/DukeUWExon/s05-vd"
#data_dir="E:/iBS/trunk/analysis/iBS.Projects/BDVD/DukeUWDNase/100bp/ruvs-r2/s05-vd"
#data_dir="E:/iBS/trunk/analysis/iBS.Projects/BDVD/DukeUWDNase/100bp/ruvs-var-r2/s05-vd"
#data_dir="E:/iBS/trunk/analysis/iBS.Projects/BDVD/UWDNaseMM9/100bp/s05-vd"
#data_dir="E:/iBS/trunk/analysis/iBS.Projects/BDVD/Histone/100bp/Broad194/s05-vd"
#data_dir="E:/iBS/trunk/analysis/iBS.Projects/BDVD/Histone/100bp/BroadUWH3K4me3/s05-vd"
#data_dir="E:/iBS/trunk/analysis/iBS.Projects/BDVD/Histone/100bp/BroadH3K4me1/s05-vd"
data_dir="E:/iBS/trunk/analysis/iBS.Projects/BDVD/Histone/100bp/BroadH3K27ac/s05-vd"

out_dir=data_dir

vd_fn=paste(data_dir,"/vd.txt",sep="")

vdTbl=read.table(vd_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)


KsCnt=nrow(vdTbl)

bu=rep(0,KsCnt)
b=rep(0,KsCnt)
b_bl=rep(0,KsCnt)
b_wl=rep(0,KsCnt)

for(i in 1:KsCnt){
	k = vdTbl[i,"k"]
	extW=vdTbl[i,"extW"]
	SS_t = vdTbl[i,"SS_t"]
	SS_r =vdTbl[i,"SS_r"]
	SS_bu = vdTbl[i,"SS_bu"]
	SS_b = vdTbl[i,"SS_b"]
	SS_u = vdTbl[i,"SS_u"]
	
	b_SS_t = vdTbl[i,"b_SS_t"]
	b_SS_bl = vdTbl[i,"b_SS_bl"]
	b_SS_wl = vdTbl[i,"b_SS_wl"]
	
	
	# Biological + Unwanted 
	bu[i] = SS_bu/SS_t
	b[i] = SS_b/SS_t
	b_bl[i] = b[i]*(1-b_SS_bl/b_SS_t)
	b_wl[i] = b[i]*(1-b_SS_wl/b_SS_t)
}

oidxs=getRUVConfigOrders(vdTbl[,"k"],vdTbl[,"extW"])
bu=bu[oidxs]
b=b[oidxs]
b_bl=b_bl[oidxs]
b_wl=b_wl[oidxs]
vdTbl=vdTbl[oidxs,]
runNames=getRUVConfigTexts(vdTbl[,"k"],vdTbl[,"extW"])

colIdxs=c(1:KsCnt)
propMat=cbind(b_bl,b-b_bl,bu-b,1-bu)
colnames(propMat)=c("b_wl","b_bl","u","e")
rownames(propMat)=runNames[colIdxs]



##
## eigen values - fraction of variance explained
##
pdf(file = paste(out_dir,"/vd_plot.pdf",sep=""))
#plot bars first
colIdxs=c(1:KsCnt)
plot(colIdxs,bu[colIdxs], type="h", xlab="",col="gray",lty=2,
     ylab="Proportion", bty = "n",xaxt='n',xlim=c(0.8, KsCnt+0.2),ylim=c(0, 1))
lines(colIdxs, bu[colIdxs], type="o", lwd=2,
     lty=1, col="deepskyblue", pch=19)

lines(colIdxs, b[colIdxs], type="o", lwd=2,
     lty=1, col="green", pch=19)

lines(colIdxs, b_bl[colIdxs], type="o", lwd=2,
     lty=1, col="blue", pch=19)

axis(side=1, at=colIdxs, labels=runNames[colIdxs])
dev.off()


##
## stacked barplot
##
pdf(file = paste(out_dir,"/vd_stackbarplot.pdf",sep=""),width=5,height=4)
#plot bars first
colIdxs=c(1:KsCnt)
barplot(t(propMat),col=c("#4dac26","#b8e186","#e7298a","#f4cae4"),
	xlab="k (# unwanted factors)", ylab="Proportion")
dev.off()

##
## plot unwanted fraction as a function of K
##
learntKidxs=2:11
uFractions=rep(0,length(learntKidxs))
for(i in learntKidxs)
{
	uFractions[i]=(propMat[i,"u"]-propMat[i-1,"u"])/propMat[i,"u"]
}

pdf(file = paste(out_dir,"/uwanted_fractions.pdf",sep=""))
#plot bars first
ks=vdTbl[learntKidxs,"k"]
maxK=max(ks)
plot(ks,uFractions[learntKidxs], type="h", xlab="k (# unwanted factors)",col="gray",lty=2,
     ylab="Percentage of increase", bty = "n",xaxt='n',ylim=c(0,1),xlim=c(0.8, maxK+0.2))

lines(ks, uFractions[learntKidxs], type="o", lwd=3,
     lty=1, col="deepskyblue", pch=19)
axis(side=1, at=ks, labels=as.character(ks))
abline(h=0.15, col="pink", lwd=1,lty=3)
dev.off()