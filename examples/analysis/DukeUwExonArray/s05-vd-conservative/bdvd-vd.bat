SET thisScriptPath=%~dp0
cd %thisScriptPath%
SET RExe="C:\Program Files\R\R-3.2.2\bin\x64\RScript.exe"

%RExe% --no-restore --no-save %thisScriptPath%bdvd-vd.R

echo off
<nul set /p "=Reaches the end, press a key to exit..."
pause >nul