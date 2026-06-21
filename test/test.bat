@echo off
setlocal
pushd "%~dp0"
chcp 65001 >nul

set "EXE=%~2"
if not defined EXE set "EXE=%~dp0..\bin\esupath.exe"
set "TEST_VAR=ESUPATH_TEST"
set "HAS_ERR=0"

if not exist "%EXE%" goto NO_EXE
if not exist "dst" mkdir "dst"

if /I "%~1"=="env"    goto CHECK_ENV
if /I "%~1"=="user"   goto CHECK_USER
if /I "%~1"=="system" goto CHECK_SYSTEM

call :CHECK_OPTIONS
call :CHECK_ADVANCED
call :CHECK_TARGET e
call :CHECK_TARGET u
goto CHECK_SYSTEM

:CHECK_ENV
call :CHECK_OPTIONS
call :CHECK_ADVANCED
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
set "TARGET_LONG=--env"
set "BATCH_OPT="

if /I "%ESU%"=="u" set "TARGET_LONG=--user"
if /I "%ESU%"=="s" set "TARGET_LONG=--system"

if /I "%ESU%"=="e" (
    set "%TEST_VAR%="
) else (
    "%EXE%" -y %TARGET% --var %TEST_VAR% --delete-var >nul 2>&1
)

call :SET_APPLY 01
> "%OUT%" echo ==== append / semicolon split / duplicate ====
"%EXE%" -y %TARGET_LONG% %BATCH_OPT% --var %TEST_VAR% -a "y:\foo" "y:\bar;y:\baz (1)" "y:\foo" >>"%OUT%" 2>&1
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
"%EXE%" -y %TARGET% %BATCH_OPT% --var %TEST_VAR% -a "y:\last" --clear -p "y:\top" >>"%OUT%" 2>&1
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

>>"%OUT%" echo ==== delete missing variable ====
"%EXE%" -y %TARGET% %BATCH_OPT% --var %TEST_VAR% --delete-var >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

call :CMP "%ESU%.txt"
exit /b 0

:CHECK_OPTIONS
set "OUT=dst\options.txt"
set "%TEST_VAR%="

> "%OUT%" echo ==== help ====
"%EXE%" --help >dst\help.txt 2>&1
call :RECORD_RC "%OUT%"
call :RECORD_NONEMPTY "%OUT%" "help output" "dst\help.txt"

>>"%OUT%" echo ==== short help ====
"%EXE%" -h >dst\help_short.txt 2>&1
call :RECORD_RC "%OUT%"
call :RECORD_NONEMPTY "%OUT%" "short help output" "dst\help_short.txt"

>>"%OUT%" echo ==== invalid option ====
"%EXE%" --not-an-option >dst\invalid_option.txt 2>&1
call :RECORD_RC "%OUT%"
call :RECORD_CONTAINS "%OUT%" "invalid option message" "dst\invalid_option.txt" "Bad option --not-an-option"
call :RECORD_NONEMPTY "%OUT%" "invalid option help output" "dst\invalid_option.txt"

>>"%OUT%" echo ==== invalid variable name ====
"%EXE%" -l --var "BAD-NAME" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

>>"%OUT%" echo ==== missing --var argument ====
"%EXE%" -l --var >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

>>"%OUT%" echo ==== missing --batch argument ====
"%EXE%" --silent --batch >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

>>"%OUT%" echo ==== missing response file ====
"%EXE%" @dst\not-found.rsp >>"%OUT%" 2>&1
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

>>"%OUT%" echo ==== user and system append ====
"%EXE%" -y -u -s --var %TEST_VAR% -a "y:\foo" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

>>"%OUT%" echo ==== list without target ====
"%EXE%" --list --var %TEST_VAR% >dst\list_all.txt 2>&1
call :RECORD_RC "%OUT%"
call :RECORD_LIST_LABEL "%OUT%" "SYSTEM Registry"
call :RECORD_LIST_LABEL "%OUT%" "USER Registry"
call :RECORD_LIST_LABEL "%OUT%" "Current Process"

call :CMP "options.txt"
exit /b 0

:CHECK_ADVANCED
set "OUT=dst\advanced.txt"
set "RSP_CRLF=dst\response_crlf.rsp"
set "RSP_LF=dst\response_lf.rsp"
set "RSP_BATCH=dst\response_result.bat"
set "RSP_BATCH_CRLF=dst\response_result_crlf.bat"
set "%TEST_VAR%="

> "%RSP_CRLF%" (
    echo # full-line comment
    echo.
    echo --silent --env --batch %RSP_BATCH% --var %TEST_VAR% --clear --append
    echo "y:\space path"
    echo "y:\mid#hash"
    echo "#quoted-head-hash"
    echo ""
    echo "y:\quote""mark"
    echo "y:\quoted"tail
    echo "y:\日本語"
)
powershell -NoProfile -Command ^
    "$p='dst\response_crlf.rsp'; $q='dst\response_lf.rsp'; $s=[IO.File]::ReadAllText($p); [IO.File]::WriteAllText($q,$s.Replace(\"`r`n\",\"`n\"),[Text.UTF8Encoding]::new($false))"

> "%OUT%" echo ==== response file CRLF ====
"%EXE%" @"%RSP_CRLF%" >dst\response_stdout.txt 2>&1
call :RECORD_RC "%OUT%"
call :RECORD_EMPTY "%OUT%" "response stdout" "dst\response_stdout.txt"
copy /y "%RSP_BATCH%" "%RSP_BATCH_CRLF%" >nul
type "%RSP_BATCH%" >>"%OUT%"
call :APPLY_FILE "%RSP_BATCH%"

>>"%OUT%" echo ==== response file LF ====
"%EXE%" @"%RSP_LF%" >dst\response_stdout.txt 2>&1
call :RECORD_RC "%OUT%"
call :RECORD_EMPTY "%OUT%" "response stdout" "dst\response_stdout.txt"
fc /b "%RSP_BATCH_CRLF%" "%RSP_BATCH%" >nul
if errorlevel 1 (
    >>"%OUT%" echo [CRLF/LF:different]
) else (
    >>"%OUT%" echo [CRLF/LF:same]
)
call :APPLY_FILE "%RSP_BATCH%"
"%EXE%" -l --env --var %TEST_VAR% >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

set "%TEST_VAR%="
set "APPLY=dst\advanced_paths_01.bat"
>>"%OUT%" echo ==== path variants ====
"%EXE%" -y --env --batch "%APPLY%" --var %TEST_VAR% --clear --append "y:\Alpha" "y:/slash/path" y:\trail\ ";y:\semi-one;;y:\semi-two;" "y:\日本語" "y:\star\one" "y:\star\two\deep" "y:\Case" "Y:\case" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_FILE "%APPLY%"

set "APPLY=dst\advanced_paths_02.bat"
>>"%OUT%" echo ==== path matching ====
"%EXE%" -y --env --batch "%APPLY%" --var %TEST_VAR% --remove "y:\star\*" "z:\not-found*" "Y:\ALPHA" "y:\slash\path" "y:\trail" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_FILE "%APPLY%"

set "%TEST_VAR%="
set "APPLY=dst\interactive_y.bat"
>>"%OUT%" echo ==== interactive Y ====
(echo y)|"%EXE%" --env --batch "%APPLY%" --var %TEST_VAR% --append "y:\yes" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_FILE "%APPLY%"

set "APPLY=dst\interactive_enter.bat"
>dst\interactive_enter.txt echo.
>>"%OUT%" echo ==== interactive Enter ====
"%EXE%" --env --batch "%APPLY%" --var %TEST_VAR% --append "y:\enter" <dst\interactive_enter.txt >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
call :APPLY_FILE "%APPLY%"

set "APPLY=dst\interactive_n.bat"
if exist "%APPLY%" del "%APPLY%"
>>"%OUT%" echo ==== interactive N ====
(echo n)|"%EXE%" --env --batch "%APPLY%" --var %TEST_VAR% --append "y:\no" >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"
if exist "%APPLY%" (
    >>"%OUT%" echo [reject batch:generated]
) else (
    >>"%OUT%" echo [reject batch:not-generated]
)

set "APPLY=dst\silent_check.bat"
>>"%OUT%" echo ==== silent output ====
"%EXE%" --silent --env --batch "%APPLY%" --var %TEST_VAR% --append "y:\silent" >dst\silent_stdout.txt 2>&1
call :RECORD_RC "%OUT%"
call :RECORD_EMPTY "%OUT%" "silent stdout" "dst\silent_stdout.txt"
call :APPLY_FILE "%APPLY%"
"%EXE%" -l --env --var %TEST_VAR% >>"%OUT%" 2>&1
call :RECORD_RC "%OUT%"

set "%TEST_VAR%="
call :CMP "advanced.txt"
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

:APPLY_FILE
if not exist "%~1" (
    set "HAS_ERR=1"
    echo [%~1] : Not generated.
    exit /b 0
)
set "APPLY_LINE="
set /p "APPLY_LINE="<"%~1"
%APPLY_LINE%
exit /b 0

:RECORD_RC
set "ACTUAL_RC=%errorlevel%"
>>"%~1" echo [exit:%ACTUAL_RC%]
exit /b 0

:RECORD_EMPTY
for %%A in ("%~3") do set "OUTPUT_SIZE=%%~zA"
if "%OUTPUT_SIZE%"=="0" (
    >>"%~1" echo [%~2:empty]
) else (
    >>"%~1" echo [%~2:not-empty]
    type "%~3" >>"%~1"
)
exit /b 0

:RECORD_NONEMPTY
for %%A in ("%~3") do set "OUTPUT_SIZE=%%~zA"
if "%OUTPUT_SIZE%"=="0" (
    >>"%~1" echo [%~2:empty]
) else (
    >>"%~1" echo [%~2:not-empty]
)
exit /b 0

:RECORD_CONTAINS
findstr /l /c:"%~4" "%~3" >nul
if errorlevel 1 (
    >>"%~1" echo [%~2:not-found]
) else (
    >>"%~1" echo [%~2:found]
)
exit /b 0

:RECORD_LIST_LABEL
findstr /b /l /c:"[%~2]" "dst\list_all.txt" >nul
if errorlevel 1 (
    >>"%~1" echo [%~2:not-found]
) else (
    >>"%~1" echo [%~2:found]
)
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
