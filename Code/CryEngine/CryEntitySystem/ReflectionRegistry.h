// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IReflectionRegistry.h>
#include <CryReflection/Framework.h>
#include <CryReflection/Framework_impl.inl>

namespace Cry {
namespace Entity {

class CReflectionRegistry final : public Cry::Reflection::CRegistry<IReflectionRegistry>
{
public:
	CReflectionRegistry();

	// IReflectionRegistry
	virtual bool UseType(const Cry::Reflection::ITypeDesc& typeDesc) final
	{
		return Cry::Reflection::CRegistry<IReflectionRegistry>::UseType_Internal(typeDesc);
	}

	virtual bool UseFunction(const Cry::Reflection::IFunctionDesc& functionDesc) final
	{
		return Cry::Reflection::CRegistry<IReflectionRegistry>::UseFunction_Internal(functionDesc);
	}
	// ~IReflectionRegistry
};

} // ~Entity namespace
} // ~Cry namespace
