@echo off
pushd %~dp0

if "%1"=="" goto USAGE

:RETRY
set "BESUPATH_TMP=%TEMP%\besupath_%RANDOM%_%RANDOM%.bat"
if exist %BESUPATH_TMP% goto RETRY

esupath -b %BESUPATH_TMP% %*
if errorlevel 0 goto SET_ENV

set "BESUPATH_TMP="
goto END

:SET_ENV
if exist %BESUPATH_TMP% call %BESUPATH_TMP%
goto END

:USAGE
esupath -h
goto END

:END
popd
