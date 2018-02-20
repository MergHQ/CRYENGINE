// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved. 
#include <CryCore/Platform/platform.h>

#if defined(_LIB) && CRY_PLATFORM_DURANGO
#include "CryPhysics/physinterface.h"
//A Mock CryPhysics for current set up of project dependencies
IPhysicalWorld *CreatePhysicalWorld(ISystem *pSystem)
{
	return nullptr;
}
#endif
