// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>

namespace Cry {
namespace Reflection {

struct ITypeDesc;
struct IFunctionDesc;

struct IRegistry
{
	virtual ~IRegistry() {}

	virtual CGuid                    GetGuid() const = 0;
	virtual const char*              GetLabel() const = 0;
	virtual const Type::CTypeDesc&   GetTypeDesc() const = 0;
	virtual CTypeId                  GetTypeId() const = 0;

	virtual TypeIndex::ValueType     GetTypeCount() const = 0;
	virtual const ITypeDesc*         GetTypeByIndex(TypeIndex index) const = 0;
	virtual const ITypeDesc*         GetTypeByGuid(CGuid guid) const = 0;
	virtual const ITypeDesc*         GetTypeById(CTypeId typeId) const = 0;

	virtual FunctionIndex::ValueType GetFunctionCount() const = 0;
	virtual const IFunctionDesc*     GetFunctionByIndex(FunctionIndex index) const = 0;
	virtual const IFunctionDesc*     GetFunctionByGuid(CGuid guid) const = 0;
	virtual const IFunctionDesc*     GetFunctionByName(const char* szName) const = 0;
};

} // ~Reflection namespace
} // ~Cry namespace
