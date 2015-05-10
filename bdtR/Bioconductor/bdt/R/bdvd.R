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
#' @param out output dir
#'
#' @return fstats A vector of f-statistics
#'
#' @export
#'
bdvd <- function(bdt_home,
                 data_input,
                 data_nrow = NULL,
                 data_ncol = NULL,
                 data_col_names = NULL,
                 data_skip_cols = NULL,
                 data_skip_rows = NULL,
                 data_col_sep = NULL,
                 thread_num = 4,
                 mem_size = 1000,
                 sample_groups,
                 pre_normalization = NULL,
                 common_column_sum = NULL,
                 ruv_scale = 'mlog',
                 ruv_mlog_c = 1,
                 ruv_type = 'ruvs',
                 weak_signal_lb = NULL,
                 weak_signal_ub = NULL,
                 lower_quantile_threshold = NULL,
                 all_in_quantile_fraction = NULL,
                 ruv_rowwise_adjust = NULL,
                 known_factors = NULL,
                 control_rows_method = 'all',
                 ctrl_rows_input = NULL,
                 ctrl_rows_index_base = 1,
                 permutation_num = 0,
                 start_from = NULL,
                 out) {
    cmds = c(
        'py',
        paste0(bdt_home,"/bdvd"),
        '--data-input', data_input,
        '--out', out,
        '--thread-num', as.character(thread_num),
        '--memory-size', as.character(mem_size),
        '--sample-groups', vecListToString(sample_groups),
        '--ruv-scale', ruv_scale,
        '--ruv-mlog-c', as.character(ruv_mlog_c),
        '--ruv-type', ruv_type,
        '--control-rows-method', control_rows_method,
        '--ctrl-rows-index-base', as.character(ctrl_rows_index_base),
        '--permutation-num', as.character(permutation_num))

    if (!is.null(data_nrow)) {
        cmds <- append(cmds, c('--data-nrow', as.character(data_nrow)))
    }

    if (!is.null(data_ncol)) {
        cmds <- append(cmds, c('--data-ncol', as.character(data_ncol)))
    }

    if (!is.null(data_skip_cols)) {
        cmds <- append(cmds, c('--data-skip-cols', as.character(data_skip_cols)))
    }

    if (!is.null(data_skip_rows)) {
        cmds <- append(cmds, c('--data-skip-rows', as.character(data_skip_rows)))
    }

    if (!is.null(data_col_sep)) {
        cmds <- append(cmds, c('--data-col-sep', paste0("\"",data_col_sep,"\"")))
    }

    if (!is.null(data_col_names)) {
        cmds <- append(cmds, c('--data-col-names', paste0(data_col_names, collapse=",")))
    }

    if (!is.null(weak_signal_lb)) {
        cmds <- append(cmds, c('--weak-signal-lb', as.character(weak_signal_lb)))
    }

    if (!is.null(weak_signal_ub)) {
        cmds <- append(cmds, c('--weak-signal-ub', as.character(weak_signal_ub)))
    }

    if (!is.null(lower_quantile_threshold)) {
        cmds <- append(cmds, c('--lower-quantile-threshold', as.character(lower_quantile_threshold)))
    }

    if (!is.null(all_in_quantile_fraction)) {
        cmds <- append(cmds, c('--all-in-quantile-fraction', as.character(all_in_quantile_fraction)))
    }


    if (!is.null(ruv_rowwise_adjust)) {
        cmds <- append(cmds, c('--ruv-rowwise-adjust', ruv_rowwise_adjust))
    }

    if (!is.null(ctrl_rows_input)) {
        cmds <- append(cmds, c('--ctrl-rows-input', ctrl_rows_input))
    }

    if (!is.null(pre_normalization)) {
        cmds <- append(cmds, c('--pre-normalization', as.character(pre_normalization)))
    }

    if (!is.null(common_column_sum)) {
        cmds <- append(cmds, c('--common-column-sum', as.character(common_column_sum)))
    }


    if (!is.null(start_from)) {
        cmds <- append(cmds, c('--start-from', start_from))
    }

    if (.Platform$OS.type == "windows") {
        command = cmds[1]
        args = cmds[-1]
    } else {
        command = cmds[2]
        args = cmds[-c(1,2)]
    }

    system2(command, args)

    ret <- readBdvdOutput(out)

    return (ret)
}
