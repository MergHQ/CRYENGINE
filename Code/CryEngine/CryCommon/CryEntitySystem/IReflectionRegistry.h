// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/IRegistry.h>

namespace Cry {
namespace Entity {

struct IReflectionRegistry : public Reflection::IRegistry
{
public:
	virtual bool UseType(const Cry::Reflection::ITypeDesc& typeDesc) = 0;
	virtual bool UseFunction(const Cry::Reflection::IFunctionDesc& functionDesc) = 0;
};

} // ~Entity namespace
} // ~Cry namespace
