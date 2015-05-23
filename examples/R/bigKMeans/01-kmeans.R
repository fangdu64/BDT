rm(list=ls())
library("bdt")

bdtDatasetsDir = 'D:/bdtDatasets'
thisScriptDir = 'D:/BDT/examples/R/bigKMeans'
bdt_home = 'D:/BDT/build/windows/install'

ret = bigKmeans(
    bdt_home = bdt_home,
    data_input = paste0("text-mat@",bdtDatasetsDir,"/txtMat/dnase_test.txt"),
    data_nrow = 92554,
    data_ncol = 45,
    k = 100,
    thread_num = 4,
    dist_type = 'Euclidean',
    max_iter = 100,
    min_expchange = 0.0001,
    out = paste0(thisScriptDir,"/01-out"))

clusterAssignments = readIntVec(ret$clusterAssignmentVec) 

centroids = readMat(ret$centroidsMat) 