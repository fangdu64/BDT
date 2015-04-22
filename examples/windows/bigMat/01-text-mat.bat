@ECHO OFF
SET thisScriptPath=%~dp0
SET pathConfigBat=%~dp0..\..\config\bdt_path_win.bat
call %pathConfigBat%

py %bdtInstallDir%\bigMat ^
	--input text-mat@%bdtDatasetsDir%\txtMat\dnase_test.txt ^
	--nrow 92554 ^
	--ncol 45 ^
	--out %thisScriptPath%01-out