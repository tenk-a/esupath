@echo off
setlocal
pushd "%~dp0"
chcp 65001 >nul

set "EXE=%~dp0..\bin\esupath.exe"
set "TEST_VAR=ESUPATH_TEST"
set "HAS_ERR=0"

if not exist "%EXE%" goto NO_EXE
if not exist "dst" mkdir "dst"

if /I "%~1"=="env"    goto CHECK_ENV
if /I "%~1"=="user"   goto CHECK_USER
if /I "%~1"=="system" goto CHECK_SYSTEM

call :CHECK_OPTIONS
call :CHECK_TARGET e
call :CHECK_TARGET u
goto CHECK_SYSTEM

:CHECK_ENV
call :CHECK_OPTIONS
call :CHECK_TARGET e
goto END

:CHECK_USER
call :CHECK_TARGET u
goto END

:CHECK_SYSTEM
net session >nul 2>&1
if errorlevel 1 (
    powershell -NoProfile -ExecutionPolicy Bypass -Command ^
        "$p = Start-Process -FilePath '%~f0' -ArgumentList 'system' -Verb RunAs -PassThru -Wait; exit $p.ExitCode"
    if errorlevel 1 set "HAS_ERR=1"
    goto END
)
call :CHECK_TARGET s
goto END

:CHECK_TARGET
set "ESU=%~1"
set "OUT=dst\%ESU%.txt"
set "RSP=dst\%ESU%_args.rsp"
set "TARGET=-%ESU%"
set "BATCH_OPT="

if /I "%ESU%"=="e" (
    set "%TEST_VAR%="
) else (
    "%EXE%" -y %TARGET% --var %TEST_VAR% --delete-var >nul 2>&1
)

call :SET_APPLY 01
> "%OUT%" echo ==== append / semicolon split / duplicate ====
"%EXE%" -y %TARGET% %BATCH_OPT% --var %TEST_VAR% -a "y:\foo" "y:\bar;y:\baz (1)" "y:\foo" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_ENV

call :SET_APPLY 02
>>"%OUT%" echo ==== prepend / append long options ====
"%EXE%" --yes %TARGET% %BATCH_OPT% --var %TEST_VAR% --prepend "y:\top" --append "y:\last" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_ENV

call :SET_APPLY 03
>>"%OUT%" echo ==== remove exact and wildcards ====
"%EXE%" -y %TARGET% %BATCH_OPT% --var %TEST_VAR% --remove "y:\b?r" "y:\**z*" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_ENV

call :SET_APPLY 04
>>"%OUT%" echo ==== move existing entries ====
"%EXE%" -y %TARGET% %BATCH_OPT% --var %TEST_VAR% -p "y:\foo" -a "y:\top" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_ENV

call :SET_APPLY 05
>>"%OUT%" echo ==== clear / option order ====
"%EXE%" -y %TARGET% %BATCH_OPT% --var %TEST_VAR% -a "y:\last" -c -p "y:\top" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_ENV

>>"%OUT%" echo ==== list ====
"%EXE%" --list %TARGET% --var %TEST_VAR% >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

call :SET_APPLY 07
>>"%OUT%" echo ==== silent ====
"%EXE%" --silent %TARGET% %BATCH_OPT% --var %TEST_VAR% --append "y:\silent" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_ENV

call :SET_APPLY 08
> "%RSP%" echo --yes %TARGET% %BATCH_OPT% --var %TEST_VAR% --remove "y:\silent" --append "y:\response file"
>>"%OUT%" echo ==== response file ====
"%EXE%" @"%RSP%" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_ENV

if /I "%ESU%"=="e" (
    >>"%OUT%" echo ==== batch file ====
    type "%APPLY%" >>"%OUT%"
)

call :SET_APPLY 09
>>"%OUT%" echo ==== delete variable ====
"%EXE%" -y %TARGET% %BATCH_OPT% --var %TEST_VAR% --delete-var >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_ENV

>>"%OUT%" echo ==== list after delete ====
"%EXE%" -l %TARGET% --var %TEST_VAR% >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

call :CMP "%ESU%.txt"
exit /b 0

:CHECK_OPTIONS
set "OUT=dst\options.txt"
set "%TEST_VAR%="

> "%OUT%" echo ==== help ====
"%EXE%" --help >nul 2>&1
call :RECORD_RC "%OUT%"

>>"%OUT%" echo ==== invalid option ====
"%EXE%" --not-an-option >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

>>"%OUT%" echo ==== invalid variable name ====
"%EXE%" -l --var "BAD-NAME" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

>>"%OUT%" echo ==== edit without target ====
"%EXE%" -y --var %TEST_VAR% -a "y:\foo" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

>>"%OUT%" echo ==== list with edit operation ====
"%EXE%" -e --var %TEST_VAR% -l -a "y:\foo" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

>>"%OUT%" echo ==== delete without target ====
"%EXE%" --var %TEST_VAR% --delete-var >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

>>"%OUT%" echo ==== delete with edit operation ====
"%EXE%" -e --var %TEST_VAR% --delete-var -r "y:\foo" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

>>"%OUT%" echo ==== no directories ====
"%EXE%" -y -e --var %TEST_VAR% >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

call :CMP "options.txt"
exit /b 0

:SET_APPLY
set "APPLY=dst\%ESU%_apply_%~1.bat"
set "BATCH_OPT="
if /I "%ESU%"=="e" set "BATCH_OPT=--batch %APPLY%"
exit /b 0

:APPLY_ENV
if /I not "%ESU%"=="e" exit /b 0
if not exist "%APPLY%" (
    set "HAS_ERR=1"
    echo [%APPLY%] : Not generated.
    exit /b 0
)
set "APPLY_LINE="
set /p "APPLY_LINE="<"%APPLY%"
%APPLY_LINE%
exit /b 0

:RECORD_RC
set "ACTUAL_RC=%errorlevel%"
>>"%~1" echo [exit:%ACTUAL_RC%]
exit /b 0

:CMP
if not exist "src\%~1" (
    set "HAS_ERR=1"
    echo [%~1] : No expected file. Generated in dst.
    exit /b 0
)
fc /a "src\%~1" "dst\%~1" >nul
if errorlevel 1 (
    set "HAS_ERR=1"
    echo [%~1] : Failed.
    exit /b 0
)
echo [%~1] : OK.
exit /b 0

:NO_EXE
echo No "%EXE%"
set "HAS_ERR=1"

:END
popd
exit /b %HAS_ERR%
