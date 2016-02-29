rm(list=ls())
library("bdt")
library(parallel)

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

## 15 biological groups with common samples, i.e., each group contains samples from both Duke and UW
sampleGroups=list(
    c(1, 2, 3, 4, 158, 159, 160),
    c(5, 6, 7, 177, 178, 179),
    c(8, 9, 10, 155, 156),
    c(11, 12, 13, 184, 185, 186),
    c(14, 15, 169, 170),
    c(16, 17, 192),
    c(39, 197, 198),
    c(45, 46, 47, 284, 285, 286, 287),
    c(63, 64, 275, 276),
    c(67, 68, 288, 289),
    c(76, 77, 199, 200, 201),
    c(101, 102, 103, 215, 216))

exportSampleIds = unlist(sampleGroups)

need_export = TRUE
num_threads = 16
unwanted_factors = c(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50, 60, 70, 80, 0)
known_factors =    c(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  1)
config_names =     as.character(unwanted_factors)
# last one is for know factor only
config_names[length(config_names)] = 'KF'

if (need_export) {
    exportRet = bdvdExport(
        bdt_home = bdtHome,
        thread_num = num_threads,
        mem_size = 2000,
        column_ids = exportSampleIds,
        bdvd_dir = paste0(thisScriptDir, '/../s01-bdvd/out'),
        component = 'signal+random',
        artifact_detection = 'conservative',
        unwanted_factors = unwanted_factors,
        known_factors = known_factors,
        rowidx_from = 0,
        rowidx_to = 18524,
        out = paste0(thisScriptDir,"/out"))
} else {
    exportRet = readBdvdExportOutput(paste0(thisScriptDir,"/out"))
}

## use local column indexs hereafter
groupId = 0
colGroups = lapply(sampleGroups, function(x) {
    groupId  <<- groupId + 1
    return (rep(groupId, length(x)))
})

colGroups = unlist(colGroups)
colGroups=as.factor(colGroups)
gCnt=nlevels(colGroups)

KsCnt = length(unwanted_factors)
Fstats= vector(mode="list", length=KsCnt)
names(Fstats) = config_names

for (i in 1:KsCnt) {
    mat = exportRet$mats[[i]]
    colCnt = mat$colCnt
    rowCnt = mat$rowCnt
    k = unwanted_factors[i]
    extW = known_factors[i]

    print(paste0("k=", k, ",extW=", extW))

    # read in data matrix for current config
    mat = as.list(data.frame(t(readMat(mat))))
    Fstats[[i]] = mclapply(mat, function(x) {
        n.i = tapply(x, colGroups, length)
        m.i = tapply(x, colGroups, mean)
        v.i = tapply(x, colGroups, var)
        n = sum(n.i)
        wgVar=(sum((n.i - 1) * v.i)/(n - gCnt))
        if(wgVar<0.000001) {
            return (NA)
        }

        f_stats = ((sum(n.i * (m.i - mean(x))^2)/(gCnt - 1))/wgVar)
        return (f_stats)
    }, mc.cores = num_threads)
}

Fstats = lapply(Fstats, unlist, use.names = FALSE)

Fstats.mean = lapply(Fstats, mean, na.rm = TRUE)
Fstats.median = lapply(Fstats, median, na.rm = TRUE)

plotOutDir = paste0(thisScriptDir, "/out")

##
## F-stats box plot
##
pdf(file = paste0(plotOutDir, "/f_boxplot.pdf"))
plotdata <- boxplot(Fstats, ylab = "F-statistic", outline = FALSE)
dev.off()

##
## F-stats mean plot
##
pdf(file = paste0(plotOutDir, "/f_means_plot.pdf"))
#plot bars first
colIdxs=c(1:KsCnt)
plot(colIdxs, Fstats.mean[colIdxs], type = "h", xlab = "", col = "gray", lty = 2,
     ylab = "F-stats Mean", bty = "n", xaxt = 'n', xlim = c(0.8, KsCnt + 0.2))

lines(colIdxs, Fstats.mean[colIdxs], type = "o", lwd = 2, lty = 1, col = "deepskyblue", pch = 19)
axis(side = 1, at = colIdxs, labels = names(Fstats)[colIdxs])
dev.off()

##
## F-stats median plot
##
pdf(file = paste0(plotOutDir,"/f_medians_plot.pdf"))
#plot bars first
plot(colIdxs, Fstats.median[colIdxs], type = "h", xlab = "", col = "gray", lty = 2,
     ylab = "F-stats Median", bty = "n", xaxt = 'n', xlim = c(0.8, KsCnt + 0.2))

lines(colIdxs, Fstats.median[colIdxs], type="o", lwd=2,
     lty=1, col="deepskyblue", pch=19)
axis(side=1, at=colIdxs, labels=names(Fstats)[colIdxs])
dev.off()