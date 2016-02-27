rm(list=ls())
library("bdt")
library(parallel)

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

# export 250K bins with signal
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
        rowidxs_input = paste0("text-rowids@", bdtDatasetsDir, "/DukeUwDnase/100bp/RowIdxs/signalRandom.txt"),
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

    print(paste0("k=", k, ",extW=", extW))

    # read in data matrix for current config
    mat = as.list(data.frame(t(readMat(mat))))
    Tstats[[i]] = mclapply(mat, rowMultiTTest, groupPairList, mc.cores = num_threads)
}

plotOutDir = paste0(thisScriptDir,"/out")

## the first sample name in each cell type as group name
groupNames = sapply(groupPairList, function(g, exportedColumnNames) {
    return (exportedColumnNames[g[[1]][1]])
}, exportedColumnNames = exportRet$mats[[1]]$colNames)

# save T.MAE for each cell line, each run
TMAE.gk = vector(mode="list", length = length(groupPairList))

for (g in 1:length(groupPairList)) {
    # get t-statistic vector for g-th cell line
    T = lapply(Tstats, function(Tk, g) {
        sapply(Tk, function(Tki, g) {
            Tki$tstats[g]
        }, g = g, USE.NAMES = FALSE)
    }, g = g)
    
    # generate T stats box plot
    pdf(file = paste0(plotOutDir,"/t_boxplot_g", g, ".pdf"))
    plotdata <- boxplot(T, ylab = "T-statistic", outline = FALSE, main=groupNames[g])
    abline(h=0, col="pink", lwd=1)
    dev.off()

    ##
    ## Mean Square Error line plot as a function of k
    ##

    T.MSE = lapply(T, function(x) {
        sum(x^2, na.rm = TRUE) / sum(!is.na(x))
    })
    
    pdf(file = paste0(plotOutDir,"/t_mse_plot_g", g, ".pdf"))
    #plot bars first
    colIdxs = c(1:KsCnt)
    plot(colIdxs, T.MSE[colIdxs], type = "h", xlab = "", col = "gray", lty = 2,
        ylab = "T-stats MSE", bty = "n", xaxt = 'n', xlim = c(0.8, KsCnt+0.2), main=groupNames[g])
    lines(colIdxs, T.MSE[colIdxs], type = "o", lwd = 2, lty = 1, col = "deepskyblue", pch = 19)
    axis(side = 1, at = colIdxs, labels = names(T)[colIdxs])
    dev.off()

    ##
    ## Mean Absolute Error line plot as a function of k
    ##
    T.MAE = lapply(T, function(x) {
        sum(abs(x), na.rm = TRUE) / sum(!is.na(x))
    })

    pdf(file = paste0(plotOutDir, "/t_mae_plot_g", g, ".pdf"))
    plot(colIdxs, T.MAE[colIdxs], type="h", xlab= "", col = "gray", lty=2,
      ylab="T-stats MAE", bty = "n", xaxt ='n', xlim = c(0.8, KsCnt+0.2), main = groupNames[g])

    colIdxs=c(1:KsCnt)
    lines(colIdxs, T.MAE[colIdxs], type = "o", lwd = 2, lty = 1, col = "deepskyblue", pch = 19)
    axis(side = 1, at = colIdxs, labels = names(T)[colIdxs])
    dev.off()

    TMAE.gk[[g]] = T.MAE
}


##
## Median Abosulte Error comparison for different Ks
##
KF_runId = length(config_names)
noRuv_runId = 1
RuvK1_runId = 2
RuvK2_runId = 3
RuvK3_runId = 4

runIDs = c(noRuv_runId, KF_runId, RuvK1_runId, RuvK2_runId, RuvK3_runId)
groupCnt = length(groupPairList)
for(i in 1:(length(runIDs)-1)) {
    for(j in (i+1):length(runIDs)) {
        run_A = runIDs[i]
        run_B = runIDs[j]
        name_A = names(Tstats)[run_A]
        name_B = names(Tstats)[run_B]

        TMAEs_A = rep(0, groupCnt)
        TMAEs_B = rep(0, groupCnt)

        for(g in 1:groupCnt) {
            TMAEs_A[g] = TMAE.gk[[g]][[run_A]]
            TMAEs_B[g] = TMAE.gk[[g]][[run_B]]
        }
        print(TMAEs_A)
        print(TMAEs_B)

        max_v = max(c(TMAEs_A, TMAEs_B)) *1.1
        min_v = min(c(TMAEs_A, TMAEs_B)) *1.1
        lessCnt = sum(TMAEs_B < TMAEs_A)
        btest = binom.test(lessCnt, length(TMAEs_A), p = 0.5, alternative="two.sided")
        pVal = btest$p.value
        pdf(file = paste0(plotOutDir, "/t_mae_x_", name_A, "_y_", name_B, ".pdf"))
        plot(c(min_v,max_v), c(min_v,max_v), type="n", xlab=name_A, ylab=name_B, main=paste0("p-val ", pVal))
        abline(a=0, b=1, col="gray", lwd=1, lty = 2)
        points(TMAEs_A, TMAEs_B, col = adjustcolor("deepskyblue",1), pch=20, cex=2.5)
        dev.off()
    }
}
