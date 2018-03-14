start_sources()
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Source Files"
		"ParticleSystemUnitTests.cpp"
)
end_sources()

CryUnitTestSuite(Cry3DEngineUnitTest SOLUTION_FOLDER "UnitTests")
use_scaleform()
include_directories("${CRYENGINE_DIR}/Code/CryEngine/Cry3DEngine")
target_compile_definitions(${THIS_PROJECT} PRIVATE _LIB CRY_IS_MONOLITHIC_BUILD _LAUNCHER)
if (NOT OPTION_STATIC_LINKING)
	target_link_libraries(${THIS_PROJECT} PRIVATE CrySystemLib Cry3DEngineLib)
else()
	target_link_libraries(${THIS_PROJECT} PRIVATE CrySystem Cry3DEngine)
endif()

