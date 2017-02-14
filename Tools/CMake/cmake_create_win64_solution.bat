@echo off
pushd "%~dp0\..\.."
mkdir solutions_cmake\win64 > nul 2>&1
cd solutions_cmake\win64
..\..\Tools\CMake\Win32\bin\cmake -G "Visual Studio 14 2015 Win64" -D CMAKE_TOOLCHAIN_FILE=Tools\CMake\toolchain\windows\WindowsPC-MSVC.cmake ..\..
if ERRORLEVEL 1 (
	pause
) else (
	echo Starting cmake-gui...
	start ..\..\Tools\CMake\Win32\bin\cmake-gui .
)
popd
