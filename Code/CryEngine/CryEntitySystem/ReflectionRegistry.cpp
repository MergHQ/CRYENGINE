// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include "ReflectionRegistry.h"

namespace Cry {
namespace Entity {

CReflectionRegistry::CReflectionRegistry()
	: Reflection::CRegistry<IReflectionRegistry>("Entity Registry", Type::DescOf<CReflectionRegistry>(), "{97434587-500E-40E4-85F0-141CD90D8902}"_cry_guid)
{
}

} // ~Entity namespace
} // ~Cry namespace
