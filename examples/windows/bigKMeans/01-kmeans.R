bdtInstallDir='C:/work/BDT/build/windows/install'
bdtDatasetsDir = 'C:/work/bdtDatasets'
thisScriptDir = 'C:/work/BDT/examples/windows/bigKMeans'
args = c(
	paste0(bdtInstallDir,"/bigKmeans"),
	'--data-input', paste0("text-mat@",bdtDatasetsDir,"/txtMat/dnase_test.txt"),
	'--data-nrow', '92554',
	'--data-ncol', '45',
	'--k', '100',
	'--out', paste0(thisScriptDir,"/01-out"),
	'--thread-num', '4',
	'--dist-type', 'Euclidean',
	'--max-iter', '100',
	'--min-expchg', '0.0001')
system2('py', args)