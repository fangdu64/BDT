# CONSTANTS
# ANNOTATION_DATA_DIR	

readInKnownImprintedClusters<-function()
{
	inFile = "__ANNOTATION_DATA_DIR__/known_imprinted_clusters_hg19.txt"
	tbl=read.table(inFile,
		sep="\t",header= TRUE)
	return (tbl)
}

readInKnownAMRs<-function()
{
	inFile = "__ANNOTATION_DATA_DIR__/known_amrs_hg19.txt"
	tbl=read.table(inFile,
		sep="\t",header= TRUE,
		fill=TRUE)
	return (tbl)
}

getKnownAMRIntervals<-function(refs)
{
	tbl= readInKnownAMRs()
	rowFlags = (tbl[,"Previously.identified"]=="Y" & tbl[,"chrom"]%in%refs)
	return (as.matrix(tbl[rowFlags,c(3,4)]))
}