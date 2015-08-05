bfvFiles = c(__BFV_FILES__)
outMatNames = c(__OUT_MAT_NAMES__)
Mats <- lapply(1:length(bfvFiles), function(i) {
    mat <- list(
        name = outMatNames[i],
        storePathPrefix = bfvFiles[i],
        rowCnt = __MAT_ROW_CNT__,
        colCnt = __MAT_COL_CNT__,
        colNames = c(__MAT_COL_NAMES__),
        colIds = c(__MAT_COL_IDS__))
    return (mat)
})
