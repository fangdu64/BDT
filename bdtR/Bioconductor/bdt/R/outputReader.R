#'
#' read output from bigMat
#' @export
#'
readBigMatOutput <- function(outDir) {
    ee <- new.env()
    fileName = paste0(outDir,"/log/output.R")
    sys.source(fileName, ee)
    return (ee$bigMat)
}
