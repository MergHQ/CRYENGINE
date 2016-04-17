@ECHO OFF
REM Script that generates ILM header filles
REM Run this after updating the SDK
REM Make sure to check out the other files from source-control, so they are writable

REM Create a temporary spec file for WAF
PUSHD ..\..\..
PUSHD _WAF_\specs
>ilm.json ECHO {
>>ilm.json ECHO "description" : "Temp configuration for ILM tools",
>>ilm.json ECHO "valid_platforms" : [ "win_x64" ],
>>ilm.json ECHO "valid_configuration" : [ "release" ],
>>ilm.json ECHO "visual_studio_name" : "ILM",
>>ilm.json ECHO "modules" : [ "toFloat", "eLut" ]
>>ilm.json ECHO }
POPD

REM Build H5 tools as specified in wscript
REM The binaries will be stored in Code\Libs\hdf5\bin
ECHO Building tools
cry_waf.exe build_win_x64_release --project-spec ilm --output-folder-win64 Code\Libs\ilmbase\bin --output-extension-release= --ask-for-user-input False --console-mode True --generate-vs-projects-automatically False
DEL _WAF_\specs\ilm.json
POPD

REM Run the tools just generated with WAF
ECHO Executing tools to generate sources
>toFloat.h bin\toFloat.exe
>eLut.h bin\eLut.exe

REM Clean up the bin folder, it doesn't have to be checked in
ECHO Done
RMDIR /S /Q bin
