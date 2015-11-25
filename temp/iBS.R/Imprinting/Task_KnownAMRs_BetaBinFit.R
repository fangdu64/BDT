# CONSTANTS
# RSCRIPT_DIR	
# EV_TABLE_FILE	E:/BDT/install/evtbl2subset_out/chr7_1_159138663.tsv
# BetaBin_outterWinSize	5000
# BetaBin_innerWinSize	1000
# OUT_DIR	
# REF	chr7
# MIN_ROWCNT_FOR_EVTBL	50

rm(list=ls())
setwd("__RSCRIPT_DIR__")

#bigTbl=read.table("E:/BDT/install/evtbl2subset_out/chr7_1_159138663.tsv",
bigTbl=read.table("__EV_TABLE_FILE__",
	sep="\t",comment.char = "",
	colClasses= c("factor","integer","integer","character","integer","integer","integer", "factor","factor","integer","integer","integer","integer","integer"), quote = "", # ' in Qual1 will mess up the parsing
	)
colnames(bigTbl)=c("RefName","RefOffset","ReadID","Allele","IsWaston","IsForward","SamFlag","Qual1","Qual2","SeqCycle","AlLen","AlScore","MAPQ","MateID")

source("BetaBinFit.R")
source("ReadKnownImprinting.R")

getSubEvTbl<-function(posTo_inner, posTo_outer, lastRowID, subTbl)
{
	bigTblRowCnt = nrow(subTbl)
	rowID = lastRowID
	rowIDFrom = lastRowID +1
	posTo1_reached=FALSE
	while(rowID<bigTblRowCnt)
	{
		rowID = rowID+1
		RefOffset = subTbl[rowID,"RefOffset"]
		if((!posTo1_reached) && (RefOffset>posTo_inner))
		{
			lastRowID = rowID-1
			posTo1_reached = TRUE
		}
		if(RefOffset>posTo_outer)
		{
			rowID = rowID-1
			break
		}
	}
	rowIDTo= rowID

	ret=list(evTbl = subTbl[rowIDFrom:rowIDTo,], lastRowID = lastRowID)
	return (ret)
}


#outterWinSize=5000 #bp
outterWinSize=__BetaBin_outterWinSize__ #bp

#innerWinSize =1000
innerWinSize =__BetaBin_innerWinSize__


outfile = file("__OUT_DIR__/__REF__.betabin_ab.txt", "w")
AMRs = getKnownAMRIntervals(c("__REF__"))

for(i in 1:nrow(AMRs))
{
	amr_size=AMRs[i,2]-AMRs[i,1]
	w_from = max(1,(as.integer(AMRs[i,1]/innerWinSize)-20))
	w_to = (as.integer(AMRs[i,2]/innerWinSize)+20)
	posFrom = 1+innerWinSize*(w_from-1)
	posTo =w_to*innerWinSize
	
	subTblRowFlags = bigTbl[,"RefOffset"]>=posFrom & bigTbl[,"RefOffset"]<=posTo
	subTbl = bigTbl[subTblRowFlags,]
	
	lastRowID = 0

	for(w in w_from:w_to)
	{
		posFrom_inner = 1+innerWinSize*(w-1)
		posTo_inner = innerWinSize*w
		
		posFrom_outer = posFrom_inner
		posTo_outer = posTo_inner+outterWinSize-innerWinSize
		
		posFrom_center = posFrom_outer+(outterWinSize-innerWinSize)/2
		posTo_center = posFrom_center+innerWinSize-1
		
		ret=getSubEvTbl(posTo_inner, posTo_outer, lastRowID, subTbl)
		evTbl=ret$evTbl
		lastRowID = ret$lastRowID
		if( is.null(evTbl) || nrow(evTbl)<__MIN_ROWCNT_FOR_EVTBL__)
		{
			next
		}
		
		if(w%%100==0)
			print(w)
		
		posOffset = min(evTbl[,"RefOffset"])
		#convert all sites to local positions, 1-based
		evTbl[,"RefOffset"] = evTbl[,"RefOffset"] - posOffset + 1

		CpGSiteCnt=length(unique(evTbl[,"RefOffset"]))
		ReadCnt=length(unique(evTbl[,"ReadID"]))
		CpGRange=max(evTbl[,"RefOffset"]) - min(evTbl[,"RefOffset"])+1

		est.moment = betaBinFit.Moments(evTbl)
		est.vgam = betaBinFit.VGAM(evTbl)
		
		lineInfo = paste(w,format(posFrom_outer,scientific=FALSE),
			format(posTo_outer,scientific=FALSE),
			format(posFrom_center,scientific=FALSE),
			format(posTo_center,scientific=FALSE), CpGSiteCnt, ReadCnt, CpGRange, est.moment$mu_hat, est.moment$M_hat, est.vgam$mu_hat, est.vgam$M_hat,sep="\t")
		cat(lineInfo,"\n", file =outfile)
	}
}
close(outfile)


