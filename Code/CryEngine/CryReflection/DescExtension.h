// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/IDescExtension.h>

#include <vector>

namespace Cry {
namespace Reflection {

template<typename INTERFACE_TYPE>
class CExtensibleDesc : public INTERFACE_TYPE
{
public:
	CExtensibleDesc() {}
	~CExtensibleDesc();

	// IExtensibleDesc
	virtual bool            AddExtension(CDescExtension* pExtension) const override;
	virtual CDescExtension* FindExtensionByTypeId(CTypeId typeId) const override;
	// ~IExtensibleDesc

protected:
	// Note: We always allow to add extensions even though it's a const object.
	mutable std::vector<CDescExtension*> m_extensions;
};

template<typename INTERFACE_TYPE>
inline CExtensibleDesc<INTERFACE_TYPE>::~CExtensibleDesc()
{
	for (CDescExtension* pExtension : m_extensions)
	{
		delete pExtension;
	}
}

template<typename INTERFACE_TYPE>
inline bool CExtensibleDesc<INTERFACE_TYPE >::AddExtension(CDescExtension* pExtension) const
{
	const CTypeId typeId = pExtension->GetTypeId();
	auto condition = [typeId](CDescExtension* pExtension) -> bool
	{
		return pExtension->GetTypeId() == typeId;
	};

	const auto result = std::find_if(m_extensions.begin(), m_extensions.end(), condition);
	CRY_ASSERT(result == m_extensions.end(), "Extension of type '%s' is already registered.", pExtension->GetLabel());

	if (result == m_extensions.end())
	{
		m_extensions.emplace_back(pExtension);
		return true;
	}
	return false;
}

template<typename INTERFACE_TYPE>
inline CDescExtension* CExtensibleDesc<INTERFACE_TYPE >::FindExtensionByTypeId(CTypeId typeId) const
{
	auto condition = [typeId](CDescExtension* pExtension) -> bool
	{
		return pExtension->GetTypeId() == typeId;
	};

	auto result = std::find_if(m_extensions.begin(), m_extensions.end(), condition);
	if (result != m_extensions.end())
	{
		return *result;
	}

	return nullptr;
}

} // ~Reflection namespace
} // ~Cry namespace
