// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#include <CryCore/Platform/platform.h>
#include <CryCore/Platform/platform_impl.inl>
#include <CryCore/Platform/CryPlatformDefines.h>
#include "UnitTest.h"

#if CRY_PLATFORM_WINDOWS
#include <tchar.h>

GTEST_API_ int main(int argc, char **argv) {

// Essential step to allow editor plugins to be delay loaded
// Editor plugin DLLs are placed in a sub folder, thus can't be loaded by the test executable directly.
// See also linker flag /DELAYLOAD
#if defined(EDITOR_PLUGIN_UNIT_TEST)
	TCHAR pluginDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, pluginDir);
	_tcscat(pluginDir, TEXT("\\EditorPlugins"));
	SetDllDirectory(pluginDir);
#endif

	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

#elif CRY_PLATFORM_LINUX

GTEST_API_ int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

#endif
