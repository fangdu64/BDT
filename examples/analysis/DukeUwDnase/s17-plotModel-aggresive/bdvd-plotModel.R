rm(list=ls())
library("bdt")
library(latticeExtra)

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

exportSampleIds = 1:425 # export all columns
need_export = TRUE
num_threads = 24
rowIdxFrom = 10010
rowIdxTo = 11000

unwanted_factors = 3 #set k=3
known_factors =    0 #not using any known factor
pdf_width = 12
pdf_height = 24
exportList = list(
    list(outName = 'Y', component = 'signal+random', unwanted_factors = 0, known_factors = 0),
    list(outName = 'Xb', component = 'signal', unwanted_factors = unwanted_factors, known_factors = known_factors),
    list(outName = 'Xb+e', component = 'signal+random', unwanted_factors = unwanted_factors, known_factors = known_factors),
    list(outName = 'Wa', component = 'artifact', unwanted_factors = unwanted_factors, known_factors = known_factors),
    list(outName = 'e', component = 'random', unwanted_factors = unwanted_factors, known_factors = known_factors))

plotOutDir = thisScriptDir

for (i in 1:length(exportList)) {
    export = exportList[[i]]
    outDir = paste0(thisScriptDir, '/out_', export$outName)
    if (need_export) {
        exportRet = bdvdExport(
            bdt_home = bdtHome,
            thread_num = num_threads,
            mem_size = 16000,
            column_ids = exportSampleIds,
            bdvd_dir = paste0(thisScriptDir, '/../s02-bdvd/out'),
            component = export$component,
            artifact_detection = 'aggressive',
            unwanted_factors = export$unwanted_factors,
            known_factors = export$known_factors,
            rowidx_from = rowIdxFrom,
            rowidx_to = rowIdxTo,
            out = outDir)
    } else {
        exportRet = readBdvdExportOutput(outDir)
    }

    exportMat = exportRet$mats[[1]]
    mat = readMat(exportMat)
    colnames(mat) = exportMat$colNames
    A = mat
    row.ord = dim(A)[1]:1
    col.ord = 1:dim(A)[2]
    A = A[row.ord, col.ord]
    A = t(A)

    pdf(file = paste0(plotOutDir, "/", export$outName, "_plot.pdf"), width = pdf_width, height= pdf_height)
    print(levelplot(A,
        scales = list(x = list(draw=F), y = list(draw=F)),
        main = NA,
        at = unique(c(seq(-2, -1,length = 4),
            seq(-1, 8, length =100),
            seq(8,  10, length = 4))),
        colorkey = FALSE,
        col.regions = colorRampPalette(c("blue", "white", "red"))(1e3),
        xlab="",
        ylab=""))
    dev.off()

    if(export$outName == "Xb") {
        A = mat
        BL= matrix(0, nrow(A), ncol(A))
        for(i in 1:nrow(A)) {
            BL[i,] = mean(A[i,])
        }

        WL = A-BL
        row.ord = dim(BL)[1]:1
        col.ord = 1:dim(BL)[2]
        BL = BL[row.ord, col.ord]
        BL = t(BL)

        pdf(file = paste0(plotOutDir, "/", export$outName, "_BL_plot.pdf"), width = pdf_width, height= pdf_height)
        print(levelplot(BL,
            scales = list(x = list(draw=F), y = list(draw=F)),
            main = NA,
            at = unique(c(seq(-2, -1,length=4),
                seq(-1, 8,length=100),
                seq(8, 10,length=4))),
            colorkey = FALSE,
            col.regions = colorRampPalette(c("blue", "white", "red"))(1e3),
            xlab="",
            ylab=""))
        dev.off()

        row.ord = dim(WL)[1]:1
        col.ord = 1:dim(WL)[2]
        WL = WL[row.ord, col.ord]
        WL = t(WL)
        pdf(file = paste0(plotOutDir, "/", export$outName, "_WL_plot.pdf"), width = pdf_width, height= pdf_height)
        print(levelplot(WL,
            scales = list(x = list(draw=F), y = list(draw=F)),
            main=NA,
            at=unique(c(seq(-2, -1, length=4),
                seq(-1, 8, length=100),
                seq(8, 10,length=4))),
            colorkey=FALSE,
            col.regions = colorRampPalette(c("blue", "white", "red"))(1e3),
            xlab="",
            ylab=""))
        dev.off()
    }
}
