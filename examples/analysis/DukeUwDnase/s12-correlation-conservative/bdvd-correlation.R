rm(list=ls())
library("bdt")
library(parallel)
library("ROCR")
library(latticeExtra)

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

## some ultility functions
pairInConfiguration <- function(x, y, xs, ys) {
    matched = 0
    for (m in 1:length(xs)) {
        if (x == xs[m] && y == ys[m]) {
            matched = m
            break
        }
    }

    return (matched)
}

pairInTwoConfig <- function(x1, y1, xs1, ys1, x2, y2, xs2, ys2) {
    matched=0
    for (m in 1:length(xs1)) {
        if(x1 == xs1[m] && y1 == ys1[m] && x2 == xs2[m] && y2 == ys2[m]) {
            matched = m
            break
        }
    }

    return (matched)
}

getConfigCnt <- function(ks, ns) {
    oval = unique(ks+ns*1000) #put known factors to rightmost
    return (length(oval))
}

getConfigOrder <- function(ks, ns, k, n) {
    oval = unique(ks+ns*1000) #put known factors to rightmost
    oval = oval[order(oval)]
    v = k + n*1000
    for( i in 1:length(oval)) {
        if(v==oval[i])
            return (i)
    }
    return (0)
}

getRConfigTexts <- function(ks, ns) {
    oval=unique(ks+ns*1000) #put known factors to rightmost
    oval=oval[order(oval)]
    txts=rep("", length(oval))
    for( i in 1:length(oval)) {
        if(oval[i] >= 1000) {
            txts[i]="KF"
        } else {
            txts[i] = as.character(oval[i]%%1000)
        }
    }
    return (txts)
}

twoMatRowCor <- function(i, mat1, mat2, rowIDs1, rowIDs2) {
    r = cor(mat1[rowIDs1[i],], mat2[rowIDs2[i],])
    return (r)
}

readVectorFromTxt <- function(txtFile) {
    vec = read.table(txtFile, sep = "\t")
    vec = vec[,1]
    return (vec)
}


need_export = FALSE
num_threads = 36
## 132 cell types both in DNase and Exon dataset

## export DNase data
## use the first sample in each cell type as column id and obtain cell type level measurement 
dnaseSampleIds = c(1, 3, 9, 17, 19, 22, 28, 36, 39, 42, 49, 55, 60, 62, 68, 71, 
         73, 75, 79, 81, 84, 88, 90, 93, 95, 97, 100, 102, 104, 109, 
         112, 115, 117, 119, 121, 125, 127, 130, 137, 146, 154, 157, 
         159, 161, 164, 168, 171, 175, 177, 181, 185, 190, 201, 209, 
         215, 217, 221, 223, 225, 227, 229, 231, 233, 235, 237, 244, 
         249, 251, 252, 259, 261, 262, 264, 265, 267, 269, 271, 273, 
         275, 276, 278, 280, 282, 284, 286, 288, 290, 296, 298, 300, 
         302, 304, 308, 310, 312, 314, 316, 318, 320, 322, 324, 326, 
         328, 329, 331, 333, 335, 337, 339, 341, 351, 353, 371, 374, 
         376, 378, 380, 382, 386, 388, 390, 392, 396, 398, 400, 402, 
         404, 409, 414, 420, 422, 424)


unwanted_factors_dnase = c(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 0)
known_factors_dnase =    c(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  1)
config_names_dnase =     as.character(unwanted_factors_dnase)
# last one is for know factor only
config_names_dnase[length(config_names_dnase)] = 'KF'

if (need_export) {
    exportDNaseRet = bdvdExport(
        bdt_home = bdtHome,
        thread_num = num_threads,
        mem_size = 16000,
        column_ids = dnaseSampleIds,
        bdvd_dir = paste0(thisScriptDir, '/../s02-bdvd/out'),
        component = 'signal', #cell type level measurement 
        artifact_detection = 'conservative',
        unwanted_factors = unwanted_factors_dnase,
        known_factors = known_factors_dnase,
        rowidxs_input = paste0("text-rowids@", bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/DNase_UniqueFeatureIdxs.txt"),
        rowidxs_index_base = 0,
        out = paste0(thisScriptDir,"/Dnase"))
} else {
    exportDNaseRet = readBdvdExportOutput(paste0(thisScriptDir,"/Dnase"))
}


## export Exon data
## use the first sample in each cell type as column id and obtain cell type level measurement 
exonSampleIds = c(78, 63, 55, 54, 80, 99, 43, 121, 124, 127, 72, 5, 22, 24, 40,
         26, 28, 30, 18, 101, 51, 119, 11, 37, 86, 8, 76, 84, 45, 104, 
         48, 82, 92, 90, 14, 115, 117, 109, 1, 111, 67, 65, 39, 130, 98, 
         133, 32, 107, 57, 61, 16, 69, 35, 88, 94, 96, 204, 206, 208, 210, 
         212, 237, 277, 166, 164, 214, 153, 279, 233, 317, 319, 321, 328, 
         297, 217, 295, 280, 271, 323, 324, 219, 243, 231, 245, 221, 239, 
         223, 299, 301, 202, 282, 171, 247, 315, 251, 253, 255, 257, 259, 
         241, 249, 225, 310, 261, 265, 263, 180, 182, 303, 227, 267, 187, 
         305, 189, 273, 326, 269, 229, 235, 311, 193, 313, 290, 195, 173, 
         292, 161, 157, 176, 294, 306, 308)

unwanted_factors_exon = c(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 0)
known_factors_exon =    c(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  1)
config_names_exon =     as.character(unwanted_factors_exon)
# last one is for know factor only
config_names_exon[length(config_names_exon)] = 'KF'

# export randomly selected rows from Exon dataset
if (need_export) {
    exportExonNoiseRet = bdvdExport(
        bdt_home = bdtHome,
        thread_num = num_threads,
        mem_size = 16000,
        column_ids = exonSampleIds,
        bdvd_dir = paste0(thisScriptDir, '/../../DukeUwExonArray/s01-bdvd/out'),
        component = 'signal', #cell type level measurement 
        artifact_detection = 'conservative',
        unwanted_factors = unwanted_factors_exon,
        known_factors = known_factors_exon,
        rowidxs_input = paste0("text-rowids@", bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s02-Random-PairIdxs/Exon_UniqueFeatureIdxs.txt"),
        rowidxs_index_base = 0,
        out = paste0(thisScriptDir,"/ExonNoise"))
} else {
    exportExonNoiseRet = readBdvdExportOutput(paste0(thisScriptDir,"/ExonNoise"))
}

# export associated rows (via TSS) from Exon dataset
if (need_export) {
    exportExonSignalRet = bdvdExport(
        bdt_home = bdtHome,
        thread_num = num_threads,
        mem_size = 16000,
        column_ids = exonSampleIds,
        bdvd_dir = paste0(thisScriptDir, '/../../DukeUwExonArray/s01-bdvd/out'),
        component = 'signal', #cell type level measurement 
        artifact_detection = 'conservative',
        unwanted_factors = unwanted_factors_exon,
        known_factors = known_factors_exon,
        rowidxs_input = paste0("text-rowids@", bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/Exon_UniqueFeatureIdxs.txt"),
        rowidxs_index_base = 0,
        out = paste0(thisScriptDir,"/ExonSignal"))
} else {
    exportExonSignalRet = readBdvdExportOutput(paste0(thisScriptDir,"/ExonSignal"))
}

# 1-based row ids
rowIDs_s1 = readVectorFromTxt(paste0(bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/DNase_RowIDs.txt"))
rowIDs_s2 = readVectorFromTxt(paste0(bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/Exon_RowIDs.txt"))
rowIDs_n1 = readVectorFromTxt(paste0(bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/DNase_RowIDs.txt"))
rowIDs_n2 = readVectorFromTxt(paste0(bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s02-Random-PairIdxs/Exon_RowIDs.txt"))
rowIDs = 1:length(rowIDs_s1)


# a subset of configs are to be used for analysis
#KsMate1 = c(0, 0, 1, 2, 2, 3, 3)
#NsMate1 = c(0, 1, 0, 0, 0, 0, 0)

#KsMate2 = c(0, 0, 1, 2, 3, 2, 3)
#NsMate2 = c(0, 1, 0, 0, 0, 0, 0)

#OnewayConfig = TRUE

KsMate1 = unwanted_factors_dnase
NsMate1 = known_factors_dnase
KsMate2 = unwanted_factors_exon
NsMate2 = known_factors_exon

OnewayConfig = FALSE

N = length(KsMate1) * length(KsMate2)

if(OnewayConfig){
	N = length(KsMate1)
}

corSignals = vector(mode="list", length = N)
corNoises = vector(mode="list", length = N)

runInfos = data.frame(
    name = rep("", N),
    k1 = rep(0, N),
    n1 = rep(0, N),
    k2 = rep(0, N),
    n2 = rep(0, N),
    stringsAsFactors = FALSE)

H1H0Ratio = 1
MAX_FDR = 0.05

##
## compute correlations for signal pairs
##
n = 0
print("compute correlations for signal pairs")
for (i in 1:length(unwanted_factors_dnase)) {
    k_1 = unwanted_factors_dnase[i]
    extW_1 = known_factors_dnase[i]

    if (pairInConfiguration(k_1, extW_1, KsMate1, NsMate1 )== 0) {
        next
    }
 
    mat1 = readMat(exportDNaseRet$mats[[i]])

    for (j in 1:length(unwanted_factors_exon)) {
        k_2 = unwanted_factors_exon[j]
        extW_2 = known_factors_exon[j]

        cfg2=0
        if (OnewayConfig) {
            cfg2 = pairInTwoConfig(k_1, extW_1, KsMate1, NsMate1, k_2, extW_2, KsMate2, NsMate2)
        } else {
            cfg2 = pairInConfiguration(k_2, extW_2, KsMate2, NsMate2)
        }

        if (cfg2 == 0) {
            next
        }

        mat2 = readMat(exportExonSignalRet$mats[[j]])
        
        n = n + 1
        runInfos[n,"k1"] = k_1
        runInfos[n,"n1"] = extW_1
        runInfos[n,"k2"] = k_2
        runInfos[n,"n2"] = extW_2
        runInfos[n,"name"] = paste(config_names_dnase[i], config_names_exon[j], sep = ",")
        print(runInfos[n, "name"])
        corSignals[[n]] = mclapply(rowIDs, twoMatRowCor, mat1, mat2, rowIDs_s1, rowIDs_s2, mc.cores=num_threads)
    }
}


##
## compute correlations for background pairs
##
n = 0
print("compute correlations for background pairs")
for (i in 1:length(unwanted_factors_dnase)) {
    k_1 = unwanted_factors_dnase[i]
    extW_1 = known_factors_dnase[i]

    if (pairInConfiguration(k_1, extW_1, KsMate1, NsMate1 )== 0) {
        next
    }

    mat1 = readMat(exportDNaseRet$mats[[i]])

    for (j in 1:length(unwanted_factors_exon)) {
        k_2 = unwanted_factors_exon[j]
        extW_2 = known_factors_exon[j]

        cfg2=0
        if (OnewayConfig) {
            cfg2 = pairInTwoConfig(k_1, extW_1, KsMate1, NsMate1, k_2, extW_2, KsMate2, NsMate2)
        } else {
            cfg2 = pairInConfiguration(k_2, extW_2, KsMate2, NsMate2)
        }

        if (cfg2 == 0) {
            next
        }
        
        mat2 = readMat(exportExonNoiseRet$mats[[j]])
        
        n = n + 1
        print(runInfos[n, "name"])
        corNoises[[n]] = mclapply(rowIDs, twoMatRowCor, mat1, mat2, rowIDs_n1, rowIDs_n2, mc.cores=num_threads)
    }
}

plotOutDir = paste0(thisScriptDir, "/Dnase")

##
## AUC Table
##
runRowCnt = getConfigCnt(runInfos[,"k1"], runInfos[,"n1"])
runColCnt = getConfigCnt(runInfos[,"k2"], runInfos[,"n2"])
runAUCs = matrix(0, runRowCnt, runColCnt)

#max TPR within given FDR level
runTPRs = matrix(0, runRowCnt, runColCnt)

#max TPR within given FDR level
runSensitivity = matrix(0, runRowCnt, runColCnt)

FullRowCnt = length(corSignals[[1]])
TopCnt = min(50000, FullRowCnt)

##
## Signal and Noise density
##
for(n in 1:N) {
    signalScores = unlist(corSignals[[n]])
    noiseScores = unlist(corNoises[[n]])

    pdf(file = paste0(plotOutDir, "/sn_density_", runInfos[n,"name"], ".pdf"))
    plot(density(noiseScores, na.rm = TRUE, bw = 0.01), lwd = 3, col = "deepskyblue", xlim = c(-1, 1), ylim = c(0, 3))
    lines(density(signalScores, na.rm = TRUE), lwd = 3, col = "red")
    dev.off()
}

##
## Sensitivity plot
##
pdf(file = paste0(plotOutDir, "/accuracy.pdf"))
plot(c(0, TopCnt), c(80, 100), type = "n", xlab = "top # of pairs", ylab = "% of signal pairs", xlim = c(0, TopCnt))
colors = c("salmon4", "red2", "dodgerblue3", "darkorange1", "green2", "black")
colors = rep(colors, as.integer(N/length(colors))+1)
linetype <- rep(1, N) 
plotchar <- rep(19, N)
sens = rep(0, N)
legendTxts = rep("", N)
for(i in 1:N) {
    scores = c(unlist(corSignals[[i]]), unlist(corNoises[[i]]))
    lbs = c(rep(1,FullRowCnt), rep(0,FullRowCnt))

    oRowIDs = order(scores, decreasing = TRUE)
    scores = scores[oRowIDs[1:TopCnt]]
    lbs = lbs[oRowIDs[1:TopCnt]]
    xs = seq(from=100, to=TopCnt, by=100)
    ys = rep(1, length(xs))
    for( j in 1:length(xs)) {
        ys[j] = sum(lbs[1:xs[j]])/xs[j]
    }
    
    print(xs)
    print(ys)
    sens[i] = ys[length(xs)]
    lines(xs, ys*100, type = "l", lwd = 2, lty = linetype[i], col = colors[i], pch = plotchar[i])
    legendTxts[i] = paste0("[", runInfos[i,"name"],"], accuracy ", sprintf("%.3f",sens[i]))
}

# add a legend 
legend(200, 94, legend = legendTxts, cex = 1, col = colors, pch = plotchar, lty = linetype, bty = "n")
dev.off()

q(save="no")

##
## AUC Table
##
print(N)
for(n in 1:N) {
    scores=c(unlist(corSignals[[n]]), unlist(corNoises[[n]]))
    lbs = c(rep(1,FullRowCnt), rep(0,FullRowCnt))
    oRowIDs = order(scores, decreasing = TRUE)
    scores = scores[oRowIDs[1:TopCnt]]
    lbs = lbs[oRowIDs[1:TopCnt]]
    pred <- prediction(scores, lbs)
    perf <- performance(pred,"auc")
    runRowID = getConfigOrder(
        runInfos[,"k1"], runInfos[,"n1"],
        runInfos[n,"k1"], runInfos[n,"n1"])
    runColID = getConfigOrder(
        runInfos[,"k2"], runInfos[,"n2"],
        runInfos[n,"k2"], runInfos[n,"n2"])
    runAUCs[runRowID, runColID] = as.numeric(perf@y.values)

    perf <- performance(pred,"tpr","fpr")
    xs = as.numeric(unlist(perf@x.values)) #fpr
    ys = as.numeric(unlist(perf@y.values)) #tpr
    fdr = xs/(xs+ys*H1H0Ratio)
    runTPRs[runRowID, runColID] = max(ys[fdr<MAX_FDR], na.rm = TRUE)
}

xlabls = getConfigTexts(runInfos[,"k1"], runInfos[,"n1"])
xats=1:length(xlabls)
ylabls = getConfigTexts(runInfos[,"k2"], runInfos[,"n2"])
yats=1:length(ylabls)

##
## AUC matrix plot
##
minAUC = min(runAUCs)
maxAUC = max(runAUCs)
pdf(file = paste0(plotOutDir,"/aucs.pdf"))
levelplot(runAUCs,
    scales = list(x = list(at=xats, labels=xlabls), y = list(at=yats, labels=ylabls),tck = c(1,0)),
    main="AUC",
    colorkey = FALSE,
    xlab="DNase",
    ylab="Exon",
    at=unique(c(seq(minAUC-0.01, maxAUC+0.01,length=100))),
    col.regions = colorRampPalette(c("white", "red"))(1e2),
    panel=function(x,y,z,...) {
        panel.levelplot(x,y,z,...)
        panel.text(x, y, round(z,2))})
dev.off()


##
## TPR matrix plot
##
minTPR = min(runTPRs)
maxTPR = max(runTPRs)
pdf(file = paste0(plotOutDir, "/tprs.pdf"))
levelplot(runTPRs,
    scales = list(x = list(at=xats, labels=xlabls), y = list(at=yats, labels=ylabls),tck = c(1,0)),
    main=paste("Max TPR with FDR <",MAX_FDR,sep=""),
    colorkey = FALSE,
    xlab="DNase",
    ylab="Exon",
    at = unique(c(seq(minTPR-0.01, maxTPR+0.01,length=100))),
    col.regions = colorRampPalette(c("white", "red"))(1e2),
    panel=function(x,y,z,...) {
        panel.levelplot(x,y,z,...)
        panel.text(x, y, round(z,2))})
dev.off()

##
## ROC plot
##
pdf(file = paste0(plotOutDir,"/roc.pdf"))
plot(c(0,1), c(0,1), type="n", xlab="False positive rate", ylab="True positive rate", xlim=c(0,1))
abline(a=0, b=1, col="gray", lwd=1, lty = 2)
colors =c("salmon4", "red2", "dodgerblue3", "darkorange1", "green2", "black")
colors=rep(colors,as.integer(N/length(colors))+1)
linetype <- rep(1,N) 
plotchar <- rep(19,N)
aucs = rep(0,N)
legendTxts = rep("",N)
FullRowCnt = length(corSignals[[1]])
TopCnt = min(50000, FullRowCnt)
for(i in 1:N) {
    scores=c(unlist(corSignals[[i]]), unlist(corNoises[[i]]))
    lbs=c(rep(1,FullRowCnt), rep(0,FullRowCnt))

    oRowIDs = order(scores,decreasing = TRUE)
    scores=scores[oRowIDs[1:TopCnt]]
    lbs=lbs[oRowIDs[1:TopCnt]]
    pred <- prediction( scores, lbs)
    perf <- performance(pred,"tpr","fpr")
    xs=as.numeric(unlist(perf@x.values))
    ys=as.numeric(unlist(perf@y.values))
    perf <- performance(pred,"auc")
    aucs[i]=perf@y.values
    lines(xs, ys, type="l", lwd=2, lty=linetype[i], col=colors[i], pch=plotchar[i])
    legendTxts[i]=paste("[",runInfos[i,"name"],"], auc ",sprintf("%.2f",aucs[i]),sep="")
}

# add a legend 
legend(0.6,0.6, legend=legendTxts,
 cex=1, col=colors, pch=plotchar, lty=linetype, bty ="n")
dev.off()

##
## FDR plot
##
pdf(file = paste0(plotOutDir, "/fdr.pdf", sep=""))
plot(c(0,1), c(0,1), type="n", xlab="False discovery rate", ylab="True positive rate", xlim=c(0,1))
abline(a=0,b=1, col="gray", lwd=1, lty = 2)
#colors <- 1:N
linetype <- rep(1,N) 
plotchar <- rep(19,N)
aucs = rep(0,N)
legendTxts=rep("",N)
for(i in 1:N) {
    scores = c(unlist(corSignals[[i]]), unlist(corNoises[[i]]))
    lbs=c(rep(1,FullRowCnt), rep(0,FullRowCnt))
    oRowIDs = order(scores,decreasing = TRUE)
    scores=scores[oRowIDs[1:TopCnt]]
    lbs=lbs[oRowIDs[1:TopCnt]]
    pred <- prediction( scores, lbs)
    perf <- performance(pred,"tpr","fpr")
    xs=as.numeric(unlist(perf@x.values)) #fpr
    ys=as.numeric(unlist(perf@y.values)) #tpr
    fdr=xs/(xs+ys*H1H0Ratio)
    maxTPR=max(ys[fdr<MAX_FDR],na.rm = TRUE)
    aucs[i]=maxTPR
    xs=fdr
    #perf <- performance(pred,"auc")
    #aucs[i]=perf@y.values
    lines(xs, ys, type="l", lwd=2, lty=linetype[i], col=colors[i], pch=plotchar[i])
    legendTxts[i]=paste("[",runInfos[i,"name"],"], TPR ",sprintf("%.2f",aucs[i]),sep="")
}
# add a legend 
legend(0.6,1.05, legend=legendTxts,
 cex=1, col=colors, pch=plotchar, lty=linetype, bty ="n")
dev.off()