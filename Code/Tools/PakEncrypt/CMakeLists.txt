cmake_minimum_required(VERSION 3.6)

start_sources()
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Root"
		"stdafx.cpp"
		"stdafx.h"
)
end_sources()

link_libraries(tomcrypt_staticcrt)
include_directories("../CryCommonTools/ZipDir")
add_definitions(
	-D_CRT_SECURE_NO_WARNINGS
	-Dmalloc=malloc               # for tomcrypt
	-DUNICODE
	-D_UNICODE
	-D_HAS_EXCEPTIONS=0
)
if(WINDOWS)
	add_compile_options(/wd4267 /wd4244)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options(-Wno-unused-but-set-variable)
endif()

CryConsoleApplication(PakEncrypt           PCH stdafx.cpp SOLUTION_FOLDER "Tools")
target_sources(${THIS_PROJECT}             PRIVATE "PakCrypt.cpp")
target_compile_definitions(${THIS_PROJECT} PRIVATE _ENCRYPT)
force_static_crt()

CryConsoleApplication(PakDecrypt           PCH stdafx.cpp SOLUTION_FOLDER "Tools")
target_sources(${THIS_PROJECT}             PRIVATE "PakCrypt.cpp")
target_compile_definitions(${THIS_PROJECT} PRIVATE _DECRYPT)
force_static_crt()

CryConsoleApplication(PakSign              PCH stdafx.cpp SOLUTION_FOLDER "Tools")
target_sources(${THIS_PROJECT}             PRIVATE "PakCrypt.cpp")
target_compile_definitions(${THIS_PROJECT} PRIVATE _SIGN)
force_static_crt()

CryConsoleApplication(KeyGen               PCH stdafx.cpp SOLUTION_FOLDER "Tools")
target_sources(${THIS_PROJECT}             PRIVATE "KeyGen/KeyGen.cpp")
target_compile_definitions(${THIS_PROJECT} PRIVATE _KEYGEN)
force_static_crt()

# Compiling this requires (user-supplied) header files in compatible format.
# It creates a key.dat, and is intended as a migration tool from the old workflow.
#CryConsoleApplication(KeyImport            SOLUTION_FOLDER "Tools")
#target_sources(${THIS_PROJECT}             PRIVATE "KeyGen/KeyImport.cpp" "KeyGen/key_private.h" "KeyGen/key_public.h")
#force_static_crt()

# This is provided in order to move from the current workflow back to the old one.
# It is not anticipated tha this will be necessary, but is provided as a backup.
#CryConsoleApplication(KeyExtract           SOLUTION_FOLDER "Tools")
#target_sources(${THIS_PROJECT}             PRIVATE "KeyGen/KeyExtract.cpp")
#force_static_crt()
