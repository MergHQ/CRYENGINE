get_filename_component(_CryCommon_install_prefix "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)

# Load dependencies
include(${CMAKE_CURRENT_LIST_DIR}/BoostConfig.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/yasliConfig.cmake)


set(CryCommon_INCLUDE_DIRS ${_CryCommon_install_prefix}/include/CryCommon ${yasli_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

