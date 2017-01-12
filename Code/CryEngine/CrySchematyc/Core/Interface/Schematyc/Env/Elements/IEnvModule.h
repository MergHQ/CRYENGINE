// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Env/IEnvElement.h"

namespace Schematyc
{

struct IEnvModule : public IEnvElementBase<EEnvElementType::Module>
{
	virtual ~IEnvModule() {}
};

} // Schematyc
