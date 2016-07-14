add_library(Boost INTERFACE IMPORTED)
get_filename_component(_Boost_install_prefix "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)

set_target_properties(Boost PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${_Boost_install_prefix}/include/SDKs/boost)

