#'
#' run big k-means++ implemented in Big Data Tools (BDT)
#'
#' @param bdt_home installation director for bdt
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
bigKmeansC <- function(bdt_home,
                      thread_num,
                      master_host,
                      master_port,
                      out) {
    cmds = c(
        'py',
        paste0(bdt_home,"/bigKmeansC"),
        '--out', out,
        '--thread-num', as.character(thread_num),
        '--master-host', master_host,
        '--master-port', as.character(master_port))

    if (.Platform$OS.type == "windows") {
        command = cmds[1]
        args = cmds[-1]
    } else {
        command = cmds[2]
        args = cmds[-c(1,2)]
    }
	
    system2(command, args)
}
