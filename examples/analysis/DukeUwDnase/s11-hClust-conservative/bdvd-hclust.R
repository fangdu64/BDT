rm(list=ls())
library("bdt")
library(parallel)
library(latticeExtra)

thisScriptDir = getScriptDir()
source(paste0(thisScriptDir, '/../../../config/bdt_path.R'))

##
## record which members (colIDs) are merged at each hclust level
##
getHClustMergeFlags <- function(hclust) {
    n=length(hclust$height) + 1
    mergeMemberFlags=matrix(0,n-1,n)
    mg=hclust$merge
    for (i in 1:(n-1)) {
        for( j in 1:2) {
            v=mg[i,j]
            if ( v < 0) {
                # leaf node
                mergeMemberFlags[i,(-v)] = 1
            } else {
                mergeMemberFlags[i,] = mergeMemberFlags[i,] + mergeMemberFlags[v,]
            }
        }
    }

    return(mergeMemberFlags)
}

##
## hclust score
##
getHClustScore <- function(mergeMemberFlags, memberIDs, nonmemberIDs, method = "strict") {
    memberCnt = length(memberIDs)
    if (memberCnt < 2) {
        return(1)
    }
    
    METHODS <- c("strict", "precision")
    method <- pmatch(method, METHODS)
    
    n=ncol(mergeMemberFlags)
    cs=0
    cs_cnt=0.0
    for (i in 1:(memberCnt-1)) {
        for(j in (i+1):memberCnt) {
            mergeNonmemberCnt = 0
            mergeMemberCnt = 0
            cs_cnt = cs_cnt + 1
            mi = memberIDs[i]
            mj = memberIDs[j]
            for(k in 1:(n-1)) {
                if (mergeMemberFlags[k,mi] == 1 && mergeMemberFlags[k,mj] == 1) {
                    #at which level, i and j meet together
                    mergeNonmemberCnt = sum(mergeMemberFlags[k, nonmemberIDs])
                    mergeMemberCnt = sum(mergeMemberFlags[k, memberIDs])
                    break
                }
            }

            mscore=0
            if (method==1) {
                if (mergeNonmemberCnt==0) {
                    mscore=1
                }
            } else if (method == 2) {
                mscore = mergeMemberCnt / (mergeMemberCnt + mergeNonmemberCnt)
            }

            cs=cs+mscore
        }
    }

    return(cs/cs_cnt)
}


pdf_width = 18
pdf_height = 16
## 15 biological groups with common samples, i.e., each group contains samples from both Duke and UW
sampleGroups=list(
    c(3, 4, 219, 220),
    c(55, 56, 57, 58, 59, 254, 255),
    c(79, 80, 256),
    c(81, 82, 83, 257, 258),
    c(90, 91, 92, 292, 293),
    c(97, 98, 99, 294, 295),
    c(100, 101, 306, 307),
    c(104, 105, 106, 345, 346),
    c(112, 113, 114, 347, 348),
    c(121, 122, 349, 350),
    c(137, 138, 139, 355, 356),
    c(154, 155, 156, 361, 362),
    c(159, 160, 365, 366),
    c(185, 186, 384, 385),
    c(211, 212, 406, 407))

exportSampleIds = unlist(sampleGroups)
sampleLabs = rep('UW', length(exportSampleIds))
sampleLabs[which(exportSampleIds <= 218)] = 'Duke'
sampleColors = sapply(sampleLabs, function(x) {
    col = 1;
    if (x == 'UW') {
        col = 2
    }

    return (col)
}, USE.NAMES = FALSE);

# export randomly selected 250K bins with signal
num_threads = 24
unwanted_factors = c(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50, 60, 70, 80, 0)
known_factors =    c(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  1)
config_names =     as.character(unwanted_factors)
# last one is for know factor only
config_names[length(config_names)] = 'KF'

# reuse the existing exported results from s08-Fstats-conservative
exportRet = readBdvdExportOutput(paste0(thisScriptDir, "/../s08-Fstats-conservative/out"))


## use local column indexs hereafter
groupId = 0
colGroups = lapply(sampleGroups, function(x) {
    groupId  <<- groupId + 1
    return (rep(groupId, length(x)))
})

colGroups = unlist(colGroups)
colGroups=as.factor(colGroups)
gCnt=nlevels(colGroups)

KsCnt = length(unwanted_factors)
clustScores= vector(mode="list", length=KsCnt)
names(clustScores) = config_names


plotOutDir = paste0(thisScriptDir, "/out")
dir.create(plotOutDir, showWarnings = FALSE)

for (i in 1:KsCnt) {
    mat = exportRet$mats[[i]]
    colCnt = mat$colCnt
    rowCnt = mat$rowCnt
    sampleNames= mat$colNames
    k = unwanted_factors[i]
    extW = known_factors[i]
    print(paste0("k=", k, ",extW=", extW))

    # read in data matrix for current config
    mat = readMat(mat)

    #compute correlation between columns (samples)
    A = cor(mat)
    colnames(A) = paste0(sampleNames, "_", sampleLabs)
    rownames(A) = colnames(A)
    d=(1 - A)/2

    #row distance
    rd=as.dist(d)
    #col distance
    cd=as.dist(t(d))

    hclust_rd = hclust(rd)
    mergeMemberFlags = getHClustMergeFlags(hclust_rd)
    cscores = rep(0, gCnt)

    for(j in 1:gCnt) {
        memberIDs = which(colGroups == j)
        nonmemberIDs = which(colGroups != j)
        cscores[j] = getHClustScore(mergeMemberFlags, memberIDs, nonmemberIDs)
    }

    clustScores[[i]] = cscores

    dd.row <- as.dendrogram(hclust_rd)
    row.ord <- order.dendrogram(dd.row)
    dd.col <- as.dendrogram(hclust(cd))
    col.ord <- order.dendrogram(dd.col)

    # plot correlation matrix
    pdf(file = paste0(plotOutDir, "/hclust_corrmat_k", k, "_n", extW, ".pdf"),
        width = pdf_width, height = pdf_height)
    print(levelplot(
        A[row.ord, col.ord], main="", aspect = "fill", 
        scales = list(x = list(rot = 90, col = sampleColors[col.ord]),
                      y = list(col = sampleColors[row.ord])),
        colorkey = list(space = "left"),
        at=unique(c(seq(0, 0.2, length = 4),
                    seq(0.2, 0.8, length = 100),
                    seq(0.8, 1, length = 4))),
        col.regions = colorRampPalette(c("blue", "white", "red"))(1e3),
        xlab="",
        ylab="",
        legend = list(
            right = list(
                fun = dendrogramGrob,
                args = list(x = dd.col, ord = col.ord, side = "right")),
            top = list(
                fun = dendrogramGrob,
                args = list(x = dd.row, side = "top")))))
    dev.off()

    # plot dendrogram only
    pdf(file = paste0(plotOutDir, "/hclust_labels_k", k, "_n", extW, ".pdf"), width = pdf_width, height = 7)
    plot(hclust_rd, hang = -1)
    dev.off()
}

sink(paste0(plotOutDir, "/hclust_scores.txt"))
clustScores = lapply(clustScores, unlist, use.names = FALSE)

clustScores.mean = lapply(clustScores, mean, na.rm = TRUE)
clustScores.correctNum = lapply(clustScores, function(x) {
    sum(x == 1)
})

print(clustScores)
print(clustScores.mean)
print(clustScores.correctNum)
sink()
