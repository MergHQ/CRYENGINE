// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/IBaseTypeDesc.h>
#include <CryReflection/ITypeDesc.h>

namespace Cry {
namespace Reflection {

class CBaseTypeDesc final : public IBaseTypeDesc
{
public:
	CBaseTypeDesc(CTypeId typeId, Type::CBaseCastFunction baseCastFunction);

	// IBaseTypeDesc
	virtual CTypeId                 GetTypeId() const final;
	virtual Type::CBaseCastFunction GetBaseCastFunction() const final;
	virtual const ITypeDesc*        GetTypeDesc() const final;
	// ~IBaseTypeDesc
private:
	CTypeId                 m_typeId;
	Type::CBaseCastFunction m_baseCastFunction;
};

} // ~Reflection namespace
} // ~Cry namespace
