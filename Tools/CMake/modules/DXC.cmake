if(WINDOWS)
    add_library(DXC SHARED IMPORTED GLOBAL)
    set_target_properties(DXC PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "USE_DXC"
        INTERFACE_INCLUDE_DIRECTORIES "${SDK_DIR}/DXC"
    )

    include_directories("${SDK_DIR}/DXC/include")

    set_target_properties(DXC PROPERTIES IMPORTED_LOCATION "${SDK_DIR}/DXC/bin/dxcompiler.dll")
    set_target_properties(DXC PROPERTIES IMPORTED_IMPLIB "${SDK_DIR}/DXC/lib/dxcompiler.lib")
    set_target_properties(DXC PROPERTIES INTERFACE_LINK_LIBRARIES "${SDK_DIR}/DXC/lib/dxcompiler.lib")
    deploy_runtime_files("${SDK_DIR}/DXC/bin/dxcompiler.dll")
endif()
