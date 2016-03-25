rm(list=ls())
library("bdt")

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

sampleGroups=list(
    c(1, 2),
    c(3, 4),
    c(5, 6),
    c(7, 8, 71, 72),
    c(9, 10),
    c(11, 12, 102, 103),
    c(13, 14, 104, 105),
    c(15, 16, 111, 112),
    c(17, 18),
    c(19, 20),
    c(21, 22, 23, 123, 124),
    c(24, 25, 129, 130),
    c(26, 27, 135),
    c(28, 29),
    c(30, 31),
    c(32, 33, 34, 139, 140),
    c(35, 36, 141, 142),
    c(37, 38, 39),
    c(40, 41),
    c(42, 43),
    c(44, 45),
    c(46, 47),
    c(48, 49),
    c(50, 51),
    c(52, 53),
    c(54, 55),
    c(56, 57),
    c(58, 59),
    c(60, 61),
    c(62),
    c(63, 64),
    c(65, 66),
    c(67, 68),
    c(69),
    c(70),
    c(73, 74),
    c(75, 76),
    c(77, 78),
    c(79, 80),
    c(81, 82),
    c(83, 84),
    c(85, 86),
    c(87, 88),
    c(89, 90),
    c(91),
    c(92, 93),
    c(94, 95),
    c(96, 97),
    c(98, 99),
    c(100, 101),
    c(106),
    c(107, 108),
    c(109, 110),
    c(113, 114),
    c(115, 116),
    c(117, 118),
    c(119, 120),
    c(121, 122),
    c(125, 126),
    c(127, 128),
    c(131, 132),
    c(133, 134),
    c(136),
    c(137, 138),
    c(143, 144),
    c(145, 146),
    c(147, 148),
    c(149, 150),
    c(151, 152),
    c(153, 154),
    c(155, 156),
    c(157, 158),
    c(159, 160))

broadSampleCnt = 39
uwSampleCnt = 121
knownFactors = list(c(rep.int(1, broadSampleCnt), rep.int(0, uwSampleCnt)))

need_run_bdvd = TRUE
if (need_run_bdvd) {
    bdvdRet = bdvd(
        bdt_home = bdtHome,
        thread_num = 24,
        mem_size = 16000,
        data_input = paste0('bigmat@', thisScriptDir, '/../s01-bam2mat/out'),
        pre_normalization = 'column-sum', #use column-sum median
        sample_groups = sampleGroups,
        known_factors = knownFactors,
        ruv_type = 'ruvs',
        control_rows_method = 'weak-signal',
        weak_signal_lb = 1, # no up-bound
        ruv_rowwise_adjust = 'unitary-length',
        permutation_num = 1,
        out = paste0(thisScriptDir, "/out"))
} else {
    bdvdRet = readBdvdOutput(paste0(thisScriptDir, "/out"))
}

plotOutDir = paste0(thisScriptDir, "/out")


eigenValues = readVec(bdvdRet$eigenValues)
eigenVectors = readMat(bdvdRet$eigenVectors)
permutatedEigenValues = readMat(bdvdRet$permutatedEigenValues)
Wt = readMat(bdvdRet$Wt)

e = 0.000001
eigenValues[which(eigenValues<=e)]=0

L = nrow(eigenValues)

T = rep(0, L)
for(k in 1:L){
	T[k] = eigenValues[k] / sum(eigenValues)
}

maxK = 20
pdf(file = paste0(plotOutDir, "/k_evalueation.pdf"))
plotdata <- plot(1:L, T, type = "h", xlab = "", col = "gray", lty = 2,
     ylab = "Proportion", bty = "n", xaxt = 'n', xlim = c(0.8, maxK))

lines(1:L, T, type = "o", lwd = 2, lty = 1, col = "deepskyblue", pch = 19)
axis(side = 1, at = 1:L, labels = as.character(1:L))

##
## null statistics
##

B = ncol(permutatedEigenValues)
T_0= vector(mode="list", length=L)
for(k in 1:L) {
    T_0[[k]] = rep(0, B);
}

for(b in 1:B){
    pev = permutatedEigenValues[,b];
    pev[which(eigenValues<=e)]=0
    for(k in 1:L) {
        T_0[[k]][b] = pev[k] / sum(pev)
    }
}

##
## box plot for null statistics
##
plotdata <- boxplot(T_0, add=TRUE, outline = FALSE)
dev.off()