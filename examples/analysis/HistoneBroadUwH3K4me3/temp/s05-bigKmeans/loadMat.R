rm(list=ls())
library("bdt")

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

exportRet = readBdvdExportOutput(paste0(thisScriptDir, '/../s04-export-Uw/out'))

# dataMat is a 1552302 (bins) x 121 (samples) matrix
dataMat = readMat(exportRet$mats[[1]])
print(str(dataMat))
print(dataMat[1,])
