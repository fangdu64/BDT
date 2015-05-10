#'
#' read output from bigMat
#' @export
#'
readBigMatOutput <- function(outDir) {
    env <- new.env()
    fileName = paste0(outDir,"/logs/output.R")
    sys.source(fileName, env)
    if (env$inputType == 'bams')  {
        ret <- list(bigMat = env$bigMat,
                    binMap = env$binMap)
    } else {
        ret <- list(bigMat = env$bigMat)
    }

    return (ret)
}

#'
#' read output from bigKmeans
#' @export
#'
readBigKmeansOutput <- function(outDir) {
    env <- new.env()
    fileName = paste0(outDir,"/logs/output.R")
    sys.source(fileName, env)
    ret = list(dataMat = env$dataMat,
               seedsMat = env$seedsMat,
               centroidsMat = env$centroidsMat,
               clusterAssignmentVec = env$clusterAssignmentVec)

    return (ret)
}

#'
#' read output from bdvd
#' @export
#'
readBdvdOutput <- function(outDir) {
    env <- new.env()
    fileName = paste0(outDir,"/logs/output.R")
    sys.source(fileName, env)
    ret = list(eigenValues = env$eigenValues,
               eigenVectors = env$eigenVectors,
               permutatedEigenValues = env$permutatedEigenValues,
               Wt = env$Wt)

    return (ret)
}
