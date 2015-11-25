
readInBetaBinShrinkage<-function(betaBin_ab_file)
{
	shrinkageTbl_sub=read.table(betaBin_ab_file,
	sep="\t",comment.char = "",
	colClasses=		   c("integer","character","character","character","character","integer", "integer", "integer", "numeric","numeric","numeric","numeric"),
	fill=TRUE,
	quote = "", # ' in Qual1 will mess up the parsing
	)
	#colnames(shrinkageTbl_sub)=c("winID","posFrom_outer","posTo_outer","posFrom_center","posTo_center","CpGSiteCnt","ReadCnt", "CpGRange","moment_mu","moment_M","vgam_mu","vgam_M")
	
	maxWinID = max(shrinkageTbl_sub[,1])+1
	MuMTbl_Moment=matrix(NaN,maxWinID,2)
	MuMTbl_Moment[shrinkageTbl_sub[,1],]=as.matrix(shrinkageTbl_sub[,c(9,10)])
	colnames(MuMTbl_Moment)=c("Mu","M")
	MuMTbl_VGAM=matrix(NaN,maxWinID,2)
	MuMTbl_VGAM[shrinkageTbl_sub[,1],]=as.matrix(shrinkageTbl_sub[,c(11,12)])
	colnames(MuMTbl_VGAM)=c("Mu","M")
	ret=list(MuMTbl_Moment=MuMTbl_Moment, MuMTbl_VGAM=MuMTbl_VGAM)
	return (ret)
}

readInBetaBinShrinkage.test<-function()
{
	betaBin_ab_file = "E:/BDT/install/evtbl2subset_out/chr7_1_159138663.betabin_ab_5k_1k.txt"
	shrinkageTbl = readInBetaBinShrinkage(betaBin_ab_file)
	
}