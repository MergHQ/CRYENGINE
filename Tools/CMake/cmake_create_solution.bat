@ECHO OFF
setlocal
pushd %~dp0
set generating_solution=true

if NOT DEFINED solution_path call :SelectPlatform
if NOT DEFINED solution_path goto NoCompilerSelected

if EXIST "%~dp0\..\..\%solution_path%\CMakeCache.txt" goto :RunCMake

echo Detecting available compilers...
set choices=
call :VisitCompilers :PrintCompiler

IF %i% EQU 0 (
	ECHO No VS installations found!
	popd
	PAUSE
	goto :eof
)
set compiler_choice=1
if %i% GTR 1 call :ChooseCompiler
if %compiler_choice% EQU 0 goto NoCompilerSelected
call :VisitCompilers :GenerateCMake
popd
goto :eof

:NoCompilerSelected
echo Nothing selected, aborting.
popd
pause
goto :eof

:ChooseCompiler
choice /C %choices% /N /M "Choose VS version: "
set compiler_choice=%ERRORLEVEL%
goto :eof

REM VisitCompilers(func)
REM Call to execute func(generator_name) for every compiler found. %i% is set to the 1-based index of the compiler.
REM To add more compilers, add lines of the form "call :CheckCompiler %1 <registry_key> <generator_name> <display_name>"
:VisitCompilers
set i=0
call :CheckCompiler %1 "HKEY_CLASSES_ROOT\WDExpress.DTE.14.0" "Visual Studio 14 2015%generator_suffix%" "Visual Studio 2015 Express"
call :CheckCompiler %1 "HKEY_CLASSES_ROOT\VisualStudio.DTE.14.0" "Visual Studio 14 2015%generator_suffix%" "Visual Studio 2015"
call :CheckCompiler %1 "HKEY_CLASSES_ROOT\VisualStudio.DTE.15.0" "Visual Studio 15 2017%generator_suffix%" "Visual Studio 2017"
call :CheckCompiler %1 "HKEY_CLASSES_ROOT\VisualStudio.DTE.16.0" "Visual Studio 16 2019" "Visual Studio 2019"
goto :eof


REM CheckCompiler(func, reg_key, generator_name, display_name)
REM If reg_key is found in the registry, calls func(generator_name, display_name)
:CheckCompiler
reg query %2 >> nul 2>&1
if ERRORLEVEL 1 goto :eof
set /a i+=1
call %1 %3 %4
goto :eof

REM PrintCompiler(generator_name, display_name)
REM Prints the name of the generator given, along with its index.
:PrintCompiler
echo %i%. %2
set choices=%choices%%i%
goto :eof

REM GenerateCMake(generator_name, display_name)
REM Runs CMake with a generator matching the specified index.
:GenerateCMake
if %i% EQU %compiler_choice% call :RunCMake %1 %2
goto :eof

REM RunCMake(generator_name, display_name)
REM Runs CMake to generate a solution. If specified, uses generator_name as the generator.
:RunCMake
pushd "%~dp0\..\.."
mkdir %solution_path% > nul 2>&1
cd %solution_path%
if "%compiler_choice%" EQU "" (
..\..\Tools\CMake\Win32\bin\cmake -D CMAKE_TOOLCHAIN_FILE=Tools\CMake\%toolchain% %extra_args% %rerun_extra_args%..\..
) ELSE (
echo Generating CMake solution for %2
..\..\Tools\CMake\Win32\bin\cmake -G %1 -D CMAKE_TOOLCHAIN_FILE=Tools\CMake\%toolchain% %extra_args% %initial_extra_args% ..\..
)
if ERRORLEVEL 1 (
PAUSE
) ELSE (
ECHO Starting cmake-gui...
start ..\..\Tools\CMake\Win32\bin\cmake-gui .
)
popd
goto :eof

REM PLATFORM FILES
REM ==============
REM A platform file is a plain text file located in the toolchain folder.
REM It consists of a series of "(key)=(value)" settings that are used to specify how to run CMake for a given platform.
REM Note: *Spacing is important*. Any spaces on either side of the = will be interpreted as part of the key or value.
REM To leave a value blank, place a line break immediately after the = character.
REM
REM As a minimum, the following four entries must be defined:
REM platform_name - The name shown for the platform, if this batch file needs to show all of them
REM generator_suffix - The string to put after the base name for the Visual Studio generator. Should be " Win64" (including the space) for 64-bit Windows, otherwise blank.
REM solution_path - The path where the solution should be placed, relative to the code root folder.
REM toolchain - Path to the toolchain file for this platform, relative to the Tools\CMake folder.
REM
REM You may set a few optional entries to make it easier to work with multiple solutions for the same platform:
REM initial_extra_args - Extra arguments to send to CMake when creating the solution for the first time
REM rerun_extra_args - Extra arguments to send to CMake when creating the solution *after* (but not including) the first time
REM extra_args - Extra arguments to send to CMake in all cases
REM For example, you could have a dedicated .platform file which turns off Sandbox by default.
REM
REM The batch file can handle up to 9 different platform files when run directly.
REM If you need more than 9 platform files, you will need to explicitly load the desired platform before running this batch file.
REM See e.g. cmake_create_win64_solution.bat for an example of how to do this.

REM SelectPlatform
REM Asks the user to select a platform to generate a solution for.
:SelectPlatform
set choices=
call :VisitPlatforms :PrintPlatform
choice /C %choices% /N /M "Choose platform: "
set platform_choice=%ERRORLEVEL%
call :VisitPlatforms :LoadSelectedPlatform
goto :eof

REM VisitPlatforms(func)
REM Call to execute func(platform_file) for every compiler found. %i% is set to the 1-based index of the platform, ordered by filename.
REM To add more platforms, create more .platform files in the toolchain folder.
:VisitPlatforms
set i=0
for /f %%p in ('dir toolchain\*.platform /b ^| sort') do call %1 %%p
goto :eof

REM PrintPlatform(platform_file)
REM Prints the name of the platform given, along with its index.
:PrintPlatform
set /a i+=1
call :ReadPlatform %1
echo %i%. %platform_name%
set choices=%choices%%i%
goto :eof

REM ReadPlatform(platform_file)
REM Reads data for the platform file given.
:ReadPlatform
set initial_extra_args=
set rerun_extra_args=
set extra_args=
for /f "tokens=1,2 delims==" %%j in (toolchain\%1) do set %%j=%%k
goto :eof

REM LoadSelectedPlatform(platform_file)
REM Reads data for the platform file matching the given index.
:LoadSelectedPlatform
set /a i+=1
if %i% EQU %platform_choice% call :ReadPlatform %1
goto :eof
