// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvElement.h"
#include "Schematyc/Utils/EnumFlags.h"

namespace Schematyc
{
// Forward declare classes.
class CAnyValue;
class CCommonTypeInfo;
class CTypeName;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CAnyValue)

enum class EEnvDataTypeFlags
{
	None       = 0,
	Switchable = BIT(0) // #SchematycTODO : Do we still need this or can we just say that any type with a toString method is switchable?
};

typedef CEnumFlags<EEnvDataTypeFlags> EnvDataTypeFlags;

struct IEnvDataType : public IEnvElementBase<EEnvElementType::DataType>
{
	virtual ~IEnvDataType() {}

	virtual EnvDataTypeFlags       GetFlags() const = 0;
	virtual const CCommonTypeInfo& GetTypeInfo() const = 0;
	virtual CAnyValuePtr           Create() const = 0;
};
} // Schematyc
