@ECHO OFF
SET thisScriptPath=%~dp0
SET pathConfigBat=%~dp0..\..\..\config\bdt_path_win.bat
call %pathConfigBat%

py %bdtInstallDir%\bdvdRowSelection ^
	--out %thisScriptPath%out ^
	--thread-num 4 ^
	--memory-size 1000 ^
	--row-selector  with-signal ^
	--bdvd-dir %thisScriptPath%..\s01-bdvd\out ^
	--with-signal-threshold 5 ^
	--with-signal-col-cnt 3 ^
	--component signal+random ^
	--scale mlog ^
	--artifact-detection conservative ^
	--unwanted-factors 0 ^
	--known-factors 0