// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>
#include <CryType/TypeFunctions.h>

namespace Cry {
namespace Reflection {

struct ITypeDesc;

struct IBaseTypeDesc
{
	virtual ~IBaseTypeDesc() {}

	virtual CTypeId                 GetTypeId() const = 0;
	virtual Type::CBaseCastFunction GetBaseCastFunction() const = 0;
	virtual const ITypeDesc*        GetTypeDesc() const = 0;
};

} // ~Reflection namespace
} // ~Cry namespace
