// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "CryCore/Platform/CryPlatformDefines.h"
#include <CryCore/Platform/platform.h>

#if defined(_LIB) && CRY_PLATFORM_DURANGO
#include "CryPhysics/physinterface.h"
//A hack due to the current dependency setup
IPhysicalWorld *CreatePhysicalWorld(ISystem *pSystem)
{
	return nullptr;
}
#endif

