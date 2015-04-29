rm(list=ls())
library("bdt")
bdt_home = 'C:/work/BDT/build/windows/install'
bdtDatasetsDir = 'C:/work/bdtDatasets'
thisScriptDir = 'C:/work/BDT/examples/windows/bigMat'

bigMat(
    data = paste0("text-mat@",bdtDatasetsDir,"/txtMat/dnase_test.txt"),
    nrow = 92554,
    ncol = 45,
    out = paste0(thisScriptDir,"/01-out"))
