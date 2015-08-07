rm(list=ls())
library("bdt")

thisScriptDir = 'C:/work/BDT/examples/R/bigKMeans'
## thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../config/bdt_path.R'))

ret = bigKmeans(
    bdt_home = bdtHome,
    data_input = paste0("text-mat@",bdtDatasetsDir,"/txtMat/dnase_test.txt"),
    data_nrow = 92554,
    data_ncol = 45,
    k = 200,
    thread_num = 2,
    dist_type = 'Euclidean',
    max_iter = 100,
    min_expchange = 0.0001,
    slave_num = 1,
    out = paste0(thisScriptDir,"/02-master-out"))

clusterAssignments = readIntVec(ret$clusterAssignmentVec) 

centroids = readMat(ret$centroidsMat) 