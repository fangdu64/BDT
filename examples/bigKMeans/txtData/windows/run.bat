SET scriptPath=%~dp0
SET pathConfigBat=%~dp0..\..\..\config\bdt_path_win.bat
call %pathConfigBat%

cd %bdtInstallDir%
py bigKmeans ^
	--data %bdtDatasetsDir%\txtMat\dnase_test.txt ^
	--nrow 92554 ^
	--ncol 45 ^
	--k 100 ^
	--out %scriptPath%out ^
	--thread-num 2 ^
	--dist-type Euclidean ^
	--thread-num 2 ^
	--max-iter 100 ^
	--min-expchg 0.0001