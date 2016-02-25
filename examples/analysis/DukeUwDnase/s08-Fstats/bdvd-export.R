rm(list=ls())
library("bdt")

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

## 15 biological groups with common samples, i.e., each group contains samples from both Duke and UW
commonGroups=list(
    c(3, 4, 219, 220),
    c(55, 56, 57, 58, 59, 254, 255),
    c(79, 80, 256),
    c(81, 82, 83, 257, 258),
    c(90, 91, 92, 292, 293),
    c(97, 98, 99, 294, 295),
    c(100, 101, 306, 307),
    c(104, 105, 106, 345, 346),
    c(112, 113, 114, 347, 348),
    c(121, 122, 349, 350),
    c(137, 138, 139, 355, 356),
    c(154, 155, 156, 361, 362),
    c(159, 160, 365, 366),
    c(185, 186, 384, 385),
    c(211, 212, 406, 407))

exportColumns = unlist(commonGroups)

# export bins with signal. We further narrow down to 254,371 bins that are assoicate with
# RefSeq transcripts with unique transcript clsuter in Exon-Array data.
# See Impact of RUV on Exon-DNase correlation section for detail
# The resultant 254,371 bins are in DNase_UniqueFeatureIdxs.txt file
need_export = FALSE
unwanted_factors = c(0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100)
known_factors =    c(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0)
if (need_export) {
    ret = bdvdExport(
        bdt_home = bdtHome,
        thread_num = 24,
        mem_size = 16000,
        column_ids = exportColumns,
        bdvd_dir = paste0(thisScriptDir, '/../s02-bdvd/out'),
        component = 'signal+random',
        artifact_detection = 'conservative',
        unwanted_factors = unwanted_factors,
        known_factors = known_factors,
        rowidxs_input = paste0("text-rowids@", bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/DNase_UniqueFeatureIdxs.txt"),
        rowidxs_index_base = 0,
        out = paste0(thisScriptDir,"/out"))
} else {
    ret = readBdvdExportOutput(paste0(thisScriptDir,"/out"))
}

KsCnt = length(unwanted_factors)
Fstats= vector(mode="list", length=KsCnt)
names(Fstats) = paste0("k=", unwanted_factors)

for(i in 1:KsCnt){
    mat = ret$mats[[i]]
    colCnt = mat$colCnt
    rowCnt = mat$rowCnt

	k = unwanted_factors[i]
	extW = known_factors[i]
	print0(paste("k=",k,",extW=",extW)

	#mat= readBigMatrix(colCnt,rowCnt,dataFile)
	mat= as.list(data.frame(t(readMat(mat))))
	#Fstats[[i]]=apply(mat,1, function(x) {
	Fstats[[i]]=mclapply(mat,function(x) {
		n.i=tapply(x,colGroups,length)
		m.i=tapply(x,colGroups,mean)
		v.i=tapply(x,colGroups,var)
		n = sum(n.i)
		wgVar=(sum((n.i - 1) * v.i)/(n - gcnt))
		if(wgVar<0.000001)
		{
			return(NA)
		}
        f_stats = ((sum(n.i * (m.i - mean(x))^2)/(gcnt - 1))/wgVar)
		return (f_stats)
	},mc.cores=num_threads)
	
	#Fstats[[i]]=Fstats[[i]][Fstats[[i]]>=0]
}