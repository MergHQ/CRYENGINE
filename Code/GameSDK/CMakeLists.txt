option(OPTION_GAMESDK "Build GameSDK sample project" ON)
if (NOT OPTION_GAMESDK)
	return()
endif()

add_subdirectory(GameDll)
