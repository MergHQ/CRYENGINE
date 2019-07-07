// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryReflection/ICoreRegistry.h>

#include <CrySystem/ICryPlugin.h>
#include <CrySystem/IEngineModule.h>

#include <limits>
#include <type_traits>

#ifdef CRYREFLECTION_EXPORTS
	#define CRY_REFLECTION_API DLL_EXPORT
#else
	#define CRY_REFLECTION_API DLL_IMPORT
#endif

namespace Cry {
namespace Reflection {

struct IModule : public Cry::IDefaultModule
{
	CRYINTERFACE_DECLARE_GUID(IModule, "{4465B4A8-4E5F-4813-85E0-A187A849AA7B}"_cry_guid);

private:
	friend ICoreRegistry*  GetCoreRegistry();
	virtual ICoreRegistry* GetCoreRegistry() = 0;
};

inline ICoreRegistry* GetCoreRegistry()
{
	if (gEnv->pReflection)
	{
		ICoreRegistry* pCoreRegistry = gEnv->pReflection->GetCoreRegistry();
		if (pCoreRegistry)
		{
			return pCoreRegistry;
		}
		CRY_ASSERT(pCoreRegistry, "Core registry must be non-null.");
	}
	CRY_ASSERT(gEnv->pReflection, "Reflection module not yet initialized.");

	return nullptr;
}

namespace CoreRegistry {

inline TypeIndex::ValueType GetTypeCount()
{
	ICoreRegistry* pCoreRegistry = Reflection::GetCoreRegistry();
	if (pCoreRegistry)
	{
		return pCoreRegistry->GetTypeCount();
	}
	return TypeIndex();
}

inline const Reflection::ITypeDesc* GetTypeById(CTypeId typeId)
{
	ICoreRegistry* pCoreRegistry = Reflection::GetCoreRegistry();
	if (pCoreRegistry)
	{
		return pCoreRegistry->GetTypeById(typeId);
	}
	return nullptr;
}

inline const Reflection::ITypeDesc* GetTypeByIndex(TypeIndex index)
{
	ICoreRegistry* pCoreRegistry = Reflection::GetCoreRegistry();
	if (pCoreRegistry)
	{
		return pCoreRegistry->GetTypeByIndex(index);
	}
	return nullptr;
}

inline const Reflection::ITypeDesc* GetTypeByGuid(CGuid guid)
{
	ICoreRegistry* pCoreRegistry = Reflection::GetCoreRegistry();
	if (pCoreRegistry)
	{
		return pCoreRegistry->GetTypeByGuid(guid);
	}
	return nullptr;
}

inline TypeIndex::ValueType GetFunctionCount()
{
	ICoreRegistry* pCoreRegistry = Reflection::GetCoreRegistry();
	if (pCoreRegistry)
	{
		return pCoreRegistry->GetFunctionCount();
	}
	return TypeIndex();
}

inline const Reflection::IFunctionDesc* GetFunctionByIndex(FunctionIndex index)
{
	ICoreRegistry* pCoreRegistry = Reflection::GetCoreRegistry();
	if (pCoreRegistry)
	{
		return pCoreRegistry->GetFunctionByIndex(index);
	}
	return nullptr;
}

inline const Reflection::IFunctionDesc* GetFunctionByGuid(CGuid guid)
{
	ICoreRegistry* pCoreRegistry = Reflection::GetCoreRegistry();
	if (pCoreRegistry)
	{
		return pCoreRegistry->GetFunctionByGuid(guid);
	}
	return nullptr;
}

inline const Reflection::IFunctionDesc* GetFunctionByName(const char* name)
{
	ICoreRegistry* pCoreRegistry = Reflection::GetCoreRegistry();
	if (pCoreRegistry)
	{
		return pCoreRegistry->GetFunctionByName(name);
	}
	return nullptr;
}

inline size_t GetRegistriesCount()
{
	ICoreRegistry* pCoreRegistry = Reflection::GetCoreRegistry();
	if (pCoreRegistry)
	{
		return pCoreRegistry->GetRegistriesCount();
	}
	return size_t();
}

inline IRegistry* GetRegistryByIndex(size_t index)
{
	ICoreRegistry* pCoreRegistry = Reflection::GetCoreRegistry();
	if (pCoreRegistry)
	{
		return pCoreRegistry->GetRegistryByIndex(index);
	}
	return nullptr;
}

} // ~CoreRegistry namespace

} // ~Reflection namespace
} // ~Cry namespace
