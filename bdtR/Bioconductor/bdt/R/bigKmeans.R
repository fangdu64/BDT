#'
#' run big k-means++ implemented in Big Data Tools (BDT)
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
#' @param k The number of clusters
#' @param thread_num The number of threads used to do clustering
#' @param dist The distance used
#' @param max_iter max number of iteration of the kmeans
#' @param min_expchange min change of explained variance
#' @param out output dir
#'
#' @return fstats A vector of f-statistics
#'
#' @export
#'
bigKmeans <- function(bdt_home,
                      data,
                      nrow = NULL,
                      ncol = NULL,
                      k = 100,
                      thread_num = 4,
                      dist = 'Euclidean',
                      max_iter = 100,
                      min_expchange = 0.0001,
                      out) {
    cmds = c(
        'py',
        paste0(bdt_home,"/bigKmeans"),
        '--data-input', data,
        '--k', as.character(k),
        '--out', out,
        '--thread-num', as.character(thread_num),
        '--dist-type', 'Euclidean',
        '--max-iter', as.character(max_iter),
        '--min-expchg', as.character(min_expchange))
    if (!is.null(nrow)) {
        cmds <- append(cmds, c('--data-nrow', as.character(nrow)))
    }

    if (!is.null(ncol)) {
        cmds <- append(cmds, c('--data-ncol', as.character(ncol)))
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
