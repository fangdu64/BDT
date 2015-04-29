rm(list=ls())
library("bdt")
bdt_home = 'D:/BDT/build/windows/install'
bdtDatasetsDir = 'D:/bdtDatasets'
thisScriptDir = 'D:/BDT/examples/windows/bigKMeans'
a = bigKmeans(
    bdt_home = "D:/BDT/build/windows/install",
    input = paste0("text-mat@",bdtDatasetsDir,"/txtMat/dnase_test.txt"),
    row_cnt = 92554,
    col_cnt = 45,
    k = 100,
    thread_num = 4,
    dist_type = 'Euclidean',
    max_iter = 100,
    min_expchange = 0.0001,
    out = paste0(thisScriptDir,"/01-out"))