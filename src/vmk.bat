set BIN_DIR=..\bin
if not exist %BIN_DIR% mkdir %BIN_DIR%
copy ..\README.md %BIN_DIR%\esupath.md
copy besupath.bat %BIN_DIR%\besupath.bat

set BLD_DIR=..\build
if not exist %BLD_DIR% mkdir %BLD_DIR%

pushd %BLD_DIR%
cmake ..
cmake --build . --config Release
cmake --install .
popd
