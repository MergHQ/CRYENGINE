

set (CRYENGINE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

add_library(CryBrofiler INTERFACE IMPORTED)
set_target_properties(CryBrofiler PROPERTIES
	INTERFACE_COMPILE_DEFINITIONS "USE_BROFILER"
	INTERFACE_INCLUDE_DIRECTORIES "${CRYENGINE_SOURCE_DIR}/Code/SDKs/Brofiler"	
)

if (CMAKE_CL_64)
	set_target_properties(CryBrofiler PROPERTIES INTERFACE_LINK_LIBRARIES "${CRYENGINE_SOURCE_DIR}/Code/SDKs/Brofiler/ProfilerCore64.lib")
else()
	set_target_properties(CryBrofiler PROPERTIES INTERFACE_LINK_LIBRARIES "${CRYENGINE_SOURCE_DIR}/Code/SDKs/Brofiler/ProfilerCore32.lib")
endif(CMAKE_CL_64)
