// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef cryphysics_h
#define cryphysics_h
#pragma once

#ifdef PHYSICS_EXPORTS
	#define CRYPHYSICS_API DLL_EXPORT
#else
	#define CRYPHYSICS_API DLL_IMPORT
#endif

#define vector_class Vec3_tpl


#include <CryMemory/CrySizer.h>



//#include "utils.h"
#include <CryMath/Cry_Math.h>
#include <CryPhysics/primitives.h>

extern "C" CRYPHYSICS_API IPhysicalWorld *CreatePhysicalWorld(struct ISystem *pLog);

#endif