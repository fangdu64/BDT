#'
#' run bdvd-export
#' @export
#'
bdvdExport <- function(bdt_home,
                 thread_num = 1,
                 mem_size = 1000,
                 column_ids,
                 bdvd_dir,
                 component,
                 unwanted_factors,
                 rowidx_from = NULL,
                 rowidx_to = NULL,
                 export_scale = NULL,
                 artifact_detection = NULL,
                 known_factors = NULL,
                 export_names = NULL,
                 rowidxs_input = NULL,
                 rowidxs_index_base = NULL,
                 start_from = NULL,
                 out) {
    cmds = c(
        'py',
        paste0(bdt_home,"/bdvdExport"),
        '--out', out,
        '--thread-num', as.character(thread_num),
        '--memory-size', as.character(mem_size),
        '--column-ids', paste0(column_ids, collapse=","),
        '--bdvd-dir', bdvd_dir,
        '--component', component,
        '--unwanted-factors', paste0(unwanted_factors, collapse=","))


    if (!is.null(rowidx_from)) {
        cmds <- append(cmds, c('--rowidx-from', as.character(rowidx_from)))
    }

    if (!is.null(rowidx_to)) {
        cmds <- append(cmds, c('--rowidx-to', as.character(rowidx_to)))
    }

    if (!is.null(export_scale)) {
        cmds <- append(cmds, c('--scale', export_scale))
    }

    if (!is.null(artifact_detection)) {
        cmds <- append(cmds, c('--artifact-detection', artifact_detection))
    }

    if (!is.null(known_factors)) {
        cmds <- append(cmds, c('--known-factors', paste0(known_factors, collapse=",")))
    }

    if (!is.null(export_names)) {
        cmds <- append(cmds, c('--export-names', paste0(export_names, collapse=",")))
    }

    if (!is.null(rowidxs_input)) {
        cmds <- append(cmds, c('--rowidxs-input', rowidxs_input))
    }

    if (!is.null(rowidxs_index_base)) {
      cmds <- append(cmds, c('--rowidxs-index-base', as.character(rowidxs_index_base)))
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

    ret <- readBdvdExportOutput(out)

    return (ret)
}
