#'
#' run big k-means++ implemented in Big Data Tools (BDT)
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
#' @param k The number of clusters
#' @param thread_num The number of threads used to do clustering
#' @param dist_type The distance used
#' @param max_iter max number of iteration of the kmeans
#' @param min_expchange min change of explained variance
#' @param out output dir
#'
#' @return fstats A vector of f-statistics
#'
#' @export
#'
bigKmeans <- function(bdt_home,
                      input,
                      row_cnt = NULL,
                      col_cnt = NULL,
                      k = 100,
                      thread_num = 4,
                      dist_type = 'Euclidean',
                      max_iter = 100,
                      min_expchange = 0.0001,
                      out) {
    cmds = c(
        'py',
        paste0(bdt_home,"/bigKmeans"),
        '--data-input', input,
        '--k', as.character(k),
        '--out', out,
        '--thread-num', as.character(thread_num),
        '--dist-type', dist_type,
        '--max-iter', as.character(max_iter),
        '--min-expchg', as.character(min_expchange))
    if (!is.null(row_cnt)) {
        cmds <- append(cmds, c('--data-nrow', as.character(row_cnt)))
    }

    if (!is.null(col_cnt)) {
        cmds <- append(cmds, c('--data-ncol', as.character(col_cnt)))
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
