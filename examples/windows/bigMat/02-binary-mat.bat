@ECHO OFF
SET thisScriptPath=%~dp0
SET pathConfigBat=%~dp0..\..\config\bdt_path_win.bat
call %pathConfigBat%

py %bdtInstallDir%\bigMat ^
	--input binary-mat@%bdtDatasetsDir%\binaryMat\dnase_test.bfv ^
	--nrow 92554 ^
	--ncol 45 ^
	--out %thisScriptPath%02-out