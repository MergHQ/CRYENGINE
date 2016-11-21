// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Script/IScriptElement.h"

namespace Schematyc
{
struct IScriptInterfaceImpl : public IScriptElementBase<EScriptElementType::InterfaceImpl>
{
	virtual ~IScriptInterfaceImpl() {}

	virtual EDomain GetDomain() const = 0;
	virtual SGUID   GetRefGUID() const = 0;
};
} // Schematyc
