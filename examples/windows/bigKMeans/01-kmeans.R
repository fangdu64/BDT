rm(list=ls())
require("bdt")
bdt_home = 'C:/work/BDT/build/windows/install'
bdtDatasetsDir = 'C:/work/bdtDatasets'
thisScriptDir = 'C:/work/BDT/examples/windows/bigKMeans'
a = bdt::bigKMeans(
    bdt_home = "C:/work/BDT/build/windows/install",
    data = paste0("text-mat@",bdtDatasetsDir,"/txtMat/dnase_test.txt"),
    nrow = 92554,
    ncol = 45,
    k = 100,
    thread_num = 4,
    dist = 'Euclidean',
    max_iter = 100,
    min_expchange = 0.0001,
    out = paste0(thisScriptDir,"/01-out"))