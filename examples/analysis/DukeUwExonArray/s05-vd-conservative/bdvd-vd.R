rm(list=ls())
library("bdt")
library(parallel)
library(latticeExtra)

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))



pdf_width = 18
pdf_height = 16

num_threads = 4
unwanted_factors = c(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0)
known_factors =    c(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1)

need_vd = TRUE

if (need_vd) {
    vdTbl = bdvdVd(
        bdt_home = bdtHome,
        thread_num = num_threads,
        mem_size = 2000,
        bdvd_dir = paste0(thisScriptDir, '/../s01-bdvd/out'),
        artifact_detection = 'conservative',
        unwanted_factors = unwanted_factors,
        known_factors = known_factors,
        out = paste0(thisScriptDir,"/out"))
} else {
    vdTbl = readBdvdVdOutput(paste0(thisScriptDir,"/out"))
}

outPdf = paste0(thisScriptDir,"/out/vd.pdf")
plotVD(outPdf, vdTbl)