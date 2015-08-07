rm(list=ls())
library("bdt")

thisScriptDir = 'C:/work/BDT/examples/R/bigKMeans'
## thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../config/bdt_path.R'))

bigKmeansC(
    bdt_home = bdtHome,
    thread_num = 2,
    master_host = 'localhost',
    master_port = 50662,
    out = paste0(thisScriptDir,"/02-slave-out"))
