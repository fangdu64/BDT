@ECHO OFF
SET thisScriptPath=%~dp0
SET pathConfigBat=%~dp0..\..\config\bdt_path_win.bat
call %pathConfigBat%

py %bdtInstallDir%\bigKmeans ^
	--data-input kmeans-data-mat@%thisScriptPath%01-out ^
	--seeds-input kmeans-seeds-mat@%thisScriptPath%01-out ^
	--k 100 ^
	--out %thisScriptPath%03-out ^
	--thread-num 4 ^
	--dist-type Euclidean ^
	--max-iter 100 ^
	--min-expchg 0.0001
