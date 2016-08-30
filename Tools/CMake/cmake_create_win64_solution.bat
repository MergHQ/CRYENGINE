@echo off
pushd "%~dp0\..\.."
mkdir Solution_Win64 > nul 2>&1
cd Solution_Win64
..\Tools\CMake\Win32\bin\cmake -G "Visual Studio 14 2015 Win64" -D CMAKE_SYSTEM_VERSION=10.0.10586.0 ..
echo Solution generated in Solution_Win64. Run cmake-gui to configure it.
pause
popd
