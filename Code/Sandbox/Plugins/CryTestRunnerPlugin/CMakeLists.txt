start_sources()
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Root"
		"CryTestNode.h"
		"CryTestRunnerPlugin.h"
		"CryTestRunnerPlugin.cpp"
		"CryTestRunnerSystem.h"
		"CryTestRunnerSystem.cpp"
		"StdAfx.h"
		"StdAfx.cpp"
)

end_sources()

CryEditorPlugin(CryTestRunnerPlugin PCH "StdAfx.cpp" SOLUTION_FOLDER "Sandbox/Plugins")

