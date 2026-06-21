@echo off
::
:: Move the path to MS Coreutils to either the beginning or the end of the PATH.
::
pushd %~dp0
set "coreutilsPath=%ProgramFiles%\coreutils\bin"
set "move_op="
set "RegMode="

:ARG_LOOP
if /I "%1"==""        goto ARG_LOOP_END
if /I "%1"=="first"   set "move_op=--prepend"
if /I "%1"=="last"    set "move_op=--append"
if /I "%1"=="top"     set "move_op=--prepend"
if /I "%1"=="bottom"  set "move_op=--append"
if /I "%1"=="system"  set "RegMode=--system"
if /I "%1"=="user"    set "RegMode=--user"
shift
goto ARG_LOOP
:ARG_LOOP_END

if "%RegMode%%move_op%"=="" goto USAGE

:RETRY
::set "ESUPATH_TMP=%TEMP%\coreutilsPrioChange_sub_%RANDOM%_%RANDOM%.bat"
set "ESUPATH_TMP=coreutilsPrioChange_sub_%RANDOM%_%RANDOM%.bat"
if exist "%ESUPATH_TMP%" goto RETRY

esupath --silent --batch "%ESUPATH_TMP%" %RegMode% %move_op% "%coreutilsPath%"
if exist %ESUPATH_TMP% call %ESUPATH_TMP%
::if exist %ESUPATH_TMP% del  %ESUPATH_TMP%

goto END

:USAGE
chcp 65001
@echo coreutilsPrio.bat [first/last] [system/user]
@echo MS 製 coreutils\bin を環境変数 PATH の先頭または最後に移動する.
@echo   first   先頭へ移動.
@echo   last    最後へ移動.
@echo   system  SYSTEM レジストリの PATH も変更. (通常)
@echo   user    USER レジストリの PATH も変更. (ユーザーが独自設定していたとき用)
@echo:
@echo syste/user 未指定時は現プロセスのPATHのみ変更.
@echo coreutils インストール時、SYSTEM レジストリの PATH に追加されている.
@echo のでレジストリの PATH も変更する場合は system を指定のこと.
::@echo:
::@echo ※ user は ユーザー側で USERレジストリに移動していた場合用.
::@echo    だが、それだと windows/system32 を前後できないのであまり意味はない.
goto END

:END
set "ESUPATH_TMP="
set "coreutilsPath="
set "move_op="
set "RegMode="
popd
