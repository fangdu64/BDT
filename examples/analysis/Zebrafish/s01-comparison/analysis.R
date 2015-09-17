## adapted from https://github.com/jtleek/svaseq/blob/master/zebrafish.Rmd

rm(list=ls())
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
library(bdt)

## Load and process the zebrafish data
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


## Estimate latent factors with different methods

## Set null and alternative models (ignore batch)
mod1 = model.matrix(~group)
mod0 = cbind(mod1[,1])

## Estimate batch with svaseq (unsupervised)
batch_unsup_sva = svaseq(dat0,mod1,mod0,n.sv=1)$sv

## Estimate batch with svaseq (supervised)
batch_sup_sva = svaseq(dat0,mod1,mod0,controls=controls,n.sv=1)$sv

## Estimate batch with pca
ldat0 = log(dat0 + 1)
batch_pca = svd(ldat0 - rowMeans(ldat0))$v[,1]

## Estimate batch with ruv (controls known)
batch_ruv_cp <- RUVg(dat0, cIdx= spikes, k=1)$W


## Estimate batch with ruv (residuals)
## this procedure follows the RUVSeq vignette
## http://www.bioconductor.org/packages/devel/bioc/vignettes/RUVSeq/inst/doc/RUVSeq.pdf

x <- as.factor(group)
design <- model.matrix(~x)
y <- DGEList(counts=dat0, group=x)
y <- calcNormFactors(y, method="upperquartile")
y <- estimateGLMCommonDisp(y, design)
y <- estimateGLMTagwiseDisp(y, design)
fit <- glmFit(y, design)
res <- residuals(fit, type="deviance")
seqUQ <- betweenLaneNormalization(dat0, which="upper")
controls = rep(TRUE,dim(dat0)[1])
batch_ruv_res = RUVr(seqUQ,controls,k=1,res)$W


## Estimate batch with ruv empirical controls
## this procedure follows the RUVSeq vignette
## http://www.bioconductor.org/packages/devel/bioc/vignettes/RUVSeq/inst/doc/RUVSeq.pdf

y <- DGEList(counts=dat0, group=x)
y <- calcNormFactors(y, method="upperquartile")
y <- estimateGLMCommonDisp(y, design)
y <- estimateGLMTagwiseDisp(y, design)

fit <- glmFit(y, design)
lrt <- glmLRT(fit, coef=2)

controls=rank(lrt$table$LR) <= 400
batch_ruv_emp <- RUVg(dat0, controls, k=1)$W


## Estimate batch with bdvd-ruvg
thisScriptDir = 'C:/work/BDT/examples/analysis/Zebrafish/s01-comparison'
## thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

ret = bdvd(
    bdt_home = bdtHome,
    data_input = paste0("binary-mat@", bdtDatasetsDir, "/zebrafish/data.bfv"),
    data_nrow = 20865,
    data_ncol = 6,
    data_col_names = c('Ctl1','Ctl3','Ctl5','Trt9','Trt11','Trt13'),
    sample_groups = list(c(1,2,3), c(4,5,6)),
    ruv_type = 'ruvg',
    control_rows_method = 'specified-rows',
    ctrl_rows_input = paste0("text-rowids@", bdtDatasetsDir, "/zebrafish/control-rows.txt"),
    ctrl_rows_index_base = 1,
    out = paste0(thisScriptDir, "/out"))

Wt = readMat(ret$Wt)
batch_bdvd_ruvg =Wt[1,]

## Plot batch estimates

## Plot the results
plot(batch_unsup_sva,pch=19,col=trop[1], main="unsupervised sva")
plot(batch_sup_sva,pch=19,col=trop[1], main="supervised sva")
plot(batch_pca,pch=19,col=trop[2], main="pca")
plot(batch_ruv_cp,pch=19,col=trop[3], main="control probes ruv")
plot(batch_ruv_res,pch=19,col=trop[3],main="residual ruv")
plot(batch_ruv_emp,pch=19,col=trop[3], main="empirical controls ruv")
plot(batch_bdvd_ruvg,pch=19,col=trop[3], main="bdvd ruvg")

## Plot absolute correlation between batch estimates
batchEstimates = cbind(group, batch_unsup_sva, batch_sup_sva,
    batch_pca, batch_ruv_cp, batch_ruv_res, batch_ruv_emp, batch_bdvd_ruvg)
colnames(batchEstimates) = c("group","usva","ssva","pca","ruvcp","ruvres","ruvemp","bdvd-ruvg")

corr = abs(cor(batchEstimates))
cols = colorRampPalette(c(trop[2],"white",trop[1]))
par(mar=c(5,5,5,5))
corrplot(corr,method="ellipse",type="lower",col=cols(100), tl.pos="d")

## Compare results with different methods to supervised svaseq
dge <- DGEList(counts=dat0)
dge <- calcNormFactors(dge)
catplots = tstats = vector("list",8)
adj = c( "+ batch_sup_sva","+ batch_unsup_sva",
         "+ batch_ruv_cp", "+ batch_ruv_res",
         "+ batch_ruv_emp","+ batch_pca", "+ batch_bdvd_ruvg", "")


for(i in 1:8){
  design = model.matrix(as.formula(paste0("~ group",adj[i])))
  v <- voom(dge,design,plot=FALSE)
  fit <- lmFit(v,design)
  fit <- eBayes(fit)
  tstats[[i]] = abs(fit$t[,2])
  names(tstats[[i]]) = as.character(1:dim(dat0)[1])
  catplots[[i]] = CATplot(-rank(tstats[[i]]),-rank(tstats[[1]]),maxrank=400,make.plot=F)
}

plot(catplots[[2]],ylim=c(0,1),col=trop[1],lwd=3,type="l",ylab="Concordance Between Supervised svaseq and other methods",xlab="Rank")
lines(catplots[[3]],col=trop[2],lwd=3)
lines(catplots[[4]],col=trop[2],lwd=3,lty=2)
lines(catplots[[5]],col=trop[2],lwd=3,lty=3)
lines(catplots[[6]],col=trop[3],lwd=3)
lines(catplots[[7]],col=trop[4],lwd=3)


legend(200,0.5,legend=c("Unsup. svaseq","RUV CP","RUV Res.", "RUV Emp.","PCA", "Bdvd RUVg", "No adjustment"),col=trop[c(1,2,2,2,3,2,4)],lty=c(1,1,2,3,1,1,1),lwd=3)