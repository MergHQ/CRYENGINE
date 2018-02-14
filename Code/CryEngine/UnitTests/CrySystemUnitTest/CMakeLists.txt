start_sources()
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Source Files"
        "xml.cpp"
	SOURCE_GROUP "Root"
		"StdAfx.cpp"
		"StdAfx.h"
)
end_sources()

CryUnitTestSuite(CrySystemUnitTest PCH "StdAfx.cpp" SOLUTION_FOLDER "UnitTests")

use_scaleform()
include_directories("${CRYENGINE_DIR}/Code/CryEngine/CrySystem")
target_compile_definitions(${THIS_PROJECT} PRIVATE _LIB CRY_IS_MONOLITHIC_BUILD _LAUNCHER)
if (NOT ORBIS)
	target_link_libraries(${THIS_PROJECT} PRIVATE CrySystemLib)
endif()

