// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// #SchematycTODO : Replace IEnvElementPtr with std::unique_ptr?

#pragma once

#include "Schematyc/Env/IEnvElement.h"
#include "Schematyc/Utils/Assert.h"
#include "Schematyc/Utils/GUID.h"

namespace Schematyc
{

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

	inline CEnvRegistrationScope(IEnvElementRegistrar& elementRegistrar, const SGUID& scopeGUID)
		: m_elementRegistrar(elementRegistrar)
		, m_scopeGUID(scopeGUID)
	{}

	inline CEnvRegistrationScope Register(const IEnvElementPtr& pElement)
	{
		SCHEMATYC_CORE_ASSERT(pElement);
		if (pElement)
		{
			m_elementRegistrar.Register(pElement, m_scopeGUID);
			return CEnvRegistrationScope(m_elementRegistrar, pElement->GetGUID());
		}
		return *this;
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
