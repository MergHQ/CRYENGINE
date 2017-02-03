// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Script/IScriptElement.h"
#include "Schematyc/Utils/EnumFlags.h"
#include "Schematyc/Utils/PreprocessorUtils.h"

namespace Schematyc
{

// Forward declare classes.
class CClassProperties;
class CTransform;

enum class EScriptComponentInstanceFlags
{
	None     = 0,
	EnvClass = BIT(0)   // Component is part of environment class and can't be removed.
};

typedef CEnumFlags<EScriptComponentInstanceFlags> ScriptComponentInstanceFlags;

struct IScriptComponentInstance : public IScriptElementBase<EScriptElementType::ComponentInstance>
{
	virtual ~IScriptComponentInstance() {}

	virtual SGUID                        GetTypeGUID() const = 0;
	virtual ScriptComponentInstanceFlags GetComponentInstanceFlags() const = 0;
	virtual bool                         HasTransform() const = 0;
	virtual void                         SetTransform(const CTransform& transform) = 0;
	virtual const CTransform&            GetTransform() const = 0;
	virtual const CClassProperties&      GetProperties() const = 0;
};

} // Schematyc
