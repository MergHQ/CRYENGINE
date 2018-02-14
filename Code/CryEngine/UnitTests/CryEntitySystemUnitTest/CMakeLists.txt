start_sources()
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Source Files"
		"Entity.cpp"
)
end_sources()

CryUnitTestSuite(CryEntitySystemUnitTest SOLUTION_FOLDER "UnitTests")
use_scaleform()
include_directories("${CRYENGINE_DIR}/Code/CryEngine/CrySystem")
include_directories("${CRYENGINE_DIR}/Code/CryEngine/CryEntitySystem")
target_compile_definitions(${THIS_PROJECT} PRIVATE _LIB CRY_IS_MONOLITHIC_BUILD _LAUNCHER)
if (NOT ORBIS)
	target_link_libraries(${THIS_PROJECT} PRIVATE CrySystemLib)
	target_link_libraries(${THIS_PROJECT} PRIVATE CryEntitySystemLib)
endif()

