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
#' @return ret a list representing bigKmeans results
#'
#' @export
#'
bigKmeans <- function(bdt_home,
                      data_input,
                      data_nrow = NULL,
                      data_ncol = NULL,
                      data_col_names = NULL,
                      data_skip_cols = NULL,
                      data_skip_rows = NULL,
                      data_col_sep = NULL,
                      k = 100,
                      thread_num = 4,
                      dist_type = 'Euclidean',
                      max_iter = 100,
                      min_expchange = 0.0001,
                      seeding_method = NULL,
                      seeds_input = NULL,
                      seeds_nrow = NULL,
                      seeds_ncol = NULL,
                      start_from = NULL,
                      slave_num = NULL,
                      out) {
    cmds = c(
        'py',
        paste0(bdt_home,"/bigKmeans"),
        '--data-input', data_input,
        '--k', as.character(k),
        '--out', out,
        '--thread-num', as.character(thread_num),
        '--dist-type', dist_type,
        '--max-iter', as.character(max_iter),
        '--min-expchg', as.character(min_expchange))
    if (!is.null(data_nrow)) {
        cmds <- append(cmds, c('--data-nrow', as.character(data_nrow)))
    }

    if (!is.null(data_ncol)) {
        cmds <- append(cmds, c('--data-ncol', as.character(data_ncol)))
    }

    if (!is.null(data_col_names)) {
        cmds <- append(cmds, c('--data-col-names', paste0(data_col_names, collapse=",")))
    }

    if (!is.null(seeding_method)) {
        cmds <- append(cmds, c('--seeding-method', seeding_method))
    }

    if (!is.null(seeds_nrow)) {
        cmds <- append(cmds, c('--seeds-nrow', as.character(seeds_nrow)))
    }

    if (!is.null(seeds_ncol)) {
        cmds <- append(cmds, c('--seeds-ncol', as.character(seeds_ncol)))
    }

    if (!is.null(seeds_input)) {
        cmds <- append(cmds, c('--seeds-input', seeds_input))
    }

    if (!is.null(start_from)) {
        cmds <- append(cmds, c('--start-from', start_from))
    }

    if (!is.null(slave_num)) {
        cmds <- append(cmds, c('--slave-num', slave_num))
    }

    if (.Platform$OS.type == "windows") {
        command = cmds[1]
        args = cmds[-1]
    } else {
        command = cmds[2]
        args = cmds[-c(1,2)]
    }

    system2(command, args)

    ret <- readBigKmeansOutput(out)

    return (ret)
}
