@echo off
setlocal

set SCRIPT_DIR=%~dp0
pushd "%SCRIPT_DIR%"

where cmake >nul 2>nul
if errorlevel 1 (
	echo ERROR: CMake is not installed or not available in PATH.
	goto :fail
)

cmake -S . -B build-win -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto :fail

cmake --build build-win --config Release
if errorlevel 1 goto :fail

echo Build completed successfully.
popd
exit /b 0

:fail
echo Build failed.
popd
exit /b 1
