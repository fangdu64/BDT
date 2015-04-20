@ECHO OFF
SET scriptPath=%~dp0
SET pathConfigBat=%~dp0..\..\..\config\bdt_path_win.bat
call %pathConfigBat%

py %bdtInstallDir%\bigKmeans ^
	--start-from 2-run-kmeans ^
	--input-type binary-mat ^
	--data %bdtDatasetsDir%\binaryMat\dnase_test.bfv ^
	--nrow 92554 ^
	--ncol 45 ^
	--k 100 ^
	--out %scriptPath%out ^
	--thread-num 4 ^
	--dist-type Euclidean ^
	--max-iter 100 ^
	--min-expchg 0.0001