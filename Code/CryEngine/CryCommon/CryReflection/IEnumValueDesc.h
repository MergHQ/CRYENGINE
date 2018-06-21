// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/BaseTypes.h>

namespace Cry {
namespace Reflection {

struct IEnumValueDesc
{
	virtual ~IEnumValueDesc() {}

	virtual const char* GetLabel() const = 0;
	virtual const char* GetDescription() const = 0;
	virtual uint64      GetValue() const = 0;
};

} // ~Reflection namespace
} // ~Cry namespace
