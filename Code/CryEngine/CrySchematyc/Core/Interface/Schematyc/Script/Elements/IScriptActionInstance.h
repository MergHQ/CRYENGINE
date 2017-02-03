// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Script/IScriptElement.h"

namespace Schematyc
{

// Forward declare classes.
class CClassProperties;

struct IScriptActionInstance : public IScriptElementBase<EScriptElementType::ActionInstance>
{
	virtual ~IScriptActionInstance() {}

	virtual SGUID                   GetActionTypeGUID() const = 0;
	virtual SGUID                   GetComponentInstanceGUID() const = 0;
	virtual const CClassProperties& GetProperties() const = 0;
};

} // Schematyc
