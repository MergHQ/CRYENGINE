@echo off
pushd "%~dp0\..\.."
mkdir Solution_Win32 > nul 2>&1
cd Solution_Win32
..\Tools\CMake\Win32\bin\cmake -G "Visual Studio 14 2015" -D CMAKE_SYSTEM_VERSION=10.0.10586.0 ..
echo Solution generated in Solution_Win32. Run cmake-gui to configure it.
pause
popd
