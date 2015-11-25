estimateSeqCycleErrors<-function(baseStatisticFile, totalCols, errorCols)
{
	bsTbl=read.table(baseStatisticFile,
		sep="\t",header= TRUE)
	maxPos=max(bsTbl[,"cycle"])+1 #change to 1-based
	totalSums = apply(bsTbl[,totalCols],1,sum)
	errorSums = apply(bsTbl[,errorCols],1,sum)
	cycError=errorSums/totalSums*1.5 #as we only estimate error base that do not present either two methylated states
	return (cycError)
}

getSeqCycleErrors<-function(baseStatisticFile, type = "all")
{
	TYPES <- c("all", "watson", "crick", "forward","reverse")
    type <- pmatch(type, TYPES)
	if(type==1)
	{
		totalCols=c("w_cnt","wr_cnt","c_cnt","cr_cnt")
		errorCols=c("w_err","wr_err","c_err","cr_err")
	}
	else if(type==2)
	{
		totalCols=c("w_cnt","wr_cnt")
		errorCols=c("w_err","wr_err")
	}
	else if(type==3)
	{
		totalCols=c("c_cnt","cr_cnt")
		errorCols=c("c_err","cr_err")
	}
	else if(type==4)
	{
		totalCols=c("w_cnt","c_cnt")
		errorCols=c("w_err","c_err")
	}
	else if(type==5)
	{
		totalCols=c("wr_cnt","cr_cnt")
		errorCols=c("wr_err","cr_err")
	}
	
	cycError=estimateSeqCycleErrors(baseStatisticFile,totalCols,errorCols)
	return (cycError)
}

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


constructReadsWithEvidence<-function(evTbl, siteStatistics, minMUCntPerSite, minEvCntPerRead)
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
			if(Ms[i]!=-1 && siteStatistics$siteMUCnt[site]>=minMUCntPerSite)
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

## Log-likelihood for Read k under H0, Site-wise methylation
H0.loglike<-function(k, readID2EvRowIDs, evTbl, siteStatistics) {
	
	evRowIDs = readID2EvRowIDs[[k]]
	Ms = getMStates(evRowIDs, evTbl)
	Jk = length(evRowIDs)
	L=0
	for(j in 1:Jk)
	{
		evRowID = evRowIDs[j]
		m=Ms[j]
		site=evTbl[evRowID,"RefOffset"]
		p=siteStatistics$siteMRatio[site]
		L = L + m*log(p)+(1-m)*log(1-p)
	}
	return (L)
}

## Log-likelihood for Read k under H2, consistently methylated
H1.loglike<-function(k, readID2EvRowIDs, evTbl, cycErrorsF, cycErrorsR) {
	
	evRowIDs = readID2EvRowIDs[[k]]
	Ms = getMStates(evRowIDs, evTbl)
	Jk = length(evRowIDs)
	L=0
	for(j in 1:Jk)
	{
		evRowID = evRowIDs[j]
		m=Ms[j]
		cy = evTbl[evRowID,"SeqCycle"]+1
		IsForward = evTbl[evRowID,"IsForward"]
		q = cycErrorsF[cy]
		if(IsForward==0)
		{
			q = cycErrorsR[cy]
		}
		L = L + (1-m)*log(q)+ m*log(1-q)
	}
	return (L)
}

## Log-likelihood for Read k under H1, consistently un-methylated
H2.loglike<-function(k, readID2EvRowIDs, evTbl, cycErrorsF, cycErrorsR) {
	
	evRowIDs = readID2EvRowIDs[[k]]
	Ms = getMStates(evRowIDs, evTbl)
	Jk = length(evRowIDs)
	L=0
	for(j in 1:Jk)
	{
		evRowID = evRowIDs[j]
		m=Ms[j]
		site=evTbl[evRowID,"RefOffset"]
		cy = evTbl[evRowID,"SeqCycle"]+1
		IsForward = evTbl[evRowID,"IsForward"]
		q = cycErrorsF[cy]
		if(IsForward==0)
		{
			q = cycErrorsR[cy]
		}
		L = L + m*log(q)+(1-m)*log(1-q)
	}
	return (L)
}

calcClassLike<-function(readID2EvRowIDs, evTbl, cycErrorsF, cycErrorsR, siteStatistics)
{
	K=length(readID2EvRowIDs)
	classLike=matrix(0,K,3)
	for(k in 1:K)
	{
		classLike[k,1]=H0.loglike(k,readID2EvRowIDs,evTbl,siteStatistics)
		classLike[k,2]=H1.loglike(k,readID2EvRowIDs,evTbl,cycErrorsF,cycErrorsR)
		classLike[k,3]=H2.loglike(k,readID2EvRowIDs,evTbl,cycErrorsF,cycErrorsR)
	}
	return (classLike)
}

# data all from H0
calcH0LogLike<-function(readID2EvRowIDs, evTbl, siteStatistics)
{
	L = 0.0
	K=length(readID2EvRowIDs)
	for(k in 1:K)
	{
		L=L+H0.loglike(k,readID2EvRowIDs,evTbl,siteStatistics)
	}
	return (L)
}

H12.loglike<-function(k, readID2EvRowIDs, evTbl, cycErrorsF, cycErrorsR, mixing)
{
  ll=log(mixing*exp(H1.loglike(k,readID2EvRowIDs,evTbl,cycErrorsF,cycErrorsR)) 
	+ (1.0 - mixing)*exp(H2.loglike(k,readID2EvRowIDs,evTbl,cycErrorsF,cycErrorsR)))
  return (ll)
}


# data mixing of H1, H2
calcH12LogLike<-function(readID2EvRowIDs, evTbl, cycErrorsF, cycErrorsR, mixing)
{
	L = 0.0
	K=length(readID2EvRowIDs)
	for(k in 1:K)
	{
		L=L+H12.loglike(k,readID2EvRowIDs,evTbl,cycErrorsF, cycErrorsR, mixing)
	}
	return (L)
}

# Tag Mosaicity Fit, Model 1
tmfit1<-function(classLike, tol=1e-3, max.iter=100)
{
	K = nrow(classLike)
	H = 3
	p = rep(1/H,H)
	err=tol+1
	for(i.iter in 1:max.iter)
	{
		if((i.iter%%50) == 0) {
			print(paste("We have run the first ", i.iter, " iterations, err=",err, sep=""))
			#print(loglike.old)
		}
		
		## compute posterior class membership
		pClassLike = matrix(0,K,H)
		Eold = matrix(0,K,H)
		for(h in 1:H) {
			pClassLike[,h] = classLike[,h]+log(p[h])
		}
		
		# scale to avoid overflow
		tempmax<-apply(pClassLike,1,max)
		for(h in 1:H) {
			pClassLike[,h] = exp(pClassLike[,h]-tempmax)
		}
		tempsum<-apply(pClassLike,1,sum)
		
		## update occurrence rate
		for(h in 1:H) {
			Eold[,h] = pClassLike[,h]/tempsum
		}
		p.new = (apply(Eold,2,sum)+1)/(K+H)
		
		## evaluate convergence
		err<-max(abs(p.new-p)/p)
		
		## evaluate whether the log.likelihood increases
		#loglike.new<-(sum(tempmax+log(tempsum))+sum(log(p.new))+sum(log(q.new)+log(1-q.new)))/xrow
		
		p<-p.new
		
		if(err<tol) {
			break;
		}
	}
	
	## compute posterior p
	pClassLike = matrix(0,K,H)
	for(h in 1:H) {
			pClassLike[,h] = classLike[,h]+log(p[h])
	}
	
	# scale to avoid overflow
	tempmax<-apply(pClassLike,1,max)
	for(h in 1:H) {
		pClassLike[,h] = exp(pClassLike[,h]-tempmax)
	}
	tempsum<-apply(pClassLike,1,sum)
	
	# ratio of the posterior probability that a read is from non-site-wise methylated

	post.ratio = (pClassLike[,2]+pClassLike[,3])/tempsum
	score=sum(post.ratio>0.5)/length(post.ratio)
	
	ret = list(prior=p, score=score, iter=i.iter)
	return (ret)
}

# Tag Mosaicity Fit, Model 2
tmfit2<-function(classLike, tol=1e-3, max.iter=100)
{
	K = nrow(classLike)
	H = 3
	p = rep(1/H,H)
	err=tol+1
	for(i.iter in 1:max.iter)
	{
		if((i.iter%%50) == 0) {
			print(paste("We have run the first ", i.iter, " iterations, err=",err, sep=""))
			#print(loglike.old)
		}
		
		## compute posterior class membership
		pClassLike = matrix(0,K,H)
		Eold = matrix(0,K,H)
		for(h in 1:H) {
			pClassLike[,h] = classLike[,h]+log(p[h])
		}
		
		# scale to avoid overflow
		tempmax<-apply(pClassLike,1,max)
		for(h in 1:H) {
			pClassLike[,h] = exp(pClassLike[,h]-tempmax)
		}
		tempsum<-apply(pClassLike,1,sum)
		
		## update occurrence rate
		for(h in 1:H) {
			Eold[,h] = pClassLike[,h]/tempsum
		}
		
		## enforcing p1=p2
		tempEoldSum = apply(Eold,2,sum)
		p1 = 0.5*(tempEoldSum[2]+tempEoldSum[3]+2)/(K+H)
		p.new = c(1-2*p1,p1,p1)
		
		## evaluate convergence
		err<-max(abs(p.new-p)/p)
		
		## evaluate whether the log.likelihood increases
		#loglike.new<-(sum(tempmax+log(tempsum))+sum(log(p.new))+sum(log(q.new)+log(1-q.new)))/xrow
		
		p<-p.new
		
		if(err<tol) {
			break;
		}
	}
	
	## compute posterior p
	pClassLike = matrix(0,K,H)
	for(h in 1:H) {
			pClassLike[,h] = classLike[,h]+log(p[h])
	}
	
	# scale to avoid overflow
	tempmax<-apply(pClassLike,1,max)
	for(h in 1:H) {
		pClassLike[,h] = exp(pClassLike[,h]-tempmax)
	}
	tempsum<-apply(pClassLike,1,sum)
	
	# ratio of the posterior probability that a read is from non-site-wise methylated

	post.ratio = (pClassLike[,2]+pClassLike[,3])/tempsum
	score=sum(post.ratio>0.5)/length(post.ratio)
	
	ret = list(prior=p, score=score, iter=i.iter)
	return (ret)
}

# Tag Mosaicity Fit, Model 4
tmfit4<-function(readID2EvRowIDs, evTbl, cycErrorsF, cycErrorsR, siteStatistics)
{
	mixing =0.5
	H0LogLike = calcH0LogLike(readID2EvRowIDs, evTbl, siteStatistics)
	H12LogLike = calcH12LogLike(readID2EvRowIDs, evTbl, cycErrorsF, cycErrorsR, mixing)
	llr_stat = -2*(H0LogLike - H12LogLike)
	ret = list(ll0=H0LogLike, ll12=H12LogLike, score=llr_stat)
	return (ret)
}
