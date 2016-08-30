
add_library(yasli INTERFACE IMPORTED)

set (CRYENGINE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")
#target_include_directories(yasli INTERFACE
#	"D:/code/ce_main/Code/SDKs/yasli"
#	"D:/code/ce_main/Code/Libs/yasli"
#)
set_property(TARGET yasli PROPERTY INTERFACE_INCLUDE_DIRECTORIES
	"${CRYENGINE_SOURCE_DIR}/Code/SDKs/yasli"
	"${CRYENGINE_SOURCE_DIR}/Code/Libs/yasli"
)