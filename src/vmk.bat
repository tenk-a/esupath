set BIN_DIR=..\bin
if not exist %BIN_DIR% mkdir %BIN_DIR%
copy ..\README.md %BIN_DIR%\esupath.md
copy batch\besupath.bat      %BIN_DIR%\besupath.bat
copy batch\coreutilsPrio.bat %BIN_DIR%\coreutilsPrio.bat

set BLD_DIR=..\build
if not exist %BLD_DIR% mkdir %BLD_DIR%

pushd %BLD_DIR%
cmake ..
cmake --build . --config Release
cmake --install .
popd
