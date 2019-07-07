// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>

namespace Cry {
namespace Reflection {

class CFunction;
struct IRegistry;
struct ITypeDesc;
struct IFunctionDesc;

typedef void (* ReflectTypeFunction)(ITypeDesc& typeDesc);
typedef void (* ReflectFunctionFunction)(IFunctionDesc& functionDesc);

struct ICoreRegistry
{
	virtual ITypeDesc*           RegisterType(const Type::CTypeDesc& typeDesc, CGuid guid, ReflectTypeFunction pReflectFunc, const SSourceFileInfo& srcPos) = 0;
	virtual TypeIndex::ValueType GetTypeCount() const = 0;
	virtual const ITypeDesc*     GetTypeByIndex(TypeIndex index) const = 0;
	virtual const ITypeDesc*     GetTypeByGuid(CGuid guid) const = 0;
	virtual const ITypeDesc*     GetTypeById(CTypeId typeId) const = 0;

	virtual IFunctionDesc*       RegisterFunction(const CFunction& function, CGuid guid, const char* szLabel, ReflectFunctionFunction pReflectFunc, const SSourceFileInfo& srcPos) = 0;
	virtual TypeIndex::ValueType GetFunctionCount() const = 0;
	virtual const IFunctionDesc* GetFunctionByIndex(FunctionIndex index) const = 0;
	virtual const IFunctionDesc* GetFunctionByGuid(CGuid guid) const = 0;
	virtual const IFunctionDesc* GetFunctionByName(const char* szName) = 0;

	virtual size_t               GetRegistriesCount() const = 0;
	virtual IRegistry*           GetRegistryByIndex(size_t index) const = 0;
	virtual IRegistry*           GetRegistryByGuid(CGuid guid) const = 0;
	virtual IRegistry*           GetRegistryById(CTypeId typeId) const = 0;

	virtual void                 RegisterRegistry(IRegistry* pRegistry) = 0;
};

} // ~Reflection namespace
} // ~Cry namespace
