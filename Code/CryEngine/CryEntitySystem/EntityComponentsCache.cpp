// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#ifndef RELEASE
#include "EntityComponentsCache.h"
#include "Entity.h"

//////////////////////////////////////////////////////////////////////////
void CEntityComponentsCache::StoreEntities()
{
	CRY_PROFILE_FUNCTION(PROFILE_ENTITY);

	IEntityItPtr it = g_pIEntitySystem->GetEntityIterator();
	while (CEntity* pEntity = static_cast<CEntity*>(it->Next()))
	{
		if (!pEntity->IsGarbage())
		{
			StoreEntity(*static_cast<CEntity*>(pEntity));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentsCache::RestoreEntities()
{
	CRY_PROFILE_FUNCTION(PROFILE_ENTITY);

	RemoveEntitiesSpawnedDuringGameMode();

	IEntityItPtr it = g_pIEntitySystem->GetEntityIterator();
	while (CEntity* pEntity = static_cast<CEntity*>(it->Next()))
	{
		if (!pEntity->IsGarbage())
		{
			LoadEntity(*static_cast<CEntity*>(pEntity));
		}
	}

	m_componentPropertyCache.clear();
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentsCache::RemoveEntitiesSpawnedDuringGameMode()
{
	for (const EntityId entityId : m_entitiesSpawnedDuringEditorGameMode)
	{
		if (CEntity* pEntity = g_pIEntitySystem->GetEntityFromID(entityId))
		{
			CEntity* pParent = static_cast<CEntity*>(pEntity->GetParent());

			// Childs of irremovable entity are not deleted (Needed for vehicles weapons for example)
			if (pParent != nullptr && pParent->GetFlags() & ENTITY_FLAG_UNREMOVABLE)
			{
				continue;
			}

			g_pIEntitySystem->RemoveEntity(entityId, true);
		}
	}

	m_entitiesSpawnedDuringEditorGameMode.clear();
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentsCache::StoreComponent(CEntity& entity, IEntityComponent& component)
{
	CRY_PROFILE_FUNCTION(PROFILE_ENTITY);

	// Only Schematyc editable and user added components are eligible for storing
	if (!component.GetComponentFlags().Check(EEntityComponentFlags::SchematycEditable) &&
	    !component.GetComponentFlags().Check(EEntityComponentFlags::UserAdded))
	{
		return;
	}

	std::unique_ptr<Schematyc::CClassProperties> pClassProperties(new Schematyc::CClassProperties());
	pClassProperties->Read(component.GetClassDesc(), &component);

	SDoubleGUID doubleGuid = { entity.GetGuid(), component.GetGUID() };
	m_componentPropertyCache.insert(std::make_pair(doubleGuid, std::move(pClassProperties)));
}

//////////////////////////////////////////////////////////////////////////
void CEntityComponentsCache::LoadComponent(CEntity& entity, IEntityComponent& component)
{
	CRY_PROFILE_FUNCTION(PROFILE_ENTITY);

	// Only Schematyc editable and user added components are eligible for storing
	if (!component.GetComponentFlags().Check(EEntityComponentFlags::SchematycEditable) &&
	    !component.GetComponentFlags().Check(EEntityComponentFlags::UserAdded))
	{
		return;
	}

	auto iter = m_componentPropertyCache.find({ entity.GetGuid(), component.GetGUID() });
	if (iter != m_componentPropertyCache.end())
	{
		auto& pClassProperties = iter->second;
		bool bMembersChanged = !pClassProperties->Compare(component.GetClassDesc(), &component);
		if (bMembersChanged)
		{
			// Restore members from the previously recorded class properties
			pClassProperties->Apply(component.GetClassDesc(), &component);
			// Notify this component about changed property.
			SEntityEvent entityEvent(ENTITY_EVENT_COMPONENT_PROPERTY_CHANGED);
			component.SendEvent(entityEvent);
		}
	}
}

void CEntityComponentsCache::StoreEntity(CEntity& entity)
{
	entity.GetComponentsVector().ForEach([this, &entity](const SEntityComponentRecord& rec) -> EComponentIterationResult
	{
		if (!rec.pComponent->GetClassDesc().GetName().IsEmpty())
		{
			StoreComponent(entity, *rec.pComponent.get());
		}

		return EComponentIterationResult::Continue;
	});
}

void CEntityComponentsCache::LoadEntity(CEntity& entity)
{
	entity.GetComponentsVector().ForEach([this, &entity](const SEntityComponentRecord& rec) -> EComponentIterationResult
	{
		if (!rec.pComponent->GetClassDesc().GetName().IsEmpty())
		{
			LoadComponent(entity, *rec.pComponent.get());
		}

		return EComponentIterationResult::Continue;
	});
}

#endif