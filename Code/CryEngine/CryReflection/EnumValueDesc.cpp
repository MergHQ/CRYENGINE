// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "EnumValueDesc.h"

namespace Cry {
namespace Reflection {

CEnumValueDesc::CEnumValueDesc()
{

}

CEnumValueDesc::CEnumValueDesc(const char* szLabel, uint64 value, const char* szDescription)
	: m_label(szLabel)
	, m_description(szDescription)
	, m_value(value)
{

}

} // ~Reflection namespace
} // ~Cry namespace
