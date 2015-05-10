#'
#' run bigMat implemented in Big Data Tools (BDT)
#'
#' @param bdt_home installation director for bdt
#' @param input An input dataset of the format: input-type@@location,
#' supported types are:
#'    text-mat@@path to a text file
#'    binary-mat@@path to a binary file
#'    bams@@path to a file listing bam files
#'
#' @param row_cnt The number of rows of the input matrix (if input types are text-mat or binary-mat)
#' @param col_cnt The number of columns of the input matrix (if input types are text-mat or binary-mat)
#' @param out output dir
#'
#' @return fstats A vector of f-statistics
#'
#' @export
#'
bigMat <- function(bdt_home,
                   input,
                   row_cnt = NULL,
                   col_cnt = NULL,
                   skip_cols = NULL,
                   skip_rows = NULL,
                   col_sep = NULL,
                   out) {
    cmds = c(
        'py',
        paste0(bdt_home,"/bigMat"),
        '--input', input,
        '--out', data_mat)

    if (!is.null(row_cnt)) {
        cmds <- append(cmds, c('--nrow', as.character(row_cnt)))
    }

    if (!is.null(col_cnt)) {
        cmds <- append(cmds, c('--ncol', as.character(col_cnt)))
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
