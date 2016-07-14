add_library(Boost INTERFACE IMPORTED)
set_target_properties(Boost PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${SDK_DIR}/boost)

install(DIRECTORY ${SDK_DIR}/boost/boost DESTINATION include/SDKs/boost)
install(FILES ${CMAKE_CURRENT_LIST_DIR}/BoostConfig.cmake DESTINATION share/cmake)
