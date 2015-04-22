@ECHO OFF
SET thisScriptPath=%~dp0
SET pathConfigBat=%~dp0..\..\config\bdt_path_win.bat
call %pathConfigBat%

py %bdtInstallDir%\bigKmeans ^
	--start-from 3-run-kmeans ^
	--data-input text-mat@%bdtDatasetsDir%\txtMat\dnase_test.txt ^
		--data-nrow 92554 ^
		--data-ncol 45 ^
	--k 100 ^
	--out %thisScriptPath%01-out ^
	--thread-num 4 ^
	--dist-type Euclidean ^
	--max-iter 100 ^
	--min-expchg 0.0001