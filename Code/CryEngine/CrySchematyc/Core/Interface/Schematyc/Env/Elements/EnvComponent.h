// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMemory/STLPoolAllocator.h>

#include "Schematyc/FundamentalTypes.h"
#include "Schematyc/Env/EnvElementBase.h"
#include "Schematyc/Env/Elements/IEnvComponent.h"
#include "Schematyc/Utils/GUID.h"
#include "Schematyc/Utils/Properties.h"

#define SCHEMATYC_MAKE_ENV_COMPONENT(component, name) Schematyc::EnvComponent::MakeShared<component>(name, SCHEMATYC_SOURCE_FILE_INFO)

namespace Schematyc
{
// Forward declare interfaces.
struct INetworkSpawnParams;

template<typename COMPONENT> class CEnvComponent : public CEnvElementBase<IEnvComponent>
{
private:

	typedef std::vector<SEnvComponentDependency> Dependencies;

public:

	inline CEnvComponent(const char* szName, const SSourceFileInfo& sourceFileInfo)
		: CEnvElementBase(Schematyc::GetTypeInfo<COMPONENT>().GetGUID(), szName, sourceFileInfo)
	{}

	// IEnvElement

	virtual bool IsValidScope(IEnvElement& scope) const override
	{
		switch (scope.GetElementType())
		{
		case EEnvElementType::Module:
		case EEnvElementType::Class:
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

	// IEnvComponent

	virtual const char* GetIcon() const override
	{
		return m_icon.c_str();
	}

	virtual EnvComponentFlags GetFlags() const override
	{
		return m_flags;
	}

	virtual uint32 GetDependencyCount() const override
	{
		return m_dependencies.size();
	}

	virtual const SEnvComponentDependency* GetDependency(uint32 dependencyIdx) const override
	{
		return dependencyIdx < m_dependencies.size() ? &m_dependencies[dependencyIdx] : nullptr;
	}

	virtual bool IsDependency(const SGUID& guid) const override
	{
		for (const SEnvComponentDependency& dependency : m_dependencies)
		{
			if (dependency.guid == guid)
			{
				return true;
			}
		}
		return false;
	}

	virtual CComponentPtr Create() const override
	{
		return std::allocate_shared<COMPONENT>(m_allocator);
	}

	virtual const IProperties* GetProperties() const override
	{
		return m_pProperties.get();
	}

	virtual const INetworkSpawnParams* GetNetworkSpawnParams() const override
	{
		return m_pNetworkSpawnParams.get();
	}

	virtual IComponentPreviewer* GetPreviewer() const override
	{
		return m_pPreviewer.get();
	}

	// ~IEnvComponent

	inline void SetIcon(const char* szIcon)
	{
		m_icon = szIcon;
	}

	inline void SetFlags(EnvComponentFlags flags)
	{
		m_flags = flags;
	}

	inline void AddDependency(EEnvComponentDependencyType type, const SGUID& guid)
	{
		m_dependencies.emplace_back(type, guid);
	}

	template<typename TYPE> inline void SetProperties(const TYPE& properties)
	{
		m_pProperties = Properties::MakeShared(properties);
	}

	template<typename TYPE> inline void SetNetworkSpawnParams(const INetworkSpawnParams& networkSpawnParams)
	{
		m_pNetworkSpawnParams.reset(new TYPE(networkSpawnParams));
	}

	template<typename TYPE> inline void SetPreviewer(const TYPE& previewer)
	{
		m_pPreviewer.reset(new TYPE(previewer));
	}

private:

	string                               m_icon;
	EnvComponentFlags                    m_flags;
	Dependencies                         m_dependencies;
	stl::STLPoolAllocator<COMPONENT>     m_allocator;
	IPropertiesPtr                       m_pProperties; // #SchematycTODO : Store using std::unique_ptr?
	std::unique_ptr<INetworkSpawnParams> m_pNetworkSpawnParams;
	std::unique_ptr<IComponentPreviewer> m_pPreviewer;
};

namespace EnvComponent
{
template<typename TYPE> inline std::shared_ptr<CEnvComponent<TYPE>> MakeShared(const char* szName, const SSourceFileInfo& sourceFileInfo)
{
	SCHEMATYC_VERIFY_TYPE_IS_REFLECTED(TYPE);

	return std::make_shared<CEnvComponent<TYPE>>(szName, sourceFileInfo);
}
} // EnvComponent
} // Schematyc
