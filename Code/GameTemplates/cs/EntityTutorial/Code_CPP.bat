@echo OFF

setlocal ENABLEEXTENSIONS
set projectcfg=project.cfg
set vssln=Code\Game\Solutions\Game.sln
set cerootvar=CRYENGINEROOT
set regkeyname=HKEY_CURRENT_USER\Software\Crytek\CryEngine

if not exist "%projectcfg%" (
    1>&2 echo "Error: %projectcfg% not found!"
	pause
    exit /b 1
)

echo CRYENGINE Version: %engine_version%

for /F "tokens=*" %%I in (%projectcfg%) do set %%I

if not defined engine_version (
	1>&2 echo "Error: no engine_version entry found in %projectcfg%!"
	pause
    exit /b 1
)

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
	
	start "" "%vssln%"
	exit
) else (
    1>&2 echo "Error: Engine version %engine_version% not found!"
	pause
    exit /b 1
)