// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryReflection/Framework/Common.h>

namespace Cry {
namespace Reflection {

struct ITypeDesc;

class CDescExtension
{
public:
	CDescExtension(const char* szLabel, CTypeId typeId)
		: m_szLabel(szLabel)
		, m_typeId(typeId)
	{}

	CDescExtension(CDescExtension&& desc)
		: m_szLabel(desc.m_szLabel)
		, m_typeId(desc.m_typeId)
	{}

	virtual ~CDescExtension() {}

	virtual void OnRegister(const ITypeDesc& typeDesc) {}

	CTypeId      GetTypeId() const                     { return m_typeId; }
	const char*  GetLabel() const                      { return m_szLabel; }

private:
	const char*   m_szLabel;
	const CTypeId m_typeId;
};

struct IExtensibleDesc
{
	virtual ~IExtensibleDesc() {}

	virtual bool            AddExtension(CDescExtension* pExtension) const = 0;
	virtual CDescExtension* FindExtensionByTypeId(CTypeId typeId) const = 0;

	// Extension Helpers
	template<typename EXTENSION_TYPE>
	const EXTENSION_TYPE* FindExtension() const
	{
		static_assert(std::is_base_of<CDescExtension, typename Type::PureType<EXTENSION_TYPE>::Type>::value, "Type doesn't derive from 'Cry::Reflection::CDescExtension'.");

		constexpr CTypeId typeId = Type::IdOf<EXTENSION_TYPE>();
		return static_cast<EXTENSION_TYPE*>(FindExtensionByTypeId(typeId));
	}

	template<typename EXTENSION_TYPE, typename ... PARAM_TYPES>
	EXTENSION_TYPE* AddExtension(PARAM_TYPES&& ... params) const
	{
		static_assert(std::is_base_of<CDescExtension, typename Type::PureType<EXTENSION_TYPE>::Type>::value, "Type doesn't derive from 'Cry::Reflection::CDescExtension'.");

		EXTENSION_TYPE* pExtension = new EXTENSION_TYPE(std::forward<PARAM_TYPES>(params) ...);
		if (!AddExtension(pExtension))
		{
			delete pExtension;
			return nullptr;
		}

		return pExtension;
	}
};

} // ~Reflection namespace
} // ~Cry namespace
