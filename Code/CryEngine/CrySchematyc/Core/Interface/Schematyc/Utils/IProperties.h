// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>

namespace Schematyc
{
// Forward declare interfaces.
struct IProperties;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IProperties)

struct IProperties
{
	virtual ~IProperties() {}

	virtual void*            GetValue() = 0;
	virtual const void*      GetValue() const = 0;
	virtual IPropertiesPtr   Clone() const = 0;
	virtual void             Serialize(Serialization::IArchive& archive) = 0;
};
} // Schematyc
