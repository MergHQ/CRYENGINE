// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "VariableDesc.h"

#include <CryReflection/Framework.h>

namespace Cry {
namespace Reflection {

const ITypeDesc* CVariableDesc::GetTypeDesc() const
{
	return GetCoreRegistry()->GetTypeById(m_typeId);
}

} // ~Reflection namespace
} // ~Cry namespace
