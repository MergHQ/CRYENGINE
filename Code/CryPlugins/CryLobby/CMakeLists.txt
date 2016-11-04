option(PLUGIN_CRYLOBBY "Enable CryLobby" ON)
if (NOT PLUGIN_CRYLOBBY)
	return()
endif()

add_subdirectory(Module)
