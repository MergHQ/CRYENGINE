@echo off

set CMAKE=%~dp0../../../Tools/CMake/Win32/bin/cmake.exe

if not exist .\Intermediate mkdir .\Intermediate
Pushd .\Intermediate
%CMAKE% -G "Visual Studio 11 2012 Win64" "%~dp0/Source"
Popd
powershell "$s=(New-Object -COM WScript.Shell).CreateShortcut('%~dp0\MaxPluginTools.sln.lnk');$s.TargetPath='%~dp0\Intermediate\MaxPluginTools.sln';$s.Save()"

pause