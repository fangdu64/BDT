rm(list=ls())
library("bdt")

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))


## perform ttest for a list of group pairs
## dataRow: row vector
## groupPairList: a list of group pairs each for t test
rowMultiTTest <- function(dataRow, groupPairList, var.equal=FALSE) {
    gCnt = length(groupPairList)
    tstats = rep(0, gCnt)
    pvals = rep(0, gCnt)
    for (g in 1:gCnt) {
        g1Vals = dataRow[groupPairList[[g]][[1]]]
        g2Vals = dataRow[groupPairList[[g]][[2]]]

        if ((max(g1Vals) == min(g1Vals)) && (max(g2Vals) == min(g2Vals))) {
            tstats[g]=NA
            pvals[g]=NA
        } else {
            tt = t.test(g1Vals, g2Vals, var.equal=var.equal)
            tstats[g] = tt$statistic
            pvals[g]=tt$p.value
        }
    }

    ret = list(tstats = tstats, pvals = pvals)
    return (ret)
}

## 14 biological groups with common samples, i.e., each group contains samples from Duke and UW
## index by sample ids
sampleGroupPairList = list(
    list(c(3, 4), c(219, 220)),
    list(c(55, 56, 57, 58, 59), c(254, 255)),
    list(c(81, 82, 83), c(257, 258)),
    list(c(90, 91, 92), c(292, 293)),
    list(c(97, 98, 99), c(294, 295)),
    list(c(100, 101), c(306, 307)),
    list(c(104, 105, 106), c(345, 346)),
    list(c(112, 113, 114), c(347, 348)),
    list(c(121, 122), c(349, 350)),
    list(c(137, 138, 139), c(355, 356)),
    list(c(154, 155, 156), c(361, 362)),
    list(c(159, 160), c(365, 366)),
    list(c(185, 186), c(384, 385)),
    list(c(211, 212), c(406, 407)))

exportSampleIds = unlist(sampleGroupPairList)

# export bins with signal. We further narrow down to 254,371 bins that are assoicate with
# RefSeq transcripts with unique transcript clsuter in Exon-Array data.
# See Impact of RUV on Exon-DNase correlation section for detail
# The resultant 254,371 bins are in DNase_UniqueFeatureIdxs.txt file
need_export = TRUE
num_threads = 24
unwanted_factors = c(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50, 60, 70, 80, 0)
known_factors =    c(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  1)
config_names =     as.character(unwanted_factors)

# last one is for know factor only
config_names[length(config_names)] = 'KF'

if (need_export) {
    exportRet = bdvdExport(
        bdt_home = bdtHome,
        thread_num = num_threads,
        mem_size = 16000,
        column_ids = exportSampleIds,
        bdvd_dir = paste0(thisScriptDir, '/../s02-bdvd/out'),
        component = 'signal+random',
        artifact_detection = 'conservative',
        unwanted_factors = unwanted_factors,
        known_factors = known_factors,
        rowidxs_input = paste0("text-rowids@", bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/DNase_UniqueFeatureIdxs.txt"),
        rowidxs_index_base = 0,
        out = paste0(thisScriptDir,"/out"))
} else {
    exportRet = readBdvdExportOutput(paste0(thisScriptDir,"/out"))
}

## use local column indexs hereafter
localColId = 1
groupPairList = lapply(sampleGroupPairList, function(x) {
    sg1 = x[[1]]
    sg2 = x[[2]]
    g1 = localColId : (localColId + length(sg1) - 1)
    localColId <<- localColId + length(sg1)
    g2 = localColId : (localColId + length(sg2) - 1)
    localColId <<- localColId + length(sg2)
    return (list(g1, g2))
})

KsCnt = length(unwanted_factors)
Tstats= vector(mode="list", length=KsCnt)
names(Tstats) = config_names

for (i in 1:KsCnt) {
    mat = exportRet$mats[[i]]
    colCnt = mat$colCnt
    rowCnt = mat$rowCnt
    k = unwanted_factors[i]
    extW = known_factors[i]

    print0(paste("k=", k, ",extW=", extW)

    # read in data matrix for current config
    mat = as.list(data.frame(t(readMat(mat))))
    Tstats[[i]] = mclapply(mat, rowMultiTTest, groupPairList, mc.cores = num_threads)
}

plotOutDir = paste0(thisScriptDir,"/out")

## the first sample name in each cell type as group name
groupNames = sapply(groupPairList, function(g, exportedColumnNames) {
    return (exportedColumnNames[g[[1]][1]])
}, exportedColumnNames = exportRet$mats[[1]]$colNames)

for (g in 1:length(groupPairList)) {
    # get t-statistic vector for g-th cell line
    T = lapply(Tstats, function(Tk, g) {
        sapply(Tk, function(Tki, g) {
            Tki$tstats[g]
        }, g = g, use.names = FALSE)
    }, g = g)
    
    # generate T stats box plot
    pdf(file = paste0(plotOutDir,"/t_boxplot_g", g, ".pdf"))
    plotdata <- boxplot(T, ylab = "T-statistic", outline = FALSE, main=groupNames[g])
    abline(h=0, col="pink", lwd=1)
    dev.off()
}













