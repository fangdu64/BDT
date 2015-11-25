# CONSTANTS
# RSCRIPT_DIR	
library(scales)
setwd("E:/iBS/trunk/core/src/iBS.R/BDVD")


rm(list=ls())
source("Common.R")
out_dir="E:/iBS/trunk/analysis/iBS.Projects/BDVD/DukeUWDNase/100bp/ruvs-r1/s02-ruv-fstats"
load(paste(out_dir,"/fstats.rda",sep=""))
oidxs=getRUVConfigOrders(matTbl[,"k"],matTbl[,"extW"])
Fstats=Fstats[oidxs]
matTbl=matTbl[oidxs,]
names(Fstats)=getRUVConfigTexts(matTbl[,"k"],matTbl[,"extW"])
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