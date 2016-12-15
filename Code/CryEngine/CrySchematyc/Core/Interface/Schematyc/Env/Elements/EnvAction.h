// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryNetwork/ISerialize.h>
#include <CryMemory/CrySizer.h>
#include <CryMemory/STLPoolAllocator.h>

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Env/EnvElementBase.h"
#include "Schematyc/Env/Elements/IEnvAction.h"
#include "Schematyc/Utils/Any.h"
#include "Schematyc/Utils/GUID.h"
#include "Schematyc/Utils/Properties.h"

#define SCHEMATYC_MAKE_ENV_ACTION(action, name) Schematyc::EnvAction::MakeShared<action>(name, SCHEMATYC_SOURCE_FILE_INFO)

namespace Schematyc
{
template<typename ACTION> class CEnvAction : public CEnvElementBase<IEnvAction>
{
public:

	inline CEnvAction(const char* szName, const SSourceFileInfo& sourceFileInfo)
		: CEnvElementBase(Schematyc::GetTypeInfo<ACTION>().guid, szName, sourceFileInfo)
	{}

	// IEnvElement

	virtual bool IsValidScope(IEnvElement& scope) const override
	{
		switch (scope.GetElementType())
		{
		case EEnvElementType::Module:
		case EEnvElementType::Class:
		case EEnvElementType::Component:
			{
				return true;
			}
		default:
			{
				return false;
			}
		}
	}

	// ~IEnvElement

	// IEnvAction

	virtual const char* GetIcon() const override
	{
		return m_icon.c_str();
	}

	virtual EnvActionFlags GetFlags() const override
	{
		return m_flags;
	}

	virtual CActionPtr Create() const override
	{
		return std::allocate_shared<ACTION>(m_allocator);
	}

	virtual const IProperties* GetProperties() const override
	{
		return m_pProperties.get();
	}

	// ~IEnvAction

	inline void SetIcon(const char* szIcon)
	{
		m_icon = szIcon;
	}

	inline void SetFlags(const EnvActionFlags& flags)
	{
		m_flags = flags;
	}

	template<typename TYPE> inline void SetProperties(const TYPE& properties)
	{
		m_pProperties = Properties::MakeShared(properties);
	}

private:

	string                        m_icon;
	EnvActionFlags                m_flags;
	stl::STLPoolAllocator<ACTION> m_allocator;
	IPropertiesPtr                m_pProperties;
};

namespace EnvAction
{
template<typename TYPE> inline std::shared_ptr<CEnvAction<TYPE>> MakeShared(const char* szName, const SSourceFileInfo& sourceFileInfo)
{
	SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(TYPE);

	return std::make_shared<CEnvAction<TYPE>>(szName, sourceFileInfo);
}
} // EnvAction
} // Schematyc
