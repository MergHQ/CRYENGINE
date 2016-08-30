@echo off
pushd "%~dp0\..\.."
mkdir Solution_Android > nul 2>&1
cd Solution_Android
..\Tools\CMake\Win32\bin\cmake -G "Visual Studio 14 2015" -D CMAKE_TOOLCHAIN_FILE=Tools\CMake\Android.cmake ..
echo Solution generated in Solution_Android. Run cmake-gui to configure it.
pause
popd
