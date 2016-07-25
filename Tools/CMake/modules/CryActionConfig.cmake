

set (CRYENGINE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")
#add_subdirectory ("${CRYENGINE_SOURCE_DIR}/Code/CryEngine/CryAction" "${CMAKE_CURRENT_BINARY_DIR}/blah")

add_library(CryAction INTERFACE IMPORTED)
set_property(TARGET CryAction PROPERTY INTERFACE_INCLUDE_DIRECTORIES
	"${CRYENGINE_SOURCE_DIR}/Code/CryEngine/CryAction"
)

