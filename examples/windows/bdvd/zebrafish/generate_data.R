setwd("E:/iBS/trunk/analysis/iBS.Projects/BDVD/Zebrafish/01-generateInput-R")
library(zebrafishRNASeq)
library(RSkittleBrewer)
library(genefilter)
library(RUVSeq)
library(edgeR)
library(sva)
library(ffpe)
library(RColorBrewer)
library(corrplot)
library(limma)
trop = RSkittleBrewer('tropical')

data(zfGenes)
filter = apply(zfGenes, 1, function(x) length(x[x>5])>=2)
filtered = zfGenes[filter,]
genes = rownames(filtered)[grep("^ENS", rownames(filtered))]
controls = grepl("^ERCC", rownames(filtered))
spikes =  rownames(filtered)[grep("^ERCC", rownames(filtered))]
group = as.factor(rep(c("Ctl", "Trt"), each=3))
set = newSeqExpressionSet(as.matrix(filtered),
                           phenoData = data.frame(group, row.names=colnames(filtered)))
dat0 = counts(set)

# output controls

write.table(which(controls), "control-rows.txt", sep="\t", row.names = FALSE, col.names = FALSE)

# output data
rowCnt = dim(dat0)[1]
colCnt = dim(dat0)[2]

bfvFile = "data.bfv"
totalValueCnt = rowCnt*colCnt
con=file(bfvFile, "wb")
writeBin(as.double(c(t(dat0))), con, size=8, endian = "little")
close(con)

readback=readBigMatrix(colCnt,rowCnt,bfvFile)