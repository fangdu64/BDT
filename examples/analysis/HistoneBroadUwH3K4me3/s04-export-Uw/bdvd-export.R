rm(list=ls())
library("bdt")

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

## the Uw samples
exportColumns = 40:160

ret = bdvdExport(
    bdt_home = bdtHome,
    column_ids = exportColumns,
    bdvd_dir = paste0(thisScriptDir, '/../s02-bdvd/out'),
    component = 'signal+random',
    unwanted_factors = 0, ## no RUV
    rowidxs_input = paste0("text-rowids@", thisScriptDir, '/../s03-signalBins/out/rowIdxs.txt'),
    rowidxs_index_base = 0,
    out = paste0(thisScriptDir,"/out"))
