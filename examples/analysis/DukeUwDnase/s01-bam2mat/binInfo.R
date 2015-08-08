## This is an example to get the mapping from Genome position to bin index

rm(list=ls())
library("bdt")

## thisScriptDir = getScriptDir()
thisScriptDir = '/dcs01/gcode/fdu1/repositories/BDT/examples/analysis/DukeUwDnase/s01-bam2mat'
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

## read bam2mat output
output = readBigMatOutput(paste0(thisScriptDir,"/out"))

## bin map info
binMap = output.binMap
print(binMap)

## get Genome position by bin index
## bin index is 0-based
rowIdxs = c(101, 5055549, 11946429, 16950329, 23039435, 26701809, 30362725, 30362945)
for (binIdx in rowIdxs) {
    ret = getGenomePosByBinIdx(binMap, binIdx)
    ## bpFrom and bpTo are 0-based
    print(paste0('chromosome: ',ret$ref, ', bp:[',ret$bpFrom, ', ',ret$bpTo,')'))
}

## get bin index by Genome position
refs = c("chr1", "chr3", "chr6")
bps = c(10140, 13104810, 132100750)
for (i in 1:length(refs)) {
    binIdx = getBinIdxByGenomePos(binMap, refs[i], bps[i])
    print(paste0('rowIdx: ', binIdx))
}

