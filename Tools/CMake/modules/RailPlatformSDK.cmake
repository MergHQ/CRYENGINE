include_guard(GLOBAL)

if(WIN64)
	add_library(RailPlatformSDK MODULE IMPORTED GLOBAL)
	set_target_properties(RailPlatformSDK PROPERTIES PUBLIC_INCLUDE_DIRECTORIES "${SDK_DIR}/RailPlatformSDK")

	deploy_runtime_files(${SDK_DIR}/RailPlatformSDK/rail/libs/win/Release_64/rail_api64.dll)
elseif(WIN32)
	add_library(RailPlatformSDK MODULE IMPORTED GLOBAL)
	set_target_properties(RailPlatformSDK PROPERTIES PUBLIC_INCLUDE_DIRECTORIES "${SDK_DIR}/RailPlatformSDK")

	deploy_runtime_files(${SDK_DIR}/RailPlatformSDK/rail/libs/win/Release_32/rail_api.dll)
endif()


