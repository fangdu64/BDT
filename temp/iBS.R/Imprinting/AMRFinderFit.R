
estimateSiteStatistics<-function(evTbl)
{
	maxPos = max(evTbl[,"RefOffset"])
	MUCnts =rep(0,maxPos)
	MCnts =rep(0,maxPos)
	for(i in 1:nrow(evTbl))
	{
		mismatched=FALSE
		Allele = evTbl[i,"Allele"]
		IsWaston = evTbl[i,"IsWaston"]
		IsForward = evTbl[i,"IsForward"]
		
		watsonMU=0
        watsonM=0
        crickMU=0
        crickM=0
        if(IsWaston==1 && Allele=="C")
		{
            watsonMU=1
            watsonM=1
		}
        else if(IsWaston==1 && Allele=="T")
		{
            watsonMU=1
		}
        else if(IsWaston==0 && Allele=="G")
		{
            #G->C Mythylated
            crickMU=1
            crickM=1
		}
        else if(IsWaston==0 && Allele=="A")
		{
            #A->T UnMythylated
            crickMU=1
		}
        else
		{
            mismatched = TRUE
		}
		
		loci=evTbl[i,"RefOffset"]
		MUCnts[loci]=MUCnts[loci]+watsonMU+crickMU
		MCnts[loci]=MCnts[loci]+watsonM+crickM
	}
	
	siteMRatio=MCnts/MUCnts
	for(i in 1:length(siteMRatio))
	{
		if(is.na(siteMRatio[i]))
		{
			next
		}
		if(siteMRatio[i]<0.01)
		{
			siteMRatio[i]=0.01
		}
		else if(siteMRatio[i]>0.99)
		{
			siteMRatio[i]=0.99
		}
	}
	ret = list(siteMUCnt=MUCnts, siteMRatio=siteMRatio)
	return (ret)
}

estimateSiteStatisticsWithShrinkage<-function(evTbl, posOffset, shrinkageTbl, shrinkageTbl_innerWinSize)
{
	maxPos = max(evTbl[,"RefOffset"])
	MUCnts =rep(0,maxPos)
	MCnts =rep(0,maxPos)
	siteWinID=rep(0,maxPos)
	for(i in 1:nrow(evTbl))
	{
		mismatched=FALSE
		Allele = evTbl[i,"Allele"]
		IsWaston = evTbl[i,"IsWaston"]
		IsForward = evTbl[i,"IsForward"]
		
		watsonMU=0
        watsonM=0
        crickMU=0
        crickM=0
        if(IsWaston==1 && Allele=="C")
		{
            watsonMU=1
            watsonM=1
		}
        else if(IsWaston==1 && Allele=="T")
		{
            watsonMU=1
		}
        else if(IsWaston==0 && Allele=="G")
		{
            #G->C Mythylated
            crickMU=1
            crickM=1
		}
        else if(IsWaston==0 && Allele=="A")
		{
            #A->T UnMythylated
            crickMU=1
		}
        else
		{
            mismatched = TRUE
		}
		
		loci=evTbl[i,"RefOffset"]
		siteWinID[loci]=as.integer((evTbl[i,"RefOffset"]+posOffset-2)/shrinkageTbl_innerWinSize)+1
		MUCnts[loci]=MUCnts[loci]+watsonMU+crickMU
		MCnts[loci]=MCnts[loci]+watsonM+crickM
		
	}
	
	siteMRatio=MCnts/MUCnts
	for(i in 1:length(siteMRatio))
	{
		if(is.na(siteMRatio[i]))
		{
			next
		}
		mu_hat = NA
		M_hat = NA
		if(siteWinID[i]<=nrow(shrinkageTbl))
		{
			mu_hat=shrinkageTbl[siteWinID[i],1]
			M_hat=shrinkageTbl[siteWinID[i],2]
		}
		
		if(!(is.na(mu_hat)) && !(is.na(M_hat)) && M_hat>0)
		{
			siteMRatio[i]=(MCnts[i]+M_hat*mu_hat)/(MUCnts[i]+M_hat)
			next
		}
		
		if(siteMRatio[i]<0.01)
		{
			siteMRatio[i]=0.01
		}
		else if(siteMRatio[i]>0.99)
		{
			siteMRatio[i]=0.99
		}
	}
	ret = list(siteMUCnt=MUCnts, siteMRatio=siteMRatio)
	return (ret)
}

# evTbl is unfiltered data
calcSiteMUCnt<-function(evTbl)
{
	maxPos = max(evTbl[,"RefOffset"])
	MUCnts =rep(0,maxPos)
	for(i in 1:nrow(evTbl))
	{
		mismatched=FALSE
		Allele = evTbl[i,"Allele"]
		IsWaston = evTbl[i,"IsWaston"]
		IsForward = evTbl[i,"IsForward"]
		
		watsonMU=0
        watsonM=0
        crickMU=0
        crickM=0
        if(IsWaston==1 && Allele=="C")
		{
            watsonMU=1
            watsonM=1
		}
        else if(IsWaston==1 && Allele=="T")
		{
            watsonMU=1
		}
        else if(IsWaston==0 && Allele=="G")
		{
            #G->C Mythylated
            crickMU=1
            crickM=1
		}
        else if(IsWaston==0 && Allele=="A")
		{
            #A->T UnMythylated
            crickMU=1
		}
        else
		{
            mismatched = TRUE
		}
		
		loci=evTbl[i,"RefOffset"]
		MUCnts[loci]=MUCnts[loci]+watsonMU+crickMU
	}
	
	return (MUCnts)
}

# get a vector of mythlation states
getMStates<-function(evRowIDs, evTbl)
{
	Ms= rep(-1,length(evRowIDs)) #no evidence
	j=0
	for(i in evRowIDs)
	{
		j=j+1
		
		Allele = evTbl[i,"Allele"]
		IsWaston = evTbl[i,"IsWaston"]
		IsForward = evTbl[i,"IsForward"]
		
		M=-1 #no evidence
        if(IsWaston==1 && Allele=="C")
		{
            M=1
		}
        else if(IsWaston==1 && Allele=="T")
		{
            M=0
		}
        else if(IsWaston==0 && Allele=="G")
		{
            #G->C Mythylated
            M=1
		}
        else if(IsWaston==0 && Allele=="A")
		{
            #A->T UnMythylated
            M=0
		}
		Ms[j]=M
	}
	
	return (Ms)
}

# after filtering, the reads are fitted into ARMFinder
constructReadsWithEvidence<-function(evTbl, siteMUCnts, minMUCntPerSite, minEvCntPerRead)
{
	readIDs = unique(evTbl[,"ReadID"])
	readID2EvRowIDs= vector(mode="list", length=length(readIDs))
	validFlags=vector(length=length(readIDs))
	for(k in 1:length(readIDs))
	{
		readID = readIDs[k]
		evRowIDs = which(evTbl[,"ReadID"]==readID) # evidence belong to the read
		Ms = getMStates(evRowIDs,evTbl)
		siteFlags=vector(length=length(evRowIDs))
		for(i in 1:length(evRowIDs))
		{
			site=evTbl[evRowIDs[i],"RefOffset"]
			if(Ms[i]!=-1 && siteMUCnts[site]>=minMUCntPerSite)
			{
				siteFlags[i]=TRUE
			}
		}
		evRowIDs=evRowIDs[siteFlags]
		if(length(evRowIDs)<minEvCntPerRead) # at leaset should have evidence at two sites
		{
			next
		}
		readID2EvRowIDs[[k]] = evRowIDs
		validFlags[k] =TRUE
	}
	
	readID2EvRowIDs = readID2EvRowIDs[validFlags] # remove reads without evidence
	return (readID2EvRowIDs)
}

calcValidSites<-function(readID2EvRowIDs,evTbl)
{
	maxPos = max(evTbl[,"RefOffset"])
	MUCnts =rep(0,maxPos)
	K=length(readID2EvRowIDs)
	for(k in 1:K)
	{
		evRowIDs = readID2EvRowIDs[[k]]
		Jk = length(evRowIDs)
		for(j in 1:Jk)
		{
			evRowID = evRowIDs[j]
			site=evTbl[evRowID,"RefOffset"]
			MUCnts[site]=MUCnts[site]+1
		}
	}
	
	return (sites=which(MUCnts>0))
}

# ================================================================================

# epiread = k, readID2EvRowIDs, evTbl,
# a = siteMRatios
H0.log_likelihood<-function(k, readID2EvRowIDs, evTbl, siteMRatios) {
	evRowIDs = readID2EvRowIDs[[k]]
	Ms = getMStates(evRowIDs, evTbl)
	Jk = length(evRowIDs)
	L=0
	for(j in 1:Jk)
	{
		evRowID = evRowIDs[j]
		m=Ms[j]
		site=evTbl[evRowID,"RefOffset"]
		p=siteMRatios[site]
		
		L = L + m*log(p)+(1-m)*log(1-p) # cause problems when p=0 or p=1
	}
	return (L)
}

mixing.log_likelihood<-function(k, readID2EvRowIDs, evTbl, mixing,a1, a2)
{
  ll=log(mixing*exp(H0.log_likelihood(k, readID2EvRowIDs, evTbl, a1)) + (1.0 - mixing)*exp(H0.log_likelihood(k, readID2EvRowIDs, evTbl, a2)))
  return (ll)
}


double
H12.log_likelihood<-function(readID2EvRowIDs,evTbl, mixing, a1,a2)
{
	L = 0.0
	K=length(readID2EvRowIDs)
	for(k in 1:K)
	{
		L = L + mixing.log_likelihood(k, readID2EvRowIDs, evTbl, mixing, a1,a2)
	}
	
	return (L)
}


# reads = (readID2EvRowIDs, evTbl)
# a= siteMRatios
fit_epiallele<-function(pseudo,readID2EvRowIDs,evTbl,indicators, a)
{
	maxPos = max(evTbl[,"RefOffset"])
	MUCnts =rep(0,maxPos)
	MCnts =rep(0,maxPos)
	
	K=length(readID2EvRowIDs)
	for(k in 1:K)
	{
		weight = indicators[k]
		evRowIDs = readID2EvRowIDs[[k]]
		Ms = getMStates(evRowIDs, evTbl)
		Jk = length(evRowIDs)
		for(j in 1:Jk)
		{
			evRowID = evRowIDs[j]
			m=Ms[j]
			site=evTbl[evRowID,"RefOffset"]
			MCnts[site]=MCnts[site]+weight*m
			MUCnts[site]=MUCnts[site]+weight
		}
	}

	for(i in 1:maxPos)
	{
		a[i]=(MCnts[i]+pseudo)/(MUCnts[i] + 2*pseudo)
	}
	
	return (a)
}

# reads = (readID2EvRowIDs, evTbl)
# a = siteMRatios
fit_single_epiallele<-function(readID2EvRowIDs, evTbl, pseudo, a) 
{
	K=length(readID2EvRowIDs) #reads.size()
	indicators=rep(1.0,K)
	a = fit_epiallele(pseudo, readID2EvRowIDs, evTbl, indicators,a)

	score = 0.0
	for(k in 1:K)
	{
		score = score + H0.log_likelihood(k, readID2EvRowIDs, evTbl, a)
	}
	
	ret = list(score=score, a=a)
	return (ret)
}

#====================================================================================
# two-allele model


# reads = (readID2EvRowIDs, evTbl)
expectation_step<-function(readID2EvRowIDs, evTbl, mixing, a1, a2, indicators)
{
	K=length(readID2EvRowIDs) #reads.size()
	log_mixing1 = log(mixing)
	log_mixing2 = log(1.0 - mixing)
	
	score = 0
	for(k in 1:K)
	{
		ll1 = log_mixing1 + H0.log_likelihood(k, readID2EvRowIDs, evTbl, a1)
		ll2 = log_mixing2 + H0.log_likelihood(k, readID2EvRowIDs, evTbl, a2)
		
		log_denom = log(exp(ll1) + exp(ll2))
		score = score+ log_denom
		
		indicators[k] = exp(ll1 - log_denom)
		
	}
	ret = list(score=score, indicators=indicators)
	return (ret)
}

rescale_indicators<-function(mixing, indic)
{
	n_reads = length(indic)
	total = sum(indic)
	ratio = total/n_reads
	if(mixing<ratio)
	{
		for (i in 1:n_reads)
		{
			indic[i] = indic[i]*(mixing/ratio)
		}
	} 
	else
	{
		adjustment = mixing/(1.0 - ratio)
		for (i in 1:n_reads)
		{
			indic[i] = 1.0 - (1.0 - indic[i])*adjustment
		}
	}
	return (indic)
}

maximization_step<-function(readID2EvRowIDs, evTbl, indicators, a1, a2, pseudo)
{
	inverted_indicators = 1.0 - indicators
	
	# Fit the regular model parameters. Since the two epialleles'
	# likelihoods are summed, we need to make sure the pseudocount
	#is proportional to the pseudocount used in the single allele model.
	a1 = fit_epiallele(0.5*pseudo, readID2EvRowIDs, evTbl, indicators,a1)
	a2 = fit_epiallele(0.5*pseudo, readID2EvRowIDs, evTbl, inverted_indicators,a2)
	
	ret = list(a1=a1, a2=a2)
	return (ret) 
}


# reads = (readID2EvRowIDs, evTbl)
expectation_maximization<-function(max_itr, readID2EvRowIDs, evTbl,mixing, indicators, a1, a2,pseudo)
{
	prev_score = -1e30
	EPIREAD_STATS_TOLERANCE = 1e-20
	for (i in 1:max_itr)
	{
		ret = expectation_step(readID2EvRowIDs, evTbl, mixing, a1, a2, indicators)
		indicators = ret$indicators
		score = ret$score
		indicators = rescale_indicators(mixing, indicators)
		ret = maximization_step(readID2EvRowIDs, evTbl, indicators, a1, a2, pseudo)
		a1 = ret$a1
		a2 = ret$a2

		if((prev_score - score)/prev_score < EPIREAD_STATS_TOLERANCE)
		{
			break
		}
		  
		prev_score = score
	}
	
	ret = list(a1=a1, a2=a2, em_iter=i, prev_score=prev_score)
	return (ret)
}


# reads = (readID2EvRowIDs, evTbl)
resolve_epialleles<-function(max_itr, readID2EvRowIDs, evTbl,mixing,a1, a2, pseudo)
{
	K=length(readID2EvRowIDs) #reads.size()
	indicators=rep(0.0,K)

	for(k in 1:K)
	{
		l1 = H0.log_likelihood(k, readID2EvRowIDs, evTbl, a1)
		l2 = H0.log_likelihood(k, readID2EvRowIDs, evTbl, a2)
		indicators[k] = exp(l1 - log(exp(l1) + exp(l2)))
	}

	ret = expectation_maximization(max_itr, readID2EvRowIDs, evTbl, mixing, indicators, a1, a2,pseudo)
	return (ret)
}

# =====================================================================================
# reads = (readID2EvRowIDs, evTbl)
compute_model_likelihoods<-function(max_itr, low_prob, high_prob, readID2EvRowIDs, evTbl, sites, pseudo) 
{
	mixing = 0.5
	maxPos = max(evTbl[,"RefOffset"])
	# try a single epi-allele and compute its log likelihood
	a0=rep(0.5,maxPos)
	ret = fit_single_epiallele(readID2EvRowIDs, evTbl, pseudo,a0)
	single_score=ret$score
	a0=ret$a
	#print(a0[sites])
	# initialize the pair epi-alleles and indicators, and do the actual
	# computation to infer alleles, compute its log likelihood
	
	
	a1=rep(low_prob,maxPos)
	a2=rep(high_prob,maxPos)

	ret=resolve_epialleles(max_itr, readID2EvRowIDs, evTbl, mixing, a1, a2, pseudo)
	a1=ret$a1
	a2=ret$a2
	#print(a1[sites])
	#print(a2[sites])
	em_iter=ret$em_iter
	pair_score = H12.log_likelihood(readID2EvRowIDs, evTbl, mixing, a1, a2)
	
	ret = list(single_score=single_score, pair_score=pair_score, em_iter=em_iter)
	return (ret)
}


test_asm_lrt<-function(max_itr, low_prob, high_prob, sites, readID2EvRowIDs, evTbl)
{
	PSEUDOCOUNT = 1e-10
	
	ret = compute_model_likelihoods(max_itr, low_prob, high_prob, readID2EvRowIDs, evTbl,sites,PSEUDOCOUNT)
	single_score=ret$single_score
	pair_score=ret$pair_score
	em_iter=ret$em_iter
	# degrees of freedom = 2*n_cpgs for two-allele model 
	# minus n_cpgs for one-allele model
	df = length(sites)

	llr_stat = -2*(single_score - pair_score)
	p_value = 1.0 - pchisq(llr_stat, df)
	ret = list(p_value=p_value, llr_stat=llr_stat,df=df, em_iter=em_iter)
	return (ret)
}