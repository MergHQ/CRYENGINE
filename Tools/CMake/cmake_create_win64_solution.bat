@echo off
setlocal
pushd %~dp0
for /f "tokens=1,2 delims==" %%j in (toolchain\win64.platform) do set %%j=%%k
call cmake_create_solution.bat
popd
