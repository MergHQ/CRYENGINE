// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvElement.h"

namespace Schematyc
{

// Forward declare classes.
class CClassDesc;

struct IEnvSignal : public IEnvElementBase<EEnvElementType::Signal>
{
	virtual ~IEnvSignal() {}

	virtual const CClassDesc& GetDesc() const = 0;
};

} // Schematyc
