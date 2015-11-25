# ki~Bin(ni,pi)
# pi~Beta(mu,M)
# where mu=a/(a+b), M=a+b

# Further Bayesian considerations
# http://en.wikipedia.org/wiki/Beta-binomial_distribution

betaBinFit.Moments<-function(evTbl)
{
	maxPos = max(evTbl[,"RefOffset"])
	MUCnts =rep(0,maxPos)
	MCnts =rep(0,maxPos)
	validFlags=vector(length=maxPos)
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
		validFlags[loci] =TRUE
	}
	
	kis=MCnts[validFlags]
	nis=MUCnts[validFlags]
	N=length(kis)
	mu_hat = sum(kis)/sum(nis)
	
	s2=N*sum(nis*(kis/nis-mu_hat)*(kis/nis-mu_hat))
	s2=s2/(N-1)/sum(nis)
	
	M_hat = mu_hat*(1-mu_hat)-s2
	M_hat = M_hat/(s2-mu_hat*(1-mu_hat)*sum(1/nis)/N)
	
	ret = list(mu_hat=mu_hat, M_hat=M_hat)
	return (ret)
}

betaBinFit.Moments.test<-function()
{
	N <- 10; s1 <- exp(1); s2 <- exp(2)
	y <- rbetabinom.ab(n = 100, size = N, shape1 = s1, shape2 = s2)
	
	kis=y
	nis=rep(10,length(kis))
	N=length(kis)
	mu_hat = sum(kis)/sum(nis)
	
	s2=N*sum(nis*(kis/nis-mu_hat)*(kis/nis-mu_hat))
	s2=s2/(N-1)/sum(nis)
	
	M_hat = mu_hat*(1-mu_hat)-s2
	M_hat = M_hat/(s2-mu_hat*(1-mu_hat)*sum(1/nis)/N)
	
	ret = list(mu_hat=mu_hat, M_hat=M_hat)
}

library("VGAM")
# ki~Bin(ni,pi)
# pi~Beta(mu,M)
# where mu=a/(a+b), M=a+b

# Further Bayesian considerations
# http://en.wikipedia.org/wiki/Beta-binomial_distribution

betaBinFit.VGAM<-function(evTbl)
{
	maxPos = max(evTbl[,"RefOffset"])
	MUCnts =rep(0,maxPos)
	MCnts =rep(0,maxPos)
	validFlags=vector(length=maxPos)
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
		validFlags[loci] =TRUE
	}
	
	kis=MCnts[validFlags]
	nis=MUCnts[validFlags]
	
	mu_hat = NaN
	M_hat = NaN

	tryCatch({
		fit <- vglm(cbind(kis, nis-kis) ~ 1, betabinomial.ab, trace = FALSE)
		a = as.numeric(Coef(fit)[1])
		b = as.numeric(Coef(fit)[2])
		mu_hat = a/(a+b)
		M_hat = a+b
	},
	warning  = function(war){
	}, error = function(e){
	})
	ret = list(mu_hat=mu_hat, M_hat=M_hat)
	
	return (ret)
}
