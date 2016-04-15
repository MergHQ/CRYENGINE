// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "System.h"
#if CRY_PLATFORM_ANDROID && defined(DEDICATED_SERVER)
	#include "AndroidConsole.h"
#endif

#include "UnixConsole.h"
#if CRY_PLATFORM_LINUX
extern __attribute__((visibility("default"))) CUNIXConsole* pUnixConsole;
#endif
