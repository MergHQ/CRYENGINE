@ECHO OFF
REM Script that generates the header and source files for HDF5
REM Requires H5pubconf.h has already been created (with CMake or manually)
REM Run this after updating H5pubconf.h (or updating the SDK)
REM Make sure to check out the other files from source-control, so they are writable

REM Test for H5pubconf.h
IF NOT EXIST H5pubconf.h (
	ECHO H5pubconf.h must exist in Code\Libs\hdf5, other headers will be created
	EXIT /B 1
)

REM Test for perl
>NUL PERL --version
IF ERRORLEVEL 1 (
	ECHO Perl not in PATH
	EXIT /B 1
)

REM Relative to this file
SET H5SDK=..\..\SDKs\hdf5

REM Resolve full path to H5 SDK
PUSHD %H5SDK%
SET H5SDK=%CD%
POPD

REM Execute perl scripts to create headers
REM Note: These are stored in the current directory (ie, Libs/hdf5)
ECHO Generating headers from PERL
PERL %H5SDK%\bin\make_err %H5SDK%\src\H5err.txt
PERL %H5SDK%\bin\make_vers %H5SDK%\src\H5vers.txt
PERL %H5SDK%\bin\make_overflow %H5SDK%\src\H5overflow.txt

REM Create a temporary spec file for WAF
PUSHD ..\..\..
PUSHD _WAF_\specs
>h5.json ECHO {
>>h5.json ECHO "description" : "Temp configuration for H5 tools",
>>h5.json ECHO "valid_platforms" : [ "win_x64" ],
>>h5.json ECHO "valid_configuration" : [ "release" ],
>>h5.json ECHO "visual_studio_name" : "H5",
>>h5.json ECHO "modules" : [ "H5Detect", "H5LibSettings" ]
>>h5.json ECHO }
POPD

REM Build H5 tools as specified in wscript
REM The binaries will be stored in Code\Libs\hdf5\bin
ECHO Building tools
cry_waf.exe build_win_x64_release --project-spec h5 --output-folder-win64 Code\Libs\hdf5\bin --output-extension-release= --ask-for-user-input False --console-mode True --generate-vs-projects-automatically False
DEL _WAF_\specs\h5.json
POPD

REM Run the tools just generated with WAF
ECHO Executing tools to generate sources
>H5Tinit.c bin\H5Detect.exe
>H5lib_settings.c bin\H5LibSettings.exe

REM Clean up the bin folder, it doesn't have to be checked in
ECHO Done
RMDIR /S /Q bin
