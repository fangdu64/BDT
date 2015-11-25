library(parallel)
library(latticeExtra)
rm(list=ls())
setwd("__RSCRIPT_DIR__")
source("Common.R")

PDN_Fk<-function(Ks, Ss, Nd)
{
	f=rep(0,length(Ks))
	a=rep(0,length(Ks))
	a[2]=1-3/(4*Nd)
	f[1]=1
	for(k in 2:length(Ks))
	{
		if(k>2)
		{
			a[k]=a[k-1]+(1-a[k-1])/6
		}
		if(Ss[k-1]==0)
		{
			f[k]=1
		}
		else
		{
			f[k]=Ss[k]/(a[k]*Ss[k-1])
		}
	}
	
	return (f)
}

data_dir="__DATA_DIR__"
out_dir="__OUT_DIR__"
pdf_width=20
pdf_height=7

results_mats_fn=paste(data_dir,"/results_mats.txt",sep="")
matTbl=read.table(results_mats_fn,stringsAsFactors =FALSE,
		sep="\t",header= TRUE)

Ks=matTbl[,"K"]
Ss=matTbl[,"Distorsion"]
Nd=matTbl[1,"ColCnt"]
fk=PDN_Fk(Ks,Ss,Nd)
K_best=Ks[which.min(fk)]
vexp=matTbl[,"Explained"]
#names(fk)=as.character(Ks)
##
## eigen values - fraction of variance explained
##
pdf(file = paste(out_dir,"/fks_plot.pdf",sep=""))
#plot bars first
plot(Ks,fk, type="h", xlab="k",col="gray",lty=2,
     ylab="F(k)", bty = "n",xaxt='n',main=paste(" Optimal K =",K_best))

lines(Ks, fk, type="o", lwd=2,
     lty=1, col="deepskyblue", pch=19)
axis(side=1, at=Ks, labels=as.character(Ks))
abline(h=0.85, col="pink", lwd=1,lty=3)
dev.off()


##
## fraction of variance explained
##
pdf(file = paste(out_dir,"/explained_plot.pdf",sep=""))
#plot bars first
plot(Ks,vexp, type="h", xlab="k",col="gray",lty=2,
     ylab="Variance Explained", bty = "n",xaxt='n',ylim=c(0,1))

lines(Ks, vexp, type="o", lwd=2,
     lty=1, col="deepskyblue", pch=19)
axis(side=1, at=Ks, labels=as.character(Ks))
dev.off()
