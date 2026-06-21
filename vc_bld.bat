pushd %~dp0
set ROOT_DIR=%CD%
set BIN_DIR=%ROOT_DIR%\bin
set SRC_DIR=%ROOT_DIR%\src
set BLD_DIR=%ROOT_DIR%\build
if not exist %BIN_DIR% mkdir %BIN_DIR%
copy %ROOT_DIR%\README.md              %BIN_DIR%\esupath.md
copy %SRC_DIR%\batch\besupath.bat      %BIN_DIR%\besupath.bat
copy %SRC_DIR%\batch\coreutilsPrio.bat %BIN_DIR%\coreutilsPrio.bat

if not exist %BLD_DIR% mkdir %BLD_DIR%

pushd %BLD_DIR%
cmake ..
cmake --build . --config Release
cmake --install .
if exist %BIN_DIR%\esupath.exe (
  ctest --test-dir vc-x64 -C Release -L env
  ctest --test-dir vc-x64 -C Release -L user
)
popd
