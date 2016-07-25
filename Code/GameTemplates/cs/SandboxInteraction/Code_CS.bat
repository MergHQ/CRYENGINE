@echo OFF

set cesharp_parent_folder=Code\CryManaged
set cesharp_folder=%cesharp_parent_folder%\CESharp
set vssln=Code\SandboxInteraction.sln

set projectcfg=project.cfg
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

for /F "usebackq tokens=2,* skip=2" %%L in (
	`reg query "HKEY_CURRENT_USER\Software\Crytek\CryEngine" /v %engine_version%`	
) do set "engine_root=%%M"

if not defined engine_root (
	for /F "usebackq tokens=2,* skip=2" %%L in (
		`reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Crytek\CryEngine" /v %engine_version%`
	) do set "engine_root=%%M"
)

if defined engine_root (
	if not exist "%engine_root%" (
		1>&2 echo "Error: %engine_root% not found!"
		pause
		exit /b 1
	)
	
	:: Set Root Directory for this Version.
	set "CRYENGINEROOT=%engine_root%"
	
	:: Copy CryEngine.Common.dll to local bin folders (May have changed).
	xcopy /Y/R/D "%engine_root%\bin\win_x64\CryEngine.Common.*" "bin\win_x64\" >NUL
	xcopy /Y/R/D "%engine_root%\bin\win_x64\CryEngine.Common.*" "bin\win_x64\mono\AddIns\" >NUL

	:: Link to C# Core source code.
	if not exist "%engine_root%\%cesharp_folder%" (
		1>&2 echo "Error: %engine_root%\%cesharp_folder% not found!"
		pause
		exit /b 1
	)
	if exist %cesharp_folder% (rd %cesharp_folder%)
	if not exist %cesharp_parent_folder% (md %cesharp_parent_folder%)
	mklink /J %cesharp_folder% "%engine_root%\%cesharp_folder%" >NUL
	
	:: Run MonoDevelop with local solution.
	start "" "%engine_root%\Tools\MonoDevelop\bin\MonoDevelop.exe" "%vssln%"

) else (
    1>&2 echo "Error: Engine version %engine_version% not found!"
	pause
    exit /b 1
)
