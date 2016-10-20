option(EXTENSION_CRYLOBBY "Enable CryLobby" ON)
if (NOT EXTENSION_CRYLOBBY)
	return()
endif()

add_subdirectory(Module)
