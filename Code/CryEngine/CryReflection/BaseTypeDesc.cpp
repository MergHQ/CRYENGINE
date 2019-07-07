// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "BaseTypeDesc.h"

#include <CryReflection/Framework.h>

namespace Cry {
namespace Reflection {

CBaseTypeDesc::CBaseTypeDesc(CTypeId typeId, Type::CBaseCastFunction baseCastFunction)
	: m_typeId(typeId)
	, m_baseCastFunction(baseCastFunction)
{

}

CTypeId CBaseTypeDesc::GetTypeId() const
{
	return m_typeId;
}

Type::CBaseCastFunction CBaseTypeDesc::GetBaseCastFunction() const
{
	return m_baseCastFunction;
}

const ITypeDesc* CBaseTypeDesc::GetTypeDesc() const
{
	return CoreRegistry::GetTypeById(m_typeId);
}

} // ~Reflection namespace
} // ~Cry namespace
