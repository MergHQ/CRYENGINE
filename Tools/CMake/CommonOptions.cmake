# Options made available to both the engine and project cmake files

option(OPTION_UQS_SCHEMATYC_SUPPORT "Enable UQS functions in Schematyc" ON)

if(OPTION_UQS_SCHEMATYC_SUPPORT)
	list(APPEND global_defines UQS_SCHEMATYC_SUPPORT=1)
else()
	list(APPEND global_defines UQS_SCHEMATYC_SUPPORT=0)
endif()
