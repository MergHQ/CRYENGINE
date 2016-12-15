// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Schematyc
{
// Forward declare interfaces.
struct IRuntimeClass;
// Forward declare structures.
struct SGUID;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IRuntimeClass)

struct IRuntimeRegistry
{
	virtual ~IRuntimeRegistry() {}

	virtual IRuntimeClassConstPtr GetClass(const SGUID& guid) const = 0;
};
} // Schematyc
