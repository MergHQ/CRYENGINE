// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Script/IScriptElement.h"
#include "Schematyc/Utils/EnumFlags.h"
#include "Schematyc/Utils/PreprocessorUtils.h"

namespace Schematyc
{
// Forward declare interfaces.
struct IProperties;
// Forward declare structures.

// Forward declare classes.
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
	virtual const IProperties*           GetProperties() const = 0;
};
} // Schematyc
