#START-FILE-LIST

start_sources()
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Root"
		"${SDK_DIR}/Detours/4.0.1/src/creatwth.cpp"
		"${SDK_DIR}/Detours/4.0.1/src/detours.cpp"
		"${SDK_DIR}/Detours/4.0.1/src/detours.h"
		"${SDK_DIR}/Detours/4.0.1/src/detver.h"
		"${SDK_DIR}/Detours/4.0.1/src/disasm.cpp"
		"${SDK_DIR}/Detours/4.0.1/src/disolarm.cpp"
		"${SDK_DIR}/Detours/4.0.1/src/disolarm64.cpp"
		"${SDK_DIR}/Detours/4.0.1/src/disolia64.cpp"
		"${SDK_DIR}/Detours/4.0.1/src/disolx64.cpp"
		"${SDK_DIR}/Detours/4.0.1/src/disolx86.cpp"
		"${SDK_DIR}/Detours/4.0.1/src/image.cpp"
		"${SDK_DIR}/Detours/4.0.1/src/modules.cpp"
	)

end_sources()
#END-FILE-LIST

CryEngineStaticModule(Detours SOLUTION_FOLDER "Libs" MSVC_PERMISSIVE)
target_compile_options(${THIS_PROJECT} PRIVATE /W4 /WX /Zi /MT /Gy /Gm- /Zl /Od)
target_compile_definitions(${THIS_PROJECT} PRIVATE _AMD64_ WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x501) # _AMD64_ is required by detours.h, right now we only support win64
target_include_directories(${THIS_PROJECT} INTERFACE "${SDK_DIR}/Detours/4.0.1/src")
