// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvElement.h"

namespace Schematyc
{

// Forward declare classes.
class CComponent;
class CComponentDesc;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CComponent)

struct IEnvComponent : public IEnvElementBase<EEnvElementType::Component>
{
	virtual ~IEnvComponent() {}

	virtual const CComponentDesc& GetDesc() const = 0;
	virtual CComponentPtr         CreateFromPool() const = 0;
};

} // Schematyc
