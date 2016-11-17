// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Script/IScriptElement.h"

namespace Schematyc
{
struct IScriptState : public IScriptElementBase<EScriptElementType::State>
{
	virtual ~IScriptState() {}

	virtual SGUID GetPartnerGUID() const = 0;
};
} // Schematyc
