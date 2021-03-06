rm(list=ls())
library("bdt")

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

sampleGroups=list(
    c(1, 2),
    c(3, 4, 219, 220),
    c(5, 6),
    c(7, 8),
    c(9, 10),
    c(11, 12),
    c(13, 14),
    c(15, 16),
    c(17, 18),
    c(19, 20, 21),
    c(22, 23),
    c(24, 25),
    c(26, 27),
    c(28, 29),
    c(30, 31),
    c(32, 33),
    c(34, 35),
    c(36, 37, 38),
    c(39, 40, 41),
    c(42, 43, 44),
    c(45, 46),
    c(47, 48),
    c(49, 50),
    c(51, 52),
    c(53, 54),
    c(55, 56, 57, 58, 59, 254, 255),
    c(60, 61),
    c(62, 63),
    c(64, 65),
    c(66, 67),
    c(68, 69, 70),
    c(71, 72),
    c(73, 74),
    c(75, 76),
    c(77, 78),
    c(79, 80, 256),
    c(81, 82, 83, 257, 258),
    c(84, 85),
    c(86, 87),
    c(88, 89),
    c(90, 91, 92, 292, 293),
    c(93, 94),
    c(95, 96),
    c(97, 98, 99, 294, 295),
    c(100, 101, 306, 307),
    c(102, 103),
    c(104, 105, 106, 345, 346),
    c(107, 108),
    c(109, 110, 111),
    c(112, 113, 114, 347, 348),
    c(115, 116),
    c(117, 118),
    c(119, 120),
    c(121, 122, 349, 350),
    c(123, 124),
    c(125, 126),
    c(127, 128, 129),
    c(130, 131, 132),
    c(133, 134),
    c(135, 136),
    c(137, 138, 139, 355, 356),
    c(140, 141, 142),
    c(143, 144, 145),
    c(146, 147),
    c(148, 149, 150),
    c(151, 152, 153),
    c(154, 155, 156, 361, 362),
    c(157, 158),
    c(159, 160, 365, 366),
    c(161, 162, 163),
    c(164, 165),
    c(166, 167),
    c(168, 169, 170),
    c(171, 172),
    c(173, 174),
    c(175, 176),
    c(177, 178),
    c(179, 180),
    c(181, 182),
    c(183, 184),
    c(185, 186, 384, 385),
    c(187, 188, 189),
    c(190, 191, 192),
    c(193, 194, 195),
    c(196, 197, 198),
    c(199, 200),
    c(201, 202),
    c(203, 204),
    c(205, 206),
    c(207, 208),
    c(209, 210),
    c(211, 212, 406, 407),
    c(213, 214),
    c(215, 216),
    c(217, 218),
    c(221, 222),
    c(223, 224),
    c(225, 226),
    c(227, 228),
    c(229, 230),
    c(231, 232),
    c(233, 234),
    c(235, 236),
    c(237, 238),
    c(239, 240),
    c(241),
    c(242),
    c(243),
    c(244),
    c(245, 246),
    c(247, 248),
    c(249, 250),
    c(251),
    c(252, 253),
    c(259, 260),
    c(261),
    c(262, 263),
    c(264),
    c(265, 266),
    c(267, 268),
    c(269, 270),
    c(271, 272),
    c(273, 274),
    c(275),
    c(276, 277),
    c(278, 279),
    c(280, 281),
    c(282, 283),
    c(284, 285),
    c(286, 287),
    c(288, 289),
    c(290, 291),
    c(296, 297),
    c(298, 299),
    c(300, 301),
    c(302, 303),
    c(304, 305),
    c(308, 309),
    c(310, 311),
    c(312, 313),
    c(314, 315),
    c(316, 317),
    c(318, 319),
    c(320, 321),
    c(322, 323),
    c(324, 325),
    c(326, 327),
    c(328),
    c(329, 330),
    c(331, 332),
    c(333, 334),
    c(335, 336),
    c(337, 338),
    c(339, 340),
    c(341, 342),
    c(343),
    c(344),
    c(351, 352),
    c(353, 354),
    c(357, 358),
    c(359, 360),
    c(363, 364),
    c(367, 368),
    c(369, 370),
    c(371),
    c(372, 373),
    c(374, 375),
    c(376, 377),
    c(378, 379),
    c(380, 381),
    c(382, 383),
    c(386, 387),
    c(388, 389),
    c(390, 391),
    c(392, 393),
    c(394, 395),
    c(396, 397),
    c(398, 399),
    c(400, 401),
    c(402, 403),
    c(404, 405),
    c(408),
    c(409, 410),
    c(411),
    c(412, 413),
    c(414, 415),
    c(416),
    c(417),
    c(418),
    c(419),
    c(420, 421),
    c(422, 423),
    c(424, 425))

dukeSampleCnt = 218
uwSampleCnt = 207
knownFactors = list(c(rep.int(1, dukeSampleCnt), rep.int(0, uwSampleCnt)))

need_run_bdvd = FALSE
if (need_run_bdvd) {
    bdvdRet = bdvd(
        bdt_home = bdtHome,
        thread_num = 40,
        mem_size = 16000,
        data_input = paste0('bigmat@', thisScriptDir, '/../s01-bam2mat/out'),
        pre_normalization = 'column-sum',
        common_column_sum = 60000000,
        sample_groups = sampleGroups,
        known_factors = knownFactors,
        ruv_type = 'ruvs',
        control_rows_method = 'weak-signal',
        weak_signal_lb = 1,
        weak_signal_ub = 1000,
        ruv_rowwise_adjust = 'unitary-length',
        permutation_num = 4,
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