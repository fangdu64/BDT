# CONSTANTS
# ANNOTATION_DATA_DIR	

readBigMatrix<-function(colCnt,rowCnt,bfvFile)
{
	totalValueCnt = rowCnt*colCnt
	con=file(bfvFile, "rb")
	Y = readBin(con, double(),n=totalValueCnt,endian = "little")
	Y=matrix(data=Y,nrow=rowCnt,ncol=colCnt,byrow=TRUE)
	close(con)
	return (Y)
}

readBigMatrixAuto<-function(colCnt,rowCnt,bfvFilePrefix, batchFileSizeMB=1024)
{
	szValueType=8
	batchFileSize=batchFileSizeMB*1024*1024
	rowBytesSize=colCnt*szValueType
	if(batchFileSize%%rowBytesSize!=0){
		#data row will not split in two batch files
		batchFileSize = batchFileSize-(batchFileSize%%rowBytesSize)
	}
	valueCntPerBatchFile = as.integer(batchFileSize / szValueType)
	rowCntPerBatchFile = as.integer(valueCntPerBatchFile / colCnt)
	batchFileIdxFrom=0
	batchFileIdxTo= as.integer((rowCnt-1)/rowCntPerBatchFile)
	if(batchFileIdxTo==batchFileIdxFrom)
	{
		bfvFile=paste(bfvFilePrefix,".bfv",sep="")
		return (readBigMatrix(colCnt,rowCnt,bfvFile))
	}
	else
	{
		return (readBigMatrixBatches(colCnt,rowCnt,bfvFile,batchFileSizeMB))
	}
	
}

readBigMatrixBatches<-function(colCnt,rowCnt,bfvFilePrefix, batchFileSizeMB=1024)
{
	szValueType=8
	batchFileSize=batchFileSizeMB*1024*1024
	rowBytesSize=colCnt*szValueType
	if(batchFileSize%%rowBytesSize!=0){
		#data row will not split in two batch files
		batchFileSize = batchFileSize-(batchFileSize%%rowBytesSize)
	}
	valueCntPerBatchFile = as.integer(batchFileSize / szValueType)
	rowCntPerBatchFile = as.integer(valueCntPerBatchFile / colCnt)
	batchFileIdxFrom=0
	batchFileIdxTo= as.integer((rowCnt-1)/rowCntPerBatchFile)
	Y=matrix(NA,nrow=rowCnt,ncol=colCnt)
	for(i in batchFileIdxFrom:batchFileIdxTo)
	{
		bfvFile=paste(bfvFilePrefix,"_",i,".bfv",sep="")
		rowIDFrom = i*rowCntPerBatchFile
		if(i==batchFileIdxTo){
			thisBatchRowCnt = rowCnt%%rowCntPerBatchFile
		}
		else{
			thisBatchRowCnt = rowCntPerBatchFile
		}
		
		y=readBigMatrix(colCnt,thisBatchRowCnt,bfvFile)
		batchRowIDs=(rowIDFrom+1):(rowIDFrom+thisBatchRowCnt)
		Y[batchRowIDs,]=y
	}
	return (Y)
}

readBigMatrixInt<-function(colCnt,rowCnt,bfvFile)
{
	totalValueCnt = rowCnt*colCnt
	con=file(bfvFile, "rb")
	Y = readBin(con, integer(),size=4,n=totalValueCnt,endian = "little")
	Y=matrix(data=Y,nrow=rowCnt,ncol=colCnt,byrow=TRUE)
	close(con)
	return (Y)
}

readVectorFromTxt<-function(txtFile)
{
	vec=read.table(txtFile, sep="\t")
	vec=vec[,1]
	return (vec)
}

pairInConfiguration<-function(x,y,xs,ys)
{
	matched=0
	for(m in 1:length(xs)){
		if(x==xs[m] && y==ys[m]){
			matched=m
			break
		}
	}
	return (matched)
}

pairInTwoConfig<-function(x1,y1,xs1,ys1,x2,y2,xs2,ys2)
{
	matched=0
	for(m in 1:length(xs1)){
		if(x1==xs1[m] && y1==ys1[m]
			&& x2==xs2[m] && y2==ys2[m]){
			matched=m
			break
		}
	}
	return (matched)
}

getRUVConfigOrder<-function(ks,ns,k,n)
{
	oval=unique(ks+ns*1000) #put known factors to rightmost
	oval=oval[order(oval)]
	v=k+n*1000
	for( i in 1:length(oval))
	{
		if(v==oval[i])
			return (i)
	}
	return (0)
}

getRUVConfigCnt<-function(ks,ns)
{
	oval=unique(ks+ns*1000) #put known factors to rightmost
	return (length(oval))
}

getRUVConfigOrders<-function(ks,ns)
{
	oval=unique(ks+ns*1000) #put known factors to rightmost
	return (order(oval))
}

getRUVConfigTexts<-function(ks,ns)
{
	oval=unique(ks+ns*1000) #put known factors to rightmost
	oval=oval[order(oval)]
	txts=rep("",length(oval))
	for( i in 1:length(oval))
	{
		if(oval[i]>=1000){
			txts[i]="KF"
		}
		else{
			txts[i]=as.character(oval[i]%%1000)
		}
	}
	return (txts)
}

getRUVConfigLearntAndKnown<-function(ks,ns)
{
	oval=unique(ks+ns*1000) #put known factors to rightmost
	oidxs=order(oval)
	oval=oval[oidxs]
	j=1
	for( i in 1:length(oval))
	{
		if(oval[i]>=1000){
			j=i
			break
		}
	}
	ret=list(learnt=oidxs[1:j-1],known=oidxs[j:length(oval)])
	return (ret)
}

getRUVConfigText<-function(k,n)
{
	if(n>0){
		return ("KF")
	}
	else{
		return (as.character(k))
	}
}

rowMultiTTest<-function(x,groupPair1,groupPair2,var.equal=FALSE)
{
	G=length(groupPair1)
	tstats=rep(0,G)
	pvals=rep(0,G)
	for(g in 1:G)
	{
		g1Vals=x[groupPair1[[g]]]
		g2Vals=x[groupPair2[[g]]]
		if((max(g1Vals)==min(g1Vals)) && (max(g2Vals)==min(g2Vals)))
		{
			tstats[g]=NA
			pvals[g]=NA
		}
		else
		{
			tt=t.test(g1Vals,g2Vals,var.equal=var.equal)
			tstats[g]=tt$statistic
			pvals[g]=tt$p.value
		}
	}
	ret=list(tstats=tstats,pvals=pvals)
	return (ret)
}

twoMatRowCor<-function(i,mat1,mat2,rowIDs1,rowIDs2)
{
	r=cor(mat1[rowIDs1[i],],mat2[rowIDs2[i],])
	return (r)
}

##
## record which members (colIDs) are merged at each hclust level
##
getHClustMergeFlags<-function(hclust)
{
	n=length(hclust$height)+1
	mergeMemberFlags=matrix(0,n-1,n)
	mg=hclust$merge
	for(i in 1:(n-1))
	{
		for( j in 1:2)
		{
			v=mg[i,j]
			if(v<0){
				# leaf node
				mergeMemberFlags[i,(-v)]=1
			}
			else{
				mergeMemberFlags[i,]=mergeMemberFlags[i,]+mergeMemberFlags[v,]
			}
		}
	}
	return(mergeMemberFlags)
}

##
## hclust score
##
getHClustScore<-function(mergeMemberFlags, memberIDs, nonmemberIDs, method = "strict")
{
	memberCnt=length(memberIDs)
	if(memberCnt<2)
	{
		return(1)
	}
	
	METHODS <- c("strict", "precision")
    method <- pmatch(method, METHODS)
	
	n=ncol(mergeMemberFlags)
		
	cs=0
	cs_cnt=0.0
	for(i in 1:(memberCnt-1))
	{
		for(j in (i+1):memberCnt)
		{
			mergeNonmemberCnt=0
			mergeMemberCnt=0
			cs_cnt=cs_cnt+1
			mi=memberIDs[i]
			mj=memberIDs[j]
			for(k in 1:(n-1))
			{
				if(mergeMemberFlags[k,mi]==1 && mergeMemberFlags[k,mj]==1)
				{
					#at which level, i and j meet together
					mergeNonmemberCnt=sum(mergeMemberFlags[k,nonmemberIDs])
					mergeMemberCnt=sum(mergeMemberFlags[k,memberIDs])
					break
				}
			}
			mscore=0
			if(method==1)
			{
				if(mergeNonmemberCnt==0)
				{
					mscore=1
				}
			}
			else if(method==2)
			{
				mscore=mergeMemberCnt/(mergeMemberCnt+mergeNonmemberCnt)
			}
			
			cs=cs+mscore
		}
	}
	return(cs/cs_cnt)
}

#
# prediction
#
twoMatPredictionMeanTestError<-function(i, mat_X, mat_Y, pmPredictorRowIDs, pmResponseRowIDs, colIDs_train, colIDs_test)
{
	Y_rowID = pmResponseRowIDs[i]
	X_rowIDs= pmPredictorRowIDs[[i]]
	Y_train = mat_Y[Y_rowID,colIDs_train]
	X_train = t(mat_X[X_rowIDs,colIDs_train])
	data_train=data.frame(y=Y_train,X_train)
	
	Y_test = mat_Y[Y_rowID,colIDs_test]
	X_test = t(mat_X[X_rowIDs,colIDs_test])
	
	fit = lm(y~.,data=data_train)
	Y_predict=predict(fit,data.frame(X_test))
	rmse=sqrt(mean((Y_predict-Y_test)^2))
	return (rmse)
}

#
# prediction
#
twoMatPredictionTestErrors<-function(i, mat_X, mat_Y, pmPredictorRowIDs, pmResponseRowIDs, colIDs_train, colIDs_test)
{
	Y_rowID = pmResponseRowIDs[i]
	X_rowIDs= pmPredictorRowIDs[[i]]
	Y_train = mat_Y[Y_rowID,colIDs_train]
	X_train = t(mat_X[X_rowIDs,colIDs_train])
	data_train=data.frame(y=Y_train,X_train)
	
	Y_test = mat_Y[Y_rowID,colIDs_test]
	X_test = t(mat_X[X_rowIDs,colIDs_test])
	
	fit = lm(y~.,data=data_train)
	Y_predict=predict(fit,data.frame(X_test))
	#a vector
	absErrors=abs(Y_predict-Y_test)
	return (absErrors)
}


#
# Nearest K locations
#

nearestKRowIDs<-function(locationTbl,K, centerRowID)
{
	ref=locationTbl[centerRowID,4]
	dataRowIDs=rep(0,K)
	RowCnt=nrow(locationTbl)
	MAX_ROWID=centerRowID
	MIN_ROWID=centerRowID
	minRowID=centerRowID
	maxRowID=centerRowID
	k=1
	dataRowIDs[k]=locationTbl[centerRowID,2] #data rowID
	while(TRUE)
	{
		rowID=0
		if(minRowID>1 && locationTbl[minRowID-1,4]==ref)
		{
			MIN_ROWID=minRowID-1
		}
		
		if(maxRowID<RowCnt && locationTbl[maxRowID+1,4]==ref)
		{
			MAX_ROWID=maxRowID+1
		}
		
		if(minRowID>MIN_ROWID&&maxRowID==MAX_ROWID)
		{
			minRowID=minRowID-1
			rowID=minRowID
		}
		else if(minRowID==MIN_ROWID&&maxRowID<MAX_ROWID)
		{
			maxRowID=maxRowID+1
			rowID=maxRowID
		}
		else if(minRowID>MIN_ROWID&&maxRowID<MAX_ROWID)
		{
			d_min=abs(locationTbl[minRowID,1]-locationTbl[centerRowID,1])
			d_max=abs(locationTbl[maxRowID,1]-locationTbl[centerRowID,1])
			if(d_min<d_max)
			{
				minRowID=minRowID-1
				rowID=minRowID
			}
			else
			{
				maxRowID=maxRowID+1
				rowID=maxRowID
			}
		}
		if(rowID==0)
			break
		k=k+1
		dataRowIDs[k]=locationTbl[rowID,2]
		if(k==K)
			break
	}
	
	return (dataRowIDs[1:k])
}
nearestKRowIDs.test<-function()
{
	rm(list=ls())
	setwd("E:/iBS/trunk/core/src/iBS.R/BDVD")
	source("Common.R")
	YDataDir="E:/iBS/trunk/analysis/iBS.Projects/BDVD/DNaseExonCorrelation/100bp/s04-NearbyTSS"
	locationTbl_Y=read.table(paste(YDataDir,"/OrderedTTS_Tbl.txt",sep=""),stringsAsFactors =FALSE,
			sep="\t",header= TRUE)
	locationRowIDs_Y=readVectorFromTxt(paste(YDataDir,"/Exon_LocationRowIDs.txt",sep="")) # 1-based
	nearestKRowIDs(locationTbl_Y,10,1)
}

#
# annotation
#
twoMatAnnotation<-function(i, mat_X, mat_Y, rowIDs_X,rowIDs_Y, locationTbl_Y, locationRowIDs_Y,nearbyCnt)
{
	# i is the data rowID
	nearbyDataRowIDs=nearestKRowIDs(locationTbl_Y,nearbyCnt,locationRowIDs_Y[i])
	rmax=-2
	max_rowID=-1
	for(j in nearbyDataRowIDs)
	{
		r=cor(mat_X[rowIDs_X[i],],mat_Y[j,])
		if(is.na(r)){
			next
		}
		if(rmax<r){
			rmax = r
			max_rowID =j
		}
	}
	ret=c(rmax,0)
	if(max_rowID==rowIDs_Y[i]){
		# nearest gene
		ret[2]=1
	}
	return (ret)
}