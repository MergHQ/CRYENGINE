option(OPTION_GAMEZERO "Build GameZero sample project" OFF)
if (NOT OPTION_GAMEZERO)
	return()
endif()

add_subdirectory(GameDll)
