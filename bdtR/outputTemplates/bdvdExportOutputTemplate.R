BfvFiles = c(__BFV_FILES__)
OutMatNames = c(__OUT_MAT_NAMES__)
Mats <- lapply(1:length(BfvFiles), function(i) {
	mat <- list(
		name = OutMatNames[i],
		storePathPrefix = BfvFiles[i],
		rowCnt = __MAT_ROW_CNT__,
		colCnt = __MAT_COL_CNT__,
		colNames = c(__MAT_COL_NAMES__),
		colIds = c(__MAT_COL_IDS__))
	return (mat)
})
