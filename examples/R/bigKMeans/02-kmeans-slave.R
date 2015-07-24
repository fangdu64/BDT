rm(list=ls())
library("bdt")

thisScriptDir = 'C:/work/BDT/examples/R/bigKMeans'
bdt_home = 'C:/work/BDT/build/windows/install'

bigKmeansC(
    bdt_home = bdt_home,
    thread_num = 2,
    master_host = 'localhost',
    master_port = 50662,
    out = paste0(thisScriptDir,"/02-slave-out"))
