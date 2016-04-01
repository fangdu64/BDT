rm(list=ls())
library("bdt")

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

ret = bigKmeans(
    bdt_home = bdtHome,
    data_input = paste0("output@", thisScriptDir, '/../s04-export-Uw/out'),
    k = 100,
    thread_num = 20,
    dist_type = 'Correlation',
    max_iter = 100,
    min_expchange = 0.0001,
    out = paste0(thisScriptDir,'/out'))
