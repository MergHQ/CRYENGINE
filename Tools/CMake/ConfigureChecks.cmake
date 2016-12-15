# Check for required functions/symbols provided by some of CE platforms.
include(CheckFunctionExists)


CHECK_FUNCTION_EXISTS(stpcpy STPCPYEXISTS)
if(STPCPYEXISTS)
	add_definitions(-DHAS_STPCPY)
endif()


