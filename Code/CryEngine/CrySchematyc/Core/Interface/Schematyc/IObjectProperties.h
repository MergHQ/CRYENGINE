// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/Forward.h>

namespace Schematyc
{
// Forward declare interfaces.
struct IObjectProperties;
// Forward declare structures.
struct SGUID;
// Forward declare classes.
class CAnyRef;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IObjectProperties)

struct IObjectProperties // #SchematycTODO : Derive from IProperties?
{
	virtual ~IObjectProperties() {}

	virtual IObjectPropertiesPtr Clone() const = 0;
	virtual void                 Serialize(Serialization::IArchive& archive) = 0;
	virtual bool                 Read(const CAnyRef& value, const SGUID& guid) const = 0;
};
} // Schematyc
