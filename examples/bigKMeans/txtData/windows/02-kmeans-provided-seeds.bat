@ECHO OFF
SET scriptPath=%~dp0
SET pathConfigBat=%~dp0..\..\..\config\bdt_path_win.bat
call %pathConfigBat%

py %bdtInstallDir%\bigMat ^
	--input-type kmeans-data-mat ^
	--in-dir %scriptPath%01-out ^
	--out %scriptPath%02.1-out

py %bdtInstallDir%\bigMat ^
	--input-type kmeans-seeds-mat ^
	--in-dir %scriptPath%01-out ^
	--out %scriptPath%02.2-out
	
py %bdtInstallDir%\bigKmeans ^
	--input-type existing-mat ^
	--data %scriptPath%02.1-out ^
	--seeds-mat %scriptPath%02.2-out ^
	--nrow 92554 ^
	--ncol 45 ^
	--k 100 ^
	--out %scriptPath%02.3-out ^
	--thread-num 4 ^
	--dist-type Euclidean ^
	--max-iter 100 ^
	--min-expchg 0.0001
