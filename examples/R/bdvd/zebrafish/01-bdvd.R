rm(list=ls())
library("bdt")

thisScriptDir = 'C:/work/BDT/examples/R/bdvd/zebrafish'
## thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

ret = bdvd(
    bdt_home = bdtHome,
    data_input = paste0("binary-mat@",bdtDatasetsDir,"/zebrafish/data.bfv"),
    data_nrow = 20865,
    data_ncol = 6,
    data_col_names = c('Ctl1','Ctl3','Ctl5','Trt9','Trt11','Trt13'),
	pre_normalization = 'column-sum',
	common_column_sum = 'median',
	sample_groups = list(c(1,2,3),c(4,5,6)),
    ruv_type = 'ruvg',
    control_rows_method = 'specified-rows',
    ctrl_rows_input = paste0("text-rowids@",bdtDatasetsDir,"/zebrafish/control-rows.txt"),
    permutation_num = 100,
    out = paste0(thisScriptDir,"/01-out"))

eigenValues = readVec(ret$eigenValues)
eigenVectors = readMat(ret$eigenVectors)
permutatedEigenValues = readMat(ret$permutatedEigenValues)
Wt = readMat(ret$Wt)