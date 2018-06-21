// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryType/Type.h>
#include <CryReflection/ITypeDesc.h>
#include <CryEntitySystem/IEntityComponent.h>
#include <CryEntitySystem/IReflectionRegistry.h>

namespace Cry {
namespace EntitySystem {

class CTypeExtension : public Reflection::CDescExtension
{
public:
	CTypeExtension()
		: Reflection::CDescExtension("CEntityTypeExtension", Type::IdOf<CTypeExtension>())
		, m_szIcon(nullptr)
		, m_szEditorCategory(nullptr)
		, m_componentFlags(IEntityComponent::EFlags::HiddenFromUser)
	{

	}

	CTypeExtension(const char* szIcon, const char* szEditorCategory, const EntityComponentFlags& componentFlags)
		: Reflection::CDescExtension("CEntityTypeExtension", Type::IdOf<CTypeExtension>())
		, m_szIcon(szIcon)
		, m_szEditorCategory(szEditorCategory)
		, m_componentFlags(componentFlags)
	{

	}

	// CDescExtension
	virtual void OnRegister(const Reflection::ITypeDesc& typeDesc) final
	{
		if (gEnv->pEntitySystem)
		{
			Cry::Entity::IReflectionRegistry* pRegistry = gEnv->pEntitySystem->GetReflectionRegistry();
			if (pRegistry)
			{
				pRegistry->UseType(typeDesc);
				return;
			}
			CRY_ASSERT_MESSAGE(pRegistry, "Reflection registry not initialized.");
		}
		CRY_ASSERT_MESSAGE(gEnv->pEntitySystem, "EntitySystem not initialized.");
	}
	// ~CDescExtension

	void SetIcon(const char* szIcon)
	{
		m_szIcon = szIcon;
	}

	const char* GetIcon() const
	{
		return m_szIcon;
	}

	void SetEditorCategory(const char* szEditorCategory)
	{
		m_szEditorCategory = szEditorCategory;
	}

	const char* GetEditorCategory() const
	{
		return m_szEditorCategory;
	}

	void SetComponentFalgs(const EntityComponentFlags& flags)
	{
		m_componentFlags = flags;
	}

	const EntityComponentFlags& GetComponentFlags() const
	{
		return m_componentFlags;
	}

private:
	const char*          m_szIcon;
	const char*          m_szEditorCategory;
	EntityComponentFlags m_componentFlags;
};

} // ~EntitySystem namespace
} // ~Cry namespace
