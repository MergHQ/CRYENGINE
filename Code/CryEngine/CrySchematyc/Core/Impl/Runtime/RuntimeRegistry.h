// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Runtime/IRuntimeRegistry.h>
#include <Schematyc/Utils/GUID.h>

#ifdef RegisterClass
#undef RegisterClass
#endif

namespace Schematyc
{
// Forward declare classes.
class CRuntimeClass;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CRuntimeClass)

class CRuntimeRegistry : public IRuntimeRegistry
{
private:

	typedef std::unordered_map<SGUID, CRuntimeClassConstPtr> Classes;

public:

	// IRuntimeRegistry
	virtual IRuntimeClassConstPtr GetClass(const SGUID& guid) const override;
	// ~IRuntimeRegistry

	void                  RegisterClass(const CRuntimeClassConstPtr& pClass);
	void                  ReleaseClass(const SGUID& guid);
	CRuntimeClassConstPtr GetClassImpl(const SGUID& guid) const;

	void                  Reset();

private:

	Classes m_classes;
};
} // Schematyc
