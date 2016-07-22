@echo OFF

setlocal ENABLEEXTENSIONS
set projectcfg=project.cfg
set regkeyname=HKEY_CURRENT_USER\Software\Crytek\CryEngine
set cesharap_parent_folder=Code\CryManaged
set cesharap_folder=Code\CryManaged\CESharp
set bin_folder=bin\win_x64
set cerootvar=CRYENGINEROOT
set vssln=Code\Towerdefense.sln

if not exist "%projectcfg%" (
    1>&2 echo "Error: %projectcfg% not found!"
	pause
    exit /b 1
)

for /F "tokens=*" %%I in (%projectcfg%) do set %%I

if not defined engine_version (
	1>&2 echo "Error: no engine_version entry found in %projectcfg%!"
	pause
    exit /b 1
)

echo CRYENGINE Version: %engine_version%

for /F "usebackq tokens=2,* skip=2" %%L in (
	`reg query "%regkeyname%" /v %engine_version%`
) do set "engine_root=%%M"

if defined engine_root (
	if not exist "%engine_root%" (
		1>&2 echo "Error: %engine_root% not found!"
		pause
		exit /b 1
	)
	
	echo CRYENGINE Root: %engine_root%

	:: Set Root Directory for this Version.
	setx %cerootvar% "%engine_root%" >NUL

	:: Copy CryEngine.Common.dll to local bin folders (May have changed).
	xcopy /Y/R/D "%engine_root%\%bin_folder%\CryEngine.Common.dll" "%bin_folder%\" >NUL
	xcopy /Y/R/D "%engine_root%\%bin_folder%\CryEngine.Common.dll" "%bin_folder%\mono\AddIns\" >NUL

	:: Link to C# Core source code.
	if not exist "%engine_root%\%cesharap_folder%" (
		1>&2 echo "Error: %engine_root%\%cesharap_folder% not found!"
		pause
		exit /b 1
	)
	if exist %cesharap_folder% (rd %cesharap_folder%)
	if not exist %cesharap_parent_folder% (md %cesharap_parent_folder%)
	mklink /J %cesharap_folder% "%engine_root%\%cesharap_folder%" >NUL
	
	:: Run MonoDevelop with local solution.
	start "" "%engine_root%\Tools\MonoDevelop\bin\MonoDevelop.exe" "%vssln%"
	
	exit
) else (
    1>&2 echo "Error: Engine version %engine_version% not found!"
	pause
    exit /b 1
)