// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Utils/Delegate.h"

namespace Schematyc
{
// Forward declare structures.
	struct SGUID;

typedef CDelegate<void (const SGUID&)> ScriptDependencyEnumerator;

enum class EScriptDependencyType
{
	Load,
	Compile,
	Event,
	Reference
};
} // Schematyc
