@echo OFF

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
	set "CRYENGINEROOT=%engine_root%"
	start "" "Code\Game\Solutions\Game.sln"
	exit
) else (
    1>&2 echo "Error: Engine version %engine_version% not found!"
	pause
    exit /b 1
)