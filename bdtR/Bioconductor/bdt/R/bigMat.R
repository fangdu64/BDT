#'
#' run bigMat implemented in Big Data Tools (BDT)
#'
#' @param bdt_home installation director for bdt
#' @param data An input dataset of the format: input-type@@location,
#' supported types are:
#'    text-mat@@path to a text file
#'    binary-mat@@path to a binary file
#'    bams@@path to a file listing bam files
#'
#' @param nrow The number of rows of the input matrix (if input types are text-mat or binary-mat)
#' @param ncol The number of columns of the input matrix (if input types are text-mat or binary-mat)
#' @param out output dir
#'
#' @return fstats A vector of f-statistics
#'
#' @export
#'
bigMat <- function(
    bdt_home,
    data,
    nrow = NULL,
    ncol = NULL,
    out) {

    cmds = c(
        'py',
        paste0(bdt_home,"/bigMat"),
        '--out', out)

    if (!is.null(nrow)) {
        cmds <- append(cmds, c('--nrow', as.character(nrow)))
    }

    if (!is.null(ncol)) {
        cmds <- append(cmds, c('--ncol', as.character(ncol)))
    }

    if (.Platform$OS.type == "windows") {
        command = cmds[1]
        args = cmds[-1]
    } else {
        command = cmds[2]
        args = cmds[-c(1,2)]
    }

    system2(command, args)

    return (1)
}
