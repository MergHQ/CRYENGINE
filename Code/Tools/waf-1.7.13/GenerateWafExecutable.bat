@echo off

rem This file builds the cry_waf executable for windows, it requires:
rem  - Python 2.7   - ensure the "PATH" Environment Variable points to your Python2.7 directory
rem  - pywin32      - installer can be downloaded from http://sourceforge.net/projects/pywin32
rem  - pyinstaller  - run: "pip2.7 install pyinstaller"


rem from issue #964

Setlocal EnableDelayedExpansion

rem Check Windows Version
set TOKEN=tokens=3*
ver | findstr /i "5\.0\." > nul
if %ERRORLEVEL% EQU 0 SET TOKEN=tokens=3*
ver | findstr /i "5\.1\." > nul
if %ERRORLEVEL% EQU 0 SET TOKEN=tokens=3*
ver | findstr /i "5\.2\." > nul
if %ERRORLEVEL% EQU 0 SET TOKEN=tokens=3*
ver | findstr /i "6\.0\." > nul
if %ERRORLEVEL% EQU 0 SET TOKEN=tokens=2*
ver | findstr /i "6\.1\." > nul
if %ERRORLEVEL% EQU 0 SET TOKEN=tokens=2*

rem Start calculating PYTHON and PYTHON_DIR
set PYTHON=
set PYTHON_DIR=

Setlocal EnableDelayedExpansion

set PYTHON_DIR_OK=FALSE
set REGPATH=

for %%i in (2.7 2.6 2.5 2.4 2.3) do (
for %%j in (HKCU HKLM) do (
for %%k in (SOFTWARE\Wow6432Node SOFTWARE) do (
for %%l in (Python\PythonCore IronPython) do (
set REG_PYTHON_EXE=python.exe
if "%%l"=="IronPython" (
set REG_PYTHON_EXE=ipy.exe
)

@echo on

set REGPATH=%%j\%%k\%%l\%%i\InstallPath
rem @echo Regpath !REGPATH!
REG QUERY "!REGPATH!" /ve 1>nul 2>nul
if !ERRORLEVEL! equ 0 (
  for /F "%TOKEN% delims=	 " %%A IN ('REG QUERY "!REGPATH!" /ve') do @set REG_PYTHON_DIR=%%B
  if exist !REG_PYTHON_DIR!  (
    set REG_PYTHON=!REG_PYTHON_DIR!!REG_PYTHON_EXE!
    rem set PYTHON_DIR_OK=TRUE
    if "!PYTHON_DIR_OK!"=="FALSE" (
      set PYTHON_DIR=!REG_PYTHON_DIR!
      set PYTHON=!REG_PYTHON!
      set PYTHON_DIR_OK=TRUE
    )

    rem set PYTHON_DIR_OK=FALSE
    rem @echo Find !REG_PYTHON!
    rem goto finished
  )
)

echo off

)
rem for l
)
rem for k
)
rem for j
)
rem for i



:finished

Endlocal & SET PYTHON_DIR=%PYTHON_DIR% & SET PYTHON=%PYTHON%

if "%PYTHON_DIR%" == "" (
rem @echo No Python dir
set PYTHON=python
goto running
)

rem @echo %PYTHON_DIR%

if "%PYTHON%" == "" (
rem @echo No Python
set PYTHON=python
goto running
)

set PYTHON_INCLUDE=%PYTHON_DIR%include
set PYTHON_LIB=%PYTHON_DIR%libs\python27.lib
set PATH=%PYTHON_DIR%;%PYTHON_DIR%Scripts;%PYTHON_DIR%Tools\Scripts;%PATH%

:running

@echo Using %PYTHON%
@echo Creating WAF Executable

pyinstaller --onefile pyinstaller_setup_waf.spec

if exist ..\..\..\cry_waf.exe  del /F ..\..\..\cry_waf.exe

copy dist\cry_waf.exe ..\..\..\cry_waf.exe

Endlocal & exit /b

