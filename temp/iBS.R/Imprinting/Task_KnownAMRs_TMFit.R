# CONSTANTS
# RSCRIPT_DIR	
# EV_TABLE_FILE	E:/BDT/install/evtbl2subset_out/chr7_1_159138663.tsv
# BaseStatisticFile	E:/Biostatistics/Imprinting/GM12878/Data/base_statistics.txt
# OUT_DIR	
# REF	chr7
# BetaBin_innerWinSize	1000
# TMFIT_OutterWinSize	500
# SlidingCGSiteCnt	1
# AMR_MARGIN_FACTOR	10
# UNIQUE_READ_CNT_MIN	10
# ShrinkageMethod	1 (0-none, 1-VAGM,2-moment)
# TMFit_Model	1 (1-Model1, 2-Model2, 3-Model3)

rm(list=ls())
setwd("__RSCRIPT_DIR__")

bigTbl=read.table("__EV_TABLE_FILE__",
	sep="\t",comment.char = "",
	colClasses= c("factor","integer","integer","character","integer","integer","integer", "factor","factor","integer","integer","integer","integer","integer"), quote = "", # ' in Qual1 will mess up the parsing
	)
colnames(bigTbl)=c("RefName","RefOffset","ReadID","Allele","IsWaston","IsForward","SamFlag","Qual1","Qual2","SeqCycle","AlLen","AlScore","MAPQ","MateID")

source("ReadBetaBinShrinkage.R")
source("TMFit.R")
source("ReadKnownImprinting.R")

getSubEvTbl<-function(slidingCGSiteCnt, outterWinSize, lastRowID, bigTbl)
{
	bigTblRowCnt = nrow(bigTbl)
	if((lastRowID+slidingCGSiteCnt)>=bigTblRowCnt)
	{
		ret=list(evTbl = NULL, 
			outter_posFrom=NULL, inner_posFrom=NULL, 
			inner_posTo = NULL, outter_posTo=NULL,
			lastRowID = NULL)
		return (ret)
	}
	
	rowID = lastRowID
	rowIDFrom = lastRowID +1
	lastRefOffset=0
	processedSiteCnt=0
	inner_posFrom=0
	inner_rowIDFrom=0
	
	inner_posTo=0
	inner_rowIDTo=0
	while(rowID<bigTblRowCnt) # in R, the rowIDs are 1 based
	{
		rowID = rowID+1
		RefOffset = bigTbl[rowID,"RefOffset"]
		if(lastRefOffset==0)
		{
			lastRefOffset=RefOffset
			inner_posFrom = RefOffset 
			inner_rowIDFrom = rowID
		}
		if(lastRefOffset!=RefOffset)
		{
			processedSiteCnt=processedSiteCnt+1
		}
		if(processedSiteCnt==slidingCGSiteCnt)
		{
			lastRowID = rowID-1
			inner_rowIDTo = lastRowID
			inner_posTo = lastRefOffset
			break
		}
		lastRefOffset = RefOffset
	}
	
	margin = as.integer((outterWinSize - (inner_posTo -inner_posFrom+1))/2)
	outter_rowIDFrom = inner_rowIDFrom
	outter_posFrom = inner_posFrom
	outter_rowIDTo = inner_rowIDTo
	outter_posTo= inner_posTo
	if(margin>0)
	{
		posTo = inner_posTo - margin
		rowID = inner_rowIDFrom
		while(rowID>0)
		{
			
			RefOffset = bigTbl[rowID,"RefOffset"]
			if(RefOffset<posTo)
			{
				break
			}
			outter_rowIDFrom = rowID
			outter_posFrom = RefOffset
			rowID = rowID-1
		}
		
		posTo = inner_posFrom + margin
		rowID = inner_rowIDTo
		while(rowID<bigTblRowCnt)
		{
			rowID = rowID+1
			RefOffset = bigTbl[rowID,"RefOffset"]
			if(RefOffset>posTo)
			{
				break
			}
			outter_rowIDTo = rowID
			outter_posTo = RefOffset
		}
	}
	
	ret=list(evTbl = bigTbl[outter_rowIDFrom:outter_rowIDTo,], 
		outter_posFrom=outter_posFrom, inner_posFrom=inner_posFrom, 
		inner_posTo = inner_posTo, outter_posTo=outter_posTo,
		lastRowID = lastRowID)
	return (ret)
}

baseStatisticFile = "__BaseStatisticFile__"
cycErrorsF = getSeqCycleErrors(baseStatisticFile, type="forward")
cycErrorsR = getSeqCycleErrors(baseStatisticFile, type="reverse")

betaBin_ab_file = "__OUT_DIR__/__REF__.betabin_ab.txt"
shrinkageTbl_innerWinSize = __BetaBin_innerWinSize__
shrinkageTbl = readInBetaBinShrinkage(betaBin_ab_file)

outterWinSize=__TMFIT_OutterWinSize__
slidingCGSiteCnt = __SlidingCGSiteCnt__
amr_margin_factor = __AMR_MARGIN_FACTOR__
uniqe_readcnt_min = __UNIQUE_READ_CNT_MIN__
shrinkage_method = __ShrinkageMethod__
tmfit_model = __TMFit_Model__
outfile = file("__OUT_DIR__/__REF__.tmfit.txt", "w")
AMRs = getKnownAMRIntervals(c("__REF__"))
w0=0
for(i in 1:nrow(AMRs))
{
	amr_size=AMRs[i,2]-AMRs[i,1]
	margin_size=amr_size*amr_margin_factor
	posFrom=AMRs[i,1]-margin_size
	posTo=AMRs[i,2]+margin_size
	
	
	subTblRowFlags = bigTbl[,"RefOffset"]>posFrom & bigTbl[,"RefOffset"]<posTo
	subTbl = bigTbl[subTblRowFlags,]
	TotalCpGSiteCnt=length(unique(subTbl[,"RefOffset"]))
	slidingWinCnt = as.integer(TotalCpGSiteCnt/slidingCGSiteCnt)
	lastRowID = 0

	for(w in 1:slidingWinCnt)
	{
		ret=getSubEvTbl(slidingCGSiteCnt, outterWinSize, lastRowID, subTbl)
		evTbl=ret$evTbl
		lastRowID = ret$lastRowID
		if( is.null(evTbl) || nrow(evTbl)<10)
		{
			next
		}
		
		if(w%%1000==0)
			print(w)
		
		posOffset = min(evTbl[,"RefOffset"])
		#convert all sites to local positions, 1-based
		evTbl[,"RefOffset"] = evTbl[,"RefOffset"] - posOffset + 1

		CpGSiteCnt=length(unique(evTbl[,"RefOffset"]))
		ReadCnt=length(unique(evTbl[,"ReadID"]))
		WindowSize=max(evTbl[,"RefOffset"]) - min(evTbl[,"RefOffset"])+1
		
		if(shrinkage_method==2)
		{
			siteStatistics = estimateSiteStatisticsWithShrinkage(evTbl, posOffset, shrinkageTbl$MuMTbl_Moment, shrinkageTbl_innerWinSize)
		}
		else if(shrinkage_method==1)
		{
			siteStatistics = estimateSiteStatisticsWithShrinkage(evTbl, posOffset, shrinkageTbl$MuMTbl_VGAM, shrinkageTbl_innerWinSize)
		}
		else
		{
			siteStatistics = estimateSiteStatistics(evTbl)
		}
		minMUCntPerSite =4
		minEvCntPerRead =2
		readID2EvRowIDs = constructReadsWithEvidence(evTbl,siteStatistics, minMUCntPerSite, minEvCntPerRead)
		if(length(readID2EvRowIDs)<uniqe_readcnt_min)
			next
		
		if(tmfit_model==1){
			classLike = calcClassLike(readID2EvRowIDs, evTbl, cycErrorsF, cycErrorsR, siteStatistics)
			p=tmfit1(classLike, tol=1e-3, max.iter=100)
		}
		else if(tmfit_model==2){
			classLike = calcClassLike(readID2EvRowIDs, evTbl, cycErrorsF, cycErrorsR, siteStatistics)
			p=tmfit2(classLike, tol=1e-3, max.iter=100)
		}
		else if(tmfit_model==4){
			
			p=tmfit4(readID2EvRowIDs, evTbl, cycErrorsF, cycErrorsR, siteStatistics)
		}
		
		if(tmfit_model==1 || tmfit_model==2)
		{
			lineInfo = paste(w+w0, format(ret$outter_posFrom,scientific=FALSE), 
				format(ret$inner_posFrom,scientific=FALSE),
				format(ret$inner_posTo,scientific=FALSE),
				format(ret$outter_posTo,scientific=FALSE),
				CpGSiteCnt, ReadCnt, WindowSize, p$iter, p$prior[1], p$prior[2], p$prior[3],p$score, sep="\t")
		}
		else if(tmfit_model==4){
			lineInfo = paste(w+w0, format(ret$outter_posFrom,scientific=FALSE), 
				format(ret$inner_posFrom,scientific=FALSE),
				format(ret$inner_posTo,scientific=FALSE),
				format(ret$outter_posTo,scientific=FALSE),
				CpGSiteCnt, ReadCnt, WindowSize, 0, p$ll0, p$ll12, p$ll12, p$score, sep="\t")
		}
		
		cat(lineInfo,"\n", file =outfile)
		
	}
	
	w0=w0+slidingWinCnt

}

close(outfile)


