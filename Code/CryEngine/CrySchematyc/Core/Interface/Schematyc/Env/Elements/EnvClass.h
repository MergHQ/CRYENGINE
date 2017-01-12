// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/FundamentalTypes.h>
#include <Schematyc/Env/EnvElementBase.h>
#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/Elements/IEnvClass.h>
#include "Schematyc/Utils/Any.h"
#include <Schematyc/Utils/TypeName.h>

#define SCHEMATYC_MAKE_ENV_CLASS(guid, name) Schematyc::EnvClass::MakeShared(guid, name, SCHEMATYC_SOURCE_FILE_INFO)

namespace Schematyc
{

class CEnvClass : public CEnvElementBase<IEnvClass>
{
private:

	typedef std::vector<SGUID> Interfaces;
	typedef std::vector<SGUID> Components;

public:

	inline CEnvClass(const SGUID& guid, const char* szName, const SSourceFileInfo& sourceFileInfo)
		: CEnvElementBase(guid, szName, sourceFileInfo)
	{}

	// IEnvElement

	virtual bool IsValidScope(IEnvElement& scope) const override
	{
		switch (scope.GetType())
		{
		case EEnvElementType::Module:
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

	// IEnvClass

	virtual uint32 GetInterfaceCount() const override
	{
		return m_interfaces.size();
	}

	virtual SGUID GetInterfaceGUID(uint32 interfaceIdx) const override
	{
		return interfaceIdx < m_interfaces.size() ? m_interfaces[interfaceIdx] : SGUID();
	}

	virtual uint32 GetComponentCount() const override
	{
		return m_components.size();
	}

	virtual SGUID GetComponentTypeGUID(uint32 componentIdx) const override
	{
		return componentIdx < m_components.size() ? m_components[componentIdx] : SGUID();
	}

	virtual CAnyConstPtr GetProperties() const override
	{
		return m_pProperties.get();
	}

	virtual IObjectPreviewer* GetPreviewer() const override
	{
		return m_pPreviewer.get();
	}

	// ~IEnvClass

	inline void AddInterface(const SGUID& guid)
	{
		m_interfaces.push_back(guid);
	}

	inline void AddComponent(const SGUID& guid)
	{
		m_components.push_back(guid);
	}

	template<typename TYPE> inline void SetProperties(const TYPE& properties)
	{
		m_pProperties = CAnyValue::MakeShared(properties);
	}

	template<typename TYPE> inline void SetPreviewer(const TYPE& previewer)
	{
		m_pPreviewer.reset(new TYPE(previewer));
	}
	
private:

	Interfaces                        m_interfaces;
	Components                        m_components;
	CAnyValuePtr                      m_pProperties;
	std::unique_ptr<IObjectPreviewer> m_pPreviewer;
};

namespace EnvClass
{

inline std::shared_ptr<CEnvClass> MakeShared(const SGUID& guid, const char* szName, const SSourceFileInfo& sourceFileInfo)
{
	return std::make_shared<CEnvClass>(guid, szName, sourceFileInfo);
}

} // EnvClass
} // Schematyc
