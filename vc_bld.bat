@echo off
setlocal
pushd "%~dp0"

set "tgt="
set "ReCo=0"
set "clean_only=0"

:L_LOOP
  if /I "%1"==""      goto L_LOOP_EXIT
  if /I "%1"=="clean" set clean_only=1
  if /I "%1"=="x64"   set tgt=vc-x64
  if /I "%1"=="win32" set tgt=vc-win32
  shift
goto L_LOOP
:L_LOOP_EXIT

:: clean
if exist "*.bak" del /q /s "*.bak"
if "%tgt%"=="" (
  call :clean vc-x64
  call :clean vc-win32
  set "tgt=vc-x64"
) else (
  call :clean %tgt%
)
if /I "%clean_only%"=="1" goto END

cmake --workflow --preset %tgt%-release
set "ReCo=%ERRORLEVEL%"
goto END

:clean
set t=%1
if exist "build\%t%"        rmdir /q /s "build\%t%"
if exist "dist\%t%-release" rmdir /q /s "dist\%t%-release"
if exist "dist\esupath-%t%-release.zip" del /q "dist\esupath-%t%-release.zip"
exit /b 0

:END
popd
endlocal & exit /b %ReCo%
