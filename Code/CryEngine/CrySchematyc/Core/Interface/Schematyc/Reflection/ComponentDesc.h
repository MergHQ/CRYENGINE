#pragma once

#include "Schematyc/Component.h"
#include "Schematyc/Reflection/TypeDesc.h"

namespace Schematyc
{

// Forward declare interfaces.
struct IComponentPreviewer;
struct INetworkSpawnParams;

enum class EComponentFlags
{
	None      = 0,
	Singleton = BIT(0), // Allow only of one instance of this component per class/object.
	Transform = BIT(1), // Component has transform.
	Socket    = BIT(2), // Other components can be attached to socket of this component.
	Attach    = BIT(3)  // This component can be attached to socket of other components.
};

typedef CEnumFlags<EComponentFlags> ComponentFlags;

enum class EComponentDependencyType
{
	None,
	Soft,   // Dependency must be initialized before component.
	Hard    // Dependency must exist and be initialized before component.
};

struct SComponentDependency
{
	inline SComponentDependency(EComponentDependencyType _type, const SGUID& _guid)
		: type(_type)
		, guid(_guid)
	{}

	EComponentDependencyType type;
	SGUID                    guid;
};

class CComponentDesc : public CCustomClassDesc
{
private:

	typedef DynArray<SComponentDependency> Dependencies;

public:

	inline void SetIcon(const char* szIcon)
	{
		m_icon = szIcon;
	}

	inline const char* GetIcon() const
	{
		return m_icon.c_str();
	}

	inline void SetComponentFlags(const ComponentFlags& flags)
	{
		m_flags = flags;
	}

	inline ComponentFlags GetComponentFlags() const
	{
		return m_flags;
	}

	inline void AddDependency(EComponentDependencyType type, const SGUID& guid)
	{
		m_dependencies.emplace_back(type, guid);
	}

	inline uint32 GetDependencyCount() const
	{
		return m_dependencies.size();
	}

	inline const SComponentDependency* GetDependency(uint32 dependencyIdx) const
	{
		return dependencyIdx < m_dependencies.size() ? &m_dependencies[dependencyIdx] : nullptr;
	}

	inline bool IsDependency(const SGUID& guid) const
	{
		for (const SComponentDependency& dependency : m_dependencies)
		{
			if (dependency.guid == guid)
			{
				return true;
			}
		}
		return false;
	}

	template<typename TYPE> inline void SetNetworkSpawnParams(const INetworkSpawnParams& networkSpawnParams)
	{
		m_pNetworkSpawnParams.reset(new TYPE(networkSpawnParams));
	}

	inline const INetworkSpawnParams* GetNetworkSpawnParams() const
	{
		return m_pNetworkSpawnParams.get();
	}

	template<typename TYPE> inline void SetPreviewer(const TYPE& previewer)
	{
		static_assert(std::is_base_of<IComponentPreviewer, TYPE>::value, "Type must be derived from IComponentPreviewer!");
		m_pPreviewer = stl::make_unique<TYPE>(previewer);
	}

	inline IComponentPreviewer* GetPreviewer() const
	{
		return m_pPreviewer.get();
	}

private:

	string                               m_icon;
	ComponentFlags                       m_flags;
	Dependencies                         m_dependencies;
	std::unique_ptr<INetworkSpawnParams> m_pNetworkSpawnParams;
	std::unique_ptr<IComponentPreviewer> m_pPreviewer;
};

SCHEMATYC_DECLARE_CUSTOM_CLASS_DESC(CComponent, CComponentDesc, CClassDescInterface)

} // Schematyc
