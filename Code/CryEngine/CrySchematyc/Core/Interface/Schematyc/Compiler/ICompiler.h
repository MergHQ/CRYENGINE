// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Utils/Signal.h"

namespace Schematyc
{
// Forward declare interfaces.
struct IRuntimeClass;
// Forward declare structures.
struct SGUID;

typedef CSignal<void (const IRuntimeClass&)> ClassCompilationSignal;

struct ICompiler
{
	virtual ~ICompiler() {}

	virtual void                           CompileAll() = 0;
	virtual void                           CompileDependencies(const SGUID& guid) = 0;
	virtual ClassCompilationSignal::Slots& GetClassCompilationSignalSlots() = 0;
};
} // Schematyc
