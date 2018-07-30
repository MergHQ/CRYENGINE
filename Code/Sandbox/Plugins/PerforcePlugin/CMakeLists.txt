start_sources()
# -- SamplePlugin.waf_files --
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Plugin"
		"StdAfx.cpp"
		"StdAfx.h"
		"PerforcePlugin.cpp"
		"PerforcePlugin.h"
)

add_sources("NoUberFile"
	SOURCE_GROUP "Root"
		"IPerforceConflictsResolver.h"
		"IPerforceExecutor.h"
		"IPerforceOutputParser.h"
		"PerforceApiExecutor.cpp"
		"PerforceApiExecutor.h"
		"PerforceApiOutputParser.cpp"
		"PerforceApiOutputParser.h"
		"PerforceConflictsResolver.cpp"
		"PerforceConflictsResolver.h"
        "PerforceFilePathUtil.cpp"
        "PerforceFilePathUtil.h"
        "PerforceFileStatFilter.h"
		"PerforceVCSAdapter.cpp"
		"PerforceVCSAdapter.h"
		"Md5Generator.cpp"
		"Md5Generator.h"
)

end_sources()
#END-FILE-LIST

set(P4API_DIR "${SDK_DIR}/p4api/2016.2")
if (NOT EXISTS ${P4API_DIR})
    message(WARNING "Perforce api library could not be found at ${P4API_DIR}")
    return()
endif()

CryEditorPlugin(PerforcePlugin PCH "StdAfx.cpp" SOLUTION_FOLDER "Sandbox/Plugins" DISABLE_MFC)
force_static_crt()
target_compile_definitions(${THIS_PROJECT} PRIVATE -DBOOST_ALL_NO_LIB)
target_include_directories(${THIS_PROJECT} PRIVATE "${P4API_DIR}/include")

target_include_directories(${THIS_PROJECT} PRIVATE "${SDK_DIR}/md5")

set_libpath_flag()
set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG " ${LIBPATH_FLAG}\"${P4API_DIR}/lib\"")
set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS_PROFILE " ${LIBPATH_FLAG}\"${P4API_DIR}/lib\"")
set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE " ${LIBPATH_FLAG}\"${P4API_DIR}/lib\"")
target_link_libraries(${THIS_PROJECT} PRIVATE libclient librpc libsupp libp4sslstub md5)
