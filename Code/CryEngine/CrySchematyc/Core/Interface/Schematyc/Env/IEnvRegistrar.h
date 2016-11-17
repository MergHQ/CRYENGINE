// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Replace IEnvElementPtr with std::unique_ptr?

#pragma once

#include "Schematyc/Utils/GUID.h"

namespace Schematyc
{
// Forward declare interfaces.
struct IEnvElement;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(IEnvElement)

struct IEnvElementRegistrar
{
	virtual ~IEnvElementRegistrar() {}

	virtual void Register(const IEnvElementPtr& pElement, const SGUID& scopeGUID) = 0;
};

class CEnvRegistrationScope
{
public:

	inline CEnvRegistrationScope(IEnvElementRegistrar& registrar, const SGUID& scopeGUID)
		: m_elementRegistrar(registrar)
		, m_scopeGUID(scopeGUID)
	{}

	inline void Register(const IEnvElementPtr& pElement)
	{
		m_elementRegistrar.Register(pElement, m_scopeGUID);
	}

private:

	IEnvElementRegistrar& m_elementRegistrar;
	SGUID                 m_scopeGUID;
};

struct IEnvRegistrar
{
	virtual ~IEnvRegistrar() {}

	virtual CEnvRegistrationScope RootScope() = 0;
	virtual CEnvRegistrationScope Scope(const SGUID& scopeGUID) = 0;
};
} // Schematyc
