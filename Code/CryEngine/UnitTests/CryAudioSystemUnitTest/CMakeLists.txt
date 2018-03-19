start_sources()
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Source Files"
		"AudioSystemTests.cpp"
)
end_sources()

CryUnitTestSuite(CryAudioSystemUnitTest SOLUTION_FOLDER "UnitTests")
use_scaleform()
include_directories("${CRYENGINE_DIR}/Code/CryEngine/CryAudioSystem")
include_directories("${CRYENGINE_DIR}/Code/CryEngine/CryAudioSystem/Common")
target_compile_definitions(${THIS_PROJECT} PRIVATE _LIB CRY_IS_MONOLITHIC_BUILD _LAUNCHER)
if (NOT OPTION_STATIC_LINKING)
	target_link_libraries(${THIS_PROJECT} PRIVATE CrySystemLib CryAudioSystemLib)
else()
	target_link_libraries(${THIS_PROJECT} PRIVATE CrySystem CryAudioSystem)
endif()

