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

getConfigTexts <- function(ks, ns) {
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

#
# prediction
#
twoMatPredictionTestErrors<-function(i, mat_X, mat_Y, pmPredictorRowIDs, pmResponseRowIDs, colIDs_train, colIDs_test)
{
    Y_rowID = pmResponseRowIDs[i]
    X_rowIDs= pmPredictorRowIDs[[i]]
    Y_train = mat_Y[Y_rowID, colIDs_train]
    X_train = t(mat_X[X_rowIDs, colIDs_train])
    data_train = data.frame(y=Y_train, X_train)

    Y_test = mat_Y[Y_rowID, colIDs_test]
    X_test = t(mat_X[X_rowIDs, colIDs_test])

    fit = lm(y~., data = data_train)
    Y_predict = predict(fit, data.frame(X_test))
    #a vector
    absErrors = abs(Y_predict-Y_test)
    return (absErrors)
}


readVectorFromTxt <- function(txtFile) {
    vec = read.table(txtFile, sep = "\t")
    vec = vec[,1]
    return (vec)
}

need_export = TRUE
num_threads = 24
## 132 cell types both in DNase and Exon dataset

## export DNase data as Mat X
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
        artifact_detection = 'aggressive',
        unwanted_factors = unwanted_factors_dnase,
        known_factors = known_factors_dnase,
        rowidxs_input = paste0("text-rowids@", bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/DNase_UniqueFeatureIdxs.txt"),
        rowidxs_index_base = 0,
        out = paste0(thisScriptDir,"/Dnase"))
} else {
    exportDNaseRet = readBdvdExportOutput(paste0(thisScriptDir,"/Dnase"))
}


## export Exon data as Mat Y
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

if (need_export) {
    exportExonRet = bdvdExport(
        bdt_home = bdtHome,
        thread_num = num_threads,
        mem_size = 16000,
        column_ids = exonSampleIds,
        bdvd_dir = paste0(thisScriptDir, '/../../DukeUwExonArray/s01-bdvd/out'),
        component = 'signal', #cell type level measurement 
        artifact_detection = 'conservative', # Y always use conservative for fair comparison
        unwanted_factors = unwanted_factors_exon,
        known_factors = known_factors_exon,
        rowidxs_input = paste0("text-rowids@", bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/Exon_UniqueFeatureIdxs.txt"),
        rowidxs_index_base = 0,
        out = paste0(thisScriptDir,"/Exon"))
} else {
    exportExonRet = readBdvdExportOutput(paste0(thisScriptDir,"/Exon"))
}


# 1-based row ids
rowIDs_X = readVectorFromTxt(paste0(bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/DNase_RowIDs.txt"))
rowIDs_Y = readVectorFromTxt(paste0(bdtDatasetsDir, "/DNaseExonCorrelation/100bp/s01-TSS-PairIdxs/Exon_RowIDs.txt"))

#DNase UW only Cell Lines, as training set, 1-based, colIDs are adjusted in a way to use exported results
colIDs_train = c(57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75,
                76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
                95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
                111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
                125, 126, 127, 128, 129, 130, 131, 132)

#DNase Duke only Cell Lines, as training set, 1-based, colIDs are adjusted in a way to use exported results
colIDs_test = c(1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 21, 22, 24, 25, 28, 30,
               32, 33, 34,36, 37, 38, 40, 42, 44, 45, 46, 47, 48, 49, 50, 52, 53, 54, 55, 56)

min_predictorCnt = 20
               
# a subset of configs are to be used for analysis
KsX = c(0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30)
NsX = c(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)

# Exon as Y, using K = 2 to remove unwanted factors
# KsY = c(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2)
# NsY = c(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)

# Exon as Y, using lab origin to remove unwanted factors
KsY = c(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0)
NsY = c(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  1,  1)

OnewayConfig = TRUE


N = length(KsY) * length(KsX)

if(OnewayConfig){
	N = length(KsY)
}


##
## Establish response (row) and predictors (rows)
##

pmResponseRowIDs = unique(rowIDs_Y)

# number of predictive models
pmCnt = length(pmResponseRowIDs)

# for each predictive model, we need its predictors
pmPredictorRowIDs = vector(mode = "list", length = pmCnt)
for (i in 1:pmCnt) {
    rowID_Y = pmResponseRowIDs[i]
    pmPredictorRowIDs[[i]] = rowIDs_X[which(rowIDs_Y == rowID_Y)]
}

pmIDs = which(lapply(pmPredictorRowIDs, length) >= min_predictorCnt)

print(paste0("Total # models:", pmCnt, ", selected models:", length(pmIDs)))

RMSE.test = vector(mode="list", length=N)
names(RMSE.test) = config_names_dnase

AbsErrors.test = vector(mode="list", length=N)
names(AbsErrors.test) = config_names_dnase


runInfos = data.frame(
    name = rep("", N),
    kx = rep(0, N),
    nx = rep(0, N),
    ky = rep(0, N),
    ny = rep(0, N),
    stringsAsFactors = FALSE)

##
## identify Y rows by correlation
##
n_noruv = 0
n = 0

for (i in 1:length(unwanted_factors_dnase)) {
    k_x = unwanted_factors_dnase[i]
    extW_x = known_factors_dnase[i]

    if (pairInConfiguration(k_x, extW_x, KsX, NsX )== 0) {
        next
    }
 
    mat_X = readMat(exportDNaseRet$mats[[i]])

    for (j in 1:length(unwanted_factors_exon)) {
        k_y = unwanted_factors_exon[j]
        extW_y = known_factors_exon[j]

        cfg2=0
        if (OnewayConfig) {
            cfg2 = pairInTwoConfig(k_x, extW_x, KsX, NsX, k_y, extW_y, KsY, NsY)
        } else {
            cfg2 = pairInConfiguration(k_y, extW_y, KsY, NsY)
        }

        if (cfg2 == 0) {
            next
        }

        mat_Y = readMat(exportExonRet$mats[[j]])
        
        n = n + 1
        if (k_x == 0 && extW_x == 0) {
            n_noruv=n
        }
        
        runInfos[n, "kx"]=k_x
        runInfos[n, "nx"]=extW_x
        runInfos[n, "ky"]=k_y
        runInfos[n, "ny"]=extW_y
        runInfos[n,"name"] = paste(config_names_dnase[i], config_names_exon[j], sep = ",")
        print(runInfos[n, "name"])
        AbsErrors.test[[n]] = mclapply(pmIDs, twoMatPredictionTestErrors, mat_X, mat_Y, pmPredictorRowIDs, pmResponseRowIDs, colIDs_train, colIDs_test, mc.cores = num_threads)
        RMSE.test[[n]] = lapply(AbsErrors.test[[n]], mean)
    }
}

absError_0 = unlist(AbsErrors.test[[n_noruv]], use.names = FALSE)
pairedErrors = lapply(AbsErrors.test, function(x) {
    pe = unlist(x, use.names = FALSE) - absError_0
    return (pe)
    })
rm(absError_0, AbsErrors.test)

RMSE.test = lapply(RMSE.test, unlist, use.names = FALSE)

names(pairedErrors) = config_names_dnase

plotOutDir = paste0(thisScriptDir, "/Dnase")
##
## RMSE.test box plot
##
pdf(file = paste0(plotOutDir, "/rmse_test_boxplot.pdf"))
plotdata <- boxplot(RMSE.test, ylab = "RMSE", outline = FALSE)
dev.off()

##
## paired test errors box plot
##
pdf(file = paste0(plotOutDir, "/paired_testerrors_boxplot.pdf"))
plotdata <- boxplot(pairedErrors[2:length(pairedErrors)], ylab = "Relative Errors", outline = FALSE)
abline(h=0, col="pink", lwd=2,lty=3)
dev.off()
